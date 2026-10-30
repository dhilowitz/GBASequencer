// GBA hardware registers and types used by the sequencer.
#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

#define REG8(a)  (*(volatile u8*)(a))
#define REG16(a) (*(volatile u16*)(a))
#define REG32(a) (*(volatile u32*)(a))

#define IWRAM_CODE __attribute__((section(".iwram"), long_call, target("arm")))

// Display
#define REG_DISPCNT   REG16(0x04000000)
#define REG_DISPSTAT  REG16(0x04000004)
#define REG_VCOUNT    REG16(0x04000006)
#define DISPCNT_MODE4    0x0004
#define DISPCNT_PAGE     0x0010
#define DISPCNT_BG2      0x0400
#define DISPSTAT_VBL_IRQ 0x0008

#define PALETTE_RAM ((volatile u16*)0x05000000)
#define VRAM_PAGE0  ((volatile u16*)0x06000000)
#define VRAM_PAGE1  ((volatile u16*)0x0600A000)

// Sound
#define REG_SOUND1CNT_L REG16(0x04000060)
#define REG_SOUND1CNT_H REG16(0x04000062)
#define REG_SOUND1CNT_X REG16(0x04000064)
#define REG_SOUND2CNT_L REG16(0x04000068)
#define REG_SOUND2CNT_H REG16(0x0400006C)
#define REG_SOUND3CNT_L REG16(0x04000070)
#define REG_SOUND3CNT_H REG16(0x04000072)
#define REG_SOUND3CNT_X REG16(0x04000074)
#define REG_SOUNDCNT_L  REG16(0x04000080)
#define REG_SOUNDCNT_H  REG16(0x04000082)
#define REG_SOUNDCNT_X  REG16(0x04000084)
#define REG_SOUNDBIAS   REG16(0x04000088)
#define WAVE_RAM        ((volatile u32*)0x04000090)
#define REG_FIFO_A      0x040000A0

// DMA
#define REG_DMA1SAD   REG32(0x040000BC)
#define REG_DMA1DAD   REG32(0x040000C0)
#define REG_DMA1CNT_H REG16(0x040000C6)
#define REG_DMA3SAD   REG32(0x040000D4)
#define REG_DMA3DAD   REG32(0x040000D8)
#define REG_DMA3CNT   REG32(0x040000DC)
#define DMA_ENABLE     0x8000
#define DMA_32         0x0400
#define DMA_REPEAT     0x0200
#define DMA_START_FIFO 0x3000
#define DMA_DEST_FIXED 0x0040
#define DMA_SRC_FIXED  0x0100

// Timers
#define REG_TM0CNT_L REG16(0x04000100)
#define REG_TM0CNT_H REG16(0x04000102)
#define REG_TM2CNT_L REG16(0x04000108)
#define REG_TM2CNT_H REG16(0x0400010A)
#define TIMER_ENABLE 0x0080
#define TIMER_IRQ    0x0040
#define TIMER_DIV64  0x0001

// Keys (active low in hardware; keys_read() inverts)
#define REG_KEYINPUT REG16(0x04000130)
#define KEY_A      0x0001
#define KEY_B      0x0002
#define KEY_SELECT 0x0004
#define KEY_START  0x0008
#define KEY_RIGHT  0x0010
#define KEY_LEFT   0x0020
#define KEY_UP     0x0040
#define KEY_DOWN   0x0080
#define KEY_R      0x0100
#define KEY_L      0x0200

// Interrupts
#define REG_IE      REG16(0x04000200)
#define REG_IF      REG16(0x04000202)
#define REG_WAITCNT REG16(0x04000204)
#define REG_IME     REG16(0x04000208)
#define IRQ_VBLANK  0x0001
#define IRQ_TIMER2  0x0020
#define BIOS_IRQ_FLAGS   REG16(0x03007FF8)
#define BIOS_IRQ_HANDLER (*(void (**)(void))0x03007FFC)

#define SRAM ((volatile u8*)0x0E000000)

static inline void vblank_intr_wait(void) {
	__asm__ volatile("swi 0x05" ::: "r0", "r1", "r2", "r3", "memory");
}
