// The Sequence screen: arrange patterns into a song, one row per track, and set the tempo.
//
//   grid   d-pad moves (R + left/right jumps 16 columns). A puts the track's current pattern in
//          the cell, or empties it if it's already there. R + up/down changes the cell's pattern.
//   tempo  up/down change it by 1, left/right by 10.
// The song plays from the cursor column to the last filled column, then loops to the start.
#include "audio.h"
#include "song.h"
#include "ui.h"
#include "video.h"
#include "gen/assets.h"

enum { FIELD_GRID, FIELD_TEMPO, NUM_FIELDS };

static struct {
	u8 field, col, row, view_left;
} ed;

static const u8 row_lines[NUM_TRACKS + 1] = { 28, 47, 67, 87, 106 };
// pale pink, yellow, green, cyan, blue, violet, peach, khaki: A-H, and again for 1-8
static const u8 pattern_tints[8] = { 248, 254, 218, 219, 213, 249, 247, 211 };

void arrange_init(void) {}

static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

void arrange_play(void) {
	int last = song_last_position();
	if (last < 0) show_status("Song is empty: add patterns");
	else audio_play_song(ed.col <= last ? ed.col : 0);
}

void arrange_input(void) {
	if (keys.pressed & KEY_L) {
		ed.field = (ed.field + 1) % NUM_FIELDS;
		return;
	}
	int shift = keys.held & KEY_R;
	if (ed.field == FIELD_TEMPO) {
		int t = song.tempo;
		if (keys.repeat & KEY_UP) t += 1;
		if (keys.repeat & KEY_DOWN) t -= 1;
		if (keys.repeat & KEY_RIGHT) t += 10;
		if (keys.repeat & KEY_LEFT) t -= 10;
		song.tempo = clamp(t, MIN_TEMPO, MAX_TEMPO);
		return;
	}

	u8* cell = &song.order[ed.row][ed.col];
	if (keys.repeat & KEY_RIGHT) ed.col = clamp(ed.col + (shift ? GRID_COLS : 1), 0, SONG_LENGTH - 1);
	if (keys.repeat & KEY_LEFT) ed.col = clamp(ed.col - (shift ? GRID_COLS : 1), 0, SONG_LENGTH - 1);
	if (shift && (keys.repeat & (KEY_UP | KEY_DOWN))) {
		if (*cell != NO_PATTERN) {
			*cell = (*cell + ((keys.repeat & KEY_UP) ? 1 : NUM_PATTERNS - 1)) % NUM_PATTERNS;
			edit_pattern[ed.row] = *cell;
		}
	} else {
		if (keys.repeat & KEY_UP) ed.row = clamp(ed.row - 1, 0, NUM_TRACKS - 1);
		if (keys.repeat & KEY_DOWN) ed.row = clamp(ed.row + 1, 0, NUM_TRACKS - 1);
	}
	cell = &song.order[ed.row][ed.col];
	if (keys.pressed & KEY_A) *cell = *cell == edit_pattern[ed.row] ? NO_PATTERN : edit_pattern[ed.row];

	if (ed.col < ed.view_left) ed.view_left = ed.col;
	if (ed.col >= ed.view_left + GRID_COLS) ed.view_left = ed.col - GRID_COLS + 1;
}

void arrange_draw(void) {
	draw_background(bg_song);
	char text[24];

	int_to_text(song.tempo, text);
	draw_text(113 - text_width(text), 10, text, C_BLACK);

	int playing = play.mode == PLAY_SONG ? play.song_pos : -1;
	int last = song_last_position();
	for (int col = 0; col < GRID_COLS; ++col) {
		int pos = ed.view_left + col, x = GRID_X + col * COL_W;
		for (int t = 0; t < NUM_TRACKS; ++t) {
			int y = row_lines[t] + 1, h = row_lines[t + 1] - y;
			int p = song.order[t][pos];
			if (pos == playing) {
				fill_rect(x, y, COL_W - 1, h, C_YELLOW);
			} else if (p != NO_PATTERN) {
				// a pale colour per pattern; a run of the same pattern joins into one block
				int joined = col < GRID_COLS - 1 && pos + 1 != playing && song.order[t][pos + 1] == p;
				fill_rect(x, y, COL_W - 1 + joined, h, pattern_tints[p & 7]);
			}
			if (p != NO_PATTERN) {
				text[0] = song_pattern_name(p);
				text[1] = 0;
				draw_text(x + 3, y + (h - 7) / 2, text, C_BLACK);
			}
		}
	}
	draw_beat_lines(ed.view_left, 29, 106);
	// end-of-song marker after the last filled column
	int end_col = last + 1 - ed.view_left;
	if (last >= 0 && end_col > 0 && end_col <= GRID_COLS)
		fill_rect(GRID_X + end_col * COL_W - 2, row_lines[0], 2, row_lines[NUM_TRACKS] - row_lines[0] + 1, C_RED);
	int cy = row_lines[ed.row];
	frame_rect(GRID_X + (ed.col - ed.view_left) * COL_W - 1, cy, COL_W + 1, row_lines[ed.row + 1] - cy + 1, C_RED);

	// scrollbar: which 16 of the 64 positions are on screen
	int track_w = 196, w = track_w * GRID_COLS / SONG_LENGTH;
	draw_thumb(29 + (track_w - w) * ed.view_left / (SONG_LENGTH - GRID_COLS), 113, w, 5);

	// position readout and a reminder of the controls
	char* p = text;
	const char* label = "Position ";
	while (*label) *p++ = *label++;
	int_to_text(ed.col + 1, p);
	draw_text(14, 126, text, C_BLACK);
	if (last >= 0) {
		p = text;
		label = "Length ";
		while (*label) *p++ = *label++;
		int_to_text(last + 1, p);
		draw_text(224 - text_width(text), 126, text, C_BLACK);
	}
	draw_text(14, 140, ed.field == FIELD_GRID ? "A set/clear  R+UP/DOWN change" : "UP/DOWN 1  LEFT/RIGHT 10",
	          C_GREY);

	if (ed.field == FIELD_GRID) draw_focus(30, 26, 196, 82);
	else draw_focus(77, 4, 64, 20);
}
