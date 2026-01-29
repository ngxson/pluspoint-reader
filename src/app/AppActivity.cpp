#include "AppActivity.h"
#include "../MappedInputManager.h"
#include "../util/StringUtils.h"

#include <SDCardManager.h>
#include <GfxRenderer.h>
#include "fontIds.h"



extern "C" {

#include <mquickjs.h>

static JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    return JS_UNDEFINED;
}

#include <mqjs_stdlib.h>

}





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

  xSemaphoreTake(renderingMutex, portMAX_DELAY);
  if (displayTaskHandle) {
    vTaskDelete(displayTaskHandle);
    displayTaskHandle = nullptr;
  }
  vSemaphoreDelete(renderingMutex);
  renderingMutex = nullptr;
}

void AppActivity::loop() {
  if (ctx.running) {
    return;
  }

  if (ctx.exited) {
    if (appTaskHandle) {
      vTaskDelete(appTaskHandle);
      appTaskHandle = nullptr;
    }
    // clean up after program exit
    ctx = ProgramContext();
    ctx.exited = false;
    updateRequired = true;
    // give back rendering control
    xSemaphoreGive(renderingMutex);
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    // delegate rendering to the app
    xSemaphoreTake(renderingMutex, portMAX_DELAY);
    startProgram(programs[selectedIdx]); // TODO: handle errors
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
    if (updateRequired) { //&& !subActivity) {
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
/*
std::string js_getstr(struct js* js, jsval_t val) {
  size_t len = 0;
  char* cstr = js_getstr(js, val, &len);
  if (!cstr) return "";
  return std::string(cstr, len);
}

#define RENDERER instance->getRenderer()

// drawCenteredText(fontId, y, text, black, style)
jsval_t drawCenteredText(struct js* js, jsval_t* args, int nargs) {
  if (nargs != 5) return js_mkerr(js, "5 args expected");
  std::string fontStr = js_getstr(js, args[0]);
  int y = js_getnum(args[1]);
  std::string text = js_getstr(js, args[2]);
  bool black = js_getbool(args[3]);
  std::string styleStr = js_getstr(js, args[4]);

  int fontId = fontIdFromString(fontStr);
  if (fontId == -1) return js_mkerr(js, "invalid font ID");
  EpdFontFamily::Style style = styleFromString(styleStr);

  RENDERER.drawCenteredText(fontId, y, text.c_str(), black, style);
  return js_mkval(JS_NULL);
}

// drawText(fontId, x, y, text, black, style)
jsval_t drawText(struct js* js, jsval_t* args, int nargs) {
  if (nargs != 6) return js_mkerr(js, "6 args expected");
  std::string fontStr = js_getstr(js, args[0]);
  int x = js_getnum(args[1]);
  int y = js_getnum(args[2]);
  std::string text = js_getstr(js, args[3]);
  bool black = js_getbool(args[4]);
  std::string styleStr = js_getstr(js, args[5]);

  int fontId = fontIdFromString(fontStr);
  if (fontId == -1) return js_mkerr(js, "invalid font ID");
  EpdFontFamily::Style style = styleFromString(styleStr);

  RENDERER.drawText(fontId, x, y, text.c_str(), black, style);
  return js_mkval(JS_NULL);
}

// clearScreen(black)
jsval_t clearScreen(struct js* js, jsval_t* args, int nargs) {
  if (nargs != 1) return js_mkerr(js, "1 arg expected");
  uint8_t black = js_getbool(args[0]);
  RENDERER.clearScreen(black ? 0x00 : 0xFF);
  return js_mkval(JS_NULL);
}

// drawButtonHints(fontId, btn1, btn2, btn3, btn4)
jsval_t drawButtonHints(struct js* js, jsval_t* args, int nargs) {
  if (nargs != 5) return js_mkerr(js, "5 args expected");
  std::string fontStr = js_getstr(js, args[0]);
  std::string btn1 = js_getstr(js, args[1]);
  std::string btn2 = js_getstr(js, args[2]);
  std::string btn3 = js_getstr(js, args[3]);
  std::string btn4 = js_getstr(js, args[4]);

  int fontId = fontIdFromString(fontStr);
  if (fontId == -1) return js_mkerr(js, "invalid font ID");

  RENDERER.drawButtonHints(fontId, btn1.c_str(), btn2.c_str(), btn3.c_str(), btn4.c_str());
  return js_mkval(JS_NULL);
}

// displayBuffer() // TODO: support refresh mode
jsval_t displayBuffer(struct js* js, jsval_t* args, int nargs) {
  if (nargs != 0) return js_mkerr(js, "0 args expected");
  RENDERER.displayBuffer();
  return js_mkval(JS_NULL);
}

// isButtonPressed(button)
jsval_t isButtonPressed(struct js* js, jsval_t* args, int nargs) {
  if (nargs != 1) return js_mkerr(js, "1 arg expected");
  int buttonInt = js_getnum(args[0]);
  auto button = static_cast<MappedInputManager::Button>(buttonInt);
  bool pressed = instance->getMappedInput().isPressed(button);
  return js_mkval(pressed ? JS_TRUE : JS_FALSE);
}

// delay(ms)
jsval_t jsDelay(struct js* js, jsval_t* args, int nargs) {
  if (nargs != 1) return js_mkerr(js, "1 arg expected");
  int ms = js_getnum(args[0]);
  vTaskDelay(ms / portTICK_PERIOD_MS);
  return js_mkval(JS_NULL);
}

// millis()
jsval_t jsMillis(struct js* js, jsval_t* args, int nargs) {
  if (nargs != 0) return js_mkerr(js, "0 args expected");
  unsigned long ms = millis();
  return js_mknum(static_cast<double>(ms));
}

// print(string)
jsval_t jsPrint(struct js* js, jsval_t* args, int nargs) {
  if (nargs != 1) return js_mkerr(js, "1 arg expected");
  std::string msg = js_getstr(js, args[0]);
  Serial.printf("[%lu] [JS ] %s\n", millis(), msg.c_str());
  return js_mkval(JS_NULL);
}

// toString(value)
jsval_t jsToString(struct js* js, jsval_t* args, int nargs) {
  if (nargs != 1) return js_mkerr(js, "1 arg expected");
  jsval_t val = args[0];
  std::string str;
  switch (js_type(val)) {
    case JS_NUM:
      str = std::to_string(js_getnum(val));
      break;
    case JS_TRUE:
    case JS_FALSE:
      str = js_getbool(val) ? "true" : "false";
      break;
    case JS_STR:
      str = js_getstr(js, val);
      break;
    default:
      str = "[object]";
      break;
  }
  return js_mkstr(js, str.c_str(), str.size());
}


// main program starter

void AppActivity::startProgram(std::string programName) {
  std::string fullPath = "/apps/" + programName;
  FsFile file = SdMan.open(fullPath.c_str(), O_RDONLY);
  assert(file && file.isOpen());
  size_t fileSize = file.size();

  // create JS context
  ctx = ProgramContext();
  ctx.mem.resize(64 * 1024); // 64KB for now
  ctx.jsCtx = js_create(ctx.mem.data(), ctx.mem.size());

  // load program code
  ctx.code.resize(fileSize);
  size_t bytesRead = file.read(&ctx.code[0], fileSize);
  assert(bytesRead == fileSize);
  file.close();
  Serial.printf("[%lu] [APP] Starting program: %s (%u bytes)\n", millis(), programName.c_str(), (unsigned)ctx.code.size());

  // prepare JS environment
  auto global = js_glob(ctx.jsCtx);
  instance = this;

  js_set(ctx.jsCtx, global, "pageWidth", js_mknum(renderer.getScreenWidth()));
  js_set(ctx.jsCtx, global, "pageHeight", js_mknum(renderer.getScreenHeight()));

  js_set(ctx.jsCtx, global, "BTN_UP", js_mknum((int)MappedInputManager::Button::Up));
  js_set(ctx.jsCtx, global, "BTN_DOWN", js_mknum((int)MappedInputManager::Button::Down));
  js_set(ctx.jsCtx, global, "BTN_LEFT", js_mknum((int)MappedInputManager::Button::Left));
  js_set(ctx.jsCtx, global, "BTN_RIGHT", js_mknum((int)MappedInputManager::Button::Right));
  js_set(ctx.jsCtx, global, "BTN_BACK", js_mknum((int)MappedInputManager::Button::Back));
  js_set(ctx.jsCtx, global, "BTN_CONFIRM", js_mknum((int)MappedInputManager::Button::Confirm));

  js_set(ctx.jsCtx, global, "delay", js_mkfun(jsDelay));
  js_set(ctx.jsCtx, global, "millis", js_mkfun(jsMillis));
  js_set(ctx.jsCtx, global, "print", js_mkfun(jsPrint));
  js_set(ctx.jsCtx, global, "isButtonPressed", js_mkfun(isButtonPressed));
  js_set(ctx.jsCtx, global, "toString", js_mkfun(jsToString));

  js_set(ctx.jsCtx, global, "drawCenteredText", js_mkfun(drawCenteredText));
  js_set(ctx.jsCtx, global, "drawText", js_mkfun(drawText));
  js_set(ctx.jsCtx, global, "clearScreen", js_mkfun(clearScreen));
  js_set(ctx.jsCtx, global, "drawButtonHints", js_mkfun(drawButtonHints));
  js_set(ctx.jsCtx, global, "displayBuffer", js_mkfun(displayBuffer));

  // clear screen before running
  renderer.clearScreen();
  renderer.displayBuffer();

  // start new task
  ctx.running = true;
  xTaskCreate(&AppActivity::taskAppTrampoline, "AppRuntimeTask",
              4096,               // Stack size
              this,               // Parameters
              1,                  // Priority
              &appTaskHandle      // Task handle
  );

  Serial.printf("[%lu] [APP] Program started\n", millis());
}

void AppActivity::appTaskLoop() {
  assert(ctx.running && "program not running");

  // run program code
  jsval_t result = js_eval(ctx.jsCtx, ctx.code.c_str(), ctx.code.size());

  if (js_type(result) == JS_ERR) {
    std::string errMsg = js_getstr(ctx.jsCtx, result);
    Serial.printf("App error: %s\n", errMsg.c_str());
  }

  // program ended
  Serial.printf("[%lu] [APP] Program ended\n", millis());
  ctx.running = false;
  ctx.exited = true;

  // keep task alive
  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
*/


void AppActivity::startProgram(std::string programName) {
  std::string fullPath = "/apps/" + programName;
  FsFile file = SdMan.open(fullPath.c_str(), O_RDONLY);
  assert(file && file.isOpen());
  size_t fileSize = file.size();

  // create JS context
  ctx = ProgramContext();
  ctx.mem.resize(64 * 1024); // 64KB for now
  ctx.jsCtx = JS_NewContext(ctx.mem.data(), ctx.mem.size(), &js_stdlib);

  // load program code
  ctx.prog.resize(fileSize);
  size_t bytesRead = file.read(&ctx.prog[0], fileSize);
  assert(bytesRead == fileSize);
  file.close();
  Serial.printf("[%lu] [APP] Starting program: %s (%u bytes)\n", millis(), programName.c_str(), (unsigned)ctx.prog.size());
}

void AppActivity::appTaskLoop() {}








