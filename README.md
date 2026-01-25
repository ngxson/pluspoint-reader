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

<img width="1252" height="839" alt="Image" src="https://github.com/user-attachments/assets/e305463d-6cc0-472c-bed3-2544890a59b2" />

The SD card is replaced by a "proxy FS", which maps the SD card to `{project_directory}/.sdcard`. It make things much, much easier to debug.

Because we are using the same chip as the real device, most hardware functionalities should stays the same, including wifi and bluetooth.

Some small caveats:
- `OMIT_FONTS` must be enabled, in order to fit the program inside 4MB flash
- You have to press RST to wake up from sleep, this is not exactly the same as power button, but works for now
