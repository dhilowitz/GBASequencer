// The song: patterns for three synths and the drum kit, the arrangement, and the tempo.
#pragma once
#include "gba.h"
#include "gen/drums.h"

#define NUM_SYNTHS   3
#define NUM_TRACKS   4      // three synths, then drums
#define TRACK_DRUMS  3
#define NUM_PATTERNS 16     // named A-H and 1-8 on screen
#define MAX_STEPS    64
#define DEFAULT_STEPS 16
#define SONG_LENGTH  64     // arrangement positions
#define NO_NOTE      0
#define NO_PATTERN   0xFF

#define MIN_NOTE 36         // C2: the lowest note the square channels can play
#define MAX_NOTE 96         // C7

#define MIN_TEMPO 40
#define MAX_TEMPO 250

typedef struct {
	u8 length;
	u8 note[MAX_STEPS];     // MIDI note number, or NO_NOTE
} SynthPattern;

typedef struct {
	u8 length;
	u8 pad;
	u16 hits[MAX_STEPS];    // bit n = drum n (row n on the drum screen)
} DrumPattern;

// How a synth sounds. Times are 0-7: 0 is off/hold/cut, 1 is quickest, 7 slowest.
enum { V_VOLUME, V_TONE, V_ATTACK, V_DECAY, V_RELEASE, V_VIBRATO, V_SLIDE, V_SWEEP, NUM_VOICE_PARAMS };
typedef struct {
	u8 param[NUM_VOICE_PARAMS];
	// V_VOLUME   squares 0-15; wave 0-4 (off, 25%, 50%, 75%, 100%)
	// V_TONE     squares 0-3 (12.5%, 25%, 50%, 75% pulse); wave 0-5 (see wave_shapes in audio.c)
	// V_ATTACK   0 = start at full volume, 1-7 = fade in
	// V_DECAY    0 = hold while the note lasts, 1-7 = fade out while held
	// V_RELEASE  at the next empty step: 0 = cut, 1-7 = fade out
	// V_VIBRATO  0 = off, 1-7 = depth
	// V_SLIDE    0 = off, 1-7 = glide time from the previous note when notes are back to back
	// V_SWEEP    Bass only: 7 = off, 6..0 = sweep down by 1..7, 8..14 = sweep up by 1..7
} SynthVoice;
#define SWEEP_OFF 7

typedef struct {
	u8 tempo;
	u8 order[NUM_TRACKS][SONG_LENGTH];  // pattern index, or NO_PATTERN
	SynthPattern synth[NUM_SYNTHS][NUM_PATTERNS];
	DrumPattern drums[NUM_PATTERNS];
	SynthVoice voice[NUM_SYNTHS];       // added in save version 2
} Song;

extern Song song;

void song_clear(void);
void song_default_voices(void);
int voice_param_max(int synth, int param);
int song_pattern_length(int track, int pattern);
int song_pattern_is_empty(int track, int pattern);
int song_last_position(void);   // last arrangement column with any pattern in it, or -1
char song_pattern_name(int pattern);  // 'A'-'H', '1'-'8'

int song_save(void);   // 1 on success
int song_load(void);   // 1 on success, 0 if there's no valid save
int song_has_save(void);
