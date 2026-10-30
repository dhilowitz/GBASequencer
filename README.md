# GBA Sequencer

A step sequencer for the Game Boy Advance: three synths, a sampled drum kit, sixteen patterns per
track, and a song arrangement screen. Started in 2004 on top of Loirak's "Tank" sample and the
DevKit Advance toolchain, left unfinished in 2006, finished in 2026 with the same art and screens.

- **Bass, Syn 2**: the two square-wave channels
- **Syn 3**: the wave channel
- Each synth's sound is set on its VOICE panel (below) and saved with the song
- **Drums**: 13 samples (two kicks, two snares, rim, clap, three toms, open and closed hats, ride, crash) mixed through DirectSound. A closed hat cuts off an open one
- **Sequence**: which pattern each track plays at each of 64 song positions, and the tempo (40-250 BPM)
- **Settings**: save the song to the cartridge's battery-backed SRAM, load it, or start a new one. A saved song loads at power-on

Patterns are 1-64 steps of sixteenth notes; each track's patterns can have different lengths.

## Controls

| Everywhere | |
|---|---|
| SELECT | next screen: Bass, Syn 2, Syn 3, Drums, Sequence |
| START | Settings (START again to go back) |
| B | play / stop. On a pattern screen it loops the pattern shown on each of the four pattern screens; on the Sequence screen it plays the song from the cursor |
| L | next field (a red box shows the active one) |

| Pattern screens (Bass, Syn 2, Syn 3, Drums) | |
|---|---|
| Grid: d-pad | move. R + left/right jumps 16 steps, R + up/down jumps an octave |
| Grid: A | put a note at the cursor, or remove it if it's already there. You hear the note when stopped |
| Patterns (A-H, 1-8): d-pad | choose the pattern to edit. A blue dot marks patterns with notes in them |
| Patterns: A / R + A | copy this pattern / paste the copy over it (same track only) |
| Length: up/down, left/right | change by 1 / by 4 |

| VOICE panel (L to the screen title on Bass, Syn 2, Syn 3) | |
|---|---|
| Up/down | pick a setting |
| Left/right | change it. When stopped, you hear the note under the grid cursor each time |
| A | play that note |

| Setting | Bass and Syn 2 (square waves) | Syn 3 (wave channel) |
|---|---|---|
| Volume | 0-15 | Off, 25%, 50%, 75%, 100% |
| Tone | pulse width 12.5%, 25%, 50%, 75% | Saw, Triangle, Square, Sine, Organ, Thin |
| Attack | Off, or fade in over 1-7 (longer as it goes up) | same, in 25% volume steps |
| Decay | Hold, or fade out while the note is held | same |
| Release | at the next empty step: Cut, or fade out over 1-7 | same |
| Vibrato | Off, or depth 1-7 (up to about a semitone) | same |
| Slide | Off, or glide time 1-7 when notes are on consecutive steps (the note isn't restarted) | same |
| Sweep | Bass only: the hardware pitch sweep, Down 7 to Up 7 | |

The square channels have one hardware fade per note, so an attack followed by a decay restarts
the note once at the top of the attack, which can click slightly. The graphs on the panel show the
tone's waveform and the envelope of a note held for half a second.

| Sequence screen | |
|---|---|
| Grid: A | put the track's current pattern (the one picked on its pattern screen) in the cell, or empty it |
| Grid: R + up/down | change the pattern in the cell |
| Grid: R + left/right | jump 16 positions |
| Tempo: up/down, left/right | change by 1 / by 10 |

The song plays up to the last filled position (marked with a red line), then loops.

## Building

Any `arm-none-eabi-gcc` works; there's no devkitPro dependency and no C library.

    make                  # builds gbasequencer.gba

The art, font, note tables and drum samples are generated into `src/gen/` (committed) from
`gfx/*.pcx` and `samples/*.wav` by `tools/mkassets.py`. To regenerate, with Python 3, Pillow and
numpy (`tools/requirements.txt`):

    make assets

### Real hardware

Emulators don't care about the Nintendo logo in the cartridge header, but a real GBA won't boot
without it, and it isn't in this repo. Copy it from any ROM you have before flashing:

    python3 tools/gbafix.py gbasequencer.gba --logo-from some-other.gba

## Testing

`tests/check.py` runs the ROM headless in mGBA, presses buttons from the scripts in
`tests/scripts/`, and checks the recorded audio (step timing, synths in time with drums, pitches),
memory (save and reload), and frame rate. Screenshots and recordings land in `tests/out/`.

    tools/harness/build.sh   # once: builds the runner against an mGBA 0.10.5 checkout in ~/SDKs/mgba
    make test

The runner (`tools/harness/harness`) also works by hand: `tools/harness/harness gbasequencer.gba < script`,
with the commands listed at the top of `tools/harness/harness.c`.

## How it works

- `src/main.c`: main loop and screen switching. Mode 4 (240x160, 256 colours), double buffered,
  redrawn every frame at 60 fps
- `src/editor.c`, `src/arrange.c`, `src/settings.c`: the screens
- `src/audio.c`: playback. Drums are mixed into DirectSound A at 18157 Hz, exactly 304 samples per
  frame, in the VBlank interrupt. The step clock counts samples, so steps land on the exact sample.
  Synth notes due part-way through a frame are written to the sound registers by a timer interrupt
  at that moment (mixing runs with interrupts re-enabled so the timer isn't held up), which keeps
  them within a fraction of a millisecond of the drums
- `src/synth.c`: the synth voices: envelopes, slide and vibrato run once per frame; the panel
  that edits them is `src/voice_panel.c`
- `src/song.c`: the song data and the SRAM save format (`GBSQ` header, version, size, checksum).
  Version 2 added the voices; version 1 saves still load, with the original sounds
- `original/`: the 2004-2006 sources and project files as they were, for reference. They don't
  build as-is (they expect DevKit Advance and `gfx\` paths on Windows)

## License

MIT; see `LICENSE`. `original/crt0.S` is Jeff Frohwein's, released into the public domain.
