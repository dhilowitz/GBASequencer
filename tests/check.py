#!/usr/bin/env python3
"""Headless tests: drive the ROM in mGBA with scripted button presses, then check the recorded
audio, memory and frame rate. Screenshots go to tests/out/ for a look by eye.

    make test            (or: python3 tests/check.py)

Needs tools/harness/harness (tools/harness/build.sh) and numpy.
"""
import os
import subprocess
import sys
import wave

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROM = os.path.join(ROOT, 'gbasequencer.gba')
HARNESS = os.path.join(ROOT, 'tools', 'harness', 'harness')
OUT = os.path.join(ROOT, 'tests', 'out')
MAP = os.path.join(ROOT, 'build', 'gbasequencer.map')

failures = []


def check(cond, what):
    print(('  ok    ' if cond else '  FAIL  ') + what)
    if not cond:
        failures.append(what)


def symbol(name):
    for line in open(MAP):
        parts = line.split()
        if len(parts) == 2 and parts[1] == name and parts[0].startswith('0x'):
            return int(parts[0], 16)
    raise KeyError(name)


def run(script, save=None):
    """Run a harness script (OUT/ is replaced by tests/out/). Returns the peek results in order."""
    args = [HARNESS, ROM] + (['--save', save] if save else [])
    res = subprocess.run(args, input=script.replace('OUT/', OUT + '/'), capture_output=True, text=True)
    if res.returncode:
        sys.exit('harness failed: ' + res.stderr)
    return [int(line.split()[3]) for line in res.stdout.splitlines() if line.startswith('peek')]


def load_wav(name):
    w = wave.open(os.path.join(OUT, name))
    x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').reshape(-1, 2)[:, 0] / 32768.0
    return x, w.getframerate()


def pitch(x, sr, start, length=0.1):
    seg = x[int(start * sr):int((start + length) * sr)]
    seg = seg - seg.mean()
    ac = np.correlate(seg, seg, 'full')[len(seg) - 1:]
    lo, hi = int(sr / 1200), int(sr / 50)
    lag = lo + np.argmax(ac[lo:hi])
    # parabolic interpolation for a finer estimate
    a, b, c = ac[lag - 1], ac[lag], ac[lag + 1]
    lag += 0.5 * (a - c) / (a - 2 * b + c)
    return sr / lag


def onsets(x, sr):
    hop, n = 64, 512
    frames = np.array([np.abs(np.fft.rfft(x[i:i + n] * np.hanning(n))) for i in range(0, len(x) - n, hop)])
    flux = np.maximum(frames[1:] - frames[:-1], 0).sum(axis=1)
    thr = flux.mean() + 3 * flux.std()
    found = []
    for i in range(1, len(flux) - 1):
        if flux[i] > thr and flux[i] >= flux[i - 1] and flux[i] >= flux[i + 1] and (not found or i - found[-1] > 40):
            found.append(i)
    return (np.array(found) + 1) * hop / sr


def script(name):
    return open(os.path.join(ROOT, 'tests', 'scripts', name)).read()


def midi_hz(n):
    return 440 * 2 ** ((n - 69) / 12)


os.makedirs(OUT, exist_ok=True)
for f in os.listdir(OUT):
    os.remove(os.path.join(OUT, f))
song = symbol('song')
SYNTH = song + 1 + 4 * 64          # song.synth[0][0]
SYNTH_SIZE = 1 + 64

print('song entry, playback and save')
save = os.path.join(OUT, 'song.sav')
run(script('song_basic.txt'), save)
x, sr = load_wav('song.wav')
t = onsets(x, sr)
check(len(t) >= 9 and t[-1] > 4.4, 'song keeps playing past its end and loops (%d beats in 5 s)' % len(t))

print('step timing')
run(script('timing.txt'))
x, sr = load_wav('timing.wav')
loud = np.abs(x) > 0.1
starts = [i for i in range(1, len(x)) if loud[i] and not loud[max(0, i - int(0.05 * sr)):i].any()]
gaps = np.diff(starts) / sr * 1000
check(len(starts) >= 10, 'heard %d kicks in 6 s at 150 BPM' % len(starts))
check(len(gaps) and np.all(np.abs(gaps - 400) < 0.5),
      'kicks are 400 ms apart, across song positions too (%s)' % np.round(gaps, 2).tolist())

print('synths and drums in sync')
def attacks(x, sr):
    """Times where the level jumps up: note and drum starts, to the sample."""
    hop = int(sr / 1000)
    env = np.array([np.abs(x[i:i + hop]).max() for i in range(0, len(x) - hop, hop)])
    found = []
    for j in range(1, len(env)):
        if env[j] > 1.25 * env[j - 1] + 0.02 and (not found or j * hop - found[-1] > 0.1 * sr):
            seg = np.abs(x[j * hop:(j + 1) * hop])
            found.append(j * hop + int(np.argmax(seg > env[j - 1] + 0.01)))
    return np.array(found) / sr


runs = {}
for name, mute in (('sync_synth.wav', 'mute 4'), ('sync_drums.wav', 'mute 0\nmute 1\nmute 2')):
    run(script('sync.txt').replace('MUTE', mute).replace('WAVNAME', name))
    runs[name] = attacks(*load_wav(name))
a, b = runs['sync_synth.wav'], runs['sync_drums.wav']
off = np.array([(t - b[np.argmin(np.abs(b - t))]) * 1000 for t in a]) if len(b) else np.array([99.0])
check(len(a) >= 8, 'heard %d synth notes in 4 s at 130 BPM' % len(a))
check(np.all(np.abs(off) < 0.5), 'each synth note starts within 0.5 ms of its kick (%s)' % np.round(off, 2).tolist())

print('reload the saved song')
peeks = run('run 30\ntap START\nrun 10\nshot OUT/reloaded.ppm\n'
            + ''.join('peek8 %d\n' % (SYNTH + 1 + s) for s in (0, 4, 8, 12))
            + 'peek8 %d\npeek8 %d\npeek8 %d\n' % (song + 1, song + 1 + 3 * 64, song + 1 + 3 * 64 + 1), save)
check(peeks[:4] == [48, 48, 48, 55], 'bass notes survive a power cycle: %s' % peeks[:4])
check(peeks[4:] == [0, 0, 0], 'arrangement survives a power cycle: %s' % peeks[4:])

print('demo song')
demo = os.path.join(OUT, 'demo.sav')
subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'demo_song.py'), demo], check=True, capture_output=True)
peeks = run('run 30\ntap START\nrun 10\npeek8 %d\npeek8 %d\npeek8 %d\npeek8 %d\n' % (song, song + 1 + 7, SYNTH + 1, song + 5458 + 8 + 5), demo)
check(peeks == [118, 1, 48, 2], 'tools/demo_song.py writes a save the ROM loads: tempo %d, bass at position 8 is pattern %d, first note %d, Syn 2 vibrato %d' % tuple(peeks))

print('synth voices')
VOICE = song + 5458                 # song.voice[3][8], after everything that version 1 had
PARAMS = ['volume', 'tone', 'attack', 'decay', 'release', 'vibrato', 'slide', 'sweep']


def voice_script(screen, settings, notes='', then=''):
    """Boot, go to synth `screen` (0-2), enter `notes`, open the VOICE panel and change settings,
    e.g. {'tone': -2}, then run `then` (with the panel still open)."""
    out = 'run 30\ntap START\n' + 'tap SELECT\n' * screen + notes + 'tap L\ntap L\ntap L\n'
    sel = 0
    for name, delta in settings.items():
        i = PARAMS.index(name)
        out += ('tap DOWN\n' * (i - sel)) if i >= sel else ('tap UP\n' * (sel - i))
        sel = i
        out += ('tap RIGHT\n' if delta > 0 else 'tap LEFT\n') * abs(delta)
    return out + 'run 150\n' + then   # let the previews from changing settings die away


def preview(screen, settings, name, frames=50):
    """Record the A-button preview: the note held for 20 frames, then released."""
    run(voice_script(screen, settings, then='wav OUT/%s.wav\ntap A 1\nrun %d\nendwav\n' % (name, frames)))
    return load_wav(name + '.wav')


def rms(x, sr, t0, t1):
    seg = x[int(t0 * sr):int(t1 * sr)]
    return float(np.sqrt(((seg - seg.mean()) ** 2).mean()))


def start_of(x):
    return np.argmax(np.abs(x - x[0]) > 0.01)


x, sr = preview(1, {}, 'v_default')
x7, _ = preview(1, {'volume': -8}, 'v_vol7')
a0, a1 = x[start_of(x):], x7[start_of(x7):]
ratio = rms(a1, sr, 0.01, 0.05) / rms(a0, sr, 0.01, 0.05)
check(0.38 < ratio < 0.56, 'volume 7 is %.2f of volume 15 (want about 0.47)' % ratio)


def duty(x, sr):
    seg = x[start_of(x) + int(0.01 * sr):][:int(0.05 * sr)]
    mid = (seg.max() + seg.min()) / 2
    return float((seg > mid).mean())


xt, _ = preview(1, {'tone': -1}, 'v_tone125')
xh, _ = preview(1, {'tone': 1}, 'v_tone50')
check(abs(duty(xt, sr) - 0.125) < 0.04 and abs(duty(xh, sr) - 0.5) < 0.05,
      'tone sets the pulse width: %.3f (12.5%%) and %.3f (50%%)' % (duty(xt, sr), duty(xh, sr)))

xa, _ = preview(1, {'attack': 3, 'decay': -7}, 'v_attack')
b = start_of(xa) / sr
early, late = rms(xa, sr, b, b + 0.04), rms(xa, sr, b + 0.25, b + 0.32)
check(late > 3 * early, 'attack fades in: %.3f in the first 40 ms, %.3f at 250 ms' % (early, late))

xp, _ = preview(1, {'volume': -7, 'attack': 1, 'decay': -7}, 'v_attack_vol8')
xq8, _ = preview(1, {'volume': -7, 'decay': -7}, 'v_vol8')
ratio = rms(xp, sr, start_of(xp) / sr + 0.2, start_of(xp) / sr + 0.3) / rms(xq8, sr, start_of(xq8) / sr + 0.2, start_of(xq8) / sr + 0.3)
check(abs(ratio - 1) < 0.1, 'an attack stops at the voice volume, not full volume (%.2f of a plain note)' % ratio)

xd, _ = preview(1, {'decay': -7}, 'v_hold')
b = start_of(xd) / sr
r1, r2 = rms(xd, sr, b + 0.02, b + 0.06), rms(xd, sr, b + 0.26, b + 0.30)
check(abs(r2 / r1 - 1) < 0.1, 'decay Hold keeps the level while the note is held (%.3f, %.3f)' % (r1, r2))

xc, _ = preview(1, {'decay': -7, 'release': -7}, 'v_cut')
xr, _ = preview(1, {'decay': -7}, 'v_release7')
bc, br = start_of(xc) / sr, start_of(xr) / sr
cut_after, rel_after = rms(xc, sr, bc + 0.40, bc + 0.45), rms(xr, sr, br + 0.40, br + 0.45)
check(cut_after < 0.01 and rel_after > 0.05,
      'release Cut stops the note at its end (%.3f); release 7 fades on (%.3f)' % (cut_after, rel_after))

xv, _ = preview(1, {'decay': -7, 'vibrato': 7}, 'v_vibrato')
b = start_of(xv) / sr
fs = [pitch(xv, sr, b + 0.02 + i * 0.025, 0.025) for i in range(10)]
spread = (max(fs) / min(fs) - 1) * 100
check(spread > 4, 'vibrato 7 moves the pitch %.1f%% (about a semitone each way)' % spread)

xs, _ = preview(0, {'sweep': 3}, 'v_sweep', 30)
b = start_of(xs) / sr
f1, f2 = pitch(xs, sr, b + 0.005, 0.02), pitch(xs, sr, b + 0.13, 0.02)
check(f2 > f1 * 1.3, 'Bass sweep Up 3 bends the pitch up: %.0f Hz, then %.0f Hz' % (f1, f2))


def harmonic3(x, sr, f0):
    b = start_of(x)
    seg = x[b + int(0.02 * sr):b + int(0.25 * sr)]
    seg = (seg - seg.mean()) * np.hanning(len(seg))
    spec = np.abs(np.fft.rfft(seg))
    freqs = np.fft.rfftfreq(len(seg), 1 / sr)
    at = lambda f: spec[np.argmin(np.abs(freqs - f))]
    return at(3 * f0) / at(f0)


xq, _ = preview(2, {'tone': 2}, 'v_wave_square')
xn, _ = preview(2, {'tone': 3}, 'v_wave_sine')
hq, hn = harmonic3(xq, sr, midi_hz(60)), harmonic3(xn, sr, midi_hz(60))
check(hq > 0.2 and hn < 0.1, 'Syn 3 tone sets the wave: 3rd harmonic %.2f for Square, %.2f for Sine' % (hq, hn))

xw, _ = preview(2, {'volume': -2}, 'v_wave_50')
xf, _ = preview(2, {}, 'v_wave_100')
ratio = rms(xw, sr, start_of(xw) / sr + 0.02, start_of(xw) / sr + 0.2) / rms(xf, sr, start_of(xf) / sr + 0.02, start_of(xf) / sr + 0.2)
check(0.4 < ratio < 0.6, 'Syn 3 volume 50%% is %.2f of 100%%' % ratio)

# Slide: C3 then C4 on the next steps, back to back
slide_notes = 'tap A\ntap RIGHT\nhold R\ntap R+UP\nhold none\ntap A\ntap RIGHT\ntap A\ntap RIGHT\ntap A\n'
run(voice_script(0, {'slide': 7}, slide_notes, 'wav OUT/v_slide.wav\ntap B\nrun 40\nendwav\n'))
xl, _ = load_wav('v_slide.wav')
b = start_of(xl) / sr
step = 60 / 120 / 4
mid, end = pitch(xl, sr, b + step + 0.10, 0.03), pitch(xl, sr, b + step + 0.38, 0.03)
check(midi_hz(48) * 1.15 < mid < midi_hz(60) * 0.9 and abs(end / midi_hz(60) - 1) < 0.02,
      'slide 7 glides from C3 to C4: %.0f Hz on the way, %.0f Hz at the end' % (mid, end))

print('voices are saved')
save = os.path.join(OUT, 'voices.sav')
run(voice_script(0, {'tone': -2, 'release': -7}) + 'tap START\ntap A\nrun 10\n', save)
peeks = run('run 30\ntap START\nrun 10\n' + ''.join('peek8 %d\n' % (VOICE + i) for i in range(16)), save)
check(peeks[:8] == [15, 0, 0, 7, 0, 0, 0, 7] and peeks[8:] == [15, 1, 0, 7, 7, 0, 0, 7],
      'Bass voice survives a power cycle: %s' % peeks[:8])
v1 = os.path.join(OUT, 'v1.sav')
open(v1, 'wb').write(open(os.path.join(ROOT, 'tests', 'fixtures', 'v1_demo.sav'), 'rb').read())
peeks = run('run 30\ntap START\nrun 10\npeek8 %d\n' % song + ''.join('peek8 %d\n' % (VOICE + i) for i in range(24)), v1)
check(peeks[0] == 118 and peeks[1:] == [15, 2, 0, 7, 7, 0, 0, 7, 15, 1, 0, 7, 7, 0, 0, 7, 4, 0, 0, 0, 0, 0, 0, 7],
      'a version 1 save still loads, with the original sounds as voices')

print('synth pitches')
run(script('pitches.txt'))
for name, note in (('bass', 48), ('syn2', 64), ('syn3', 69)):
    x, sr = load_wav(name + '.wav')
    start = np.nonzero(np.abs(x) > 0.02)[0]
    check(len(start) > 0, '%s makes a sound' % name)
    if len(start):
        f = pitch(x, sr, start[0] / sr + 0.02)
        check(abs(f / midi_hz(note) - 1) < 0.01, '%s plays MIDI %d: %.1f Hz (want %.1f)' % (name, note, f, midi_hz(note)))
x, sr = load_wav('kick.wav')
check(np.abs(x).max() > 0.2, 'kick drum is audible (peak %.2f)' % np.abs(x).max())

print('frame rate on each screen')
vb, lc = symbol('vblank_count'), symbol('loop_count')
measure = 'run 10\npeek32 %d\npeek32 %d\nrun 120\npeek32 %d\npeek32 %d\n' % (vb, lc, vb, lc)
peeks = run('run 30\ntap START\n' + ''.join(measure + 'tap SELECT\n' for _ in range(5)) + 'tap START\n' + measure)
peeks += run('run 30\ntap START\ntap L\ntap L\ntap L\n' + measure)
names = ['Bass', 'Syn 2', 'Syn 3', 'Drums', 'Sequence', 'Settings', 'Voice panel']
for i, name in enumerate(names):
    v0, l0, v1, l1 = peeks[i * 4:i * 4 + 4]
    fps = 60 * (l1 - l0) / (v1 - v0)
    check(fps > 58, '%s screen draws at %.0f fps' % (name, fps))

print('\n%d failure(s)' % len(failures) if failures else '\nall passed')
sys.exit(1 if failures else 0)
