// Headless mGBA runner for testing the ROM without a window.
//
//   harness <rom.gba> [--save file.sav] < script
//
// Script commands, one per line (# starts a comment):
//   hold KEY[+KEY...] | none   keys held from now on (A B SELECT START RIGHT LEFT UP DOWN R L)
//   run N                      run N frames
//   tap KEY[+KEY...] [N]       hold for N frames (default 2), then release for 4
//   shot out.ppm               write the current frame (240x160 PPM)
//   wav out.wav                start recording audio (32768 Hz stereo)
//   endwav                     stop recording
//   peek8|peek16|peek32 ADDR   print a value from the bus
//   mute N | unmute N          silence an audio channel: 0-3 PSG 1-4, 4-5 DirectSound A/B
#include <mgba/core/core.h>
#include <mgba/core/blip_buf.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/core/log.h>
#include <mgba-util/vfs.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char* keyNames[] = { "A", "B", "SELECT", "START", "RIGHT", "LEFT", "UP", "DOWN", "R", "L" };

static uint32_t parseKeys(const char* s) {
	uint32_t keys = 0;
	if (!strcasecmp(s, "none")) return 0;
	char buf[128];
	snprintf(buf, sizeof(buf), "%s", s);
	for (char* tok = strtok(buf, "+"); tok; tok = strtok(NULL, "+")) {
		int found = 0;
		for (int i = 0; i < 10; ++i) {
			if (!strcasecmp(tok, keyNames[i])) { keys |= 1u << i; found = 1; }
		}
		if (!found) { fprintf(stderr, "unknown key %s\n", tok); exit(2); }
	}
	return keys;
}

static int logCount;
static void quietLog(struct mLogger* logger, int category, enum mLogLevel level, const char* format, va_list args) {
	(void) logger;
	// Emulator warnings (bad memory access etc.) are summarized, not printed one by one
	if (level & (mLOG_FATAL | mLOG_ERROR | mLOG_WARN | mLOG_GAME_ERROR)) {
		if (++logCount <= 5 && getenv("HARNESS_LOG")) {
			fprintf(stderr, "[%s] ", mLogCategoryName(category));
			vfprintf(stderr, format, args);
			fputc('\n', stderr);
		}
	}
}
static struct mLogger logger = { .log = quietLog };

static struct mCore* core;
static color_t* video;
static FILE* wav;
static uint32_t wavBytes;
static unsigned frames;

static void writeWavHeader(FILE* f, uint32_t dataBytes) {
	uint32_t rate = 32768, byteRate = rate * 4;
	uint32_t riffSize = 36 + dataBytes;
	uint16_t fmt = 1, ch = 2, align = 4, bits = 16;
	uint32_t fmtSize = 16;
	fseek(f, 0, SEEK_SET);
	fwrite("RIFF", 1, 4, f); fwrite(&riffSize, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
	fwrite(&fmtSize, 4, 1, f); fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f);
	fwrite(&rate, 4, 1, f); fwrite(&byteRate, 4, 1, f); fwrite(&align, 2, 1, f); fwrite(&bits, 2, 1, f);
	fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f);
}

static void runFrames(int n) {
	int16_t samples[2048 * 2];
	for (int i = 0; i < n; ++i) {
		core->runFrame(core);
		++frames;
		struct blip_t* left = core->getAudioChannel(core, 0);
		struct blip_t* right = core->getAudioChannel(core, 1);
		int avail;
		while ((avail = blip_samples_avail(left)) > 0) {
			if (avail > 2048) avail = 2048;
			blip_read_samples(left, samples, avail, 1);
			blip_read_samples(right, samples + 1, avail, 1);
			if (wav) { fwrite(samples, 4, avail, wav); wavBytes += avail * 4; }
		}
	}
}

static void shot(const char* path) {
	FILE* f = fopen(path, "wb");
	if (!f) { perror(path); exit(1); }
	fprintf(f, "P6\n240 160\n255\n");
	for (int y = 0; y < 160; ++y) {
		for (int x = 0; x < 240; ++x) {
			color_t c = video[y * 256 + x];
			unsigned char rgb[3] = { c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF };
			fwrite(rgb, 1, 3, f);
		}
	}
	fclose(f);
}

int main(int argc, char** argv) {
	if (argc < 2) { fprintf(stderr, "usage: %s rom.gba [--save file.sav] < script\n", argv[0]); return 2; }
	const char* savePath = NULL;
	for (int i = 2; i < argc; ++i) {
		if (!strcmp(argv[i], "--save") && i + 1 < argc) savePath = argv[++i];
	}
	mLogSetDefaultLogger(&logger);
	core = mCoreFind(argv[1]);
	if (!core) { fprintf(stderr, "not a ROM: %s\n", argv[1]); return 1; }
	core->init(core);
	mCoreInitConfig(core, "harness");
	video = calloc(256 * 256, sizeof(color_t));
	core->setVideoBuffer(core, video, 256);
	core->setAudioBufferSize(core, 2048);
	if (!mCoreLoadFile(core, argv[1])) { fprintf(stderr, "load failed\n"); return 1; }
	if (savePath) {
		struct VFile* vf = VFileOpen(savePath, O_RDWR | O_CREAT);
		if (!vf || !core->loadSave(core, vf)) { fprintf(stderr, "can't use save %s\n", savePath); return 1; }
	}
	core->reset(core);
	blip_set_rates(core->getAudioChannel(core, 0), core->frequency(core), 32768);
	blip_set_rates(core->getAudioChannel(core, 1), core->frequency(core), 32768);

	char line[256];
	while (fgets(line, sizeof(line), stdin)) {
		char* hash = strchr(line, '#');
		if (hash) *hash = 0;
		char cmd[32] = "", a[200] = "", b[32] = "";
		if (sscanf(line, "%31s %199s %31s", cmd, a, b) < 1) continue;
		if (!strcmp(cmd, "hold")) core->setKeys(core, parseKeys(a));
		else if (!strcmp(cmd, "run")) runFrames(atoi(a));
		else if (!strcmp(cmd, "tap")) {
			core->setKeys(core, parseKeys(a));
			runFrames(b[0] ? atoi(b) : 2);
			core->setKeys(core, 0);
			runFrames(4);
		} else if (!strcmp(cmd, "shot")) shot(a);
		else if (!strcmp(cmd, "wav")) {
			wav = fopen(a, "wb");
			if (!wav) { perror(a); return 1; }
			wavBytes = 0;
			writeWavHeader(wav, 0);
		} else if (!strcmp(cmd, "endwav")) {
			if (wav) { writeWavHeader(wav, wavBytes); fclose(wav); wav = NULL; }
		} else if (!strcmp(cmd, "mute") || !strcmp(cmd, "unmute")) {
			core->enableAudioChannel(core, atoi(a), cmd[0] == 'u');
		} else if (!strncmp(cmd, "peek", 4)) {
			uint32_t addr = strtoul(a, NULL, 0), v;
			if (!strcmp(cmd, "peek8")) v = core->busRead8(core, addr);
			else if (!strcmp(cmd, "peek16")) v = core->busRead16(core, addr);
			else v = core->busRead32(core, addr);
			printf("%s 0x%08X = %u (0x%X) @frame %u\n", cmd, addr, v, v, frames);
		} else { fprintf(stderr, "unknown command %s\n", cmd); return 2; }
	}
	if (wav) { writeWavHeader(wav, wavBytes); fclose(wav); }
	if (logCount) fprintf(stderr, "(%d emulator warnings; set HARNESS_LOG=1 to see the first few)\n", logCount);
	core->unloadROM(core);
	core->deinit(core);
	return 0;
}
