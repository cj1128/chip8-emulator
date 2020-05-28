#include "chip8.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
static u8 fontROM[] = {
  // 4x5 font sprites (0-F)
  0xF0, 0x90, 0x90, 0x90, 0xF0,
  0x20, 0x60, 0x20, 0x20, 0x70,
  0xF0, 0x10, 0xF0, 0x80, 0xF0,
  0xF0, 0x10, 0xF0, 0x10, 0xF0,
  0xA0, 0xA0, 0xF0, 0x20, 0x20,
  0xF0, 0x80, 0xF0, 0x10, 0xF0,
  0xF0, 0x80, 0xF0, 0x90, 0xF0,
  0xF0, 0x10, 0x20, 0x40, 0x40,
  0xF0, 0x90, 0xF0, 0x90, 0xF0,
  0xF0, 0x90, 0xF0, 0x10, 0xF0,
  0xF0, 0x90, 0xF0, 0x90, 0x90,
  0xE0, 0x90, 0xE0, 0x90, 0xE0,
  0xF0, 0x80, 0x80, 0x80, 0xF0,
  0xE0, 0x90, 0x90, 0x90, 0xE0,
  0xF0, 0x80, 0xF0, 0x80, 0xF0,
  0xF0, 0x80, 0xF0, 0x80, 0x80,
};
// clang-format on

// Used to store chip8 related error message
static char errorBuf[256];

#define CHIP8_MAX_ROM (0x1000 - 0x200 - 352)

// Return 0 or non-zero
int
Chip8_GetPixel(chip8_vm *vm, int x, int y)
{
  u8 *screen = vm->screen;
  int index = y * SCREEN_WIDTH + x;
  int byteIndex = index / 8;
  int offset = index % 8;
  return screen[byteIndex] & (0x80 >> offset);
}

void
SetPixel(chip8_vm *vm, int x, int y, bool on)
{
  // printf("set pixel, x: %d, y: %d\n", x, y);
  int index = y * SCREEN_WIDTH + x;
  int byteIndex = index / 8;
  int offset = index % 8;
  u8 byte = vm->screen[byteIndex];

  if(on) {
    byte = byte | (0x80 >> offset);
  } else {
    byte = byte & (~(0x80 >> offset));
  }

  vm->screen[byteIndex] = byte;
}

char *
Chip8_GetError(void)
{
  return errorBuf;
}

static void
Err(char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vsnprintf(errorBuf, sizeof(errorBuf), format, ap);
}

void
ClearScreen(chip8_vm *vm)
{
  for(int i = 0; i < 256; i++) {
    vm->screen[i] = 0;
  }
}

chip8_vm *
Chip8_New(u8 rom[], uint romSize, random_number_fn *random)
{
  chip8_vm *vm = malloc(sizeof(chip8_vm));
  if(vm == NULL) {
    Err("error: malloc failed");
    return NULL;
  }

  if(romSize > CHIP8_MAX_ROM) {
    Err("rom size too big, should be <= %d bytes", CHIP8_MAX_ROM);
    return NULL;
  }

  // Clear to zero
  memset(vm, 0, sizeof(chip8_vm));

  // Load ROM
  memcpy(vm->ram + 0x200, rom, romSize);

  memcpy(vm->ram, fontROM, sizeof(fontROM));

  vm->screen = &vm->ram[0xf00];
  vm->_stack = (u16 *)&vm->ram[0xea0];
  vm->pc = 0x200;
  vm->sp = 0;
  vm->_size = romSize;
  vm->random = random;

  ClearScreen(vm);

  return vm;
}

// Operations based on first nibble

static void
Op_0(chip8_vm *vm, u16 ins)
{
  switch(ins) {
  case 0x00ee: {
    Assert(vm->sp > 0);
    vm->pc = vm->_stack[--vm->sp];
  } break;

  case 0x00e0: {
    ClearScreen(vm);
  } break;

  default: {
    vm->pc = ins;
  } break;
  }
}

static void
Op_1(chip8_vm *vm, u16 ins)
{
  vm->pc = ins & 0xfff;
}

static void
Op_2(chip8_vm *vm, u16 ins)
{
  Assert(vm->sp < 16);
  vm->_stack[vm->sp++] = vm->pc;
  vm->pc = ins & 0xfff;
}

static void
Op_3(chip8_vm *vm, u16 ins)
{
  u8 vx = vm->v[(ins >> 8) & 0xf];

  if(vx == (ins & 0xff)) {
    vm->pc += 2;
  }
}

static void
Op_4(chip8_vm *vm, u16 ins)
{
  u8 vx = vm->v[(ins >> 8) & 0xf];

  if(vx != (ins & 0xff)) {
    vm->pc += 2;
  }
}

static void
Op_5(chip8_vm *vm, u16 ins)
{
  Assert((ins & 0xf) == 0);
  u8 vx = vm->v[(ins >> 8) & 0xf];
  u8 vy = vm->v[(ins >> 4) & 0xf];
  if(vx == vy) {
    vm->pc += 2;
  }
}

static void
Op_6(chip8_vm *vm, u16 ins)
{
  vm->v[(ins >> 8) & 0xf] = ins & 0xff;
}

static void
Op_7(chip8_vm *vm, u16 ins)
{
  vm->v[(ins >> 8) & 0xf] += ins & 0xff;
}

static void
Op_8(chip8_vm *vm, u16 ins)
{
  int x = (ins >> 8) & 0xf;
  int y = (ins >> 4) & 0xf;

  switch(ins & 0xf) {
  // 8XY0
  case 0: {
    vm->v[x] = vm->v[y];
  } break;

  // 8XY1
  case 1: {
    vm->v[x] = vm->v[x] | vm->v[y];
  } break;

  // 8XY2
  case 2: {
    vm->v[x] = vm->v[x] & vm->v[y];
  } break;

  // 8XY3
  case 3: {
    vm->v[x] = vm->v[x] ^ vm->v[y];
  } break;

  // 8XY4
  case 4: {
    u8 vx = vm->v[x];
    u8 vy = vm->v[y];
    vm->v[x] = vx + vy;
    vm->v[0xf] = ((u16)vx + (u16)vy) > 255 ? 0x01 : 0x00;
  } break;

  // 8XY5
  case 5: {
    u8 vx = vm->v[x];
    u8 vy = vm->v[y];
    vm->v[x] = vx - vy;
    vm->v[0xf] = vx < vy ? 0x00 : 0x01;
  } break;

  // 8XY6
  case 6: {
    vm->v[0xf] = vm->v[x] & 0x1;
    vm->v[x] = vm->v[y] >> 1;
  } break;

  // 8XY7
  case 7: {
    u8 vx = vm->v[x];
    u8 vy = vm->v[y];
    vm->v[x] = vy - vx;
    vm->v[0xf] = vy < vx ? 0x00 : 0x01;
  } break;

  // 8XYE
  case 0xe: {
    vm->v[0xf] = (vm->v[x] >> 7) & 0x1;
    vm->v[x] = vm->v[y] << 1;
  } break;

  default: {
    InvalidCodePath;
  } break;
  }
}

static void
Op_9(chip8_vm *vm, u16 ins)
{
  Assert((ins & 0xf) == 0);
  u8 vx = vm->v[(ins >> 8) & 0xf];
  u8 vy = vm->v[(ins >> 4) & 0xf];
  if(vx != vy) {
    vm->pc += 2;
  }
}

static void
Op_A(chip8_vm *vm, u16 ins)
{
  vm->i = ins & 0xfff;
}

// BNNN
static void
Op_B(chip8_vm *vm, u16 ins)
{
  vm->pc = (ins & 0xfff) + vm->v[0];
}

// CXNN
static void
Op_C(chip8_vm *vm, u16 ins)
{
  vm->v[(ins >> 8) & 0xf] = vm->random() & (ins & 0xff);
}

// DXYN
// Overflow coordinates will be wrapped
// Sprites that are drawn partially off-screen will be clipped
static void
Op_D(chip8_vm *vm, u16 ins)
{
  int startX = vm->v[(ins >> 8) & 0xf];
  int startY = vm->v[(ins >> 4) & 0xf];
  int n = ins & 0xf;

  if(startX >= SCREEN_WIDTH) {
    startX = startX % SCREEN_WIDTH;
  }
  if(startY >= SCREEN_HEIGHT) {
    startY = startY % SCREEN_HEIGHT;
  }

  int endX = Minimum(startX + 8, SCREEN_WIDTH);
  int endY = Minimum(startY + n, SCREEN_HEIGHT);

  vm->v[0xf] = 0;

  for(int y = startY; y < endY; y++) {
    u8 spriteByte = vm->ram[vm->i + (y - startY)];
    for(int x = startX; x < endX; x++) {
      // NOTE: spritePixel and screenPixel are 0 or non-zero
      // not 0 or 1 !!!
      int spritePixel = spriteByte & (0x80 >> (x - startX));
      int screenPixel = Chip8_GetPixel(vm, x, y);

      if(spritePixel) {
        if(screenPixel) {
          vm->v[0xf] = 1;
        }

        SetPixel(vm, x, y, screenPixel == 0 ? true : false);
      }
    }
  }
}

static void
Op_E(chip8_vm *vm, u16 ins)
{
  u8 vx = vm->v[(ins >> 8) & 0xf];

  switch(ins & 0xff) {
  // EX9E
  case 0x9e: {
    if(vm->keystate[vx] == true) {
      vm->pc += 2;
    }
  } break;

  case 0xa1: {
    if(vm->keystate[vx] == false) {
      vm->pc += 2;
    }
  } break;

  default: {
    InvalidCodePath;
  } break;
  }
}

static void
Op_F(chip8_vm *vm, u16 ins)
{
  int x = (ins >> 8) & 0xf;

  switch(ins & 0xff) {
  // FX07
  case 0x07: {
    vm->v[x] = vm->delayTimer;
  } break;

  // FX0A
  case 0x0a: {
    vm->wait = true;
    vm->waitReg = (ins >> 8) & 0xf;
  } break;

  // FX15
  case 0x15: {
    vm->delayTimer = vm->v[x];
  } break;

  // FX18
  case 0x18: {
    vm->soundTimer = vm->v[x];
  } break;

  // FX1E
  case 0x1e: {
    vm->i += vm->v[x];
  } break;

  // FX29
  case 0x29: {
    vm->i = vm->v[x] * 5;
  } break;

  // FX33
  case 0x33: {
    u8 value = vm->v[x];
    vm->ram[vm->i] = value / 100;
    vm->ram[vm->i + 1] = (value / 10) % 10;
    vm->ram[vm->i + 2] = value % 10;
  } break;

  // FX55
  case 0x55: {
    for(int i = 0; i <= x; i++) {
      vm->ram[vm->i + i] = vm->v[i];
    }
  } break;

  // FX65
  case 0x65: {
    for(int i = 0; i <= x; i++) {
      vm->v[i] = vm->ram[vm->i + i];
    }
  } break;
  }
}

void
Chip8_Tick(chip8_vm *vm)
{
  if(vm->wait) {
    return;
  }

  Assert(vm->pc >= 0x200);

  // Debug check for handwritten short chip8 program
  if(vm->pc >= 0x200 + vm->_size)
    return;

  u16 ins = (vm->ram[vm->pc] << 8) | (vm->ram[vm->pc + 1]);
  // printf("[%3x]: %04x, delay timer: %d\n", vm->pc, ins, vm->delayTimer);
  vm->pc += 2;

  switch(ins >> 12) {
  case 0: {
    Op_0(vm, ins);
  } break;

  case 1: {
    Op_1(vm, ins);
  } break;

  case 2: {
    Op_2(vm, ins);
  } break;

  case 3: {
    Op_3(vm, ins);
  } break;

  case 4: {
    Op_4(vm, ins);
  } break;

  case 5: {
    Op_5(vm, ins);
  } break;

  case 6: {
    Op_6(vm, ins);
  } break;

  case 7: {
    Op_7(vm, ins);
  } break;

  case 8: {
    Op_8(vm, ins);
  } break;

  case 9: {
    Op_9(vm, ins);
  } break;

  case 0xa: {
    Op_A(vm, ins);
  } break;

  case 0xb: {
    Op_B(vm, ins);
  } break;

  case 0xc: {
    Op_C(vm, ins);
  } break;

  case 0xd: {
    Op_D(vm, ins);
  } break;

  case 0xe: {
    Op_E(vm, ins);
  } break;

  case 0xf: {
    Op_F(vm, ins);
  } break;
  }
}
