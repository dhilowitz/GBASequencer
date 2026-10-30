all:        tank.gba

gfx.o:  gfx.c
	gcc -c -O3 -mthumb -mthumb-interwork gfx.c

sound.o:  sound.c
	gcc -c -O3 -mthumb -mthumb-interwork sound.c

main.o: main.c gfx/bobjbomb.h gfx/A-Z.h gfx/ggrid.h gfx/llogo.h gfx/oobjtank.h gfx/sstar.h gfx/ttank.h gfx/ttxdied.h gfx/ttxstart.h positions.h saving.h
	gcc -c -O3 -mthumb -mthumb-interwork main.c

sequencer.elf:   main.o gfx.o sound.o
	    gcc -mthumb -mthumb-interwork -o sequencer.elf main.o gfx.o sound.o

sequencer.gba:   sequencer.elf
	    objcopy -O binary sequencer.elf sequencer.gba

%.h:  	    %.pcx
	    gfx/pcx2gba $<

