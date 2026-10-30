// The pattern editors: three piano rolls (Bass, Syn 2, Syn 3) and the drum grid.
//
// Fields (L moves between them, a red box shows which one is active):
//   grid      d-pad moves, A sets or clears a note, R + d-pad jumps a page / an octave
//   patterns  d-pad picks which of the 16 patterns to edit; A copies it, R + A pastes
//   length    up/down change the pattern length by 1, left/right by 4
//   voice     (synths, on the screen title) opens the VOICE panel: see voice_panel.c
#include "audio.h"
#include "song.h"
#include "ui.h"
#include "video.h"
#include "gen/assets.h"

enum { FIELD_GRID, FIELD_PATTERNS, FIELD_LENGTH, FIELD_VOICE, NUM_FIELDS };

typedef struct {
	u8 field;
	u8 step;         // cursor column
	u8 row;          // cursor: note (synths) or drum (drums)
	u8 view_left;    // first visible step
	u8 view_top;     // synths: note in the top row
} Editor;

static Editor editors[NUM_TRACKS];

static struct {
	u8 track, valid;
	union { SynthPattern synth; DrumPattern drums; } data;
} clipboard;

static const u8* const backgrounds[NUM_TRACKS] = { bg_syn1, bg_syn2, bg_syn3, bg_drums };
static const char* const note_names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

#define DRUM_ROWS NUM_DRUMS

void editor_init(void) {
	static const u8 start_note[NUM_SYNTHS] = { 48, 60, 60 };  // C3 for the bass, C4 for the others
	for (int t = 0; t < NUM_SYNTHS; ++t) {
		editors[t].row = start_note[t];
		editors[t].view_top = start_note[t] + 7;
	}
	editors[TRACK_DRUMS].row = DRUM_ROWS - 1;  // kick
}

static int length_of(int track) { return song_pattern_length(track, edit_pattern[track]); }

static void keep_cursor_visible(int track) {
	Editor* e = &editors[track];
	int length = length_of(track);
	if (e->step >= length) e->step = length - 1;
	if (e->step < e->view_left) e->view_left = e->step;
	if (e->step >= e->view_left + GRID_COLS) e->view_left = e->step - GRID_COLS + 1;
	int max_left = length > GRID_COLS ? length - GRID_COLS : 0;
	if (e->view_left > max_left) e->view_left = max_left;
	if (track != TRACK_DRUMS) {
		if (e->row > e->view_top) e->view_top = e->row;
		if (e->row < e->view_top - (GRID_ROWS - 1)) e->view_top = e->row + GRID_ROWS - 1;
		if (e->view_top > MAX_NOTE) e->view_top = MAX_NOTE;
		if (e->view_top < MIN_NOTE + GRID_ROWS - 1) e->view_top = MIN_NOTE + GRID_ROWS - 1;
	}
}

static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static void grid_input(int track) {
	Editor* e = &editors[track];
	int shift = keys.held & KEY_R;
	int length = length_of(track);
	int dx = 0, dy = 0;
	if (keys.repeat & KEY_RIGHT) dx = shift ? GRID_COLS : 1;
	if (keys.repeat & KEY_LEFT) dx = shift ? -GRID_COLS : -1;
	if (keys.repeat & KEY_UP) dy = shift && track != TRACK_DRUMS ? 12 : 1;
	if (keys.repeat & KEY_DOWN) dy = shift && track != TRACK_DRUMS ? -12 : -1;
	e->step = clamp(e->step + dx, 0, length - 1);
	if (track == TRACK_DRUMS) e->row = clamp(e->row - dy, 0, DRUM_ROWS - 1);   // row 0 is at the top
	else e->row = clamp(e->row + dy, MIN_NOTE, MAX_NOTE);

	if (keys.pressed & KEY_A) {
		if (track == TRACK_DRUMS) {
			u16* hits = &song.drums[edit_pattern[track]].hits[e->step];
			*hits ^= 1 << e->row;
			if (*hits & (1 << e->row)) audio_preview_drum(e->row);
		} else {
			u8* note = &song.synth[track][edit_pattern[track]].note[e->step];
			*note = *note == e->row ? NO_NOTE : e->row;
			if (*note) audio_preview_note(track, *note);
		}
	}
}

static void pattern_input(int track) {
	int p = edit_pattern[track];
	if (keys.repeat & KEY_RIGHT) p = (p & 8) | ((p + 1) & 7);
	if (keys.repeat & KEY_LEFT) p = (p & 8) | ((p + 7) & 7);
	if (keys.repeat & (KEY_UP | KEY_DOWN)) p ^= 8;
	edit_pattern[track] = p;

	if (keys.pressed & KEY_A) {
		if (keys.held & KEY_R) {
			if (!clipboard.valid || clipboard.track != track) {
				show_status(clipboard.valid ? "Copied from another track" : "Nothing copied yet");
			} else {
				if (track == TRACK_DRUMS) song.drums[p] = clipboard.data.drums;
				else song.synth[track][p] = clipboard.data.synth;
				show_status("Pattern pasted");
			}
		} else {
			clipboard.track = track;
			clipboard.valid = 1;
			if (track == TRACK_DRUMS) clipboard.data.drums = song.drums[p];
			else clipboard.data.synth = song.synth[track][p];
			show_status("Pattern copied");
		}
	}
}

static void length_input(int track) {
	u8* length = track == TRACK_DRUMS ? &song.drums[edit_pattern[track]].length
	                                  : &song.synth[track][edit_pattern[track]].length;
	int v = *length;
	if (keys.repeat & KEY_UP) v += 1;
	if (keys.repeat & KEY_DOWN) v -= 1;
	if (keys.repeat & KEY_RIGHT) v += 4;
	if (keys.repeat & KEY_LEFT) v -= 4;
	*length = clamp(v, 1, MAX_STEPS);
}

void editor_input(int track) {
	Editor* e = &editors[track];
	int fields = track == TRACK_DRUMS ? FIELD_VOICE : NUM_FIELDS;
	if (keys.pressed & KEY_L) e->field = (e->field + 1) % fields;
	else if (e->field == FIELD_GRID) grid_input(track);
	else if (e->field == FIELD_PATTERNS) pattern_input(track);
	else if (e->field == FIELD_LENGTH) length_input(track);
	else voice_panel_input(track, e->row);
	keep_cursor_visible(track);
}

// ---- drawing --------------------------------------------------------------------------------

static void cell_rect(int col, int row, int* x, int* y, int* w, int* h) {
	*x = GRID_X + col * COL_W;
	*y = grid_row_top(row) + 1;
	*w = COL_W - 1;
	*h = grid_row_bottom(row) - *y;
}

static void fill_cell(int col, int row, int c) {
	int x, y, w, h;
	cell_rect(col, row, &x, &y, &w, &h);
	fill_rect(x, y, w, h, c);
}

#define C_PALE_BLUE 219  // #ccffff
#define C_NOTE_EDGE 50   // #0033cc, the darker edge that makes a note look like a tile

static void fill_note_colour(int col, int row, int c, int edge) {
	int x, y, w, h;
	cell_rect(col, row, &x, &y, &w, &h);
	fill_rect(x, y, w, h, c);
	hline(x, y + h - 1, w, edge);
	vline(x + w - 1, y, h, edge);
}

static void fill_note(int col, int row) { fill_note_colour(col, row, C_BLUE, C_NOTE_EDGE); }

// Drum hits by group: cymbals and hats green, toms orange, snares/clap/rim red, kicks blue.
// Rows top to bottom: CRS RID OHH CHH HT MT LT CLP RIM SN2 SN1 BD2 BD1
static const u8 drum_colour[NUM_DRUMS][2] = {
	{ 102, 95 }, { 102, 95 }, { 102, 95 }, { 102, 95 },
	{ 239, 196 }, { 239, 196 }, { 239, 196 },
	{ 191, 155 }, { 191, 155 }, { 191, 155 }, { 191, 155 },
	{ C_BLUE, C_NOTE_EDGE }, { C_BLUE, C_NOTE_EDGE },
};

// The row a note or drum is drawn in, or -1 if it's scrolled out of view.
static int row_of(int track, int value) {
	if (track == TRACK_DRUMS) return value;
	int row = editors[track].view_top - value;
	return row >= 0 && row < GRID_ROWS ? row : -1;
}

// One key per grid row, so a key and its row line up exactly. Black keys cover the left two thirds
// of their row; the white key behind them shows on the right.
#define KEYS_X      6
#define KEYS_W      25
#define BLACK_KEY_W 19

static int is_black(int note) {
	int s = note % 12;
	return s == 1 || s == 3 || s == 6 || s == 8 || s == 10;
}

static void note_name(int note, char* out) {
	const char* n = note_names[note % 12];
	while (*n) *out++ = *n++;
	*out++ = '0' + note / 12 - 1;
	*out = 0;
}

static void draw_keyboard(int track) {
	Editor* e = &editors[track];
	char name[5];
	for (int row = 0; row < GRID_ROWS; ++row) {
		int note = e->view_top - row;
		int line = grid_row_top(row), y = line + 1, h = grid_row_bottom(row) - y;
		int selected = note == e->row;
		// the line above this key: between two white keys (B|C, E|F) it crosses the whole keyboard
		if (row > 0) {
			if (!is_black(note) && !is_black(note + 1)) hline(KEYS_X, line, KEYS_W, C_GREY);
			else hline(KEYS_X + BLACK_KEY_W, line, KEYS_W - BLACK_KEY_W, C_SHADE);
		}
		if (is_black(note)) {
			fill_rect(KEYS_X, y, BLACK_KEY_W, h, selected ? C_BLUE : C_BLACK);
			if (!selected) vline(KEYS_X + BLACK_KEY_W - 1, y, h, C_DARK);
		} else if (selected) {
			fill_rect(KEYS_X, y, KEYS_W, h, C_BLUE);
		}
		if (selected) {
			note_name(note, name);
			int w = is_black(note) ? BLACK_KEY_W : KEYS_W;
			draw_text(KEYS_X + (w - text_width(name)) / 2 + 1, y + h - 7, name, C_WHITE);
		} else if (note % 12 == 0) {
			note_name(note, name);   // octave marker on each C
			draw_text(KEYS_X + KEYS_W - text_width(name) - 1, y + h - 7, name, C_GREY);
		}
	}
}

static void draw_pattern_selector(int track) {
	// patterns with notes get a pale blue box; the one playing is yellow, like the playing column
	for (int p = 0; p < NUM_PATTERNS; ++p) {
		int x = 9 + (p & 7) * 12, y = (p & 8) ? 15 : 7;
		int playing = play.mode != PLAY_STOPPED && play.step[track] >= 0 && play.pattern[track] == p;
		if (playing) recolor_rect(x, y, 11, 7, C_WHITE, C_YELLOW);
		else if (!song_pattern_is_empty(track, p)) recolor_rect(x, y, 11, 7, C_WHITE, C_PALE_BLUE);
	}
	int p = edit_pattern[track];
	frame_rect(8 + (p & 7) * 12, (p & 8) ? 14 : 6, 13, 9, C_RED);
}

static void draw_scrollbars(int track) {
	Editor* e = &editors[track];
	int length = length_of(track);
	// bottom: which 16 of the pattern's steps are on screen
	if (length > GRID_COLS) {
		int track_w = 216 - 38;
		int w = track_w * GRID_COLS / length;
		int x = 38 + (track_w - w) * e->view_left / (length - GRID_COLS);
		draw_thumb(x, 149, w, 5);
	}
	// right: where the visible octave range sits between C2 and C7
	if (track != TRACK_DRUMS) {
		int range = MAX_NOTE - MIN_NOTE + 1, track_h = 136 - 36;
		int h = track_h * GRID_ROWS / range;
		int y = 36 + (track_h - h) * (MAX_NOTE - e->view_top) / (range - GRID_ROWS);
		draw_thumb(227, y, 5, h);

		// arrows in the scrollbar's end boxes when notes are out of view above or below
		int above = 0, below = 0;
		const SynthPattern* sp = &song.synth[track][edit_pattern[track]];
		for (int s = 0; s < sp->length; ++s) {
			if (sp->note[s] > e->view_top) above = 1;
			else if (sp->note[s] && sp->note[s] < e->view_top - (GRID_ROWS - 1)) below = 1;
		}
		for (int i = 0; i < 3; ++i) {
			if (above) hline(229 - i, 30 + i, 1 + 2 * i, C_BLUE);
			if (below) hline(229 - i, 144 - i, 1 + 2 * i, C_BLUE);
		}
	}
}

static const u8 field_boxes[NUM_FIELDS][4] = {  // x, y, w, h
	{ 30, 26, 196, 122 }, { 6, 4, 100, 20 }, { 107, 4, 57, 20 }, { 176, 4, 62, 20 },
};

void editor_draw(int track) {
	Editor* e = &editors[track];
	int pattern = edit_pattern[track];
	int length = length_of(track);
	draw_background(backgrounds[track]);

	// the VOICE panel covers the grid, so the notes under it aren't drawn
	int panel = e->field == FIELD_VOICE;
	int playing = play.mode != PLAY_STOPPED && play.pattern[track] == pattern ? play.step[track] : -1;
	int rows = track == TRACK_DRUMS ? DRUM_ROWS : GRID_ROWS;
	for (int col = 0; col < (panel ? 0 : GRID_COLS); ++col) {
		int step = e->view_left + col;
		if (step >= length) {
			for (int row = 0; row < rows; ++row) fill_cell(col, row, C_TRACK);
			continue;
		}
		if (step == playing)
			for (int row = 0; row < rows; ++row) fill_cell(col, row, C_YELLOW);
		if (track == TRACK_DRUMS) {
			u16 hits = song.drums[pattern].hits[step];
			for (int d = 0; d < NUM_DRUMS; ++d)
				if (hits & (1 << d)) fill_note_colour(col, d, drum_colour[d][0], drum_colour[d][1]);
		} else {
			int note = song.synth[track][pattern].note[step];
			int row = note ? row_of(track, note) : -1;
			if (row >= 0) fill_note(col, row);
		}
	}
	if (!panel) draw_beat_lines(e->view_left, 29, 146);

	if (track != TRACK_DRUMS) draw_keyboard(track);
	int row = row_of(track, e->row);
	if (row >= 0 && !panel) {
		int col = e->step - e->view_left;
		int on_note = track == TRACK_DRUMS ? (song.drums[pattern].hits[e->step] >> e->row) & 1
		                                   : song.synth[track][pattern].note[e->step] == e->row;
		// the 2004 cursor is a black outline; over a blue note it turns white so it stays visible
		for (int j = 0; j < CURSOR_H; ++j)
			for (int i = 0; i < CURSOR_W; ++i)
				if (cursor_sprite[j * CURSOR_W + i] != C_WHITE)
					plot(GRID_X + col * COL_W - 1 + i, grid_row_top(row) + j, on_note ? C_WHITE : C_BLACK);
	}

	draw_pattern_selector(track);
	char text[4];
	int_to_text(length, text);
	draw_text(152 - text_width(text), 11, text, C_BLACK);
	draw_scrollbars(track);

	if (e->field == FIELD_VOICE) voice_panel_draw(track);
	const u8* box = field_boxes[e->field];
	draw_focus(box[0], box[1], box[2], box[3]);
}
