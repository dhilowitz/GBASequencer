#include "video.h"
#include "gen/assets.h"

static volatile u16* back;

void video_init(void) {
	for (int i = 0; i < 256; ++i) PALETTE_RAM[i] = palette[i];
	REG_DISPCNT = DISPCNT_MODE4 | DISPCNT_BG2;
	back = VRAM_PAGE1;
}

void video_flip(void) {
	vblank_intr_wait();
	REG_DISPCNT ^= DISPCNT_PAGE;
	back = (REG_DISPCNT & DISPCNT_PAGE) ? VRAM_PAGE0 : VRAM_PAGE1;
}

// The CPU can't take interrupts during a DMA, so the copy is split into pieces of about 0.2 ms to
// let the step timer in on time.
void draw_background(const u8* bg) {
	const int chunk = 240 * 160 / 4 / 16;   // words
	for (int i = 0; i < 16; ++i) {
		REG_DMA3SAD = (u32)(bg + i * chunk * 4);
		REG_DMA3DAD = (u32)((volatile u8*)back + i * chunk * 4);
		REG_DMA3CNT = chunk | ((DMA_ENABLE | DMA_32) << 16);
	}
}

// The drawing routines run from IWRAM as ARM code: Mode 4 redraws the whole screen every frame.

// VRAM can't be written a byte at a time, so pixels are merged into their 16-bit pair.
IWRAM_CODE void plot(int x, int y, int c) {
	if ((unsigned)x >= 240 || (unsigned)y >= 160) return;
	volatile u16* p = back + ((y * 240 + x) >> 1);
	*p = (x & 1) ? (u16)((*p & 0x00FF) | (c << 8)) : (u16)((*p & 0xFF00) | c);
}

IWRAM_CODE void hline(int x, int y, int w, int c) {
	fill_rect(x, y, w, 1, c);
}

IWRAM_CODE void vline(int x, int y, int h, int c) {
	for (int i = 0; i < h; ++i) plot(x, y + i, c);
}

// Wide rows are filled by DMA from a fixed word, a few times faster than a CPU loop.
IWRAM_CODE void fill_rect(int x, int y, int w, int h, int c) {
	static volatile u32 fill_word;
	fill_word = (u32)(c | c << 8) * 0x00010001u;
	for (int j = 0; j < h; ++j) {
		int i = 0;
		if (x & 1) { plot(x, y + j, c); i = 1; }
		volatile u16* p = back + (((y + j) * 240 + x + i) >> 1);
		if (((u32)p & 2) && i + 1 < w) { *p++ = (u16)fill_word; i += 2; }
		int words = (w - i) >> 2;
		if (words >= 4) {
			REG_DMA3SAD = (u32)&fill_word;
			REG_DMA3DAD = (u32)p;
			REG_DMA3CNT = words | ((DMA_ENABLE | DMA_32 | DMA_SRC_FIXED) << 16);
			p += words * 2;
			i += words * 4;
		}
		for (; i + 1 < w; i += 2) *p++ = (u16)fill_word;
		if (i < w) plot(x + i, y + j, c);
	}
}

// Copy an image whose rows are an even number of pixels to an even x, by DMA.
void blit_rect(int x, int y, int w, int h, const u8* pixels) {
	for (int j = 0; j < h; ++j) {
		REG_DMA3SAD = (u32)(pixels + j * w);
		REG_DMA3DAD = (u32)(back + (((y + j) * 240 + x) >> 1));
		REG_DMA3CNT = (w >> 1) | (DMA_ENABLE << 16);
	}
}

IWRAM_CODE void blit_row(int x, int y, int w, const u8* src) {
	volatile u16* p = back + ((y * 240 + x) >> 1);
	const u16* s = (const u16*)src;
	for (int i = 0; i < w; i += 2) *p++ = *s++;
}

IWRAM_CODE void recolor_rect(int x, int y, int w, int h, int from, int to) {
	for (int j = 0; j < h; ++j) {
		int line = (y + j) * 240 + x;
		for (int i = 0; i < w; ++i) {
			volatile u16* p = back + ((line + i) >> 1);
			u16 v = *p;
			if ((line + i) & 1) { if ((v >> 8) == from) *p = (v & 0x00FF) | to << 8; }
			else if ((v & 0xFF) == from) *p = (v & 0xFF00) | to;
		}
	}
}

IWRAM_CODE void frame_rect(int x, int y, int w, int h, int c) {
	hline(x, y, w, c);
	hline(x, y + h - 1, w, c);
	vline(x, y, h, c);
	vline(x + w - 1, y, h, c);
}

IWRAM_CODE void draw_text(int x, int y, const char* s, int c) {
	for (; *s; ++s, x += 6) {
		int ch = (u8)*s;
		if (ch < 32 || ch > 127) ch = '?';
		const u8* rows = font_glyphs[ch - 32];
		if (x - 1 < 0 || x + 7 > 240 || y < 0 || y + 8 > 160) {
			for (int j = 0; j < 8; ++j)       // near the edge: the careful way
				for (int i = 0; i < 8; ++i)
					if (rows[j] & (0x80 >> i)) plot(x + i - 1, y + j, c);
			continue;
		}
		for (int j = 0; j < 8; ++j) {
			int bits = rows[j];
			if (!bits) continue;
			int line = (y + j) * 240 + x - 1;
			for (int i = 0; bits; ++i, bits = (bits << 1) & 0xFF) {
				if (!(bits & 0x80)) continue;
				volatile u16* p = back + ((line + i) >> 1);
				*p = ((line + i) & 1) ? (u16)((*p & 0x00FF) | (c << 8)) : (u16)((*p & 0xFF00) | c);
			}
		}
	}
}

int text_width(const char* s) {
	int n = 0;
	while (s[n]) ++n;
	return n * 6;
}

void int_to_text(int v, char* out) {
	char tmp[12];
	int n = 0;
	if (v < 0) { *out++ = '-'; v = -v; }
	do { tmp[n++] = '0' + v % 10; v /= 10; } while (v);
	while (n) *out++ = tmp[--n];
	*out = 0;
}
