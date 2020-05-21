chip8:
	mkdir -p bin
	gcc -Wall -Werror -Wno-unused-variable -lSDL2 chip8.c -o bin/chip8
