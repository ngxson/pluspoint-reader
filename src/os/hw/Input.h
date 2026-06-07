#pragma once

#include <cassert>

class Input {
  inline static Input* s_instance = nullptr;

 public:
  static Input& getInstance() {
    assert(s_instance != nullptr && "Os::boot() must be called before using Input");
    return *s_instance;
  }
  static void setInstance(Input* i) { s_instance = i; }

  enum class Button { Back, Confirm, Left, Right, Up, Down, Power, PageBack, PageForward };

  virtual ~Input() = default;

  virtual void update() = 0;
  virtual bool wasPressed(Button button) const = 0;
  virtual bool wasReleased(Button button) const = 0;
  virtual bool isPressed(Button button) const = 0;
  virtual bool wasAnyPressed() const = 0;
  virtual bool wasAnyReleased() const = 0;
};
