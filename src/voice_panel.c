// The VOICE panel on the synth screens: volume, tone, envelope and effects for one synth.
// Up/down picks a setting, left/right changes it, A plays a note. When stopped, every change plays
// the note under the cursor so you hear it.
#include "audio.h"
#include "song.h"
#include "synth.h"
#include "ui.h"
#include "video.h"
#include "gen/assets.h"

static u8 selected[NUM_SYNTHS];

static const char* const labels[NUM_VOICE_PARAMS] = {
	"Volume", "Tone", "Attack", "Decay", "Release", "Vibrato", "Slide", "Sweep",
};

// Bass has the hardware pitch sweep; the others don't.
static int num_params(int synth) { return synth == 0 ? NUM_VOICE_PARAMS : V_SWEEP; }

static void value_text(int synth, int param, char* out) {
	static const char* const pulse[4] = { "12.5%", "25%", "50%", "75%" };
	static const char* const wave_volume[5] = { "Off", "25%", "50%", "75%", "100%" };
	int v = song.voice[synth].param[param];
	const char* fixed = 0;
	switch (param) {
	case V_VOLUME: if (synth == 2) fixed = wave_volume[v]; break;
	case V_TONE: fixed = synth == 2 ? wave_shape_names[v] : pulse[v]; break;
	case V_ATTACK: case V_VIBRATO: case V_SLIDE: if (!v) fixed = "Off"; break;
	case V_DECAY: if (!v) fixed = "Hold"; break;
	case V_RELEASE: if (!v) fixed = "Cut"; break;
	case V_SWEEP:
		if (v == SWEEP_OFF) {
			fixed = "Off";
		} else {
			const char* dir = v > SWEEP_OFF ? "Up " : "Down ";
			while (*dir) *out++ = *dir++;
			int_to_text(v > SWEEP_OFF ? v - SWEEP_OFF : SWEEP_OFF - v, out);
			return;
		}
		break;
	}
	if (fixed) {
		while (*fixed) *out++ = *fixed++;
		*out = 0;
	} else {
		int_to_text(v, out);
	}
}

void voice_panel_input(int synth, int note) {
	int n = num_params(synth);
	u8* sel = &selected[synth];
	if (*sel >= n) *sel = 0;
	if (keys.repeat & KEY_UP) *sel = (*sel + n - 1) % n;
	if (keys.repeat & KEY_DOWN) *sel = (*sel + 1) % n;
	int delta = (keys.repeat & KEY_RIGHT) ? 1 : (keys.repeat & KEY_LEFT) ? -1 : 0;
	u8* v = &song.voice[synth].param[*sel];
	int changed = 0;
	if (delta) {
		int nv = *v + delta, max = voice_param_max(synth, *sel);
		if (nv >= 0 && nv <= max) {
			*v = nv;
			changed = 1;
		}
	}
	if (changed || (keys.pressed & KEY_A)) audio_preview_note(synth, note);
}

// ---- drawing ----------------------------------------------------------------------------------

#define PANEL_X 34
#define PANEL_Y 30
#define PANEL_W 190
#define PANEL_H 112
#define ROW_Y   (PANEL_Y + 18)
#define ROW_H   11
#define GRAPH_X (PANEL_X + 130)
#define GRAPH_W 52

static void connect(int x, int y0, int y1, int c) {
	if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }
	vline(x, y0, y1 - y0 + 1, c);
}

// one cycle of the tone, twice, in a box
static void draw_shape(int synth, int x, int y, int h) {
	frame_rect(x - 1, y - 1, GRAPH_W + 2, h + 2, C_GREY);
	int tone = song.voice[synth].param[V_TONE];
	static const u8 duty_eighths[4] = { 1, 2, 4, 6 };
	int prev = -1;
	for (int i = 0; i < GRAPH_W; ++i) {
		int phase = (i * 64 / GRAPH_W) & 31;   // 0-31, two cycles across the box
		int s = synth == 2 ? wave_shape_sample(tone, phase) : (phase < duty_eighths[tone] * 4 ? 15 : 0);
		int py = y + h - 2 - s * (h - 4) / 15;
		if (prev >= 0) connect(x + i, prev, py, C_BLUE);
		else plot(x + i, py, C_BLUE);
		prev = py;
	}
}

// the envelope of a note held for half a second, then released
static void draw_envelope(int synth, int x, int y, int h) {
	frame_rect(x - 1, y - 1, GRAPH_W + 2, h + 2, C_GREY);
	enum { frames_per_px = 2, release_px = 24 };
	// recalculated only when the voice changes
	static u8 levels[GRAPH_W * frames_per_px], cached_synth = 0xFF;
	static SynthVoice cached_voice;
	const u8* v = song.voice[synth].param;
	int same = cached_synth == synth;
	for (int i = 0; i < NUM_VOICE_PARAMS && same; ++i) same = cached_voice.param[i] == v[i];
	if (!same) {
		voice_levels(synth, release_px * frames_per_px, levels, GRAPH_W * frames_per_px);
		cached_voice = song.voice[synth];
		cached_synth = synth;
	}
	vline(x + release_px, y, h, C_LIGHT);   // where the note is released
	int prev = -1;
	for (int i = 0; i < GRAPH_W; ++i) {
		int level = levels[i * frames_per_px];
		int py = y + h - 1 - level * (h - 2) / 15;
		if (prev >= 0) connect(x + i, prev, py, C_BLUE);
		else plot(x + i, py, C_BLUE);
		prev = py;
	}
}

void voice_panel_draw(int synth) {
	static const u8* const bases[NUM_SYNTHS] = { voice_panel_1, voice_panel_2, voice_panel_3 };
	fill_rect(PANEL_X + PANEL_W, PANEL_Y + 3, 3, PANEL_H, C_GREY);   // shadow
	fill_rect(PANEL_X + 3, PANEL_Y + PANEL_H, PANEL_W - 3, 3, C_GREY);
	blit_rect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, bases[synth]);   // box, title and labels

	char value[12];
	for (int i = 0; i < num_params(synth); ++i) {
		int y = ROW_Y + i * ROW_H;
		int sel = i == selected[synth];
		int c = sel ? C_WHITE : C_BLACK;
		if (sel) {
			fill_rect(PANEL_X + 4, y - 2, 120, ROW_H, C_BLUE);
			draw_text(PANEL_X + 8, y, labels[i], c);
		}
		value_text(synth, i, value);
		draw_text(PANEL_X + 64, y, value, c);
		if (sel) {
			const u8* v = song.voice[synth].param;
			if (v[i] > 0) draw_text(PANEL_X + 56, y, "<", C_WHITE);
			if (v[i] < voice_param_max(synth, i)) draw_text(PANEL_X + 114, y, ">", C_WHITE);
		}
	}
	draw_shape(synth, GRAPH_X, ROW_Y, 26);
	draw_envelope(synth, GRAPH_X, ROW_Y + 36, 46);
}
