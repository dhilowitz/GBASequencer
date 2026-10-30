// Sound and playback. Everything time-critical runs in the VBlank interrupt.
//
// Drums are mixed in software into DirectSound A at 18157 Hz (304 samples per frame). The step
// clock counts in those samples, so steps land on the exact sample whatever the frame rate.
// Synth notes go to the PSG channels (synth.c): square 1, square 2 and wave. A step that falls part-way
// through a frame is written to the PSG by a timer interrupt at the matching moment, so synths
// and drums stay in time with each other.
#pragma once
#include "gba.h"

enum { PLAY_STOPPED, PLAY_PATTERNS, PLAY_SONG };

typedef struct {
	u8 mode;
	s8 step[4];         // step each track is playing, -1 if it's silent
	u8 pattern[4];      // pattern each track is playing
	u8 song_pos;        // arrangement column (PLAY_SONG)
} PlayState;

extern volatile PlayState play;

// The pattern each editor screen shows. PLAY_PATTERNS loops these together.
extern u8 edit_pattern[4];

void audio_init(void);
void audio_play_patterns(void);
void audio_play_song(int from);
void audio_stop(void);
void audio_preview_note(int synth, int note);  // when stopped: let the user hear a note they enter
void audio_preview_drum(int drum);
