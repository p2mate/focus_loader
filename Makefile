all: loader.exe

clean:
	-rm *.exe *.o *.map

loader.exe: int10_override.o loader.c
	wcl -fm -ms loader.c int10_override.o 

int10_override.o: int10_override.asm
	wasm int10_override.asm
