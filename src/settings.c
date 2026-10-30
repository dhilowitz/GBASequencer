// The Settings screen (START): save, load or clear the song, and a summary of the controls.
#include "audio.h"
#include "song.h"
#include "ui.h"
#include "video.h"
#include "gen/assets.h"

enum { ITEM_SAVE, ITEM_LOAD, ITEM_NEW, NUM_ITEMS };
static const char* const items[NUM_ITEMS] = { "Save song", "Load song", "New song" };
static int selected, confirm_new;

void settings_input(void) {
	if (keys.repeat & KEY_UP) selected = (selected + NUM_ITEMS - 1) % NUM_ITEMS, confirm_new = 0;
	if (keys.repeat & KEY_DOWN) selected = (selected + 1) % NUM_ITEMS, confirm_new = 0;
	if (!(keys.pressed & KEY_A)) return;
	switch (selected) {
	case ITEM_SAVE:
		show_status(song_save() ? "Song saved" : "Save failed");
		break;
	case ITEM_LOAD:
		audio_stop();
		show_status(song_load() ? "Song loaded" : "No saved song");
		break;
	case ITEM_NEW:
		if (!confirm_new) {
			confirm_new = 1;
			show_status("Press A again to clear");
		} else {
			confirm_new = 0;
			audio_stop();
			song_clear();
			show_status("New song");
		}
		break;
	}
}

static const char* const help[] = {
	"SELECT  next screen",
	"B       play / stop",
	"L       next field",
	"A       set / clear",
	"R+pad   bigger jumps",
	"START   back",
};

void settings_draw(void) {
	draw_background(bg_settings);
	for (int i = 0; i < NUM_ITEMS; ++i) {
		int y = 36 + i * 14;
		draw_text(36, y, items[i], C_BLACK);
		if (i == selected) frame_rect(30, y - 3, 70, 13, C_RED);
	}
	if (song_has_save()) draw_text(36, 116, "A song is saved", C_GREY);
	for (unsigned i = 0; i < sizeof(help) / sizeof(help[0]); ++i)
		draw_text(110, 36 + i * 12, help[i], C_DARK);
}
