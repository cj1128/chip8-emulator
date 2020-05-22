#include "chip8.h"
#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>

typedef struct {
  u8 *buf;
  long size;
} file_content;

typedef struct {
  SDL_Window *win;
  SDL_Renderer *renderer;
  SDL_Texture *screen;
} sdl_handle;

sdl_handle gSDL;

file_content
ReadFile(char *path)
{
  FILE *f = fopen(path, "rb");
  if(f == NULL) {
    printf("error: can not open file %s\n", path);
    exit(1);
  }

  fseek(f, 0, SEEK_END);
  long length = ftell(f);
  fseek(f, 0, SEEK_SET);

  void *buf = malloc(length);
  if(buf == NULL) {
    printf("error: malloc failed\n");
    exit(1);
  }

  size_t read = fread(buf, 1, length, f);
  if(read != length) {
    printf("error: fread failed\n");
    exit(1);
  }

  file_content result = {};
  result.buf = buf;
  result.size = length;

  return result;
}

void
UpdateKeyboard(chip8_vm *vm, bool keystate[])
{
  SDL_Event e;

  while(SDL_PollEvent(&e)) {
    if(e.type == SDL_QUIT) {
      vm->stop = true;
    }

    if(e.type == SDL_KEYDOWN) {
      switch(e.key.keysym.sym) {
      case SDLK_ESCAPE:
        vm->stop = true;
        break;
      }
    }
  }
}

#define PIXEL_ON_COLOR 0xffffffff
#define PIXEL_OFF_COLOR 0x000000ff

// 64x32
// pixel format: RGBA
void
UpdateScreen(u8 screen[])
{
  void *rawPixels;
  int pitch;
  SDL_LockTexture(gSDL.screen, NULL, &rawPixels, &pitch);

  int width = pitch / 4;
  typedef uint32_t(*pixels_t)[width];
  pixels_t pixels = (pixels_t)rawPixels;

  for(int y = 0; y < SCREEN_HEIGHT; y++) {
    for(int x = 0; x < SCREEN_WIDTH; x++) {
      int pixel = Chip8_GetPixel(screen, x, y);
      pixels[y][x] = pixel == 0 ? PIXEL_OFF_COLOR : PIXEL_ON_COLOR;
    }
  }

  SDL_UnlockTexture(gSDL.screen);

  SDL_RenderCopy(gSDL.renderer, gSDL.screen, NULL, NULL);

  SDL_RenderPresent(gSDL.renderer);
}

u8
WaitKeyboard(void)
{
  // TODO
  return 0;
}

u8
RandomNumber(void)
{
  return rand() % 256;
}

void
InitSDL(char *name, int width, int height, int scale)
{
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    printf("error: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_Window *win = SDL_CreateWindow(name,
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    width * scale,
    height * scale,
    SDL_WINDOW_SHOWN);

  if(win == NULL) {
    printf("error: %s\n", SDL_GetError());
    exit(1);
  }

  uint32_t flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
  SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, flags);
  if(renderer == NULL) {
    printf("error: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_Texture *texture = SDL_CreateTexture(renderer,
    SDL_PIXELFORMAT_RGBA8888,
    SDL_TEXTUREACCESS_STREAMING,
    width,
    height);

  if(texture == NULL) {
    printf("error: %s\n", SDL_GetError());
    exit(1);
  }

  gSDL.win = win;
  gSDL.renderer = renderer;
  gSDL.screen = texture;
}

int
main(int argc, char *argv[])
{
  if(argc == 1) {
    printf("Usage: chip8 <rom>");
    exit(1);
  }

  // Seed the rand
  srand(time(0));

  int scale = 10;
  InitSDL("Chip8 Emulator", SCREEN_WIDTH, SCREEN_HEIGHT, scale);

  file_content content = ReadFile(argv[1]);
  chip8_vm *vm = Chip8_New(content.buf,
    content.size,
    UpdateKeyboard,
    UpdateScreen,
    WaitKeyboard,
    RandomNumber);

  if(vm == NULL) {
    printf("%s\n", Chip8_GetError());
    return 1;
  }

  Chip8_Run(vm);

  return 0;
}
