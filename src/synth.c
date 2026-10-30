#include "synth.h"
#include "gen/assets.h"

// The square channels have one hardware envelope per note: a start volume and a fade up or down
// at n/64 s per volume step. Attack is a fade up from 0, then a retrigger at the target volume to
// start the decay (or hold). The wave channel has only five volume settings and no envelope, so
// its envelope runs here in software on the same 0-15 scale and timing.

enum { PHASE_OFF, PHASE_ATTACK, PHASE_HELD, PHASE_RELEASE };

typedef struct {
	u8 gate;            // a note is held
	u8 phase;
	u8 level;           // envelope level 0-15 (for the squares, our model of the hardware's)
	u8 target;          // ATTACK: level to stop at
	s8 dir;             // envelope direction: +1, -1, or 0 to hold
	u8 rate;            // n/64 s per level step, 0 = no change
	u16 clock;          // time since the last step, in 1/1024 of 1/64 s
	s16 pitch;          // in 1/16 semitones
	s16 target_pitch;
	s16 slide_step;     // per frame, while sliding
	u8 vib_phase;
	u8 preview_frames;
	u16 freq;           // last frequency written to the hardware
} SynthState;

static SynthState state[NUM_SYNTHS];
static u8 wave_bank, wave_loaded = 0xFF;

#define CLOCK_PER_FRAME 1097   // 1024 * 64 Hz / 59.73 Hz
#define SLIDE_FRAMES 3         // frames per V_SLIDE unit
#define VIBRATO_SPEED 28       // phase steps per frame of 256: about 6.5 Hz

const char* const wave_shape_names[NUM_WAVE_SHAPES] = { "Saw", "Triangle", "Square", "Sine", "Organ", "Thin" };
static const u8 wave_shapes[NUM_WAVE_SHAPES][32] = {
	{ 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 14, 13, 12, 11, 10, 9, 8, 8, 7, 6, 5, 4, 3, 2, 1 },
	{ 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 8, 9, 10, 12, 13, 14, 14, 15, 15, 15, 14, 14, 13, 12, 10, 9, 8, 6, 5, 3, 2, 1, 1, 0, 0, 0, 1, 1, 2, 3, 5, 6 },
	{ 8, 11, 14, 15, 15, 14, 14, 14, 14, 14, 13, 11, 9, 7, 6, 6, 7, 9, 9, 8, 6, 4, 2, 1, 1, 1, 1, 1, 0, 0, 1, 4 },
	{ 15, 15, 15, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

int wave_shape_sample(int shape, int i) { return wave_shapes[shape][i & 31]; }

static const u8* voice(int s) { return song.voice[s].param; }

// ---- hardware ---------------------------------------------------------------------------------

// Frequency register value for a pitch in 1/16 semitones, interpolated between semitones.
IWRAM_CODE static u16 freq_at(int synth, int pitch) {
	const u16* table = synth == 2 ? wave_freq : square_freq;
	int n = pitch >> 4, f = pitch & 15;
	if (n < 0) return table[0];
	if (n >= 127) return table[127];
	return table[n] + ((table[n + 1] - table[n]) * f >> 4);
}

// Start a square's hardware envelope (retriggers the note).
IWRAM_CODE static void square_start(int s, int level, int dir, int rate) {
	SynthState* st = &state[s];
	u16 env = level << 12 | (dir > 0 ? 0x0800 : 0) | (dir ? rate : 0) << 8 | voice(s)[V_TONE] << 6;
	st->level = level;
	st->dir = rate ? dir : 0;
	st->rate = rate;
	st->clock = 0;
	if (s == 0) {
		int sw = voice(0)[V_SWEEP];
		// sweep amount 1-7 = sweep time 7-1 (each 1/128 s), shift 3: about 2 semitones per step
		REG_SOUND1CNT_L = sw == SWEEP_OFF ? 0 : (sw > SWEEP_OFF ? 15 - sw : sw + 1) << 4 | (sw < SWEEP_OFF ? 8 : 0) | 3;
		REG_SOUND1CNT_H = env;
		REG_SOUND1CNT_X = 0x8000 | st->freq;
	} else {
		REG_SOUND2CNT_L = env;
		REG_SOUND2CNT_H = 0x8000 | st->freq;
	}
}

IWRAM_CODE static void wave_volume(int level) {
	static const u16 codes[5] = { 0x0000, 0x6000, 0x4000, 0x8000, 0x2000 };  // off, 25, 50, 75, 100%
	REG_SOUND3CNT_H = codes[(level * 4 + 7) / 15];
}

IWRAM_CODE static void wave_load(int shape) {
	if (shape == wave_loaded) return;
	// wave RAM writes go to the bank that isn't playing; then switch to it
	for (int i = 0; i < 4; ++i) {
		const u8* w = &wave_shapes[shape][i * 8];
		u32 word = 0;
		for (int b = 0; b < 4; ++b) word |= (u32)(w[b * 2] << 4 | w[b * 2 + 1]) << (b * 8);
		WAVE_RAM[i] = word;
	}
	wave_bank ^= 1;
	wave_loaded = shape;
}

IWRAM_CODE static void write_freq(int s, int retrigger) {
	u16 f = state[s].freq | (retrigger ? 0x8000 : 0);
	if (s == 0) REG_SOUND1CNT_X = f;
	else if (s == 1) REG_SOUND2CNT_H = f;
	else REG_SOUND3CNT_X = f;
}

IWRAM_CODE static void cut(int s) {
	SynthState* st = &state[s];
	st->phase = PHASE_OFF;
	st->level = 0;
	st->dir = 0;
	if (s == 0) { REG_SOUND1CNT_H = 0; REG_SOUND1CNT_X = 0x8000; }
	else if (s == 1) { REG_SOUND2CNT_L = 0; REG_SOUND2CNT_H = 0x8000; }
	else REG_SOUND3CNT_L = wave_bank << 6;
}

// ---- notes ------------------------------------------------------------------------------------

IWRAM_CODE void synth_note_on(int s, int note) {
	SynthState* st = &state[s];
	const u8* v = voice(s);
	st->preview_frames = 0;
	if (v[V_SLIDE] && st->gate && st->phase != PHASE_OFF) {
		// back-to-back notes with slide on: glide there without restarting the note. The same note
		// again just continues the glide already under way.
		if (st->target_pitch == note << 4) return;
		st->target_pitch = note << 4;
		int frames = v[V_SLIDE] * SLIDE_FRAMES;
		st->slide_step = (st->target_pitch - st->pitch) / frames;
		if (!st->slide_step) st->slide_step = st->target_pitch > st->pitch ? 1 : -1;
		return;
	}
	st->gate = 1;
	st->target_pitch = note << 4;
	st->pitch = st->target_pitch;
	st->slide_step = 0;
	st->vib_phase = 0;
	st->freq = freq_at(s, st->pitch);

	int volume = s == 2 ? (v[V_VOLUME] * 15 + 2) / 4 : v[V_VOLUME];
	if (v[V_ATTACK] && volume) {
		st->phase = PHASE_ATTACK;
		st->target = volume;
		volume = 0;
	} else {
		st->phase = PHASE_HELD;
	}
	int dir = st->phase == PHASE_ATTACK ? 1 : v[V_DECAY] ? -1 : 0;
	int rate = st->phase == PHASE_ATTACK ? v[V_ATTACK] : v[V_DECAY];
	if (s == 2) {
		wave_load(v[V_TONE]);
		st->level = volume;
		st->dir = rate ? dir : 0;
		st->rate = rate;
		st->clock = 0;
		REG_SOUND3CNT_L = 0x80 | wave_bank << 6;
		wave_volume(volume);
		write_freq(2, 1);
	} else {
		square_start(s, volume, dir, rate);
	}
}

IWRAM_CODE void synth_note_off(int s) {
	SynthState* st = &state[s];
	if (!st->gate) return;
	st->gate = 0;
	if (st->phase == PHASE_OFF) return;
	int release = voice(s)[V_RELEASE];
	if (!release) {
		cut(s);
		return;
	}
	if (st->dir < 0 && st->rate == release) {
		st->phase = PHASE_RELEASE;   // already fading at this speed: nothing to change
		return;
	}
	st->phase = PHASE_RELEASE;
	if (s == 2) {
		st->dir = -1;
		st->rate = release;
		st->clock = 0;
	} else {
		square_start(s, st->level, -1, release);
	}
}

IWRAM_CODE void synth_preview(int s, int note, int frames) {
	synth_note_off(s);
	synth_note_on(s, note);
	state[s].preview_frames = frames;
}

// ---- per frame --------------------------------------------------------------------------------

IWRAM_CODE static void envelope_tick(int s) {
	SynthState* st = &state[s];
	if (st->phase == PHASE_OFF || !st->rate) return;
	st->clock += CLOCK_PER_FRAME;
	u16 period = st->rate * 1024;
	while (st->clock >= period) {
		st->clock -= period;
		int level = st->level + st->dir;
		st->level = level < 0 ? 0 : level > 15 ? 15 : level;
	}
	if (st->phase == PHASE_ATTACK && st->level >= st->target) {
		// reached the voice's volume: start the decay, or hold
		const u8* v = voice(s);
		st->phase = PHASE_HELD;
		st->level = st->target;
		int dir = v[V_DECAY] ? -1 : 0;
		if (s == 2) {
			st->dir = dir;
			st->rate = v[V_DECAY];
			st->clock = 0;
		} else {
			square_start(s, st->level, dir, v[V_DECAY]);
		}
	}
	if (s == 2) {
		wave_volume(st->level);
		if (!st->level && st->dir < 0) cut(2);
	} else if (!st->level && st->dir < 0) {
		st->phase = PHASE_OFF;   // the hardware has faded out by itself
	}
}

IWRAM_CODE static void pitch_tick(int s) {
	SynthState* st = &state[s];
	if (st->phase == PHASE_OFF) return;
	if (s == 0 && voice(0)[V_SWEEP] != SWEEP_OFF) return;   // the hardware sweep owns the pitch
	if (st->slide_step) {
		st->pitch += st->slide_step;
		if ((st->slide_step > 0) == (st->pitch >= st->target_pitch)) {
			st->pitch = st->target_pitch;
			st->slide_step = 0;
		}
	}
	int pitch = st->pitch, depth = voice(s)[V_VIBRATO];
	if (depth) {
		st->vib_phase += VIBRATO_SPEED;
		int tri = st->vib_phase < 128 ? st->vib_phase - 64 : 192 - st->vib_phase;   // -64..64
		pitch += tri * depth * 2 / 64;
	}
	u16 f = freq_at(s, pitch);
	if (f != st->freq) {
		st->freq = f;
		write_freq(s, 0);
	}
}

IWRAM_CODE void synth_tick(void) {
	for (int s = 0; s < NUM_SYNTHS; ++s) {
		SynthState* st = &state[s];
		if (st->preview_frames && --st->preview_frames == 0) synth_note_off(s);
		envelope_tick(s);
		pitch_tick(s);
	}
}

IWRAM_CODE void synth_stop_all(void) {
	for (int s = 0; s < NUM_SYNTHS; ++s) {
		cut(s);
		state[s].gate = 0;
		state[s].preview_frames = 0;
	}
}

void synth_init(void) {
	wave_bank = 0;
	wave_loaded = 0xFF;
	REG_SOUND3CNT_L = 0;
	synth_stop_all();
}

// ---- for drawing ------------------------------------------------------------------------------

void voice_levels(int s, int release_frame, u8* out, int frames) {
	const u8* v = voice(s);
	int volume = s == 2 ? (v[V_VOLUME] * 15 + 2) / 4 : v[V_VOLUME];
	int attacking = v[V_ATTACK] && volume;
	int level = attacking ? 0 : volume, dir = attacking ? 1 : v[V_DECAY] ? -1 : 0;
	int rate = attacking ? v[V_ATTACK] : v[V_DECAY], clock = 0;
	for (int f = 0; f < frames; ++f) {
		out[f] = level;
		if (f == release_frame) {
			if (!v[V_RELEASE]) {
				level = 0;
				rate = 0;
				continue;
			}
			attacking = 0;
			dir = -1;
			rate = v[V_RELEASE];
			clock = 0;
		}
		if (!rate) continue;
		clock += CLOCK_PER_FRAME;
		while (clock >= rate * 1024) {
			clock -= rate * 1024;
			level += dir;
			level = level < 0 ? 0 : level > 15 ? 15 : level;
		}
		if (attacking && level >= volume) {
			attacking = 0;
			level = volume;
			dir = v[V_DECAY] ? -1 : 0;
			rate = v[V_DECAY];
			clock = 0;
		}
	}
}
