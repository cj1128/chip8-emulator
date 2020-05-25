# Chip8 Emulator

http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.4

## Emulator spec

All instructions are 2 bytes long and are stored most-significant-byte first.

4K RAM (0x200 ~ 0xfff),
- 0x0 ~ 0x1ff: reserverd
- 0x200 ~ 0xe9f: RAM
- 0xea0 ~ 0xeff: stack, internal use, other variables
- 0xf00 ~ 0xfff: display

64x32 Display (1 bit pixel) Start 0xf00

Font starts at 0x000. 4x5

Delay and Sound timer: 8bit

V0 ~ Vf: 16 8bit registers

I: address register

## NOTE

- 8XYE: Store vx
- 8XY6:
- FX55:
- FX65:

## Keymapping

```text
1 2 3 C        1 2 3 4
4 5 6 D   ->   Q W E R
7 8 9 E        A S D F
A 0 B F        Z X C V
```

## Tetris

Q: transform
W: left
E: right
A: drop

## Reference

https://code.austinmorlan.com/austin/chip8-emulator

https://github.com/massung/CHIP-8

## Test

https://github.com/corax89/chip8-test-rom
BC_Test

ROMS https://github.com/dmatlack/chip8

