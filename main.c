#include "chip8.h"
#include <stdlib.h>
#include <time.h>
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

  SDL_AudioDeviceID audioDevice;
  uint audioSampleCount;
  void *audioBuf;
  uint audioBufLength;
} sdl_handle;

sdl_handle gSDL;

// Return chip8 key mapping for sdl keycode
// return -1 if not found
static int
MapForChip8(u32 keycode)
{
  switch(keycode) {
    case SDLK_1:
      return 1;
    case SDLK_2:
      return 2;
    case SDLK_3:
      return 3;
    case SDLK_4:
      return 4;

    case SDLK_q:
      return 4;
    case SDLK_w:
      return 5;
    case SDLK_e:
      return 6;
    case SDLK_r:
      return 0xd;

    case SDLK_a:
      return 7;
    case SDLK_s:
      return 8;
    case SDLK_d:
      return 9;
    case SDLK_f:
      return 0xe;

    case SDLK_z:
      return 0xa;
    case SDLK_x:
      return 0;
    case SDLK_c:
      return 0xb;
    case SDLK_v:
      return 0xf;
  }

  return -1;
}

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
  fclose(f);

  file_content result = {};
  result.buf = buf;
  result.size = length;

  return result;
}

void
ProcessEvents(chip8_vm *vm)
{
  SDL_Event e;

  while(SDL_PollEvent(&e)) {
    if(e.type == SDL_QUIT) {
      vm->stop = true;
      return;
    }

    if(e.type == SDL_KEYDOWN) {
      if(e.key.keysym.sym == SDLK_ESCAPE) {
        vm->stop = true;
      }

      int chip8Key = MapForChip8(e.key.keysym.sym);
      if(chip8Key != -1) {
        vm->keystate[chip8Key] = true;
        if(vm->wait) {
          vm->wait = false;
          vm->v[vm->waitReg] = chip8Key;
        }
      }
    }

    if(e.type == SDL_KEYUP) {
      int chip8Key = MapForChip8(e.key.keysym.sym);
      if(chip8Key != -1) {
        vm->keystate[chip8Key] = false;
      }
    }
  }
}

#define PIXEL_ON_COLOR 0x8f9185ff
#define PIXEL_OFF_COLOR 0x111d2bff

// 64x32
// pixel format: RGBA
void
UpdateScreen(chip8_vm *vm)
{
  void *rawPixels;
  int pitch;
  SDL_LockTexture(gSDL.screen, NULL, &rawPixels, &pitch);

  u32(*pixels)[pitch / 4] = rawPixels;

  for(int y = 0; y < SCREEN_HEIGHT; y++) {
    for(int x = 0; x < SCREEN_WIDTH; x++) {
      int pixel = Chip8_GetPixel(vm, x, y);
      pixels[y][x] = pixel == 0 ? PIXEL_OFF_COLOR : PIXEL_ON_COLOR;
    }
  }

  SDL_UnlockTexture(gSDL.screen);

  SDL_RenderCopy(gSDL.renderer, gSDL.screen, NULL, NULL);

  SDL_RenderPresent(gSDL.renderer);
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

  // Audio
  if(SDL_GetNumAudioDevices(0) <= 0) {
    gSDL.audioDevice = 0;
  } else {
    SDL_AudioSpec want = {}, have;
    SDL_AudioDeviceID device;
    want.freq = 64 * 60;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 64;

    device = SDL_OpenAudioDevice(NULL,
      0,
      &want,
      &have,
      SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if(device == 0) {
      printf("error: %s", SDL_GetError());
      exit(1);
    }

    gSDL.audioDevice = device;
    gSDL.audioSampleCount = have.samples * have.channels;
    uint bufLength = gSDL.audioSampleCount * 4;
    gSDL.audioBuf = malloc(bufLength);
    if(gSDL.audioBuf == NULL) {
      printf("malloc failed");
      exit(1);
    }
    gSDL.audioBufLength = bufLength;

    SDL_PauseAudioDevice(device, 0);
  }
}

void
UpdateAudio(chip8_vm *vm)
{
  if(gSDL.audioDevice == 0)
    return;

  float *buf = (float *)gSDL.audioBuf;
  float value = vm->soundTimer > 0 ? 1.0f : 0.0f;

  // NOTE: only have 1 channel
  for(int i = 0; i < gSDL.audioSampleCount; i++) {
    buf[i] = value;
  }

  SDL_QueueAudio(gSDL.audioDevice, buf, gSDL.audioBufLength);
}

void
track(char *label, u32 *t)
{
  u32 tick = SDL_GetTicks();
  printf("%s: %d\n", label, tick - *t);
  *t = tick;
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
  chip8_vm *vm = Chip8_New(content.buf, content.size, RandomNumber);

  if(vm == NULL) {
    printf("%s\n", Chip8_GetError());
    return 1;
  }

  u32 renderTick = 0;
  u32 renderInterval = 1000 / 60;

  u32 timerTick = SDL_GetTicks();
  u32 timerInterval = 1000 / 60;

  while(!vm->stop) {
    if(SDL_GetTicks() - timerTick >= timerInterval) {
      timerTick = SDL_GetTicks();
      if(vm->delayTimer > 0) {
        vm->delayTimer--;
      }

      if(vm->soundTimer > 0) {
        vm->soundTimer--;
      }
    }

    ProcessEvents(vm);

    Chip8_Tick(vm);
    SDL_Delay(1);

    if(SDL_GetTicks() - renderTick >= renderInterval) {
      UpdateScreen(vm);
      renderTick = SDL_GetTicks();

      UpdateAudio(vm);
    }

#if 0
    // Debug info
    {
      static u32 perfTick = 0;
      u32 tick = SDL_GetTicks();
      printf("ms: %d, fsp: %f\n", tick - perfTick, 1000.0f / (tick - perfTick));
      perfTick = tick;
    }
#endif
  }

  return 0;
}
