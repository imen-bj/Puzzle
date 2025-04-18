prog: main1.o fonction1.o
	gcc main1.o fonction1.o -o prog -lSDL -lSDL_image -lSDL_gfx -lSDL_mixer -lSDL_ttf -g

main.o: main1.c header1.h
	gcc -c main1.c -g

fonction.o: fonction1.c header1.h
	gcc -c fonction1.c -g
