var needsRedraw = true;

var pageWidth = CP.getScreenWidth();
var pageHeight = CP.getScreenHeight();

var boxSize = 68;
var boxMargin = 2;

// Hard-coded current date: February 2, 2026
var currentYear = 2026;
var currentMonth = 2; // February

var currentDay = 1; // currently unused

var monthNames = ["", "January", "February", "March", "April", "May", "June",
                  "July", "August", "September", "October", "November", "December"];
var dayHeaders = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"];

// Returns the day of week (0=Monday, 6=Sunday) for the first day of the given month/year
function getFirstDayOfMonth(year, month) {
  // Using Zeller's congruence algorithm
  var m = month;
  var y = year;
  if (m < 3) {
    m += 12;
    y -= 1;
  }
  var q = 1; // first day of month
  var k = y % 100;
  var j = (y / 100) | 0;
  var h = (q + (((m + 1) * 26) / 10 | 0) + k + (k / 4 | 0) + (j / 4 | 0) - 2 * j) % 7;
  // Convert to Monday=0 format
  var day = (h + 5) % 7;
  return day;
}

// Returns number of days in the given month/year
function getDaysInMonth(year, month) {
  if (month === 2) {
    // Check for leap year
    if ((year % 4 === 0 && year % 100 !== 0) || year % 400 === 0) {
      return 29;
    }
    return 28;
  }
  if (month === 4 || month === 6 || month === 9 || month === 11) {
    return 30;
  }
  return 31;
}

// bg = 0: normal ; 1: gray ; 2: today (inverted)
function drawBoxedText(text, x, y, w, h, bg) {
  x += 2; // adjust offset according to real device

  CP.drawRect(x, y, w, h, false); // black border

  // TODO: support inverted colors for "today"

  // Normal or gray background
  CP.drawRect(x + boxMargin,
              y + boxMargin,
              w - 2 * boxMargin,
              h - 2 * boxMargin, true);
  
  if (bg === 1) {
    // Gray background - draw light pattern
    for (var py = y + boxMargin; py < y + h - boxMargin; py += 2) {
      for (var px = x + boxMargin; px < x + w - boxMargin; px += 2) {
        CP.drawLine(px, py, px, py, true);
      }
    }
  }
  
  var textWidth = CP.getTextWidth(CP.FONT_UI_12, text, CP.TEXT_REGULAR);
  CP.drawText(CP.FONT_UI_12,
              x + (w - textWidth) / 2,
              y + 18,
              text,
              true,
              CP.TEXT_REGULAR);
}

while (true) {
  if (needsRedraw) {
    needsRedraw = false;
    CP.clearScreen(255);
    
    // Draw title with month and year
    var title = monthNames[currentMonth] + " " + currentYear;
    CP.drawCenteredText(CP.FONT_UI_12, 20, title, true, CP.TEXT_BOLD);
    
    // Draw day headers
    var headerY = 120;
    for (var i = 0; i < 7; i++) {
      var headerWidth = CP.getTextWidth(CP.FONT_UI_12, dayHeaders[i], CP.TEXT_BOLD);
      CP.drawText(CP.FONT_UI_12,
                  boxSize * i + (boxSize - headerWidth) / 2,
                  headerY,
                  dayHeaders[i],
                  true,
                  CP.TEXT_BOLD);
    }
    
    // Calculate calendar layout
    var firstDay = getFirstDayOfMonth(currentYear, currentMonth);
    var daysInMonth = getDaysInMonth(currentYear, currentMonth);
    var prevMonthDays = getDaysInMonth(currentYear, currentMonth === 1 ? 12 : currentMonth - 1);
    
    var dayNum = 1;
    var nextMonthDay = 1;
    var startY = 160;
    
    // Draw calendar grid (6 rows max)
    for (var row = 0; row < 6; row++) {
      for (var col = 0; col < 7; col++) {
        var cellIndex = row * 7 + col;
        var x = boxSize * col;
        var y = startY + boxSize * row;
        
        if (cellIndex < firstDay) {
          // Previous month days
          var prevDay = prevMonthDays - firstDay + cellIndex + 1;
          drawBoxedText("" + prevDay, x, y, boxSize, boxSize, 1);
        } else if (dayNum <= daysInMonth) {
          // Current month days
          var bgStyle = (dayNum === currentDay) ? 2 : 0;
          drawBoxedText("" + dayNum, x, y, boxSize, boxSize, bgStyle);
          dayNum++;
        } else {
          // Next month days
          drawBoxedText("" + nextMonthDay, x, y, boxSize, boxSize, 1);
          nextMonthDay++;
        }
      }
      
      if (dayNum > daysInMonth && row >= 4) {
        break; // Stop if we've drawn all days and past row 4
      }
    }
    
    CP.displayBuffer(CP.FULL_REFRESH);
    CP.delay(200); // debounce
  }

  if (CP.btnIsPressed(CP.BTN_UP) || CP.btnIsPressed(CP.BTN_LEFT)) {
    // Previous month
    currentMonth--;
    if (currentMonth < 1) {
      currentMonth = 12;
      currentYear--;
    }
    needsRedraw = true;

  } else if (CP.btnIsPressed(CP.BTN_DOWN) || CP.btnIsPressed(CP.BTN_RIGHT)) {
    // Next month
    currentMonth++;
    if (currentMonth > 12) {
      currentMonth = 1;
      currentYear++;
    }
    needsRedraw = true;

  } else if (CP.btnIsPressed(CP.BTN_BACK)) {
    break; // exit the program
  }
}
