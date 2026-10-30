@ Startup code: cartridge header, stacks, copy IWRAM code and data, clear BSS, call main.
	.section .crt0, "ax"
	.arm
	.global _start
_start:
	b	reset
	.fill	156, 1, 0		@ Nintendo logo, filled in by tools/gbafix.py
	.ascii	"GBASEQUENCER"		@ title, 12 bytes
	.ascii	"ZGSE"			@ game code
	.ascii	"00"			@ maker code
	.byte	0x96			@ fixed value
	.byte	0			@ unit code
	.byte	0			@ device type
	.fill	7, 1, 0			@ reserved
	.byte	0			@ version
	.byte	0			@ header checksum, filled in by tools/gbafix.py
	.fill	2, 1, 0			@ reserved

reset:
	mov	r0, #0x12		@ IRQ mode
	msr	cpsr_c, r0
	ldr	sp, =0x03007FA0
	mov	r0, #0x1F		@ system mode
	msr	cpsr_c, r0
	ldr	sp, =0x03007C00		@ leaves 0x3A0 bytes for the IRQ stack (interrupts nest)

	ldr	r0, =__iwram_lma	@ IWRAM code and initialised data
	ldr	r1, =__iwram_start
	ldr	r2, =__iwram_end
1:	cmp	r1, r2
	ldrlo	r3, [r0], #4
	strlo	r3, [r1], #4
	blo	1b

	ldr	r1, =__bss_start
	ldr	r2, =__bss_end
	mov	r3, #0
2:	cmp	r1, r2
	strlo	r3, [r1], #4
	blo	2b

	ldr	r0, =main
	mov	lr, pc
	bx	r0
	b	.
	.pool
