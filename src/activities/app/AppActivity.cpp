#include "AppActivity.h"

#include <GfxRenderer.h>
#include <MappedInputManager.h>
#include <SDCardManager.h>

#include "../../util/StringUtils.h"
#include "fontIds.h"

void AppActivity::taskTrampoline(void* param) {
  auto* self = static_cast<AppActivity*>(param);
  self->displayTaskLoop();
}

void AppActivity::taskAppTrampoline(void* param) {
  auto* self = static_cast<AppActivity*>(param);
  self->appTaskLoop();
}

void AppActivity::onEnter() {
  Activity::onEnter();
  renderingMutex = xSemaphoreCreateMutex();

  selectedIdx = 0;
  programs.clear();

  // load available applications from /apps directory
  FsFile dir = SdMan.open("/apps");
  if (dir && dir.isDirectory()) {
    dir.rewindDirectory();
    for (FsFile file = dir.openNextFile(); file; file = dir.openNextFile()) {
      char name[256];
      file.getName(name, sizeof(name));
      std::string filename(name);
      // only accept .js files
      if (StringUtils::checkFileExtension(filename, ".js")) {
        programs.emplace_back(std::move(filename));
      }
      file.close();
    }
  }
  dir.close();

  // Trigger first update
  updateRequired = true;

  xTaskCreate(&AppActivity::taskTrampoline, "AppActivityTask",
              4096,               // Stack size
              this,               // Parameters
              1,                  // Priority
              &displayTaskHandle  // Task handle
  );
}

void AppActivity::onExit() {
  Activity::onExit();

  auto& runner = AppRunner::getInstance();
  runner.reset();

  xSemaphoreTake(renderingMutex, portMAX_DELAY);
  if (displayTaskHandle) {
    vTaskDelete(displayTaskHandle);
    displayTaskHandle = nullptr;
  }
  vSemaphoreDelete(renderingMutex);
  renderingMutex = nullptr;
}

void AppActivity::loop() {
  auto& runner = AppRunner::getInstance();
  if (runner.running) {
    return;
  }

  if (runner.exited) {
    if (appTaskHandle) {
      vTaskDelete(appTaskHandle);
      appTaskHandle = nullptr;
    }
    runner.reset();
    updateRequired = true;
    // give back rendering control
    xSemaphoreGive(renderingMutex);
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    // delegate rendering to the app
    xSemaphoreTake(renderingMutex, portMAX_DELAY);
    startProgram(programs[selectedIdx]);  // TODO: handle errors
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    onGoHome();
    return;
  }

  // Handle navigation
  if (mappedInput.wasPressed(MappedInputManager::Button::Up) ||
      mappedInput.wasPressed(MappedInputManager::Button::Left)) {
    // Move selection up (with wrap-around)
    selectedIdx = (selectedIdx > 0) ? (selectedIdx - 1) : (programs.size() - 1);
    updateRequired = true;
  } else if (mappedInput.wasPressed(MappedInputManager::Button::Down) ||
             mappedInput.wasPressed(MappedInputManager::Button::Right)) {
    // Move selection down (with wrap around)
    selectedIdx = (selectedIdx + 1) % programs.size();
    updateRequired = true;
  }
}

void AppActivity::displayTaskLoop() {
  while (true) {
    if (updateRequired) {  //&& !subActivity) {
      updateRequired = false;
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      render();
      xSemaphoreGive(renderingMutex);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void AppActivity::render() const {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  // Draw header
  renderer.drawCenteredText(UI_12_FONT_ID, 15, "Applications", true, EpdFontFamily::BOLD);

  if (programs.empty()) {
    renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2, "No applications found", true);
  } else {
    // Draw selection
    renderer.fillRect(0, 60 + selectedIdx * 30 - 2, pageWidth - 1, 30);

    // Draw all programs
    for (int i = 0; i < programs.size(); i++) {
      const int programY = 60 + i * 30;  // 30 pixels between programs

      // Draw program name
      renderer.drawText(UI_10_FONT_ID, 20, programY, programs[i].c_str(), i != selectedIdx);
    }
  }

  // Draw help text
  const auto labels = mappedInput.mapLabels("« Back", "Select", "", "");
  renderer.drawButtonHints(UI_10_FONT_ID, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  // Always use standard refresh for settings screen
  renderer.displayBuffer();
}

//
// APP RUNNER
//

// singleton for now
AppActivity* instance = nullptr;

int fontIdFromString(const std::string& fontStr) {
  if (fontStr == "BOOKERLY_12") return BOOKERLY_12_FONT_ID;
  if (fontStr == "BOOKERLY_14") return BOOKERLY_14_FONT_ID;
  if (fontStr == "BOOKERLY_16") return BOOKERLY_16_FONT_ID;
  if (fontStr == "BOOKERLY_18") return BOOKERLY_18_FONT_ID;
  if (fontStr == "NOTOSANS_12") return NOTOSANS_12_FONT_ID;
  if (fontStr == "NOTOSANS_14") return NOTOSANS_14_FONT_ID;
  if (fontStr == "NOTOSANS_16") return NOTOSANS_16_FONT_ID;
  if (fontStr == "NOTOSANS_18") return NOTOSANS_18_FONT_ID;
  if (fontStr == "OPENDYSLEXIC_8") return OPENDYSLEXIC_8_FONT_ID;
  if (fontStr == "OPENDYSLEXIC_10") return OPENDYSLEXIC_10_FONT_ID;
  if (fontStr == "OPENDYSLEXIC_12") return OPENDYSLEXIC_12_FONT_ID;
  if (fontStr == "OPENDYSLEXIC_14") return OPENDYSLEXIC_14_FONT_ID;
  if (fontStr == "UI_10") return UI_10_FONT_ID;
  if (fontStr == "UI_12") return UI_12_FONT_ID;
  if (fontStr == "SMALL") return SMALL_FONT_ID;
  return -1;
}

EpdFontFamily::Style styleFromString(const std::string& styleStr) {
  if (styleStr == "REGULAR") return EpdFontFamily::REGULAR;
  if (styleStr == "BOLD") return EpdFontFamily::BOLD;
  if (styleStr == "ITALIC") return EpdFontFamily::ITALIC;
  if (styleStr == "BOLD_ITALIC") return EpdFontFamily::BOLD_ITALIC;
  return EpdFontFamily::REGULAR;
}

void AppActivity::startProgram(std::string programName) {
  std::string fullPath = "/apps/" + programName;
  FsFile file = SdMan.open(fullPath.c_str(), O_RDONLY);
  assert(file && file.isOpen());
  size_t fileSize = file.size();

  if (fileSize == 0 || fileSize > AppRunner::MAX_PROG_SIZE) {
    // TODO: show as a dialog message
    Serial.printf("[%lu] [APP] Invalid program size: %u bytes, max supported = %u\n", millis(), (unsigned)fileSize,
                  (unsigned)AppRunner::MAX_PROG_SIZE);
    file.close();
    return;
  }

  // prepare runner
  auto& runner = AppRunner::getInstance();
  runner.reset();

  // load program code
  runner.prog.resize(fileSize + 1);
  size_t bytesRead = file.read(&runner.prog[0], fileSize);
  assert(bytesRead == fileSize);
  runner.prog[fileSize] = '\0';
  file.close();
  Serial.printf("[%lu] [APP] Starting program: %s (%u bytes)\n", millis(), programName.c_str(),
                (unsigned)runner.prog.size());

  // clear screen before running
  renderer.clearScreen();
  renderer.displayBuffer();

  // start new task
  runner.running = true;
  xTaskCreate(&AppActivity::taskAppTrampoline, "AppRuntimeTask",
              4096,           // Stack size
              this,           // Parameters
              1,              // Priority
              &appTaskHandle  // Task handle
  );

  Serial.printf("[%lu] [APP] Program started\n", millis());
}

void AppActivity::appTaskLoop() {
  auto& runner = AppRunner::getInstance();
  assert(runner.running && "program not running");

  // run program code
  runner.run(&renderer, &mappedInput);

  // program ended
  Serial.printf("[%lu] [APP] Program ended\n", millis());
  runner.running = false;
  runner.exited = true;

  // keep task alive until main loop cleans up
  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
