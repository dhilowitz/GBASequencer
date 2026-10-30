// Mode 4 drawing: a 240x160 8-bit bitmap, double buffered. Draw into the back page, then flip.
#pragma once
#include "gba.h"

// Colours from the shared palette (the web-safe cube at 40-255)
#define C_BLACK  40
#define C_WHITE  255
#define C_LIGHT  212   // #cccccc
#define C_GREY   169   // #999999
#define C_DARK   83    // #333333
#define C_BLUE   57    // #0066ff, the notes in the 2004 art
#define C_RED    185   // #cc0033
#define C_YELLOW 253   // #ffff99, the 2004 playback line
#define C_ORANGE 204   // #cc9966, the LENGTH label

void video_init(void);
void video_flip(void);           // waits for VBlank first
void draw_background(const u8* bg);
void plot(int x, int y, int c);
void fill_rect(int x, int y, int w, int h, int c);
void frame_rect(int x, int y, int w, int h, int c);
void blit_row(int x, int y, int w, const u8* src);     // x, w and src must be even/aligned
void blit_rect(int x, int y, int w, int h, const u8* pixels);   // x and w even
void recolor_rect(int x, int y, int w, int h, int from, int to);  // repaint one colour, keep the rest
void hline(int x, int y, int w, int c);
void vline(int x, int y, int h, int c);
void draw_text(int x, int y, const char* s, int c);   // 8x8 font, 6 px per character
int text_width(const char* s);
void int_to_text(int v, char* out);
