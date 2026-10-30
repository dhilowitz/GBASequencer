// The three synth voices on the PSG: square 1 (Bass), square 2 (Syn 2) and the wave channel (Syn 3),
// shaped by each song's SynthVoice settings. Everything here runs in the sound interrupts.
#pragma once
#include "gba.h"
#include "song.h"

void synth_init(void);
void synth_note_on(int synth, int note);
void synth_note_off(int synth);       // the step after a note is empty: start the release
void synth_tick(void);                // once per frame: envelopes, slide, vibrato
void synth_stop_all(void);
void synth_preview(int synth, int note, int frames);   // note on, then off after `frames`

#define NUM_WAVE_SHAPES 6
extern const char* const wave_shape_names[NUM_WAVE_SHAPES];
int wave_shape_sample(int shape, int i);   // 0-15, i = 0-31

// The envelope as it will sound, level 0-15 for each frame, for a note released at `release_frame`.
// Used to draw the envelope on the voice panel.
void voice_levels(int synth, int release_frame, u8* out, int frames);
