#!/usr/bin/env python3
"""Generate src/gen/assets.[ch] and src/gen/drums.[ch] from the original art and samples.

Run from the repo root:  python3 tools/mkassets.py
Needs Pillow and numpy (tools/requirements.txt). The output is committed, so building the ROM
only needs the ARM compiler.

Every image uses the 2004 art's shared palette: the 216-colour web-safe cube at indices 40-255,
plus a few flat colours added at indices 1-3 (see CUSTOM).
"""
import os
import sys
import wave

import numpy as np
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GFX = os.path.join(ROOT, 'gfx')
OUT = os.path.join(ROOT, 'src', 'gen')

WHITE, BLACK, LIGHT = 255, 40, 212


def cube(r, g, b):
    """Palette index of a web-safe colour, components 0-5."""
    return 40 + 36 * r + 6 * g + b


def load(name):
    im = Image.open(os.path.join(GFX, name + '.pcx'))
    assert im.mode == 'P'
    return im


# ---- palette ------------------------------------------------------------------------------------
master = load('ggrid')
pal = master.getpalette()[:768]
pal += [0] * (768 - len(pal))

# Flat colours for what the 2004 art could only dither out of the web-safe cube
TRACK, FRAME, SHADE = 1, 2, 3
CUSTOM = {TRACK: (232, 232, 232),   # scrollbar tracks, spare drum rows
          FRAME: (176, 160, 136),   # the warm grey page border
          SHADE: (200, 200, 200)}   # piano key edges
for i, rgb in CUSTOM.items():
    pal[i * 3:i * 3 + 3] = list(rgb)


def check_palette(im, name):
    p = im.getpalette()
    for i in set(im.getdata()):
        assert p[i * 3:i * 3 + 3] == pal[i * 3:i * 3 + 3], name + ' uses a different palette'


# ---- title font: the letters of the 2004 screen titles (BASS, SEQUENCE), plus A D G I M R T drawn
# to match ------------------------------------------------------------------------------------------
TITLE_GLYPHS = {
    'A': ['..####..', '.######.', '###..###', '##....##', '##....##', '########', '########', '##....##',
          '##....##', '##....##'],
    'B': ['#######.', '########', '##....##', '##....##', '#######.', '########', '##....##', '##....##',
          '########', '#######.'],
    'C': ['..####..', '.######.', '###..###', '##....#.', '##......', '##......', '##....#.', '###..###',
          '.######.', '..####..'],
    'D': ['######..', '#######.', '##...###', '##....##', '##....##', '##....##', '##....##', '##...###',
          '#######.', '######..'],
    'E': ['#######', '#######', '##.....', '##.....', '#######', '#######', '##.....', '##.....',
          '#######', '#######'],
    'G': ['..####..', '.######.', '###..###', '##......', '##......', '##.####.', '##.####.', '###..##.',
          '.######.', '..####..'],
    'I': ['######', '######', '..##..', '..##..', '..##..', '..##..', '..##..', '..##..', '######',
          '######'],
    'M': ['##.....##', '###...###', '####.####', '##.###.##', '##..#..##', '##.....##', '##.....##',
          '##.....##', '##.....##', '##.....##'],
    'N': ['##....##', '###...##', '####..##', '####..##', '##.##.##', '##.##.##', '##..####', '##..####',
          '##...###', '##....##'],
    'Q': ['..#####..', '.#######.', '###...###', '##.....##', '##.....##', '##.....##', '##..#..##',
          '###..####', '.#######.', '..####.#.'],
    'R': ['#######.', '########', '##....##', '##....##', '########', '#######.', '##..###.', '##...###',
          '##....##', '##....##'],
    'S': ['.#####.', '#######', '##...##', '##.....', '######.', '.######', '.....##', '##...##',
          '#######', '.#####.'],
    'T': ['########', '########', '...##...', '...##...', '...##...', '...##...', '...##...', '...##...',
          '...##...', '...##...'],
    'U': ['##....##', '##....##', '##....##', '##....##', '##....##', '##....##', '##....##', '###..###',
          '.######.', '..####..'],
}


def draw_title(img, text, right, top, spacing=2):
    """Draw text in the screen-title style with its right edge at `right`."""
    width = sum(5 if ch == ' ' else len(TITLE_GLYPHS[ch][0]) + spacing for ch in text) - spacing
    x = right - width
    for ch in text:
        if ch == ' ':
            x += 5
            continue
        rows = TITLE_GLYPHS[ch]
        for gy, row in enumerate(rows):
            for gx, px in enumerate(row):
                if px == '#':
                    img.putpixel((x + gx, top + gy), BLACK)
        x += len(rows[0]) + spacing


def clear_rect(img, x0, y0, x1, y1, colour=WHITE):
    ImageDraw.Draw(img).rectangle((x0, y0, x1 - 1, y1 - 1), fill=colour)


# ---- small font: the 2004 8x8 terminal font, plus a few glyphs it lacks ------------------------
font_img = load('AAZ_terminal_8')
check_palette(font_img, 'font')
# index in the font image for each character (from the original DrawText and the image itself)
FONT_ORDER = ' ' + 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' + 'abcdefghijklmnopqrstuvwxyz' + '0123456789' \
    + '!\x01#$%&,_./:;<=>?'
EXTRA = {
    '-': ['........', '........', '........', '.#####..', '........', '........', '........', '........'],
    '+': ['........', '...#....', '...#....', '.#####..', '...#....', '...#....', '........', '........'],
    '*': ['........', '.#.#.#..', '..###...', '.#####..', '..###...', '.#.#.#..', '........', '........'],
    '(': ['....#...', '...#....', '..#.....', '..#.....', '..#.....', '...#....', '....#...', '........'],
    ')': ['..#.....', '...#....', '....#...', '....#...', '....#...', '...#....', '..#.....', '........'],
    "'": ['...#....', '...#....', '..#.....', '........', '........', '........', '........', '........'],
}
font_rows = {}
for i, ch in enumerate(FONT_ORDER):
    rows = []
    for y in range(8):
        bits = 0
        for x in range(8):
            if font_img.getpixel((x, i * 8 + y)) == BLACK:
                bits |= 0x80 >> x
        rows.append(bits)
    font_rows[ch] = rows
for ch, pattern in EXTRA.items():
    font_rows[ch] = [sum(0x80 >> x for x in range(8) if r[x] == '#') for r in pattern]


def draw_text(img, x, y, text, colour=BLACK):
    for ch in text:
        for row, bits in enumerate(font_rows.get(ch, font_rows['?'])):
            for col in range(8):
                if bits & (0x80 >> col):
                    img.putpixel((x + col - 1, y + row), colour)
        x += 6


# ---- backgrounds --------------------------------------------------------------------------------
# The 2004 art was reduced to web-safe colours with dithering, which left the frame, scrollbars and
# piano keys speckled with pink, cyan and yellow pixels. Clean it up to flat colours.
SPECKLE = {
    cube(5, 5, 4): WHITE, cube(4, 4, 5): WHITE, cube(4, 5, 5): WHITE, cube(5, 4, 5): WHITE,
    cube(5, 4, 4): WHITE, cube(4, 5, 4): WHITE,                       # pastels
    cube(1, 2, 1): 83, cube(2, 2, 1): 83, cube(2, 1, 1): 83, cube(1, 1, 0): 83, cube(1, 0, 0): 83,
    cube(1, 0, 1): 83, cube(1, 2, 2): 83, cube(0, 0, 1): BLACK, cube(0, 1, 0): BLACK,  # tinted darks
    cube(3, 4, 3): 169, cube(3, 3, 4): 169, cube(4, 4, 3): LIGHT, cube(4, 3, 4): LIGHT,
    cube(4, 3, 2): 169, cube(4, 3, 3): 169, cube(3, 3, 2): 169,      # warm greys outside the boxes
}
# boxes whose warm colours are deliberate: pattern selector, LENGTH, TEMPO
KEEP_WARM = [(6, 4, 108, 24), (107, 4, 165, 24), (78, 5, 140, 22)]


def clean(im, scrollbar_seeds):
    out = im.copy()
    px = out.load()
    for y in range(160):
        for x in range(240):
            c = px[x, y]
            near_edge = x < 5 or x > 234 or y < 5 or y > 155 or (x > 218 and y < 28)
            kept = any(x0 <= x < x1 and y0 <= y < y1 for x0, y0, x1, y1 in KEEP_WARM)
            if near_edge and not kept and c not in (WHITE, BLACK):
                px[x, y] = FRAME      # the page border, one flat colour
            elif c in SPECKLE and not kept:
                px[x, y] = SPECKLE[c]
    # stray light-grey dots left inside white areas
    for y in range(1, 159):
        for x in range(1, 239):
            if px[x, y] == LIGHT and all(px[x + dx, y + dy] == WHITE for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                px[x, y] = WHITE
    # border pixels with no border neighbour are leftover dots
    for y in range(1, 159):
        for x in range(1, 239):
            if px[x, y] == FRAME and not any(px[x + dx, y + dy] == FRAME for dx in (-1, 0, 1) for dy in (-1, 0, 1) if dx or dy):
                px[x, y] = WHITE
    # scrollbar tracks: flat light grey inside their outlines
    for seed in scrollbar_seeds:
        ImageDraw.floodfill(out, seed, TRACK)
    return out


EDITOR_TRACKS = [(228, 80), (100, 151)]       # right and bottom scrollbars
SONG_TRACKS = [(100, 115)]
backgrounds = {}
for name, src in (('bg_syn1', 'ggrid'), ('bg_syn2', 'ggrid_syn2'), ('bg_syn3', 'ggrid_syn3'),
                  ('bg_song', 'ggrid_mixer')):
    im = load(src)
    check_palette(im, src)
    backgrounds[name] = clean(im, SONG_TRACKS if src == 'ggrid_mixer' else EDITOR_TRACKS)

# Song screen: the track label boxes were a dithered grey; make them flat
song_px = backgrounds['bg_song'].load()
for y0, y1 in ((29, 47), (48, 67), (68, 87), (88, 106)):
    for y in range(y0, y1):
        for x in range(6, 31):
            if song_px[x, y] in (WHITE, LIGHT):
                song_px[x, y] = TRACK

# Synth screens: the keyboard is drawn at run time (it scrolls), one key per grid row.
for name in ('bg_syn1', 'bg_syn2', 'bg_syn3'):
    clear_rect(backgrounds[name], 6, 28, 31, 147, WHITE)
    ImageDraw.Draw(backgrounds[name]).line((6, 146, 30, 146), fill=LIGHT)

# Drum screen: the synth layout with drum names where the keyboard was, and a DRUMS title.
DRUM_LABELS = ['CRS', 'RID', 'OHH', 'CHH', 'HT', 'MT', 'LT', 'CLP', 'RIM', 'SN2', 'SN1', 'BD2', 'BD1']
drums = clean(load('ggrid'), EDITOR_TRACKS)
clear_rect(drums, 176, 6, 234, 22)
draw_title(drums, 'DRUMS', 233, 9)
# Label column x 6..30. Grid row boundaries are at y = 28, 35, 43, ... 139, 146 (27 + 8n inside).
MINI = {  # 3x5 letters for the drum labels; the 8x8 font is too tall for the 6-7 px rows
    'B': ['##.', '#.#', '##.', '#.#', '##.'], 'C': ['.##', '#..', '#..', '#..', '.##'],
    'D': ['##.', '#.#', '#.#', '#.#', '##.'], 'H': ['#.#', '#.#', '###', '#.#', '#.#'],
    'I': ['###', '.#.', '.#.', '.#.', '###'], 'L': ['#..', '#..', '#..', '#..', '###'],
    'M': ['#.#', '###', '###', '#.#', '#.#'], 'N': ['##.', '#.#', '#.#', '#.#', '#.#'],
    'O': ['.#.', '#.#', '#.#', '#.#', '.#.'], 'P': ['##.', '#.#', '##.', '#..', '#..'],
    'R': ['##.', '#.#', '##.', '#.#', '#.#'], 'S': ['.##', '#..', '.#.', '..#', '##.'],
    'T': ['###', '.#.', '.#.', '.#.', '.#.'], '1': ['.#.', '##.', '.#.', '.#.', '###'],
    '2': ['##.', '..#', '.#.', '#..', '###'],
}


def row_top(r):
    return 28 if r == 0 else 27 + 8 * r


clear_rect(drums, 6, 28, 31, 147, LIGHT)
for row in range(16):
    y = 146 if row == 15 else row_top(row)
    ImageDraw.Draw(drums).line((6, y, 30, y), fill=BLACK)
    if row < len(DRUM_LABELS):
        text = DRUM_LABELS[row]
        x = 18 - (len(text) * 4 - 1) // 2
        ty = (row_top(row) + (146 if row == 14 else row_top(row + 1))) // 2 - 2
        for ch in text:
            for gy, line in enumerate(MINI[ch]):
                for gx, px in enumerate(line):
                    if px == '#':
                        drums.putpixel((x + gx, ty + gy), BLACK)
            x += 4
# the two spare rows under the kick drums are not playable: shade them
for row in (13, 14):
    y0, y1 = row_top(row) + 1, (146 if row == 14 else row_top(row + 1))
    clear_rect(drums, 6, y0, 31, y1, cube(3, 3, 3))
    for x in range(32, 224):
        for yy in range(y0, y1):
            if drums.getpixel((x, yy)) == WHITE:
                drums.putpixel((x, yy), TRACK)
backgrounds['bg_drums'] = drums

# Settings screen: the song screen's frame with everything inside cleared.
settings = backgrounds['bg_song'].copy()
clear_rect(settings, 5, 5, 224, 156)
clear_rect(settings, 224, 24, 236, 156)
draw_title(settings, 'SETTINGS', 220, 9)
backgrounds['bg_settings'] = settings

# Title screen: same frame, the program name and a prompt.
title = settings.copy()
clear_rect(title, 5, 5, 224, 30)
draw_title(title, 'GBA SEQUENCER', 120 + 60, 50)
draw_text(title, 120 - 6 * 9, 74, 'David Hilowitz 2004-2026')
draw_text(title, 120 - 6 * 6 + 3, 130, 'Press START')
backgrounds['bg_title'] = title

# VOICE panels: the parts that don't change, one per synth (Syn 2 and Syn 3 have no Sweep).
# Must match the layout in src/voice_panel.c.
PANEL_W, PANEL_H = 190, 112
VOICE_LABELS = ['Volume', 'Tone', 'Attack', 'Decay', 'Release', 'Vibrato', 'Slide', 'Sweep']
panels = {}
for s, title_text in enumerate(['BASS VOICE', 'SYN 2 VOICE', 'SYN 3 VOICE']):
    im = Image.new('P', (PANEL_W, PANEL_H), WHITE)
    d = ImageDraw.Draw(im)
    d.rectangle((0, 0, PANEL_W - 1, PANEL_H - 1), outline=FRAME)
    d.rectangle((1, 1, PANEL_W - 2, PANEL_H - 2), outline=FRAME)
    draw_text(im, 8, 6, title_text)
    for i, label in enumerate(VOICE_LABELS if s == 0 else VOICE_LABELS[:-1]):
        draw_text(im, 8, 18 + i * 11, label)
    panels['voice_panel_%d' % (s + 1)] = im

# ---- sprites ------------------------------------------------------------------------------------
cursor = load('oobjtank')
check_palette(cursor, 'cursor')


def c_bytes(data, per_line=24):
    lines = []
    for i in range(0, len(data), per_line):
        lines.append('\t' + ','.join(str(v) for v in data[i:i + per_line]) + ',')
    return '\n'.join(lines)


def c_image(name, im):
    data = list(im.getdata())
    return 'const u8 %s[%d] __attribute__((aligned(4))) = {\n%s\n};\n' % (name, len(data), c_bytes(data))


# ---- note frequency tables ----------------------------------------------------------------------
def midi_hz(n):
    return 440.0 * 2 ** ((n - 69) / 12.0)


# Square channels: f = 131072 / (2048 - x). Wave channel, 32-sample wave: f = 65536 / (2048 - x).
square_freq = [max(0, min(2047, round(2048 - 131072 / midi_hz(n)))) for n in range(128)]
wave_freq = [max(0, min(2047, round(2048 - 65536 / midi_hz(n)))) for n in range(128)]

# ---- write assets -------------------------------------------------------------------------------
os.makedirs(OUT, exist_ok=True)
font_chars = [chr(c) for c in range(32, 128)]
with open(os.path.join(OUT, 'assets.h'), 'w') as h:
    h.write('// Generated by tools/mkassets.py. Do not edit.\n#pragma once\n#include "../gba.h"\n\n')
    h.write('extern const u16 palette[256];\n')
    for name in backgrounds:
        h.write('extern const u8 %s[240 * 160];\n' % name)
    for name in panels:
        h.write('extern const u8 %s[%d * %d];\n' % (name, PANEL_W, PANEL_H))
    h.write('#define VOICE_PANEL_W %d\n#define VOICE_PANEL_H %d\n' % (PANEL_W, PANEL_H))
    h.write('extern const u8 cursor_sprite[%d * %d];\n' % cursor.size)
    h.write('#define CURSOR_W %d\n#define CURSOR_H %d\n' % cursor.size)
    h.write('#define C_TRACK %d\n#define C_FRAME %d\n#define C_SHADE %d\n' % (TRACK, FRAME, SHADE))
    h.write('extern const u8 font_glyphs[96][8];  // ASCII 32-127, one byte per row, MSB is x = -1\n')
    h.write('extern const u16 square_freq[128];\nextern const u16 wave_freq[128];\n')
with open(os.path.join(OUT, 'assets.c'), 'w') as c:
    c.write('// Generated by tools/mkassets.py. Do not edit.\n#include "assets.h"\n\n')
    pal16 = [(pal[i * 3] >> 3) | ((pal[i * 3 + 1] >> 3) << 5) | ((pal[i * 3 + 2] >> 3) << 10)
             for i in range(256)]
    c.write('const u16 palette[256] = {\n%s\n};\n\n' % c_bytes(pal16, 16))
    for name, im in backgrounds.items():
        c.write(c_image(name, im) + '\n')
    for name, im in panels.items():
        c.write(c_image(name, im) + '\n')
    c.write(c_image('cursor_sprite', cursor) + '\n')
    c.write('const u8 font_glyphs[96][8] = {\n')
    for ch in font_chars:
        rows = font_rows.get(ch, font_rows['?'] if ch not in ' ' else font_rows[' '])
        c.write('\t{%s},  // %r\n' % (','.join(str(r) for r in rows), ch))
    c.write('};\n\n')
    c.write('const u16 square_freq[128] = {\n%s\n};\n' % c_bytes(square_freq, 16))
    c.write('const u16 wave_freq[128] = {\n%s\n};\n' % c_bytes(wave_freq, 16))

# ---- drum samples -------------------------------------------------------------------------------
# DirectSound mixing rate: 16777216 / 924 = 18157 Hz, exactly 304 samples per frame.
RATE = 16777216 / 924
DRUMS = [  # same order as the rows on the drum screen, top to bottom
    ('crash', 'Crash.wav'), ('ride', 'Ride.wav'), ('ohat', 'Op_Hh.wav'), ('chat', 'Cl_Hh.wav'),
    ('htom', 'Hi_Tom.wav'), ('mtom', 'Mid_Tom.wav'), ('ltom', 'Low_Tom.wav'), ('clap', 'Clap.wav'),
    ('rim', 'Rimshot.wav'), ('snare2', 'Snare2.wav'), ('snare1', 'Snare1.wav'), ('bd2', 'BD2.wav'),
    ('bd1', 'BD1.wav'),
]
assert [d for d in DRUM_LABELS] and len(DRUMS) == len(DRUM_LABELS)


def resample(x, src_rate, dst_rate):
    """Windowed-sinc resampling with the cutoff just under the new Nyquist frequency."""
    ratio = dst_rate / src_rate
    n_out = int(len(x) * ratio)
    cutoff = 0.45 * min(1.0, ratio)
    taps = 32
    t = np.arange(n_out) / ratio
    base = np.floor(t).astype(int)
    out = np.zeros(n_out)
    for k in range(-taps + 1, taps + 1):
        idx = base + k
        valid = (idx >= 0) & (idx < len(x))
        d = t - idx
        w = 2 * cutoff * np.sinc(2 * cutoff * d) * (0.5 + 0.5 * np.cos(np.pi * d / taps))
        out[valid] += x[idx[valid]] * w[valid]
    return out


drum_data = []
for name, fname in DRUMS:
    w = wave.open(os.path.join(ROOT, 'samples', fname))
    assert w.getsampwidth() == 2
    raw = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(float) / 32768
    mono = raw.reshape(-1, w.getnchannels()).mean(axis=1)
    x = resample(mono, w.getframerate(), RATE)
    x /= max(1e-9, np.abs(x).max())
    # trim the tail once it stays below -48 dB, with a 10 ms fade
    loud = np.nonzero(np.abs(x) > 10 ** (-48 / 20))[0]
    x = x[:loud[-1] + 1]
    fade = min(len(x), int(RATE * 0.01))
    x[-fade:] *= np.linspace(1, 0, fade)
    s8 = np.clip(np.round(x * 120), -127, 127).astype(int)
    drum_data.append((name, list(s8)))

with open(os.path.join(OUT, 'drums.h'), 'w') as h:
    h.write('// Generated by tools/mkassets.py. Do not edit.\n#pragma once\n#include "../gba.h"\n\n')
    h.write('#define NUM_DRUMS %d\n' % len(DRUMS))
    h.write('typedef struct { const s8* data; u32 length; } DrumSample;\n')
    h.write('extern const DrumSample drum_samples[NUM_DRUMS];  // top row of the drum screen first\n')
    h.write('#define DRUM_OHAT 2\n#define DRUM_CHAT 3\n')
with open(os.path.join(OUT, 'drums.c'), 'w') as c:
    c.write('// Generated by tools/mkassets.py. Do not edit.\n')
    c.write('// %s at %.1f Hz, signed 8-bit, from samples/*.wav\n#include "drums.h"\n\n' % (
        ', '.join(n for n, _ in DRUMS), RATE))
    for name, data in drum_data:
        c.write('static const s8 drum_%s[%d] = {\n%s\n};\n' % (name, len(data), c_bytes(data, 32)))
    c.write('\nconst DrumSample drum_samples[NUM_DRUMS] = {\n')
    for name, data in drum_data:
        c.write('\t{ drum_%s, %d },\n' % (name, len(data)))
    c.write('};\n')

for name, im in backgrounds.items():
    if len(sys.argv) > 1:
        im.putpalette(pal)
        im.convert('RGB').resize((480, 320), Image.NEAREST).save(os.path.join(sys.argv[1], name + '.png'))
print('wrote', OUT, '; drums:', ', '.join('%s %.2fs' % (n, len(d) / RATE) for n, d in drum_data))
