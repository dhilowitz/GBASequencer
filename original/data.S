.arm
.text
.global main

main:
	AREA	demo_data, DATA, READONLY
	EXPORT letterTiles
	EXPORT letterPalette

letterTiles:
	INCBIN A-Z.raw
letterPalette:
	INCBIN A-Z.pal