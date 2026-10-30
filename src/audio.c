#include "audio.h"
#include "song.h"
#include "gen/assets.h"
#include "gen/drums.h"
#include "synth.h"
#include "ui.h"

#define MIX_LEN 304                 // samples per frame at 18157 Hz
#define CYCLES_PER_SAMPLE 924
#define STEP_FRAC 8                 // step clock is in 1/256 samples
// samples per 16th note = 18157 * 60 / (tempo * 4), in 1/256 samples
#define SAMPLES_PER_STEP(tempo) ((u32)(16777216u / CYCLES_PER_SAMPLE * 15u * 256u) / (tempo))

// DirectSound plays through a FIFO that holds 16-32 samples, so drums come out a little
// after they're mixed. Synth notes are written about that much later to match (16 was measured
// to line up best).
#define FIFO_LATENCY 16

#define EV_NONE 0
#define EV_OFF  0xFF

volatile PlayState play;
volatile u32 vblank_count;
u8 edit_pattern[4];

static s8 mix_buf[2][MIX_LEN] __attribute__((aligned(4)));
static s16 accum[MIX_LEN];
static int next_buf;               // the buffer being mixed, started at the next VBlank

void call_with_irqs(void (*fn)(void));  // irq.s

typedef struct { const s8* data; u32 pos, length; } Voice;
static Voice voices[NUM_DRUMS];

typedef struct { s16 offset; u8 note[NUM_SYNTHS]; } PsgEvent;
static PsgEvent next_event, timed_event;

static s32 until_step;             // 1/256 samples until the next step
static u16 pattern_counter;        // PLAY_PATTERNS: steps since start
static u8 song_step, column_length;

// requests from the main loop, picked up by the interrupt
static volatile u8 request_mode = PLAY_STOPPED, request_pos, request_pending;
static volatile s8 preview_note[NUM_SYNTHS], preview_drum = -1;
#define PREVIEW_FRAMES 20         // a previewed note is held this long, then released

IWRAM_CODE static void apply_event(const PsgEvent* ev) {
	for (int s = 0; s < NUM_SYNTHS; ++s) {
		if (ev->note[s] == EV_OFF) synth_note_off(s);
		else if (ev->note[s] != EV_NONE) synth_note_on(s, ev->note[s]);
	}
}

IWRAM_CODE static void trigger_drum(int d) {
	if (d == DRUM_CHAT) voices[DRUM_OHAT].data = 0;    // a closed hi-hat cuts off an open one
	voices[d].data = drum_samples[d].data;
	voices[d].length = drum_samples[d].length;
	voices[d].pos = 0;
}

IWRAM_CODE static int column_length_at(int pos) {
	int longest = 0;
	for (int t = 0; t < NUM_TRACKS; ++t) {
		int p = song.order[t][pos];
		if (p != NO_PATTERN && song_pattern_length(t, p) > longest) longest = song_pattern_length(t, p);
	}
	return longest ? longest : DEFAULT_STEPS;
}

// One sixteenth-note step: drums start at `offset` in the buffer being mixed, synth notes are
// queued for the same moment.
IWRAM_CODE static void sequencer_step(int offset) {
	int pattern[NUM_TRACKS], step[NUM_TRACKS];
	for (int t = 0; t < NUM_TRACKS; ++t) {
		if (play.mode == PLAY_SONG) {
			pattern[t] = song.order[t][play.song_pos];
			step[t] = pattern[t] == NO_PATTERN ? -1 : song_step % song_pattern_length(t, pattern[t]);
		} else {
			pattern[t] = edit_pattern[t];
			step[t] = pattern_counter % song_pattern_length(t, pattern[t]);
		}
	}

	next_event.offset = offset;
	for (int s = 0; s < NUM_SYNTHS; ++s) {
		int note = step[s] < 0 ? NO_NOTE : song.synth[s][pattern[s]].note[step[s]];
		// an empty step releases the note before it
		next_event.note[s] = note != NO_NOTE ? note : EV_OFF;
	}
	if (step[TRACK_DRUMS] >= 0) {
		u16 hits = song.drums[pattern[TRACK_DRUMS]].hits[step[TRACK_DRUMS]];
		for (int d = 0; d < NUM_DRUMS; ++d)
			if (hits & (1 << d)) trigger_drum(d);
	}
	for (int t = 0; t < NUM_TRACKS; ++t) {
		play.step[t] = step[t];
		play.pattern[t] = pattern[t] == NO_PATTERN ? 0 : pattern[t];
	}

	if (play.mode == PLAY_SONG) {
		if (++song_step >= column_length) {
			song_step = 0;
			int last = song_last_position();
			play.song_pos = play.song_pos >= last ? 0 : play.song_pos + 1;
			column_length = column_length_at(play.song_pos);
		}
	} else {
		++pattern_counter;
	}
}

IWRAM_CODE static void mix_voices(int from, int n) {
	for (int d = 0; d < NUM_DRUMS; ++d) {
		Voice* v = &voices[d];
		if (!v->data) continue;
		int count = v->length - v->pos;
		if (count > n) count = n;
		const s8* src = v->data + v->pos;
		s16* dst = accum + from;
		for (int i = 0; i < count; ++i) dst[i] += src[i];
		v->pos += count;
		if (v->pos >= v->length) v->data = 0;
	}
}

IWRAM_CODE static void mix(s8* out) {
	for (int i = 0; i < MIX_LEN; ++i) accum[i] = 0;
	next_event.offset = -1;
	u32 per_step = SAMPLES_PER_STEP(song.tempo);
	int pos = 0;
	while (pos < MIX_LEN) {
		int n = MIX_LEN - pos;
		if (play.mode != PLAY_STOPPED) {
			while (until_step <= 0) {
				sequencer_step(pos);
				until_step += per_step;
			}
			int to_step = (until_step + (1 << STEP_FRAC) - 1) >> STEP_FRAC;
			if (to_step < n) n = to_step;
			until_step -= n << STEP_FRAC;
		}
		mix_voices(pos, n);
		pos += n;
	}
	for (int i = 0; i < MIX_LEN; ++i) {
		int s = accum[i] * 5 >> 3;
		out[i] = s > 127 ? 127 : s < -128 ? -128 : s;
	}
}

IWRAM_CODE static void mix_next(void) {
	mix(mix_buf[next_buf]);
}

IWRAM_CODE static void stop_all(void) {
	synth_stop_all();
	for (int d = 0; d < NUM_DRUMS; ++d) voices[d].data = 0;
	REG_TM2CNT_H = 0;
	next_event.offset = timed_event.offset = -1;
}

IWRAM_CODE static void on_vblank(void) {
	// Start the buffer mixed last frame, then mix the next one.
	REG_DMA1CNT_H = 0;
	REG_DMA1SAD = (u32)mix_buf[next_buf];
	REG_DMA1CNT_H = DMA_ENABLE | DMA_START_FIFO | DMA_32 | DMA_REPEAT | DMA_DEST_FIXED;
	next_buf ^= 1;

	// Synth notes for the buffer that just started, at their offset into it.
	timed_event = next_event;
	if (timed_event.offset >= 0) {
		REG_TM2CNT_H = 0;
		REG_TM2CNT_L = 65536 - (timed_event.offset + FIFO_LATENCY) * CYCLES_PER_SAMPLE / 64;
		REG_TM2CNT_H = TIMER_ENABLE | TIMER_IRQ | TIMER_DIV64;
	}

	if (request_pending) {
		request_pending = 0;
		stop_all();
		play.mode = request_mode;
		if (request_mode == PLAY_SONG) {
			play.song_pos = request_pos;
			column_length = column_length_at(request_pos);
			song_step = 0;
		}
		pattern_counter = 0;
		until_step = 0;
		for (int t = 0; t < NUM_TRACKS; ++t) play.step[t] = -1;
	}
	if (play.mode == PLAY_STOPPED) {
		for (int s = 0; s < NUM_SYNTHS; ++s) {
			if (preview_note[s]) {
				synth_preview(s, preview_note[s], PREVIEW_FRAMES);
				preview_note[s] = 0;
			}
		}
		if (preview_drum >= 0) {
			trigger_drum(preview_drum);
			preview_drum = -1;
		}
	}

	synth_tick();
	call_with_irqs(mix_next);
}

IWRAM_CODE static void irq_handler(void) {
	u16 flags = REG_IF & REG_IE;
	REG_IF = flags;
	if (flags & IRQ_TIMER2) {
		REG_TM2CNT_H = 0;
		apply_event(&timed_event);
	}
	if (flags & IRQ_VBLANK) {
		++vblank_count;
		BIOS_IRQ_FLAGS |= IRQ_VBLANK;
		input_vblank();
		REG_IE = IRQ_TIMER2;          // while mixing, only the step timer may interrupt
		on_vblank();
		REG_IE = IRQ_VBLANK | IRQ_TIMER2;
	}
}

void audio_init(void) {
	REG_SOUNDCNT_X = 0x80;                  // sound on
	REG_SOUNDCNT_L = 0x7766;                // PSG channels 1-3 to both sides, volume 6 of 7 (headroom for drums)
	REG_SOUNDCNT_H = 0x0B06;                // PSG 100%, DirectSound A 100% to both sides, timer 0
	synth_init();

	REG_DMA1DAD = REG_FIFO_A;
	REG_TM0CNT_L = 65536 - CYCLES_PER_SAMPLE;
	REG_TM0CNT_H = TIMER_ENABLE;

	for (int t = 0; t < NUM_TRACKS; ++t) play.step[t] = -1;
	next_event.offset = timed_event.offset = -1;

	BIOS_IRQ_HANDLER = irq_handler;
	REG_DISPSTAT |= DISPSTAT_VBL_IRQ;
	REG_IE = IRQ_VBLANK | IRQ_TIMER2;
	REG_IME = 1;
}

static void request(int mode, int pos) {
	request_mode = mode;
	request_pos = pos;
	request_pending = 1;
}

void audio_play_patterns(void) { request(PLAY_PATTERNS, 0); }
void audio_play_song(int from) { request(PLAY_SONG, from); }
void audio_stop(void) { request(PLAY_STOPPED, 0); }
void audio_preview_note(int synth, int note) { preview_note[synth] = note; }
void audio_preview_drum(int drum) { preview_drum = drum; }
