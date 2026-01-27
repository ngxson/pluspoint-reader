#include <HalGPIO.h>
#include <SPI.h>
#include <esp_sleep.h>
#include <EmulationUtils.h>

#ifdef EMULATED
static uint8_t currentState;
static uint8_t lastState;
static uint8_t pressedEvents;
static uint8_t releasedEvents;
static unsigned long lastDebounceTime;
static unsigned long buttonPressStart;
static unsigned long buttonPressFinish;
#endif

void HalGPIO::begin() {
#ifndef EMULATED
  inputMgr.begin();
  SPI.begin(EPD_SCLK, SPI_MISO, EPD_MOSI, EPD_CS);
  pinMode(BAT_GPIO0, INPUT);
  pinMode(UART0_RXD, INPUT);
#else
  currentState = 0;
  lastState = 0;
  pressedEvents = 0;
  releasedEvents = 0;
  lastDebounceTime = 0;
  buttonPressStart = 0;
  buttonPressFinish = 0;
#endif
}

void HalGPIO::update() {
#ifndef EMULATED
  inputMgr.update();
#else
  const unsigned long currentTime = millis();
  uint8_t state = 0;

  EmulationUtils::Lock lock;
  EmulationUtils::sendCmd(EmulationUtils::CMD_BUTTON, "read");
  auto res = EmulationUtils::recvRespInt64();
  if (res < 0) {
    state = 0; // error reading state
  } else {
    state = static_cast<uint8_t>(res);
  }

  // Always clear events first
  pressedEvents = 0;
  releasedEvents = 0;

  // Debounce
  if (state != lastState) {
    lastDebounceTime = currentTime;
    lastState = state;
  }

  static constexpr unsigned long DEBOUNCE_DELAY = 5;
  if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (state != currentState) {
      // Calculate pressed and released events
      pressedEvents = state & ~currentState;
      releasedEvents = currentState & ~state;

      // If pressing buttons and wasn't before, start recording time
      if (pressedEvents > 0 && currentState == 0) {
        buttonPressStart = currentTime;
      }

      // If releasing a button and no other buttons being pressed, record finish time
      if (releasedEvents > 0 && state == 0) {
        buttonPressFinish = currentTime;
      }

      currentState = state;
    }
  }
#endif
}


bool HalGPIO::isPressed(const uint8_t buttonIndex) const {
#ifndef EMULATED
  return inputMgr.isPressed(buttonIndex);
#else
  return currentState & (1 << buttonIndex);
#endif
}

bool HalGPIO::wasPressed(const uint8_t buttonIndex) const {
#ifndef EMULATED
  return inputMgr.wasPressed(buttonIndex);
#else
  return pressedEvents & (1 << buttonIndex);
#endif
}

bool HalGPIO::wasAnyPressed() const {
#ifndef EMULATED
  return inputMgr.wasAnyPressed();
#else
  return pressedEvents > 0;
#endif
}

bool HalGPIO::wasReleased(const uint8_t buttonIndex) const {
#ifndef EMULATED
  return inputMgr.wasReleased(buttonIndex);
#else
  return releasedEvents & (1 << buttonIndex);
#endif
}

bool HalGPIO::wasAnyReleased() const {
#ifndef EMULATED
  return inputMgr.wasAnyReleased();
#else
  return releasedEvents > 0;
#endif
}

unsigned long HalGPIO::getHeldTime() const {
#ifndef EMULATED
  return inputMgr.getHeldTime();
#else
  // Still hold a button
  if (currentState > 0) {
    return millis() - buttonPressStart;
  }

  return buttonPressFinish - buttonPressStart;
#endif
}

void HalGPIO::startDeepSleep() {
#ifndef EMULATED
  esp_deep_sleep_enable_gpio_wakeup(1ULL << InputManager::POWER_BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
  // Ensure that the power button has been released to avoid immediately turning back on if you're holding it
  while (inputMgr.isPressed(BTN_POWER)) {
    delay(50);
    inputMgr.update();
  }
  // Enter Deep Sleep
  esp_deep_sleep_start();
#else
  Serial.printf("[%lu] [   ] Emulated start deep sleep\n", millis());
  while (true) {
    delay(1000); // no-op
  }
#endif
}

int HalGPIO::getBatteryPercentage() const {
#ifndef EMULATED
  static const BatteryMonitor battery = BatteryMonitor(BAT_GPIO0);
  return battery.readPercentage();
#else
  return 100; // Always return full battery in emulation
#endif
}

bool HalGPIO::isUsbConnected() const {
#ifndef EMULATED
  // U0RXD/GPIO20 reads HIGH when USB is connected
  return digitalRead(UART0_RXD) == HIGH;
#else
  return true;
#endif
}

bool HalGPIO::isWakeupByPowerButton() const {
#ifndef EMULATED
  const auto wakeupCause = esp_sleep_get_wakeup_cause();
  const auto resetReason = esp_reset_reason();
  if (isUsbConnected()) {
    return wakeupCause == ESP_SLEEP_WAKEUP_GPIO;
  } else {
    return (wakeupCause == ESP_SLEEP_WAKEUP_UNDEFINED) && (resetReason == ESP_RST_POWERON);
  }
#else
  return false;
#endif
}
