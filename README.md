# Chip8 Emulator

http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.4

## Emulator spec

All instructions are 2 bytes long and are stored most-significant-byte first.

4K RAM (0x200 ~ 0xfff),

64x32 Display (1 bit pixel) Start 0xf00

Font starts at 0x000. 4x5

Delay and Sound timer: 8bit

V0 ~ Vf: 16 8bit registers

I: address register

## Design

## Test

ROMS https://github.com/dmatlack/chip8

BC_Test
