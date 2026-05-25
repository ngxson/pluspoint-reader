# pluspoint-reader OS

We are building a proper OS for the Xteink X4 e-reader (ESP32-C3, 800×480 e-ink, portrait-mounted).

**Status: early proof-of-concept.** Nothing here is production-ready. Expect the architecture to shift.

---

## Directory structure

- `src/os/` - core OS abstractions (Display, Graphic, Activity system)
- `src/drivers/` - platform drivers (e.g. `sim/` for the native SDL2 simulator)
- `src/apps/` - applications including the launcher/home screen (not yet created)
- `lib/` - mostly upstream lib, do not modify
  - `lib/hal/` - upstream HAL layer for real hardware (ESP32/Arduino-coupled, do not modify)
  - `lib/sim/` - Arduino API stubs enabling native builds without touching upstream libs (CAN be modified)

---

## Key design principles

**Simulator-first.** The OS layer (`src/os/`) must compile and run as a native binary (macOS/Linux) via `pio run -e native`. All new abstractions must work in the simulator before touching real hardware.

**Abstract/HW separation.** `src/os/` contains only pure C++20 with no Arduino, FreeRTOS, or ESP32 dependencies. Hardware coupling lives in `src/drivers/` (sim) and `lib/hal/` (real HW).

**`#ifdef SIMULATOR`** is the boundary macro. Never use `#ifdef NATIVE` or similar - always `SIMULATOR`.

---

## Build

```bash
./scripts/sim.sh      # compile + run the simulator (native SDL2 window)
./scripts/run.sh      # compile + flash + monitor on real ESP32 hardware
```

---

## Singleton boot sequence

```
Os::boot()                        # defined in src/os/os.cpp
  └─ creates platform Display     # SimDisplay (SIMULATOR) or real HW display
  └─ Display::setInstance()       # sets the Display singleton
  └─ display.begin()
  └─ Graphic::getInstance()       # binds Graphic to Display, warms up singleton
```

Access pattern inside an activity:
```cpp
Graphic& g = ActivityManager::getGraphic();  // == Graphic::getInstance()
```

---

## Display / framebuffer model

- Physical panel: 800×480 landscape (1bpp, MSB-first)
- Default logical orientation: `Graphic::Portrait` (480×800)
- `Graphic` applies the same coordinate rotation as `GfxRenderer` in the upstream crosspoint-reader
- `Display` interface: `begin()`, `getFrameBuffer()`, `displayBuffer()`, `refreshDisplay()`
- Sim renders the framebuffer into a portrait SDL2 window (480×800) using the inverse Portrait transform

---

## What is NOT done yet (good starting points)

- [ ] `ActivityManager` simulation path - currently FreeRTOS-gated; needs `#ifdef SIMULATOR` threading or a single-threaded loop replacement
- [ ] A real `HelloWorldActivity` using the Activity lifecycle instead of bare `main.cpp` drawing
- [ ] Input abstraction (`IInput` / sim keyboard mapping)
- [ ] `lib/hal/` wiring - `HalDisplay` needs to implement `Display` so the real hardware path compiles
- [ ] App launcher / home screen (`src/apps/`)
- [ ] Any persistence, settings, or storage abstraction
