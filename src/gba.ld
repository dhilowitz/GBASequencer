/* ROM at 0x08000000; code marked IWRAM_CODE and all initialised data run from IWRAM. */
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY {
	rom   : ORIGIN = 0x08000000, LENGTH = 32M
	iwram : ORIGIN = 0x03000000, LENGTH = 32K - 0x100
	ewram : ORIGIN = 0x02000000, LENGTH = 256K
}

SECTIONS {
	.text : {
		KEEP(*(.crt0))
		*(.text .text.*)
		*(.glue_7 .glue_7t)
		*(.rodata .rodata.*)
		. = ALIGN(4);
	} > rom

	.iwram : {
		__iwram_start = .;
		*(.iwram .iwram.*)
		*(.data .data.*)
		. = ALIGN(4);
		__iwram_end = .;
	} > iwram AT > rom
	__iwram_lma = LOADADDR(.iwram);

	.bss (NOLOAD) : {
		__bss_start = .;
		*(.bss .bss.* COMMON)
		. = ALIGN(4);
		__bss_end = .;
	} > iwram

	/DISCARD/ : { *(.ARM.exidx*) *(.ARM.attributes) *(.comment) }
}
