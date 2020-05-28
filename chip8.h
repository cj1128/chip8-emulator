#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>

#define Assert(expr)                                                           \
  if(!(expr)) {                                                                \
    printf("assert error: %s\n", #expr);                                       \
    *(volatile int *)0 = 0;                                                    \
  }

#define InvalidCodePath Assert(!"invalid code path")
#define Minimum(a, b) ((a) < (b) ? a : b)

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef unsigned int uint;

struct chip8_vm;
typedef void(update_keyboard_fn)(struct chip8_vm *vm, bool keystate[]);
typedef void(update_screen_fn)(u8 *screen);
typedef u8(wait_keyboard_fn)(void);
typedef u8(random_number_fn)(void);

typedef struct chip8_vm {
  u8 ram[4096];
  u8 *screen; // ram[0xf00 ~ 0xfff]

  u16 *_stack;
  u16 _size; // ROM size in bytes

  u8 v[16];
  u16 i;
  u8 delayTimer;
  u8 soundTimer;
  u16 pc;
  u8 sp;

  bool keystate[16];
  bool stop;
  bool wait;
  int waitReg;

  // Platform services
  random_number_fn *random;
} chip8_vm;

chip8_vm *Chip8_New(u8 rom[], uint romSize, random_number_fn *random);

char *Chip8_GetError(void);

void Chip8_Tick(chip8_vm *vm);

int Chip8_GetPixel(chip8_vm *vm, int x, int y);

#endif
