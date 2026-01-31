#pragma once

#include <vector>
#include <cstring>
#include <GfxRenderer.h>

extern "C" {
#include <mquickjs.h>
}

class AppRunner {
 public:
  static constexpr size_t MAX_PROG_SIZE = 32 * 1024; // 32KB
  static constexpr size_t MAX_MEM_SIZE = 64 * 1024;  // 64KB

  bool running = false;
  bool exited = false;
  std::vector<char> prog;
  std::vector<char> mem;
  JSContext* jsCtx = nullptr;

  GfxRenderer* renderer = nullptr;

  AppRunner() {
    prog.resize(MAX_PROG_SIZE);
    mem.resize(MAX_MEM_SIZE);
  }

  void reset() {
    this->running = false;
    this->exited = false;
    memset(prog.data(), 0, prog.size());
    memset(mem.data(), 0, mem.size());
    this->jsCtx = nullptr;
  }

  void run(GfxRenderer* gfxRenderer);

  // need to be singleton so that the binded JS functions can access it
  static AppRunner& getInstance() {
    return instance;
  }

 private:
  static AppRunner instance;
};
