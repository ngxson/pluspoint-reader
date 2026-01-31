#pragma once

#include <GfxRenderer.h>
#include <MappedInputManager.h>

#include <cstring>
#include <vector>

extern "C" {
#include <mquickjs.h>
}

class AppRunner {
 public:
  static constexpr size_t MAX_PROG_SIZE = 32 * 1024;  // 32KB
  static constexpr size_t MAX_MEM_SIZE = 64 * 1024;   // 64KB

  bool running = false;
  bool exited = false;
  std::vector<char> prog;  // need to be alloc and set before run()
  std::vector<char> mem;
  JSContext* jsCtx = nullptr;

  GfxRenderer* renderer = nullptr;
  MappedInputManager* mappedInput = nullptr;

  void reset() {
    this->running = false;
    this->exited = false;
    this->prog.clear();
    this->mem.clear();
    this->jsCtx = nullptr;
  }

  void run(GfxRenderer* renderer, MappedInputManager* mappedInput);

  // need to be singleton so that the binded JS functions can access it
  static AppRunner& getInstance() { return instance; }

 private:
  static AppRunner instance;
};
