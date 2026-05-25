#ifdef SIMULATOR

#include "SimDisplay.h"

SimDisplay::SimDisplay(int width, int height) : w(width), h(height) {
  framebuffer.resize((w / 8) * h, 0xFF);  // 0xFF = all bits set = white
}

void SimDisplay::begin() {
  // Window is portrait (h x w) — physical panel is landscape but mounted 90° CW on real device
  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow("Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, h, w, 0);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, h, w);
}

SimDisplay::~SimDisplay() {
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

uint8_t* SimDisplay::getFrameBuffer() { return framebuffer.data(); }
uint16_t SimDisplay::getWidth() const { return (uint16_t)w; }
uint16_t SimDisplay::getHeight() const { return (uint16_t)h; }
uint16_t SimDisplay::getWidthBytes() const { return (uint16_t)(w / 8); }

void SimDisplay::displayBuffer(RefreshMode /*mode*/) {
  // Window is h×w (portrait). Apply inverse of Portrait rotation to map physical
  // framebuffer (w×h landscape) to the portrait window for display.
  // Portrait forward:  phyX = logY,  phyY = h-1-logX
  // Portrait inverse:  winX = h-1-phyY, winY = phyX   (window width=h, height=w)
  std::vector<uint32_t> pixels((size_t)h * w);
  for (int phyY = 0; phyY < h; phyY++) {
    for (int phyX = 0; phyX < w; phyX++) {
      const int byteIdx = phyY * (w / 8) + (phyX / 8);
      const int bitPos = 7 - (phyX % 8);
      const bool isBlack = !((framebuffer[byteIdx] >> bitPos) & 1);
      const int winX = h - 1 - phyY;
      const int winY = phyX;
      pixels[(size_t)winY * h + winX] = isBlack ? 0xFF000000u : 0xFFFFFFFFu;
    }
  }
  SDL_UpdateTexture(texture, nullptr, pixels.data(), h * (int)sizeof(uint32_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
  printf("[SimDisplay] turnOffScreen: %s\n", turnOffScreen ? "true" : "false");
}

void SimDisplay::refreshDisplay(RefreshMode mode) {
  displayBuffer(mode);
}

bool SimDisplay::shouldClose() const { return closed; }

void SimDisplay::pollEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) closed = true;
  }
}

#endif  // SIMULATOR
