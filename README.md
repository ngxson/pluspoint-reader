# PlusPoint

This is a fork of [crosspoint-reader](https://github.com/crosspoint-reader/crosspoint-reader) that I created out of curiousity. The main motivation is to add some coplex features that will have slim chance of being upstreamed.

For the name of this project, "Plus" has a double meaning: It means both "being more" and "my love for C++"

Philosophical design behind this fork:
- We only target **low-level** changes, things that does not directly alter the UI/UX
- We try to introduce these feature as a wrapper, avoid too many modifications on the original code
- For any changes fall outside of these category, please send your PR to upstream project

Things that this fork is set out to support:
- [x] Emulation (for a better DX)
- [x] Allow user-defined app, packaged as Javascript file under `/apps` directory
    - [ ] Cover more APIs
    - [ ] Allow app to be compressed into a more space-efficient format (inspired by BSON <--> JSON)
- ~~Allow custom fonts to be installed to SPIFFS partition~~ Abandonned, potentially duplicated efforts with other contributors
- [ ] Real-time clock (with calibration to minimize drifting)

## Emulated

This fork provides a semi-emulated solution that allows running CrossPoint / PlusPoint on a cheap barebone ESP32C3. This allow faster development without the risk of (soft) bricking the real device.

Why don't we use QEMU? Please refer to [this discussion](https://github.com/crosspoint-reader/crosspoint-reader/pull/500)

<img width="1544" height="931" alt="Image" src="https://github.com/user-attachments/assets/9e9deee2-2b27-4490-8c6e-43bca2e13bb4" />

The SD card is replaced by a "proxy FS", which maps the SD card to `{project_directory}/.sdcard`. It make things much, much easier to debug.

Because we are using the same chip as the real device, most hardware functionalities should stays the same, including wifi and bluetooth.

Some small caveats:
- `OMIT_FONTS` must be enabled, in order to fit the program inside 4MB flash
- You have to press RST to wake up from sleep, this is not exactly the same as power button, but works for now

**How to use it:**
1. Make sure you're having a ESP32C3 with 4MB of memory. Just a standalone board is enough, no need to connect any GPIO pins
2. Connect ESP32C3 to computer via USB
3. Compile this project with the `env:emulated` config and flash it to the board
4. Run `python ./ngxson/server.py`
5. Open http://localhost:8090/ on your browser
6. Enjoy

Note: You don't need to stop the program each time you upload a new firmware, the script automatically disconnects from the console to avoid interrupting the uploading process. After it's finished, you can simply press the RST button on the board and wait ~5 seconds.

Known issue(s):
- For now, the `FATAL: Timeout waiting for response` can sometimes pop up. If it stucks in bootloop, exit the python script, press RST on the board, then re-connect it.
- Sometimes, too frequent FS_READ and FS_WRITE can also trigger the issue above. If it when into bootloop, try deleting content inside `{project_directory}/.sdcard/.crosspoint`

## Custom JS apps

We have an experimental version of user-defined JS apps (built upon [cesanta/elk](https://github.com/cesanta/elk)). Only a subset of JS syntax is supported.

Applications can be loaded onto SD card `/app` folder, requires to name the file with `.js` extension.

TODO: more details on this

Example application:

```js
let count = 0;
let last_ms = 0;

while (true) {
  let now_ms = millis();
  if (now_ms - last_ms >= 1000) {
    last_ms = now_ms;
    count += 1;
    clearScreen(false); // true = black, false = white
    drawCenteredText("UI_12", pageHeight / 2, "Hello World!", true, "BOLD");
    drawCenteredText("UI_12", pageHeight / 2 + 30, "count = " + toString(count), true, "REGULAR");
    displayBuffer();
  }

  if (isButtonPressed(BTN_BACK)) {
    break; // exit the program
  }
}
```

Output: The counter is updated every 1 second

<img width="497" height="219" alt="Image" src="https://github.com/user-attachments/assets/42d51278-ba14-4f48-9e9e-cc31f76fba4c" />
