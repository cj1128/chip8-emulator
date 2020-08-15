test:
	./bin/chip8 rom/test_opcode.ch8

buildmac:
	mkdir -p bin
	gcc -Wall -Werror -Wno-unused-function -Wno-unused-variable -lSDL2 main.c chip8.c -o bin/chip8

buildwin:
	mkdir -p bin
	gcc -Wall -o bin/chip8.exe main.c chip8.c -Wall `sdl2-config --libs --cflags`
