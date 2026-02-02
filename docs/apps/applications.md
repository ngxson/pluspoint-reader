# User apps

PlusPoint is shipped with an experimental implementation of user-defined JS apps, powered by [mquickjs](https://github.com/bellard/mquickjs).

Please note that only a subset of JS syntax is supported. Please refer to the original repo for more details.

For more technical details, please refer to [this PR](https://github.com/ngxson/pluspoint-reader/pull/2)

## Requirements

Apps must be written in javascript, must has `.js` extension for normal script, or `.app` extension for compiled apps (byte code format)

Apps must be placed inside `SDCARD/apps` directory.

The largest script size (either raw script or compiled) is 32KB.

Upon running, the maximum working memory allowed is 64KB. If you're running into out-of-memory issues, please follow there tips:
- Save large assets (text, bitmap) into a dedicated file, and use FS functions to read them dynamically when needed
- Avoid reading the whole file, read chunk-by-chunk insteead. The recommended chunk size if 4096 bytes, or 4KB
- Regularly call `gc()` to trigger garbage collection

## Compile to byte code

TODO: write this part

## API reference

Please refer to [crosspoint_stdlib.c](../../lib/mquickjs/scripts/crosspoint_stdlib.c) for a complete list of API calls.

## Examples

Examples of apps can be found under `docs/apps` directory. Each app must contain a `while (true)` loop:

```js
var needsRedraw = true;

var pageWidth = CP.getScreenWidth();
var pageHeight = CP.getScreenHeight();

while (true) {
  if (needsRedraw) {
    needsRedraw = false;
    CP.clearScreen(255); // white background
    CP.drawCenteredText(CP.FONT_UI_12, pageHeight / 2, "Hello World!", true, CP.TEXT_BOLD);
    CP.displayBuffer(CP.FAST_REFRESH);
  }

  if (CP.btnIsPressed(CP.BTN_BACK)) {
    break; // exit the program
  }
}
```

The code above displays `Hello World!` text at the center of the screen. To exit the application, press Back button.
