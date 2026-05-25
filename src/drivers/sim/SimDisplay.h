#pragma once

#ifdef SIMULATOR

#include <os/hw/Display.h>
#include <SDL2/SDL.h>
#include <cstdint>
#include <vector>

class SimDisplay : public Display {
 public:
  SimDisplay(int width = 800, int height = 480);
  ~SimDisplay();

  void begin() override;

  uint8_t* getFrameBuffer() override;
  uint16_t getWidth() const override;
  uint16_t getHeight() const override;
  uint16_t getWidthBytes() const override;

  void displayBuffer(RefreshMode mode = FAST_REFRESH) override;
  void refreshDisplay(RefreshMode mode = FAST_REFRESH) override;

  bool shouldClose() const;
  void pollEvents();

 private:
  int w, h;
  std::vector<uint8_t> framebuffer;
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  SDL_Texture* texture = nullptr;
  bool closed = false;
};

#endif  // SIMULATOR
