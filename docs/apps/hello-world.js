var count = 0;
var last_ms = 0;

var pageWidth = CP.getScreenWidth();
var pageHeight = CP.getScreenHeight();

while (true) {
  var now_ms = CP.millis();
  if (now_ms - last_ms >= 1000) {
    last_ms = now_ms;
    count += 1;
    CP.clearScreen(255); // white background
    CP.drawCenteredText(CP.FONT_UI_12, pageHeight / 2, "Hello World!", true, CP.TEXT_BOLD);
    CP.drawCenteredText(CP.FONT_UI_12, pageHeight / 2 + 20, "count = " + count, true, CP.TEXT_REGULAR);
    CP.displayBuffer(CP.FAST_REFRESH);
  }

  if (CP.btnIsPressed(CP.BTN_BACK)) {
    break; // exit the program
  }
}
