// Screens and input shared by main.c, editor.c, arrange.c and settings.c.
#pragma once
#include "gba.h"

enum { SCREEN_SYN1, SCREEN_SYN2, SCREEN_SYN3, SCREEN_DRUMS, SCREEN_SONG, NUM_MAIN_SCREENS,
       SCREEN_SETTINGS = NUM_MAIN_SCREENS };

typedef struct {
	u16 held;      // keys down now
	u16 pressed;   // keys that went down this frame
	u16 repeat;    // pressed, plus auto-repeat for the d-pad while held
} Keys;

extern Keys keys;

void input_vblank(void);   // called from the VBlank interrupt

void show_status(const char* message);
void draw_focus(int x, int y, int w, int h);   // red corner brackets around the active field
void draw_thumb(int x, int y, int w, int h);   // a scrollbar thumb
void draw_beat_lines(int first_pos, int top, int bottom);  // darker line after every 4th column

// Grid geometry shared by the 2004 backgrounds: 16 columns of 12 px from x = 32,
// row boundaries at y = 28, 35, 43, ... 139, 146 on the editor screens.
#define GRID_X      32
#define GRID_COLS   16
#define COL_W       12
#define GRID_ROWS   15
#define GRID_RIGHT  224
int grid_row_top(int row);        // y of the line above the row
int grid_row_bottom(int row);     // y of the line below the row

void editor_init(void);
void editor_input(int track);
void editor_draw(int track);

void voice_panel_input(int synth, int note);
void voice_panel_draw(int synth);

void arrange_init(void);
void arrange_input(void);
void arrange_play(void);
void arrange_draw(void);

void settings_input(void);
void settings_draw(void);
