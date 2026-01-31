
#include "AppRunner.h"

#include <Arduino.h>

#include <ctime>
#include <string>
#include <vector>

#include "../../fontIds.h"

extern "C" {

#include <mquickjs.h>

static JSValue js_print(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  int i;
  JSValue v;

  for (i = 0; i < argc; i++) {
    if (i != 0) Serial.write(' ');
    v = argv[i];
    if (JS_IsString(ctx, v)) {
      JSCStringBuf buf;
      const char* str;
      size_t len;
      str = JS_ToCStringLen(ctx, &len, v, &buf);
      Serial.write((const uint8_t*)str, len);
    } else {
      JS_PrintValueF(ctx, argv[i], JS_DUMP_LONG);
    }
  }
  Serial.println();
  return JS_UNDEFINED;
}

static JSValue js_date_now(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return JS_NewInt64(ctx, (int64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000));
}

static JSValue js_performance_now(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  return JS_ThrowInternalError(ctx, "js_performance_now not implemented");
}

static JSValue js_gc(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  return JS_ThrowInternalError(ctx, "js_gc not implemented");
}

static JSValue js_load(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  return JS_ThrowInternalError(ctx, "js_load not implemented");
}

static JSValue js_setTimeout(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  return JS_ThrowInternalError(ctx, "js_setTimeout not implemented");
}

static JSValue js_clearTimeout(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  return JS_ThrowInternalError(ctx, "js_clearTimeout not implemented");
}

// Crosspoint-specific functions

AppRunner& appInstance() { return AppRunner::getInstance(); }

const char* argc_err_msg = "Expected at least %d arguments, but got %d";
#define CHECK_ARGC(minArgs)                                     \
  if (argc < minArgs) {                                         \
    return JS_ThrowTypeError(ctx, argc_err_msg, minArgs, argc); \
  }

#define GET_STRING_ARG(index, varName) \
  JSCStringBuf varName##Buf;           \
  size_t varName##Len;                 \
  const char* varName;                 \
  varName = JS_ToCStringLen(ctx, &varName##Len, argv[index], &varName##Buf);

static JSValue js_millis(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  return JS_NewInt64(ctx, millis());
}

static JSValue js_btnIsPressed(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(1);
  GET_STRING_ARG(0, buttonStr);
  if (!buttonStr) {
    return JS_EXCEPTION;
  }
  MappedInputManager::Button button;
  if (strcmp(buttonStr, "B") == 0) {
    button = MappedInputManager::Button::Back;
  } else if (strcmp(buttonStr, "C") == 0) {
    button = MappedInputManager::Button::Confirm;
  } else if (strcmp(buttonStr, "L") == 0) {
    button = MappedInputManager::Button::Left;
  } else if (strcmp(buttonStr, "R") == 0) {
    button = MappedInputManager::Button::Right;
  } else if (strcmp(buttonStr, "U") == 0) {
    button = MappedInputManager::Button::Up;
  } else if (strcmp(buttonStr, "D") == 0) {
    button = MappedInputManager::Button::Down;
  } else {
    return JS_ThrowRangeError(ctx, "invalid button id '%s'", buttonStr);
  }
  bool isPressed = appInstance().mappedInput->isPressed(button);
  return JS_NewBool(isPressed);
}

static JSValue js_getScreenWidth(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  return appInstance().renderer->getScreenWidth();
}

static JSValue js_getScreenHeight(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  return appInstance().renderer->getScreenHeight();
}

static JSValue js_clearScreen(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(1);
  int color;
  if (JS_ToInt32(ctx, &color, argv[0])) return JS_EXCEPTION;
  if (color < 0 || color > 255) {
    return JS_ThrowRangeError(ctx, "color must be between 0 and 255");
  }
  appInstance().renderer->clearScreen((uint8_t)color);
  return JS_UNDEFINED;
}

static JSValue js_displayBuffer(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(1);
  int refreshMode = HalDisplay::FAST_REFRESH;
  if (JS_ToInt32(ctx, &refreshMode, argv[0])) return JS_EXCEPTION;
  appInstance().renderer->displayBuffer((HalDisplay::RefreshMode)refreshMode);
  return JS_UNDEFINED;
}

static JSValue js_drawLine(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(5);
  int x1, y1, x2, y2, state;
  if (JS_ToInt32(ctx, &x1, argv[0]) || JS_ToInt32(ctx, &y1, argv[1]) || JS_ToInt32(ctx, &x2, argv[2]) ||
      JS_ToInt32(ctx, &y2, argv[3]) || JS_ToInt32(ctx, &state, argv[4])) {
    return JS_EXCEPTION;
  }
  appInstance().renderer->drawLine(x1, y1, x2, y2, state != 0);
  return JS_UNDEFINED;
}

static JSValue js_drawRect(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(5);
  int x, y, width, height, state;
  if (JS_ToInt32(ctx, &x, argv[0]) || JS_ToInt32(ctx, &y, argv[1]) || JS_ToInt32(ctx, &width, argv[2]) ||
      JS_ToInt32(ctx, &height, argv[3]) || JS_ToInt32(ctx, &state, argv[4])) {
    return JS_EXCEPTION;
  }
  appInstance().renderer->drawRect(x, y, width, height, state != 0);
  return JS_UNDEFINED;
}

static JSValue js_fillRect(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(5);
  int x, y, width, height, state;
  if (JS_ToInt32(ctx, &x, argv[0]) || JS_ToInt32(ctx, &y, argv[1]) || JS_ToInt32(ctx, &width, argv[2]) ||
      JS_ToInt32(ctx, &height, argv[3]) || JS_ToInt32(ctx, &state, argv[4])) {
    return JS_EXCEPTION;
  }
  appInstance().renderer->fillRect(x, y, width, height, state != 0);
  return JS_UNDEFINED;
}

static JSValue js_drawImage(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(5);
  JSCStringBuf buf;
  size_t len;  // unused for now
  const char* bitmapData = JS_ToCStringLen(ctx, &len, argv[0], &buf);
  int x, y, width, height;
  if (!bitmapData || JS_ToInt32(ctx, &x, argv[1]) || JS_ToInt32(ctx, &y, argv[2]) || JS_ToInt32(ctx, &width, argv[3]) ||
      JS_ToInt32(ctx, &height, argv[4])) {
    return JS_EXCEPTION;
  }
  appInstance().renderer->drawImage((const uint8_t*)bitmapData, x, y, width, height);
  return JS_UNDEFINED;
}

static int fontIdFromString(const char* fontIdStr) {
  if (strcmp(fontIdStr, "UI10") == 0) {
    return UI_10_FONT_ID;
  } else if (strcmp(fontIdStr, "UI12") == 0) {
    return UI_12_FONT_ID;
  } else if (strcmp(fontIdStr, "SM") == 0) {
    return SMALL_FONT_ID;
  }
  return UI_10_FONT_ID;  // default
}

static EpdFontFamily::Style textStyleFromString(const char* styleStr) {
  EpdFontFamily::Style style = EpdFontFamily::REGULAR;
  if (strcmp(styleStr, "B") == 0) {
    style = EpdFontFamily::BOLD;
  } else if (strcmp(styleStr, "I") == 0) {
    style = EpdFontFamily::ITALIC;
  } else if (strcmp(styleStr, "J") == 0) {
    style = EpdFontFamily::BOLD_ITALIC;
  }
  return style;
}

static JSValue js_getTextWidth(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(3);
  GET_STRING_ARG(0, fontIdStr);
  GET_STRING_ARG(1, text);
  GET_STRING_ARG(2, styleStr);
  if (!fontIdStr || !text || !styleStr) {
    return JS_EXCEPTION;
  }
  int fontId = fontIdFromString(fontIdStr);
  EpdFontFamily::Style style = textStyleFromString(styleStr);
  int width = appInstance().renderer->getTextWidth(fontId, text, style);
  return JS_NewInt32(ctx, width);
}

static JSValue js_drawCenteredText(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(5);
  int y, black;
  GET_STRING_ARG(0, fontIdStr);
  GET_STRING_ARG(2, text);
  GET_STRING_ARG(4, styleStr);
  if (!fontIdStr || JS_ToInt32(ctx, &y, argv[1]) || !text || JS_ToInt32(ctx, &black, argv[3]) || !styleStr) {
    return JS_EXCEPTION;
  }
  int fontId = fontIdFromString(fontIdStr);
  EpdFontFamily::Style style = textStyleFromString(styleStr);
  appInstance().renderer->drawCenteredText(fontId, y, text, black != 0, style);
  return JS_UNDEFINED;
}

static JSValue js_drawText(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(6);
  int x, y, black;
  GET_STRING_ARG(0, fontIdStr);
  GET_STRING_ARG(3, text);
  GET_STRING_ARG(5, styleStr);
  if (!fontIdStr || JS_ToInt32(ctx, &y, argv[2]) || !text || JS_ToInt32(ctx, &black, argv[4]) || !styleStr) {
    return JS_EXCEPTION;
  }
  int fontId = fontIdFromString(fontIdStr);
  EpdFontFamily::Style style = textStyleFromString(styleStr);
  appInstance().renderer->drawText(fontId, x, y, text, black != 0, style);
  return JS_UNDEFINED;
}

static JSValue js_drawButtonHints(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(5);
  GET_STRING_ARG(0, fontIdStr);
  GET_STRING_ARG(1, btn1Text);
  GET_STRING_ARG(2, btn2Text);
  GET_STRING_ARG(3, btn3Text);
  GET_STRING_ARG(4, btn4Text);
  if (!fontIdStr || !btn1Text || !btn2Text || !btn3Text || !btn4Text) {
    return JS_EXCEPTION;
  }
  int fontId = fontIdFromString(fontIdStr);
  appInstance().renderer->drawButtonHints(fontId, btn1Text, btn2Text, btn3Text, btn4Text);
  return JS_UNDEFINED;
}

static JSValue js_drawSideButtonHints(JSContext* ctx, JSValue* this_val, int argc, JSValue* argv) {
  CHECK_ARGC(3);
  GET_STRING_ARG(0, fontIdStr);
  GET_STRING_ARG(1, topBtnText);
  GET_STRING_ARG(2, bottomBtnText);
  if (!fontIdStr || !topBtnText || !bottomBtnText) {
    return JS_EXCEPTION;
  }
  int fontId = fontIdFromString(fontIdStr);
  appInstance().renderer->drawSideButtonHints(fontId, topBtnText, bottomBtnText);
  return JS_UNDEFINED;
}

#include <crosspoint_stdlib.h>
}

AppRunner AppRunner::instance;

static void dump_error(JSContext* jsCtx) {
  JSValue obj = JS_GetException(jsCtx);
  JS_PrintValueF(jsCtx, obj, JS_DUMP_LONG);
}

static void serial_log_write_func(void* opaque, const void* buf, size_t buf_len) {
  Serial.printf("[%lu] [MJS] %.*s", millis(), (int)buf_len, (const char*)buf);
  Serial.write((const uint8_t*)buf, buf_len);
  Serial.println();
}

void AppRunner::run(GfxRenderer* renderer, MappedInputManager* mappedInput) {
  this->renderer = renderer;
  this->mappedInput = mappedInput;

  this->mem.resize(MAX_MEM_SIZE);
  this->jsCtx = JS_NewContext(mem.data(), mem.size(), &js_stdlib);
  JS_SetLogFunc(jsCtx, serial_log_write_func);

  JSValue val;

  if (JS_IsBytecode((const uint8_t*)prog.data(), prog.size())) {
    Serial.printf("[%lu] [APP] Loading bytecode...\n", millis());
    if (JS_RelocateBytecode(jsCtx, (uint8_t*)prog.data(), prog.size())) {
      Serial.printf("[%lu] [APP] Failed to relocate bytecode\n", millis());
    }
    val = JS_LoadBytecode(jsCtx, (const uint8_t*)prog.data());
  } else {
    Serial.printf("[%lu] [APP] Parsing program from source...\n", millis());
    int parse_flags = 0;
    val = JS_Parse(jsCtx, prog.data(), prog.size(), "app", parse_flags);
  }

  if (JS_IsException(val)) {
    dump_error(jsCtx);
    Serial.printf("[%lu] [APP] Got exception on parsing program\n", millis());
    return;
  }

  val = JS_Run(jsCtx, val);

  if (JS_IsException(val)) {
    dump_error(jsCtx);
    Serial.printf("[%lu] [APP] Program exited with exception\n", millis());
    return;
  }

  // normal exit
}
