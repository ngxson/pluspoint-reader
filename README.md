# PlusPoint

This is a fork of [crosspoint-reader](https://github.com/crosspoint-reader/crosspoint-reader) that I created out of curiousity. The main motivation is to add some coplex features that will have slim chance of being upstreamed.

For the name of this project, "Plus" has a double meaning: It means both "being more" and "my love for C++"

Philosophical design behind this fork:
- We only target **low-level** changes, things that does not directly alter the UI/UX
- We try to introduce these feature as a wrapper, avoid too many modifications on the original code
- For any changes fall outside of these category, please send your PR to upstream project

Things that this fork is set out to support:
- [x] Emulation (for a better DX)
- [ ] Allow runtime app, similar to a Flipper Zero (TBD how to do it)

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
