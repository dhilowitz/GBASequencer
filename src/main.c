// GBA Sequencer: a three-synth, one-drum-kit step sequencer for the Game Boy Advance.
// Started in 2004 on Loirak's Tank sample; finished in 2026.
#include "gba.h"
#include "audio.h"
#include "song.h"
#include "ui.h"
#include "video.h"
#include "gen/assets.h"

Keys keys;

#define REPEAT_DELAY 14
#define REPEAT_RATE  4
#define STATUS_FRAMES 90

static int screen = SCREEN_SYN1, previous_screen;
static const char* status_text;
static int status_frames;

void show_status(const char* message) {
	status_text = message;
	status_frames = STATUS_FRAMES;
}

int grid_row_top(int row) { return row == 0 ? 28 : 27 + 8 * row; }
int grid_row_bottom(int row) { return row == GRID_ROWS - 1 ? 146 : 27 + 8 * (row + 1); }

// The keypad is sampled every VBlank and presses are collected until the main loop takes them,
// so a quick tap counts even if drawing a frame took longer than a frame.
static volatile u16 latched_pressed, latched_repeat, latched_held;
u32 loop_count;  // frames drawn; compare with vblank_count to see the frame rate

IWRAM_CODE void input_vblank(void) {
	static u16 previous;
	static int repeat_timer;
	u16 now = ~REG_KEYINPUT & 0x03FF;
	u16 pressed = now & ~previous, dpad = KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT;
	u16 repeat = pressed;
	if (pressed & dpad) {
		repeat_timer = REPEAT_DELAY;
	} else if (now & dpad) {
		if (--repeat_timer <= 0) {
			repeat |= now & dpad;
			repeat_timer = REPEAT_RATE;
		}
	}
	latched_pressed |= pressed;
	latched_repeat |= repeat;
	latched_held |= now;
	previous = now;
}

static void read_keys(void) {
	REG_IME = 0;
	keys.pressed = latched_pressed;
	keys.repeat = latched_repeat;
	keys.held = latched_held | (~REG_KEYINPUT & 0x03FF);
	latched_pressed = latched_repeat = latched_held = 0;
	REG_IME = 1;
	++loop_count;
}

void draw_focus(int x, int y, int w, int h) {
	const int len = 7;
	int r = x + w - 2, b = y + h - 2;
	fill_rect(x, y, len, 2, C_RED);
	fill_rect(x, y, 2, len, C_RED);
	fill_rect(r - len + 2, y, len, 2, C_RED);
	fill_rect(r, y, 2, len, C_RED);
	fill_rect(x, b, len, 2, C_RED);
	fill_rect(x, b - len + 2, 2, len, C_RED);
	fill_rect(r - len + 2, b, len, 2, C_RED);
	fill_rect(r, b - len + 2, 2, len, C_RED);
}

void draw_thumb(int x, int y, int w, int h) {
	fill_rect(x, y, w, h, C_GREY);
	frame_rect(x, y, w, h, C_DARK);
}

void draw_beat_lines(int first_pos, int top, int bottom) {
	for (int col = 0; col < GRID_COLS - 1; ++col)
		if ((first_pos + col + 1) % 4 == 0) vline(GRID_X + (col + 1) * COL_W - 1, top, bottom - top, C_GREY);
}

static void draw_status(void) {
	if (!status_frames) return;
	--status_frames;
	int w = text_width(status_text) + 14, h = 20;
	int x = (240 - w) / 2, y = 70;
	fill_rect(x + 2, y + 2, w, h, C_GREY);                // shadow
	fill_rect(x, y, w, h, C_WHITE);
	frame_rect(x, y, w, h, C_FRAME);
	frame_rect(x + 1, y + 1, w - 2, h - 2, C_FRAME);
	plot(x, y, C_WHITE);                                  // rounded corners
	plot(x + w - 1, y, C_WHITE);
	plot(x, y + h - 1, C_WHITE);
	plot(x + w - 1, y + h - 1, C_GREY);
	draw_text(x + 7, y + 6, status_text, C_BLACK);
}

// A little piano roll under the title, with the playhead sweeping across it.
static void draw_title_roll(int frame) {
	static const s8 melody[16] = { 0, -1, 2, -1, 4, 4, -1, 2, 0, -1, 2, 4, -1, 3, -1, 1 };  // row per step
	const int x0 = 55, y0 = 92, cw = 9, rh = 5, rows = 5, cols = 16;
	int keys_x = x0 - 13;
	for (int r = 0; r < rows; ++r) {
		fill_rect(keys_x, y0 + r * rh, 12, rh - 1, (r == 1 || r == 3) ? C_BLACK : C_WHITE);
		frame_rect(keys_x - 1, y0 + r * rh - 1, 14, rh + 1, C_GREY);
	}
	int head = (frame / 8) % cols;
	for (int c = 0; c < cols; ++c) {
		for (int r = 0; r < rows; ++r)
			fill_rect(x0 + c * cw, y0 + r * rh, cw - 1, rh - 1, c == head ? C_YELLOW : C_TRACK);
		if (melody[c] >= 0) {
			int r = rows - 1 - melody[c];
			fill_rect(x0 + c * cw, y0 + r * rh, cw - 1, rh - 1, C_BLUE);
		}
	}
	frame_rect(x0 - 1, y0 - 1, cols * cw + 1, rows * rh + 1, C_DARK);
}

static void title_screen(void) {
	int frame = 0;
	do {
		draw_background(bg_title);
		draw_title_roll(frame++);
		read_keys();
		video_flip();
	} while (!(keys.pressed & (KEY_START | KEY_A)));
}

int main(void) {
	REG_WAITCNT = 0x4317;   // faster cartridge access (ROM 3/1 wait states, prefetch on)
	video_init();
	song_clear();
	audio_init();
	editor_init();
	arrange_init();

	title_screen();
	if (song_load()) show_status("Saved song loaded");

	for (;;) {
		read_keys();

		if (keys.pressed & KEY_START) {
			if (screen == SCREEN_SETTINGS) {
				screen = previous_screen;
			} else {
				previous_screen = screen;
				screen = SCREEN_SETTINGS;
			}
		} else if ((keys.pressed & KEY_SELECT) && screen != SCREEN_SETTINGS) {
			screen = (screen + 1) % NUM_MAIN_SCREENS;
		} else if (keys.pressed & KEY_B) {
			if (play.mode != PLAY_STOPPED) audio_stop();
			else if (screen == SCREEN_SONG) arrange_play();
			else audio_play_patterns();
		} else if (screen == SCREEN_SONG) {
			arrange_input();
		} else if (screen == SCREEN_SETTINGS) {
			settings_input();
		} else {
			editor_input(screen);
		}

		if (screen == SCREEN_SONG) arrange_draw();
		else if (screen == SCREEN_SETTINGS) settings_draw();
		else editor_draw(screen);
		draw_status();
		video_flip();
	}
}
