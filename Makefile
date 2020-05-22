run:
	./bin/chip8 rom/test.ch8

chip8:
	mkdir -p bin
	gcc -Wall -Werror -Wno-unused-function -Wno-unused-variable -lSDL2 main.c chip8.c -o bin/chip8
