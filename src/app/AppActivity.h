#pragma once
#include <EInkDisplay.h>
#include <EpdFontFamily.h>

#include <string>
#include <utility>
#include <functional>
#include <elk.h>

#include "../activities/Activity.h"

// hacky solution to avoid changing too much code in main.cpp
extern std::function<void()> onGoToApps;

class AppActivity final : public Activity {
  TaskHandle_t displayTaskHandle = nullptr;
  TaskHandle_t appTaskHandle = nullptr;
  SemaphoreHandle_t renderingMutex = nullptr;
  bool updateRequired = false;
  const std::function<void()> onGoHome;

  static void taskTrampoline(void* param);
  static void taskAppTrampoline(void* param);
  [[noreturn]] void displayTaskLoop();
  [[noreturn]] void appTaskLoop();
  void render() const;
  void startProgram(std::string programName);

 public:
  explicit AppActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                            const std::function<void()>& onGoHome)
      : Activity("Settings", renderer, mappedInput), onGoHome(onGoHome) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;

  GfxRenderer& getRenderer() { return renderer; }
  MappedInputManager& getMappedInput() { return mappedInput; }

  // state
  std::vector<std::string> programs;
  size_t selectedIdx = 0;
  struct ProgramContext {
    bool running = false;
    bool exited = false;
    std::string code;
    std::vector<char> mem;
    js* jsCtx = nullptr; // note: allocated inside mem, no need free
  };
  ProgramContext ctx;
};
