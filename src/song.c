#include "song.h"

Song song;

// Tells emulators and flash carts that this cartridge has 32 KB of battery-backed SRAM.
__attribute__((used)) static const char save_type[] = "SRAM_V113";

// The sounds the sequencer had before voices could be edited.
static const SynthVoice default_voices[NUM_SYNTHS] = {
	{ { 15, 2, 0, 7, 7, 0, 0, SWEEP_OFF } },   // Bass: 50% pulse, fades out
	{ { 15, 1, 0, 7, 7, 0, 0, SWEEP_OFF } },   // Syn 2: 25% pulse, fades out
	{ { 4, 0, 0, 0, 0, 0, 0, SWEEP_OFF } },    // Syn 3: sawtooth at full volume, held, cut at a rest
};

void song_default_voices(void) {
	for (int s = 0; s < NUM_SYNTHS; ++s) song.voice[s] = default_voices[s];
}

int voice_param_max(int synth, int param) {
	switch (param) {
	case V_VOLUME: return synth == 2 ? 4 : 15;
	case V_TONE: return synth == 2 ? 5 : 3;
	case V_SWEEP: return synth == 0 ? 14 : SWEEP_OFF;
	default: return 7;
	}
}

void song_clear(void) {
	u8* p = (u8*)&song;
	for (unsigned i = 0; i < sizeof(song); ++i) p[i] = 0;
	song.tempo = 120;
	for (int t = 0; t < NUM_TRACKS; ++t)
		for (int i = 0; i < SONG_LENGTH; ++i) song.order[t][i] = NO_PATTERN;
	for (int p = 0; p < NUM_PATTERNS; ++p) {
		for (int s = 0; s < NUM_SYNTHS; ++s) song.synth[s][p].length = DEFAULT_STEPS;
		song.drums[p].length = DEFAULT_STEPS;
	}
	song_default_voices();
}

int song_pattern_length(int track, int pattern) {
	return track == TRACK_DRUMS ? song.drums[pattern].length : song.synth[track][pattern].length;
}

int song_pattern_is_empty(int track, int pattern) {
	for (int i = 0; i < MAX_STEPS; ++i) {
		if (track == TRACK_DRUMS ? song.drums[pattern].hits[i] : song.synth[track][pattern].note[i])
			return 0;
	}
	return 1;
}

int song_last_position(void) {
	for (int i = SONG_LENGTH - 1; i >= 0; --i)
		for (int t = 0; t < NUM_TRACKS; ++t)
			if (song.order[t][i] != NO_PATTERN) return i;
	return -1;
}

char song_pattern_name(int pattern) {
	return pattern < 8 ? 'A' + pattern : '1' + pattern - 8;
}

// SRAM layout: "GBSQ", format version, song size (2 bytes), the song, then a 16-bit checksum.
// SRAM has an 8-bit bus, so everything goes through byte accesses.
// Version 1 is the same minus the voices at the end of Song; it loads with the default voices.
#define SAVE_VERSION 2
#define SAVE_HEADER 8
#define V1_SIZE (sizeof(Song) - sizeof(song.voice))

static u16 checksum(const u8* p, unsigned n) {
	u16 sum = 0x5A5A;
	for (unsigned i = 0; i < n; ++i) sum = (u16)((sum << 1 | sum >> 15) + p[i]);
	return sum;
}

int song_save(void) {
	const u8* p = (const u8*)&song;
	u16 size = sizeof(song), sum = checksum(p, size);
	const u8 header[SAVE_HEADER] = { 'G', 'B', 'S', 'Q', SAVE_VERSION, 0, size & 0xFF, size >> 8 };
	for (int i = 0; i < SAVE_HEADER; ++i) SRAM[i] = header[i];
	for (unsigned i = 0; i < size; ++i) SRAM[SAVE_HEADER + i] = p[i];
	SRAM[SAVE_HEADER + size] = sum & 0xFF;
	SRAM[SAVE_HEADER + size + 1] = sum >> 8;
	// read it back
	for (unsigned i = 0; i < size; ++i)
		if (SRAM[SAVE_HEADER + i] != p[i]) return 0;
	return 1;
}

// The saved song's size if the save is valid, otherwise 0.
static unsigned saved_size(void) {
	if (SRAM[0] != 'G' || SRAM[1] != 'B' || SRAM[2] != 'S' || SRAM[3] != 'Q') return 0;
	unsigned size = SRAM[6] | SRAM[7] << 8;
	if (!(SRAM[4] == SAVE_VERSION && size == sizeof(song)) && !(SRAM[4] == 1 && size == V1_SIZE)) return 0;
	u16 sum = 0x5A5A;
	for (unsigned i = 0; i < size; ++i) sum = (u16)((sum << 1 | sum >> 15) + SRAM[SAVE_HEADER + i]);
	return (SRAM[SAVE_HEADER + size] | SRAM[SAVE_HEADER + size + 1] << 8) == sum ? size : 0;
}

int song_has_save(void) {
	return saved_size() != 0;
}

// Keep a loaded song within the ranges the editor and player assume.
static void sanitize(void) {
	if (song.tempo < MIN_TEMPO || song.tempo > MAX_TEMPO) song.tempo = 120;
	for (int t = 0; t < NUM_TRACKS; ++t)
		for (int i = 0; i < SONG_LENGTH; ++i)
			if (song.order[t][i] >= NUM_PATTERNS) song.order[t][i] = NO_PATTERN;
	for (int p = 0; p < NUM_PATTERNS; ++p) {
		for (int s = 0; s < NUM_SYNTHS; ++s) {
			SynthPattern* sp = &song.synth[s][p];
			if (sp->length < 1 || sp->length > MAX_STEPS) sp->length = DEFAULT_STEPS;
			for (int i = 0; i < MAX_STEPS; ++i)
				if (sp->note[i] != NO_NOTE && (sp->note[i] < MIN_NOTE || sp->note[i] > MAX_NOTE))
					sp->note[i] = NO_NOTE;
		}
		DrumPattern* dp = &song.drums[p];
		if (dp->length < 1 || dp->length > MAX_STEPS) dp->length = DEFAULT_STEPS;
		for (int i = 0; i < MAX_STEPS; ++i) dp->hits[i] &= (1 << NUM_DRUMS) - 1;
	}
	for (int s = 0; s < NUM_SYNTHS; ++s)
		for (int i = 0; i < NUM_VOICE_PARAMS; ++i)
			if (song.voice[s].param[i] > voice_param_max(s, i)) song.voice[s] = default_voices[s];
	song.voice[1].param[V_SWEEP] = song.voice[2].param[V_SWEEP] = SWEEP_OFF;
}

int song_load(void) {
	unsigned size = saved_size();
	if (!size) return 0;
	u8* p = (u8*)&song;
	for (unsigned i = 0; i < size; ++i) p[i] = SRAM[SAVE_HEADER + i];
	if (size == V1_SIZE) song_default_voices();
	sanitize();
	return 1;
}
