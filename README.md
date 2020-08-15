# CHIP-8 Emulator

A simple CHIP-8 emulator written in C using SDL2.

<div align="center">
  <img src="http://asset.cjting.cn/FifM5E9dOL3g2XUxyqzc1cCiptd_.png">
</div>

## Spec

- All instructions are 2 bytes long and are stored most-significant-byte first.
- 4K RAM (`0x0 ~ 0xfff`)
  - `0x0 ~ 0x1ff`: reserverd for system
  - `0x200 ~ 0xe9f`: freely used RAM
  - `0xea0 ~ 0xeff`: reserved for stack, internal use and other variables
  - `0xf00 ~ 0xfff`: for display
- 64x32 resolution display (1 bit per pixel)
- Font sprite starts at `0x000`, see details below
- 8-bit delay and sound timer
- 16 8-bit registers `V0` ~ `VF`
- A 16-bit program counter (`PC`)
- A 8-bit stack pointer (`SP`)
- One 16-bit address register `I`
- [Instruction set](https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set)
  - `8XYE`: `VF` should be the most significant bit of `VX` not `VY` prior to the shift
  - `8XY6`: Same as above
  - `FX55`: Don't increset `I`
  - `FX65`: Same as above

## Keymapping

```text
1 2 3 C        1 2 3 4
4 5 6 D   ->   Q W E R
7 8 9 E        A S D F
A 0 B F        Z X C V
```

## Build

Mac OS:

- Install SDL2: `brew install sdl2`
- Git clone the repo
- `make buildmac && make test`

Windows:

- Download MSYS2 64bit from here: https://www.msys2.org/
- Update MSYS2: `pacman -Syu`
- Install build tools and SDL2: `pacman -S make git mingw-w64-x86_64-toolchain mingw64/mingw-w64-x86_64-SDL2`
- Start MSYS2 MinGW 64-bit Shell
- Git clone the repo
- `make buildwin && make test`

## Test

There are a few roms in the `rom` directory of repo.

`BC_Test.ch8` and `test_opcode.ch8` are used for testing the fundamental functions of our emulator.

`test.ch8` is used for testing arbitrary instructions written by ourself.

All others are games for fully testing our emulator 🎮.

## Reference

- http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.4
- https://code.austinmorlan.com/austin/chip8-emulator
- https://github.com/massung/CHIP-8
