#include "GaugeUI.h"
#include "TouchManager.h"

GaugeUI::GaugeUI()
    : tft(TFT_eSPI()), spr(&tft), useSprite(true), primaryColor(UI_COLOR_CYAN),
      darkAccentColor(UI_COLOR_DARK_GRAY), backgroundColor(UI_COLOR_BLACK),
      prevSpeed(-1), prevTemp(-999), prevRpm(-1), prevTheme(255),
      prevLayout(255), prevPrimaryMetric(255), prevMetricsBitmask(0),
      prevMaxSpeed(0), prevMaxRpm(0), prevGaugeStartAngle(0),
      prevGaugeEndAngle(0), prevConnected(false), prevDemo(false),
      alertState(false), lastAlertFlash(0),
      numpadTarget(NUMPAD_TARGET_MAX_SPEED), numpadLen(0) {
  numpadBuffer[0] = '\0';
}

void GaugeUI::begin() {
  tft.init();
  tft.setRotation(1); // Landscape 320x240
  tft.fillScreen(UI_COLOR_BLACK);

  // Set default TFT_eSPI touch calibration data for 320x240 landscape
  uint16_t calData[5] = {275, 3600, 305, 3700, 1};
  tft.setTouch(calData);

  spr.setColorDepth(8);
  if (spr.createSprite(320, 240) == nullptr) {
    useSprite = false;
    Serial.println("[UI] Sprite creation failed, using direct TFT rendering.");
  } else {
    useSprite = true;
    Serial.println(
        "[UI] Double-buffered TFT Sprite Engine initialized (320x240).");
  }
}

void GaugeUI::updateThemeColors(uint8_t theme) {
  switch (theme) {
  case THEME_RED:
    primaryColor = UI_COLOR_BRIGHT_RED;
    darkAccentColor = 0x4000;
    break;
  case THEME_AMBER:
    primaryColor = UI_COLOR_ORANGE;
    darkAccentColor = 0x3A00;
    break;
  case THEME_GREEN:
    primaryColor = UI_COLOR_GREEN;
    darkAccentColor = 0x01E0;
    break;
  case THEME_CYAN:
  default:
    primaryColor = UI_COLOR_CYAN;
    darkAccentColor = 0x0210;
    break;
  }
}

void GaugeUI::drawSettingsButton() {
  // Compact Square Settings Gear Icon in Top Right (X: 292, Y: 2, W: 24, H: 24)
  if (useSprite) {
    spr.fillRoundRect(292, 2, 24, 24, 4, darkAccentColor);
    spr.drawRoundRect(292, 2, 24, 24, 4, primaryColor);

    spr.fillCircle(304, 14, 5, primaryColor);
    spr.fillCircle(304, 14, 2, darkAccentColor);
    spr.drawFastHLine(297, 14, 14, primaryColor);
    spr.drawFastVLine(304, 7, 14, primaryColor);
  } else {
    tft.fillRoundRect(292, 2, 24, 24, 4, darkAccentColor);
    tft.drawRoundRect(292, 2, 24, 24, 4, primaryColor);

    tft.fillCircle(304, 14, 5, primaryColor);
    tft.fillCircle(304, 14, 2, darkAccentColor);
    tft.drawFastHLine(297, 14, 14, primaryColor);
    tft.drawFastVLine(304, 7, 14, primaryColor);
  }
}

void GaugeUI::drawHeader(const VehicleData &data, const SystemConfig &config) {
  // Compact Square Mode Status Icon in Top Left (X: 4, Y: 2, W: 24, H: 24)
  uint16_t statusBg = config.demoMode
                          ? UI_COLOR_ORANGE
                          : (data.isConnected ? UI_COLOR_GREEN : UI_COLOR_RED);
  const char *statusLetter =
      config.demoMode ? "D" : (data.isConnected ? "O" : "?");

  if (useSprite) {
    spr.fillRoundRect(4, 2, 24, 24, 4, statusBg);
    spr.setTextColor(UI_COLOR_BLACK, statusBg);
    spr.setTextDatum(MC_DATUM);
    spr.drawString(statusLetter, 16, 14, 2);

    // Header Mode Navigation Controls (< MODE >)
    spr.fillRoundRect(84, 2, 24, 24, 4, darkAccentColor);
    spr.drawRoundRect(84, 2, 24, 24, 4, primaryColor);
    spr.fillTriangle(98, 8, 90, 14, 98, 20, primaryColor);

    spr.fillRoundRect(112, 2, 96, 24, 4, darkAccentColor);
    spr.drawRoundRect(112, 2, 96, 24, 4, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(MC_DATUM);
    const char *shortModeNames[] = {"ALL ANALOG", "ALL DIGITAL",  "NUMBERS",
                                    "FOCUSED",    "SINGLE GAUGE", "HYBRID"};
    spr.drawString(shortModeNames[config.gaugeLayout % 6], 160, 14, 2);

    spr.fillRoundRect(212, 2, 24, 24, 4, darkAccentColor);
    spr.drawRoundRect(212, 2, 24, 24, 4, primaryColor);
    spr.fillTriangle(218, 8, 226, 14, 218, 20, primaryColor);
  } else {
    tft.fillRoundRect(4, 2, 24, 24, 4, statusBg);
    tft.setTextColor(UI_COLOR_BLACK, statusBg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(statusLetter, 16, 14, 2);

    // left gauge mode arrow
    tft.fillRoundRect(84, 2, 24, 24, 4, darkAccentColor);
    tft.drawRoundRect(84, 2, 24, 24, 4, primaryColor);
    tft.fillTriangle(98, 8, 90, 14, 98, 20, primaryColor);

    // gauge mode display
    tft.fillRoundRect(112, 2, 96, 24, 4, darkAccentColor);
    tft.drawRoundRect(112, 2, 96, 24, 4, primaryColor);
    tft.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    tft.setTextDatum(MC_DATUM);
    const char *shortModeNames[] = {"ALL ANALOG", "ALL DIGITAL",  "NUMBERS",
                                    "FOCUSED",    "SINGLE GAUGE", "HYBRID"};
    tft.drawString(shortModeNames[config.gaugeLayout % 6], 160, 14, 2);

    // right gauge mode arrow
    tft.fillRoundRect(212, 2, 24, 24, 4, darkAccentColor);
    tft.drawRoundRect(212, 2, 24, 24, 4, primaryColor);
    tft.fillTriangle(218, 8, 226, 14, 218, 20, primaryColor);
  }

  drawSettingsButton();
}

// Adaptive Analog Dial Helper with Isolated Dynamic Readout & Static Unit Label
void GaugeUI::drawAnalogDialInRect(int x, int y, int w, int h, float val,
                                   float minV, float maxV, const char *label,
                                   const char *unitStr, uint16_t accentColor,
                                   bool showRedline, float redlineVal,
                                   bool isCoolantGauge, float coldVal,
                                   float hotVal, float startAngleDeg,
                                   float endAngleDeg) {
  int cx = x + (w / 2);
  int cy = y + (h / 2);
  int r = (min(w, h) / 2) - 3;
  float normVal = constrain(val, minV, maxV);
  float sweep = endAngleDeg - startAngleDeg;
  if (sweep == 0)
    sweep = 270.0f;

  if (useSprite) {
    spr.fillCircle(cx, cy, r, darkAccentColor);
    spr.drawCircle(cx, cy, r, accentColor);
    spr.drawCircle(cx, cy, r - 1, accentColor);
    spr.fillCircle(cx, cy, r - 3, UI_COLOR_BLACK);

    // Major Step Based on Range Scale
    float majorStep = 20.0f;
    if (maxV >= 5000.0f) {
      majorStep = 1000.0f;
    } else if (maxV - minV >= 180.0f) {
      majorStep = 30.0f;
    } else if (maxV - minV >= 80.0f) {
      majorStep = 20.0f;
    } else if (maxV - minV <= 16.0f) {
      majorStep = 2.0f;
    }

    float minorStep = majorStep / 2.0f;

    // Render Major & Minor Ticks Across Scale
    for (float v = minV; v <= maxV + 0.1f; v += minorStep) {
      float normFrac = (v - minV) / (maxV - minV);
      float angleDeg = startAngleDeg + (normFrac * sweep);
      float rad = (angleDeg - 90.0f) * DEG_TO_RAD;

      uint16_t tickColor = UI_COLOR_WHITE;
      if (isCoolantGauge) {
        if (v <= coldVal)
          tickColor = UI_COLOR_BLUE;
        else if (v >= hotVal)
          tickColor = UI_COLOR_RED;
        else
          tickColor = primaryColor;
      } else if (showRedline && v >= redlineVal) {
        tickColor = UI_COLOR_RED;
      }

      float rem = fmod(v - minV, majorStep);
      bool isMajor = (rem < 0.1f || (majorStep - rem) < 0.1f);

      int tickLen = isMajor ? 8 : 4;
      int x1 = cx + cos(rad) * (r - 3);
      int y1 = cy + sin(rad) * (r - 3);
      int x2 = cx + cos(rad) * (r - 3 - tickLen);
      int y2 = cy + sin(rad) * (r - 3 - tickLen);
      spr.drawLine(x1, y1, x2, y2, tickColor);

      // Smooth Proportional Font 2 for Subdued Integer Tick Labels
      if (isMajor && r >= 45) {
        int numX = cx + cos(rad) * (r - 18);
        int numY = cy + sin(rad) * (r - 18);

        spr.setTextColor(tickColor, UI_COLOR_BLACK);
        spr.setTextDatum(MC_DATUM);

        if (maxV >= 1000.0f) {
          int kVal = (int)round(v / 1000.0f);
          spr.drawNumber(kVal, numX, numY, 2);
        } else {
          spr.drawNumber((int)round(v), numX, numY, 2);
        }
      }
    }

    // Draw Arcs for Cold & Hot Regions
    if (isCoolantGauge) {
      float coldFrac = (coldVal - minV) / (maxV - minV);
      int coldEndA = (int)(startAngleDeg + (coldFrac * sweep));
      for (int a = (int)startAngleDeg; a <= coldEndA; a += 4) {
        float rad = (a - 90.0f) * DEG_TO_RAD;
        int rx = cx + cos(rad) * (r - 4);
        int ry = cy + sin(rad) * (r - 4);
        spr.fillCircle(rx, ry, 2, UI_COLOR_BLUE);
      }

      float hotFrac = (hotVal - minV) / (maxV - minV);
      int hotStartA = (int)(startAngleDeg + (hotFrac * sweep));
      for (int a = hotStartA; a <= (int)endAngleDeg; a += 4) {
        float rad = (a - 90.0f) * DEG_TO_RAD;
        int rx = cx + cos(rad) * (r - 4);
        int ry = cy + sin(rad) * (r - 4);
        spr.fillCircle(rx, ry, 2, UI_COLOR_RED);
      }
    } else if (showRedline) {
      float redStartFrac = (redlineVal - minV) / (maxV - minV);
      int startA = (int)(startAngleDeg + (redStartFrac * sweep));
      for (int a = startA; a <= (int)endAngleDeg; a += 4) {
        float rad = (a - 90.0f) * DEG_TO_RAD;
        int rx = cx + cos(rad) * (r - 4);
        int ry = cy + sin(rad) * (r - 4);
        spr.fillCircle(rx, ry, 2, UI_COLOR_RED);
      }
    }

    // Anti-Aliased Tapered Sweeping Needle
    float needleDeg =
        startAngleDeg + ((normVal - minV) / (maxV - minV) * sweep);
    float needleRad = (needleDeg - 90.0f) * DEG_TO_RAD;
    float perpRad = needleRad + 1.5707963f;

    float baseRadius = (r >= 70) ? 3.5f : 2.0f;
    int tipX = cx + cos(needleRad) * (r - 6);
    int tipY = cy + sin(needleRad) * (r - 6);

    int bx1 = cx + cos(perpRad) * baseRadius;
    int by1 = cy + sin(perpRad) * baseRadius;
    int bx2 = cx - cos(perpRad) * baseRadius;
    int by2 = cy - sin(perpRad) * baseRadius;

    uint16_t needleColor =
        (showRedline && normVal >= redlineVal)
            ? UI_COLOR_RED
            : (isCoolantGauge && normVal >= hotVal ? UI_COLOR_RED
                                                   : UI_COLOR_BRIGHT_RED);

    spr.fillTriangle(bx1, by1, bx2, by2, tipX, tipY, needleColor);
    spr.drawLine(bx1, by1, tipX, tipY, needleColor);
    spr.drawLine(bx2, by2, tipX, tipY, needleColor);
    spr.drawLine(cx, cy, tipX, tipY, UI_COLOR_WHITE);

    // Center Cap
    spr.fillCircle(cx, cy, 6, accentColor);
    spr.fillCircle(cx, cy, 3, UI_COLOR_WHITE);

    // Dynamic Number By Itself + Static Parameter Label & Unit String
    spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
    spr.setTextDatum(MC_DATUM);

    if (r >= 120) {
      // Large Semicircle / Full Screen Dial
      spr.setTextColor(accentColor, UI_COLOR_BLACK);
      spr.setTextDatum(MC_DATUM);
      spr.drawString(label, cx, cy - (r * 0.70f), 2);

      spr.setTextColor(UI_COLOR_WHITE, UI_COLOR_BLACK);
      int valInt = (int)val;
      if (valInt >= 1000 || valInt <= -100) {
        spr.drawNumber(valInt, cx, cy - (r * 0.40f), 6);
      } else {
        spr.drawNumber(valInt, cx, cy - (r * 0.40f), 7);
      }

      spr.setTextColor(accentColor, UI_COLOR_BLACK);
      spr.drawString(unitStr, cx, cy - (r * 0.15f), 2);
    } else if (r >= 70) {
      spr.drawNumber((int)val, cx, cy + 10, 6);
      spr.setTextColor(accentColor, UI_COLOR_BLACK);
      spr.drawString(label, cx, cy + (r / 1.25), 2);
      spr.drawString(unitStr, cx, cy + 38, 2);
    } else if (r >= 48) {
      spr.drawNumber((int)val, cx, cy + 6, 4);
      spr.setTextColor(accentColor, UI_COLOR_BLACK);
      spr.drawString(label, cx, cy + (r / 1.25), 2);
      spr.drawString(unitStr, cx, cy + 26, 2);
    } else {
      spr.drawNumber((int)val, cx, cy + 4, 2);
      spr.setTextColor(accentColor, UI_COLOR_BLACK);
      spr.drawString(label, cx, cy + (r / 1.25), 2);
      spr.drawString(unitStr, cx, cy + 18, 2);
    }
  }
}

// Adaptive Digital Number Card Helper (With Auto-Scaling for 4-digit RPM)
void GaugeUI::drawDigitalBoxInRect(int x, int y, int w, int h, float val,
                                   const char *label, const char *unitStr,
                                   uint16_t accentColor) {
  if (useSprite) {
    spr.fillRoundRect(x, y, w, h, 6, darkAccentColor);
    spr.drawRoundRect(x, y, w, h, 6, accentColor);

    // Static Parameter Label Header
    spr.setTextColor(UI_COLOR_LIGHT_GRAY, darkAccentColor);
    spr.setTextDatum(TC_DATUM);
    spr.drawString(label, x + (w / 2), y + 4, 2);

    // Dynamic Numerical Readout (Auto-Scale for 4+ Digit Numbers to Avoid
    // Bounds Overflow)
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(MC_DATUM);

    int valInt = (int)val;
    bool isFourDigits = (valInt >= 1000 || valInt <= -100);

    if (h >= 140) {
      if (isFourDigits && w < 160) {
        spr.drawNumber(valInt, x + (w / 2), y + (h / 2) - 4,
                       4); // Font 4 (26px) for 4 digits in narrow card
      } else {
        spr.drawNumber(valInt, x + (w / 2), y + (h / 2) - 4,
                       6); // Font 6 (48px)
      }
    } else if (h >= 80) {
      if (isFourDigits || w < 130) {
        spr.drawNumber(valInt, x + (w / 2), y + (h / 2) - 2, 4); // Font 4
      } else {
        spr.drawNumber(valInt, x + (w / 2), y + (h / 2) - 2, 6); // Font 6
      }
    } else {
      int fontNum = (w >= 80) ? 4 : 2;
      spr.drawNumber(valInt, x + (w / 2), y + (h / 2) - 2, fontNum);
    }

    // Static Unit Badge
    spr.setTextColor(accentColor, darkAccentColor);
    spr.setTextDatum(BC_DATUM);
    spr.drawString(unitStr, x + (w / 2), y + h - 4, 2);
  }
}

// Adaptive Horizontal Digital Level Bar Helper (With Clean Narrow Side Bar &
// Unit Labels)
void GaugeUI::drawLevelBarInRect(int x, int y, int w, int h, float val,
                                 float minV, float maxV, const char *label,
                                 const char *unitStr, uint16_t accentColor,
                                 bool isCoolant, float coldVal, float hotVal) {
  if (useSprite) {
    spr.fillRoundRect(x, y, w, h, 6, darkAccentColor);
    spr.drawRoundRect(x, y, w, h, 6, accentColor);

    if (w < 60) {
      // Narrow Vertical Side Bar (for Focused layout side slots)
      int barH = h - 42;
      int fillH = map(constrain(val, minV, maxV), minV, maxV, 0, barH);

      uint16_t barColor = accentColor;
      if (isCoolant) {
        if (val <= coldVal)
          barColor = UI_COLOR_BLUE;
        else if (val >= hotVal)
          barColor = UI_COLOR_RED;
      }

      spr.fillRect(x + 3, y + 20 + (barH - fillH), w - 6, fillH, barColor);

      for (int i = 0; i <= 6; i++) {
        int tickY = y + 20 + (barH * i / 6);
        spr.drawFastHLine(x + 1, tickY, 3, UI_COLOR_GRAY);
        spr.drawFastHLine(x + w - 4, tickY, 3, UI_COLOR_GRAY);
      }

      // Shortened Compact Label Header to Fit Narrow Width (e.g. COOL, RPM,
      // INTR, LOAD, BATT)
      char shortLabel[6];
      strncpy(shortLabel, label, 4);
      shortLabel[4] = '\0';

      spr.setTextColor(UI_COLOR_LIGHT_GRAY, darkAccentColor);
      spr.setTextDatum(TC_DATUM);
      spr.drawString(shortLabel, x + (w / 2), y + 2, 2);

      // Dynamic Number BY ITSELF (Bottom-center)
      spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
      spr.setTextDatum(BC_DATUM);
      spr.drawNumber((int)val, x + (w / 2), y + h - 14, 2);

      // Static Unit Badge BY ITSELF at Very Bottom
      spr.setTextColor(accentColor, darkAccentColor);
      spr.setTextDatum(BC_DATUM);
      spr.drawString(unitStr, x + (w / 2), y + h - 2, 2);
    } else if (h < 45) {
      // Stacked Horizontal Bar for 5 or 6 active metrics (Compact Stack)
      spr.setTextColor(UI_COLOR_LIGHT_GRAY, darkAccentColor);
      spr.setTextDatum(TL_DATUM);
      spr.drawString(label, x + 6, y + 2, 2);

      // Render Static Unit Badge on Far Right
      spr.setTextColor(accentColor, darkAccentColor);
      spr.setTextDatum(TR_DATUM);
      spr.drawString(unitStr, x + w - 6, y + 2, 2);
      int unitW = spr.textWidth(unitStr, 2);

      // Render Dynamic Number BY ITSELF to the Left of Static Unit
      spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
      spr.drawNumber((int)val, x + w - 10 - unitW, y + 2, 2);

      int barX = x + 6;
      int barY = y + 18;
      int barW = w - 12;
      int barH = h - 20;
      if (barH < 6)
        barH = 6;

      spr.drawRoundRect(barX, barY, barW, barH, 3, UI_COLOR_GRAY);

      int fillW = map(constrain(val, minV, maxV), minV, maxV, 0, barW - 2);
      uint16_t barColor = accentColor;
      if (isCoolant) {
        if (val <= coldVal)
          barColor = UI_COLOR_BLUE;
        else if (val >= hotVal)
          barColor = UI_COLOR_RED;
      }

      if (fillW > 0) {
        spr.fillRoundRect(barX + 1, barY + 1, fillW, barH - 2, 2, barColor);
      }
    } else {
      // Full Horizontal Digital Gauge Bar Card
      spr.setTextColor(UI_COLOR_LIGHT_GRAY, darkAccentColor);
      spr.setTextDatum(TL_DATUM);
      spr.drawString(label, x + 8, y + 4, 2);

      // Render Static Unit Badge on Top-Right
      spr.setTextColor(accentColor, darkAccentColor);
      spr.setTextDatum(TR_DATUM);
      spr.drawString(unitStr, x + w - 8, y + 6, 2);
      int unitW = spr.textWidth(unitStr, 2);

      int valInt = (int)val;
      bool isFourDigits = (valInt >= 1000 || valInt <= -100);
      int valFont = (isFourDigits && w < 160) ? 2 : 4;

      // Render Dynamic Number BY ITSELF to the Left of Static Unit
      spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
      spr.drawNumber(valInt, x + w - 12 - unitW, y + 4, valFont);

      // Horizontal Progress Bar Container
      int barX = x + 8;
      int barY = y + 28;
      int barW = w - 16;
      int barH = h - 46;
      if (barH < 10)
        barH = 10;

      spr.drawRoundRect(barX, barY, barW, barH, 4, UI_COLOR_GRAY);

      int fillW = map(constrain(val, minV, maxV), minV, maxV, 0, barW - 4);
      uint16_t barColor = accentColor;
      if (isCoolant) {
        if (val <= coldVal)
          barColor = UI_COLOR_BLUE;
        else if (val >= hotVal)
          barColor = UI_COLOR_RED;
      }

      if (fillW > 0) {
        spr.fillRoundRect(barX + 2, barY + 2, fillW, barH - 4, 3, barColor);
      }

      // Scale Ticks & Numbers Underneath Horizontal Bar
      int numY = barY + barH + 2;
      if (numY + 10 <= y + h) {
        spr.setTextColor(UI_COLOR_GRAY, darkAccentColor);

        spr.setTextDatum(TL_DATUM);
        spr.drawNumber((int)minV, barX, numY, 2);

        spr.setTextDatum(TC_DATUM);
        spr.drawNumber((int)((minV + maxV) / 2), barX + (barW / 2), numY, 2);

        spr.setTextDatum(TR_DATUM);
        spr.drawNumber((int)maxV, barX + barW, numY, 2);
      }
    }
  }
}

// Hybrid Style: Multi-Segmented Horizontal/Vertical Bar Card with
// Dynamic Number
void GaugeUI::drawSegmentedBarInRect(int x, int y, int w, int h, float val,
                                     float minV, float maxV, const char *label,
                                     const char *unitStr, uint16_t accentColor,
                                     bool showRedline, float redlineVal,
                                     bool isCoolantGauge, float coldVal,
                                     float hotVal) {
  if (useSprite) {
    spr.fillRoundRect(x, y, w, h, 6, darkAccentColor);
    spr.drawRoundRect(x, y, w, h, 6, accentColor);

    float norm = (maxV > minV)
                     ? constrain((val - minV) / (maxV - minV), 0.0f, 1.0f)
                     : 0.0f;

    if (w < 60) {
      // Narrow Vertical Segmented Ladder (for side slots)
      int barH = h - 44;
      if (barH < 10)
        barH = 10;
      int numSegs = 14;
      int gapH = 2;
      int segH = (barH - (numSegs - 1) * gapH) / numSegs;
      if (segH < 2)
        segH = 2;
      int activeSegs = round(norm * numSegs);

      char shortLabel[6];
      strncpy(shortLabel, label, 4);
      shortLabel[4] = '\0';
      spr.setTextColor(UI_COLOR_LIGHT_GRAY, darkAccentColor);
      spr.setTextDatum(TC_DATUM);
      spr.drawString(shortLabel, x + (w / 2), y + 2, 2);

      int startY = y + 18 + barH - segH;
      for (int i = 0; i < numSegs; i++) {
        int segY = startY - i * (segH + gapH);
        float segVal = minV + (i + 0.5f) * (maxV - minV) / numSegs;

        uint16_t segColor = accentColor;
        if (isCoolantGauge) {
          if (segVal <= coldVal)
            segColor = UI_COLOR_BLUE;
          else if (segVal >= hotVal)
            segColor = UI_COLOR_RED;
        } else if (showRedline && segVal >= redlineVal) {
          segColor = UI_COLOR_RED;
        } else if (i >= numSegs * 0.8f) {
          segColor = UI_COLOR_YELLOW;
        }

        if (i < activeSegs) {
          spr.fillRect(x + 4, segY, w - 8, segH, segColor);
        } else {
          spr.fillRect(x + 4, segY, w - 8, segH, 0x1082);
          spr.drawRect(x + 4, segY, w - 8, segH, 0x2945);
        }
      }

      spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
      spr.setTextDatum(BC_DATUM);
      spr.drawNumber((int)val, x + (w / 2), y + h - 14, 2);

      spr.setTextColor(accentColor, darkAccentColor);
      spr.setTextDatum(BC_DATUM);
      spr.drawString(unitStr, x + (w / 2), y + h - 2, 2);

    } else if (h < 48) {
      // Compact Horizontal Segmented Bar Card (for 5 or 6 stacked metrics)
      spr.setTextColor(UI_COLOR_LIGHT_GRAY, darkAccentColor);
      spr.setTextDatum(TL_DATUM);
      spr.drawString(label, x + 6, y + 2, 2);

      spr.setTextColor(accentColor, darkAccentColor);
      spr.setTextDatum(TR_DATUM);
      spr.drawString(unitStr, x + w - 6, y + 2, 2);
      int unitW = spr.textWidth(unitStr, 2);

      spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
      spr.drawNumber((int)val, x + w - 10 - unitW, y + 2, 2);

      int barX = x + 6;
      int barY = y + 20;
      int barW = w - 12;
      int barH = h - 24;
      if (barH < 6)
        barH = 6;

      int numSegs = 16;
      int gapW = 2;
      int segW = (barW - (numSegs - 1) * gapW) / numSegs;
      if (segW < 2)
        segW = 2;
      int activeSegs = round(norm * numSegs);

      for (int i = 0; i < numSegs; i++) {
        int segX = barX + i * (segW + gapW);
        float segVal = minV + (i + 0.5f) * (maxV - minV) / numSegs;

        uint16_t segColor = accentColor;
        if (isCoolantGauge) {
          if (segVal <= coldVal)
            segColor = UI_COLOR_BLUE;
          else if (segVal >= hotVal)
            segColor = UI_COLOR_RED;
        } else if (showRedline && segVal >= redlineVal) {
          segColor = UI_COLOR_RED;
        } else if (i >= numSegs * 0.8f) {
          segColor = UI_COLOR_YELLOW;
        }

        if (i < activeSegs) {
          spr.fillRect(segX, barY, segW, barH, segColor);
        } else {
          spr.fillRect(segX, barY, segW, barH, 0x1082);
          spr.drawRect(segX, barY, segW, barH, 0x2945);
        }
      }
    } else {
      // Standard Horizontal Segmented Bar Card (Large 1 to 4 metric slots)
      spr.setTextColor(UI_COLOR_LIGHT_GRAY, darkAccentColor);
      spr.setTextDatum(TL_DATUM);
      spr.drawString(label, x + 8, y + 4, 2);

      spr.setTextColor(accentColor, darkAccentColor);
      spr.setTextDatum(TR_DATUM);
      spr.drawString(unitStr, x + w - 8, y + 6, 2);
      int unitW = spr.textWidth(unitStr, 2);

      int valInt = (int)val;
      bool isFourDigits = (valInt >= 1000 || valInt <= -100);
      int valFont = (isFourDigits && w < 160) ? 2 : 4;

      spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
      spr.drawNumber(valInt, x + w - 12 - unitW, y + 4, valFont);

      int barX = x + 8;
      int barY = y + 30;
      int barW = w - 16;
      int barH = h - 48;
      if (barH < 10)
        barH = 10;

      int numSegs = 18;
      int gapW = 2;
      int segW = (barW - (numSegs - 1) * gapW) / numSegs;
      if (segW < 2)
        segW = 2;
      int activeSegs = round(norm * numSegs);

      for (int i = 0; i < numSegs; i++) {
        int segX = barX + i * (segW + gapW);
        float segVal = minV + (i + 0.5f) * (maxV - minV) / numSegs;

        uint16_t segColor = accentColor;
        if (isCoolantGauge) {
          if (segVal <= coldVal)
            segColor = UI_COLOR_BLUE;
          else if (segVal >= hotVal)
            segColor = UI_COLOR_RED;
        } else if (showRedline && segVal >= redlineVal) {
          segColor = UI_COLOR_RED;
        } else if (i >= numSegs * 0.8f) {
          segColor = UI_COLOR_YELLOW;
        }

        if (i < activeSegs) {
          spr.fillRect(segX, barY, segW, barH, segColor);
        } else {
          spr.fillRect(segX, barY, segW, barH, 0x1082);
          spr.drawRect(segX, barY, segW, barH, 0x2945);
        }
      }

      int numY = barY + barH + 2;
      if (numY + 10 <= y + h) {
        spr.setTextColor(UI_COLOR_GRAY, darkAccentColor);
        spr.setTextDatum(TL_DATUM);
        spr.drawNumber((int)minV, barX, numY, 2);
        spr.setTextDatum(TC_DATUM);
        spr.drawNumber((int)((minV + maxV) / 2), barX + (barW / 2), numY, 2);
        spr.setTextDatum(TR_DATUM);
        spr.drawNumber((int)maxV, barX + barW, numY, 2);
      }
    }
  } else {
    tft.fillRoundRect(x, y, w, h, 6, darkAccentColor);
    tft.drawRoundRect(x, y, w, h, 6, accentColor);
  }
}

// Hybrid Style: Sweeping Arc Segmented Bargraph wrapping central
// Digital Number Readout
void GaugeUI::drawSegmentedArcGauge(int x, int y, int w, int h, float val,
                                    float minV, float maxV, const char *label,
                                    const char *unitStr, uint16_t accentColor,
                                    bool showRedline, float redlineVal,
                                    bool isCoolantGauge, float coldVal,
                                    float hotVal) {
  if (useSprite) {
    spr.fillRoundRect(x, y, w, h, 6, darkAccentColor);
    spr.drawRoundRect(x, y, w, h, 6, accentColor);

    int cx = x + (w / 2);
    int cy = y + (h / 2) + 8;
    int rOut = (min(w, h) / 2) - 6;
    int rIn = rOut - 16;
    if (rIn < 20)
      rIn = 20;

    spr.setTextColor(UI_COLOR_LIGHT_GRAY, darkAccentColor);
    spr.setTextDatum(TC_DATUM);
    spr.drawString(label, cx, y + 4, 2);

    int valInt = (int)val;
    bool isFourDigits = (valInt >= 1000 || valInt <= -100);

    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(MC_DATUM);
    if (h > 150 && w > 150) {
      int fontNum = (isFourDigits && w < 180) ? 4 : 6;
      spr.drawNumber(valInt, cx, cy - 6, fontNum);
    } else {
      spr.drawNumber(valInt, cx, cy - 4, 4);
    }

    spr.setTextColor(accentColor, darkAccentColor);
    spr.setTextDatum(TC_DATUM);
    spr.drawString(unitStr, cx, cy + (h > 150 ? 20 : 12), 2);

    float startAng = -135.0f;
    float endAng = 135.0f;
    float sweep = endAng - startAng;
    int numSegs = 20;
    float segAngle = sweep / (float)numSegs;
    float gapAngle = 1.2f;

    float norm = (maxV > minV)
                     ? constrain((val - minV) / (maxV - minV), 0.0f, 1.0f)
                     : 0.0f;
    int activeSegs = round(norm * numSegs);

    for (int i = 0; i < numSegs; i++) {
      float a1 = startAng + i * segAngle + gapAngle;
      float a2 = startAng + (i + 1) * segAngle - gapAngle;

      float rad1 = (a1 - 90.0f) * DEG_TO_RAD;
      float rad2 = (a2 - 90.0f) * DEG_TO_RAD;

      int x1 = cx + cos(rad1) * rIn;
      int y1 = cy + sin(rad1) * rIn;
      int x2 = cx + cos(rad1) * rOut;
      int y2 = cy + sin(rad1) * rOut;
      int x3 = cx + cos(rad2) * rOut;
      int y3 = cy + sin(rad2) * rOut;
      int x4 = cx + cos(rad2) * rIn;
      int y4 = cy + sin(rad2) * rIn;

      float segVal = minV + (i + 0.5f) * (maxV - minV) / numSegs;
      uint16_t segColor = accentColor;

      if (isCoolantGauge) {
        if (segVal <= coldVal)
          segColor = UI_COLOR_BLUE;
        else if (segVal >= hotVal)
          segColor = UI_COLOR_RED;
      } else if (showRedline && segVal >= redlineVal) {
        segColor = UI_COLOR_RED;
      } else if (i >= numSegs * 0.8f) {
        segColor = UI_COLOR_YELLOW;
      }

      if (i < activeSegs) {
        spr.fillTriangle(x1, y1, x2, y2, x3, y3, segColor);
        spr.fillTriangle(x1, y1, x3, y3, x4, y4, segColor);
      } else {
        spr.fillTriangle(x1, y1, x2, y2, x3, y3, 0x1082);
        spr.fillTriangle(x1, y1, x3, y3, x4, y4, 0x1082);
        spr.drawLine(x1, y1, x2, y2, 0x2945);
        spr.drawLine(x2, y2, x3, y3, 0x2945);
        spr.drawLine(x3, y3, x4, y4, 0x2945);
        spr.drawLine(x4, y4, x1, y1, 0x2945);
      }
    }
  } else {
    tft.fillRoundRect(x, y, w, h, 6, darkAccentColor);
    tft.drawRoundRect(x, y, w, h, 6, accentColor);
  }
}

// Dynamic Adaptive Gauge Cluster Layout Renderer (Optimized for 3, 4, 5 Analog
// Dials)
void GaugeUI::renderDynamicCluster(const VehicleData &data,
                                   const SystemConfig &config) {
  MetricInfo activeMetrics[7];
  uint8_t count = 0;

  float maxSpeedVal = (config.speedUnit == UNIT_MPH)
                          ? (float)config.maxSpeed
                          : (float)round(config.maxSpeed * 1.60934f);

  if (config.isMetricEnabled(METRIC_BIT_SPEED)) {
    float spd = (config.speedUnit == UNIT_MPH)
                    ? (data.smoothedSpeedKmh / 1.60934f)
                    : data.smoothedSpeedKmh;
    const char *uStr = (config.speedUnit == UNIT_MPH) ? "MPH" : "KM/H";
    activeMetrics[count++] = {METRIC_BIT_SPEED, "SPEED",     uStr, spd, 0.0f,
                              maxSpeedVal,      primaryColor};
  }
  if (config.isMetricEnabled(METRIC_BIT_COOLANT)) {
    float temp = (config.tempUnit == UNIT_FAHRENHEIT)
                     ? ((data.smoothedCoolantTempC * 9 / 5) + 32)
                     : data.smoothedCoolantTempC;
    float minT = (config.tempUnit == UNIT_FAHRENHEIT)
                     ? ((config.minCoolantTemp * 9 / 5) + 32)
                     : config.minCoolantTemp;
    float maxT = (config.tempUnit == UNIT_FAHRENHEIT)
                     ? ((config.maxCoolantTemp * 9 / 5) + 32)
                     : config.maxCoolantTemp;
    float coldT = (config.tempUnit == UNIT_FAHRENHEIT)
                      ? ((config.coldTempThreshold * 9 / 5) + 32)
                      : config.coldTempThreshold;
    float hotT = (config.tempUnit == UNIT_FAHRENHEIT)
                     ? ((config.highTempThreshold * 9 / 5) + 32)
                     : config.highTempThreshold;
    const char *uStr = (config.tempUnit == UNIT_FAHRENHEIT) ? "deg F" : "deg C";
    uint16_t tColor =
        (data.smoothedCoolantTempC >= config.highTempThreshold)
            ? UI_COLOR_RED
            : (data.smoothedCoolantTempC <= config.coldTempThreshold
                   ? UI_COLOR_BLUE
                   : primaryColor);
    activeMetrics[count++] = {
        METRIC_BIT_COOLANT, "COOLANT", uStr, temp, minT, maxT, tColor};
  }
  if (config.isMetricEnabled(METRIC_BIT_RPM)) {
    uint16_t rColor =
        (data.smoothedRpm >= config.redlineRpm) ? UI_COLOR_RED : primaryColor;
    activeMetrics[count++] = {
        METRIC_BIT_RPM, "ENGINE RPM",         "RPM", data.smoothedRpm,
        0.0f,           (float)config.maxRpm, rColor};
  }
  if (config.isMetricEnabled(METRIC_BIT_THROTTLE)) {
    activeMetrics[count++] = {METRIC_BIT_THROTTLE,
                              "THROTTLE",
                              "%",
                              data.smoothedThrottlePct,
                              0.0f,
                              100.0f,
                              primaryColor};
  }
  if (config.isMetricEnabled(METRIC_BIT_INTAKE)) {
    float inTemp = (config.tempUnit == UNIT_FAHRENHEIT)
                       ? ((data.smoothedIntakeTempC * 9 / 5) + 32)
                       : data.smoothedIntakeTempC;
    float minInT = (config.tempUnit == UNIT_FAHRENHEIT)
                       ? ((config.minIntakeTemp * 9 / 5) + 32)
                       : config.minIntakeTemp;
    float maxInT = (config.tempUnit == UNIT_FAHRENHEIT)
                       ? ((config.maxIntakeTemp * 9 / 5) + 32)
                       : config.maxIntakeTemp;
    const char *uStr = (config.tempUnit == UNIT_FAHRENHEIT) ? "deg F" : "deg C";
    activeMetrics[count++] = {
        METRIC_BIT_INTAKE, "INTAKE AIR", uStr, inTemp, minInT, maxInT,
        primaryColor};
  }
  if (config.isMetricEnabled(METRIC_BIT_LOAD)) {
    activeMetrics[count++] = {
        METRIC_BIT_LOAD, "ENG LOAD",  "%", data.smoothedEngineLoadPct, 0.0f,
        100.0f,          primaryColor};
  }
  if (config.isMetricEnabled(METRIC_BIT_BATTERY)) {
    activeMetrics[count++] = {METRIC_BIT_BATTERY, "BATTERY", "V",
                              data.batteryVolts,  10.0f,     16.0f,
                              primaryColor};
  }

  if (count == 0)
    return;

  float coldC = (config.tempUnit == UNIT_FAHRENHEIT)
                    ? ((config.coldTempThreshold * 9 / 5) + 32)
                    : config.coldTempThreshold;
  float hotC = (config.tempUnit == UNIT_FAHRENHEIT)
                   ? ((config.highTempThreshold * 9 / 5) + 32)
                   : config.highTempThreshold;

  float startAng = (float)config.gaugeStartAngle;
  float endAng = (float)config.gaugeEndAngle;

  // Single Primary Gauge Layout (Large 180-degree Semicircle Dial)
  if (config.gaugeLayout == LAYOUT_SINGLE_GAUGE) {
    int primaryIdx = 0;
    for (int i = 0; i < count; i++) {
      if (activeMetrics[i].bitIndex == config.primaryMetricIndex) {
        primaryIdx = i;
        break;
      }
    }

    MetricInfo primaryMetric = activeMetrics[primaryIdx];
    bool isCoolantP = (primaryMetric.bitIndex == METRIC_BIT_COOLANT);

    // Giant 180 Degree Semicircle Dial spanning screen width with center at
    // bottom (cx=160, cy=222, r=150)
    drawAnalogDialInRect(
        6, 26, 308, 392, primaryMetric.value, primaryMetric.minVal,
        primaryMetric.maxVal, primaryMetric.label, primaryMetric.unitStr,
        primaryMetric.color, (primaryMetric.bitIndex == METRIC_BIT_RPM),
        (float)config.redlineRpm, isCoolantP, coldC, hotC, -90.0f, 90.0f);
    return;
  }

  // Focused Central Gauge Layout
  if (config.gaugeLayout == LAYOUT_FOCUS_CENTER) {
    int primaryIdx = 0;
    for (int i = 0; i < count; i++) {
      if (activeMetrics[i].bitIndex == config.primaryMetricIndex) {
        primaryIdx = i;
        break;
      }
    }

    MetricInfo primaryMetric = activeMetrics[primaryIdx];

    MetricInfo sideMetrics[6];
    int sideCount = 0;
    for (int i = 0; i < count; i++) {
      if (i != primaryIdx) {
        sideMetrics[sideCount++] = activeMetrics[i];
      }
    }

    bool isCoolantP = (primaryMetric.bitIndex == METRIC_BIT_COOLANT);

    if (sideCount == 0) {
      drawAnalogDialInRect(
          6, 26, 308, 210, primaryMetric.value, primaryMetric.minVal,
          primaryMetric.maxVal, primaryMetric.label, primaryMetric.unitStr,
          primaryMetric.color, (primaryMetric.bitIndex == METRIC_BIT_RPM),
          (float)config.redlineRpm, isCoolantP, coldC, hotC, startAng, endAng);
    } else if (sideCount == 1) {
      bool isCoolantS0 = (sideMetrics[0].bitIndex == METRIC_BIT_COOLANT);
      drawLevelBarInRect(4, 26, 40, 210, sideMetrics[0].value,
                         sideMetrics[0].minVal, sideMetrics[0].maxVal,
                         sideMetrics[0].label, sideMetrics[0].unitStr,
                         sideMetrics[0].color, isCoolantS0, coldC, hotC);
      drawAnalogDialInRect(
          48, 26, 266, 210, primaryMetric.value, primaryMetric.minVal,
          primaryMetric.maxVal, primaryMetric.label, primaryMetric.unitStr,
          primaryMetric.color, (primaryMetric.bitIndex == METRIC_BIT_RPM),
          (float)config.redlineRpm, isCoolantP, coldC, hotC, startAng, endAng);
    } else {
      bool isCoolantS0 = (sideMetrics[0].bitIndex == METRIC_BIT_COOLANT);
      bool isCoolantS1 = (sideMetrics[1].bitIndex == METRIC_BIT_COOLANT);
      drawLevelBarInRect(4, 26, 34, 210, sideMetrics[0].value,
                         sideMetrics[0].minVal, sideMetrics[0].maxVal,
                         sideMetrics[0].label, sideMetrics[0].unitStr,
                         sideMetrics[0].color, isCoolantS0, coldC, hotC);
      drawAnalogDialInRect(
          42, 26, 236, 210, primaryMetric.value, primaryMetric.minVal,
          primaryMetric.maxVal, primaryMetric.label, primaryMetric.unitStr,
          primaryMetric.color, (primaryMetric.bitIndex == METRIC_BIT_RPM),
          (float)config.redlineRpm, isCoolantP, coldC, hotC, startAng, endAng);
      drawLevelBarInRect(282, 26, 34, 210, sideMetrics[1].value,
                         sideMetrics[1].minVal, sideMetrics[1].maxVal,
                         sideMetrics[1].label, sideMetrics[1].unitStr,
                         sideMetrics[1].color, isCoolantS1, coldC, hotC);
    }
    return;
  }

  // ALL DIGITAL LAYOUT ENGINE: Stack all active metrics vertically as
  // full-width horizontal bars
  if (config.gaugeLayout == LAYOUT_ALL_DIGITAL) {
    int totalH = 210;
    int gap = 3;
    int cardH = (totalH - ((count - 1) * gap)) / count;
    if (cardH < 26)
      cardH = 26;

    for (int i = 0; i < count; i++) {
      int cardY = 26 + (i * (cardH + gap));
      MetricInfo &m = activeMetrics[i];
      bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
      drawLevelBarInRect(4, cardY, 312, cardH, m.value, m.minVal, m.maxVal,
                         m.label, m.unitStr, m.color, isCoolant, coldC, hotC);
    }
    return;
  }

  // HYBRID GAUGE CLUSTER ENGINE (Segmented Bar Graphs + Digital
  // Displays)
  if (config.gaugeLayout == LAYOUT_HYBRID_BARS) {
    if (count == 1) {
      MetricInfo &m = activeMetrics[0];
      bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
      drawSegmentedArcGauge(6, 26, 308, 210, m.value, m.minVal, m.maxVal,
                            m.label, m.unitStr, m.color,
                            (m.bitIndex == METRIC_BIT_RPM),
                            (float)config.redlineRpm, isCoolant, coldC, hotC);
    } else if (count == 2) {
      for (int i = 0; i < 2; i++) {
        int bx = 4 + (i * 158);
        MetricInfo &m = activeMetrics[i];
        bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
        drawSegmentedArcGauge(bx, 26, 154, 210, m.value, m.minVal, m.maxVal,
                              m.label, m.unitStr, m.color,
                              (m.bitIndex == METRIC_BIT_RPM),
                              (float)config.redlineRpm, isCoolant, coldC, hotC);
      }
    } else if (count == 3) {
      // 1 Large Sweeping Segmented Arc Gauge Top + 2 Horizontal Segmented Bars
      // Bottom
      MetricInfo &m0 = activeMetrics[0];
      MetricInfo &m1 = activeMetrics[1];
      MetricInfo &m2 = activeMetrics[2];
      drawSegmentedArcGauge(75, 26, 170, 116, m0.value, m0.minVal, m0.maxVal,
                            m0.label, m0.unitStr, m0.color,
                            (m0.bitIndex == METRIC_BIT_RPM),
                            (float)config.redlineRpm,
                            (m0.bitIndex == METRIC_BIT_COOLANT), coldC, hotC);
      drawSegmentedBarInRect(
          4, 144, 154, 92, m1.value, m1.minVal, m1.maxVal, m1.label, m1.unitStr,
          m1.color, (m1.bitIndex == METRIC_BIT_RPM), (float)config.redlineRpm,
          (m1.bitIndex == METRIC_BIT_COOLANT), coldC, hotC);
      drawSegmentedBarInRect(162, 144, 154, 92, m2.value, m2.minVal, m2.maxVal,
                             m2.label, m2.unitStr, m2.color,
                             (m2.bitIndex == METRIC_BIT_RPM),
                             (float)config.redlineRpm,
                             (m2.bitIndex == METRIC_BIT_COOLANT), coldC, hotC);
    } else if (count == 4) {
      int coords[4][2] = {{4, 26}, {162, 26}, {4, 132}, {162, 132}};
      for (int i = 0; i < 4; i++) {
        int bx = coords[i][0];
        int by = coords[i][1];
        MetricInfo &m = activeMetrics[i];
        bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
        drawSegmentedBarInRect(
            bx, by, 154, 103, m.value, m.minVal, m.maxVal, m.label, m.unitStr,
            m.color, (m.bitIndex == METRIC_BIT_RPM), (float)config.redlineRpm,
            isCoolant, coldC, hotC);
      }
    } else if (count == 5) {
      for (int i = 0; i < 3; i++) {
        int bx = 4 + (i * 105);
        MetricInfo &m = activeMetrics[i];
        bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
        drawSegmentedBarInRect(
            bx, 26, 102, 103, m.value, m.minVal, m.maxVal, m.label, m.unitStr,
            m.color, (m.bitIndex == METRIC_BIT_RPM), (float)config.redlineRpm,
            isCoolant, coldC, hotC);
      }
      for (int i = 3; i < 5; i++) {
        int bx = 56 + ((i - 3) * 105);
        MetricInfo &m = activeMetrics[i];
        bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
        drawSegmentedBarInRect(
            bx, 131, 102, 103, m.value, m.minVal, m.maxVal, m.label, m.unitStr,
            m.color, (m.bitIndex == METRIC_BIT_RPM), (float)config.redlineRpm,
            isCoolant, coldC, hotC);
      }
    } else {
      for (int i = 0; i < count && i < 6; i++) {
        int col = i % 3;
        int row = i / 3;
        int bx = 4 + (col * 105);
        int by = 26 + (row * 104);
        MetricInfo &m = activeMetrics[i];
        bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
        drawSegmentedBarInRect(
            bx, by, 102, 101, m.value, m.minVal, m.maxVal, m.label, m.unitStr,
            m.color, (m.bitIndex == METRIC_BIT_RPM), (float)config.redlineRpm,
            isCoolant, coldC, hotC);
      }
    }
    return;
  }

  // Layout 0 (ALL ANALOG), Layout 2 (ALL NUMBERS)
  if (count == 1) {
    MetricInfo &m = activeMetrics[0];
    bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
    if (config.gaugeLayout == LAYOUT_ALL_ANALOG) {
      drawAnalogDialInRect(
          6, 26, 308, 210, m.value, m.minVal, m.maxVal, m.label, m.unitStr,
          m.color, (m.bitIndex == METRIC_BIT_RPM), (float)config.redlineRpm,
          isCoolant, coldC, hotC, startAng, endAng);
    } else {
      drawDigitalBoxInRect(6, 26, 308, 210, m.value, m.label, m.unitStr,
                           m.color);
    }
  } else if (count == 2) {
    for (int i = 0; i < 2; i++) {
      int bx = 4 + (i * 158);
      MetricInfo &m = activeMetrics[i];
      bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
      if (config.gaugeLayout == LAYOUT_ALL_ANALOG) {
        drawAnalogDialInRect(
            bx, 26, 154, 210, m.value, m.minVal, m.maxVal, m.label, m.unitStr,
            m.color, (m.bitIndex == METRIC_BIT_RPM), (float)config.redlineRpm,
            isCoolant, coldC, hotC, startAng, endAng);
      } else {
        drawDigitalBoxInRect(bx, 26, 154, 210, m.value, m.label, m.unitStr,
                             m.color);
      }
    }
  } else if (count == 3) {
    // Optimized 3 Analog Dials Cluster (1 Large Centered Top + 2 Bottom Row to
    // fill 210px height)
    if (config.gaugeLayout == LAYOUT_ALL_ANALOG) {
      MetricInfo &m0 = activeMetrics[0];
      MetricInfo &m1 = activeMetrics[1];
      MetricInfo &m2 = activeMetrics[2];
      drawAnalogDialInRect(
          80, 30, 160, 120, m0.value, m0.minVal, m0.maxVal, m0.label,
          m0.unitStr, m0.color, (m0.bitIndex == METRIC_BIT_RPM),
          (float)config.redlineRpm, (m0.bitIndex == METRIC_BIT_COOLANT), coldC,
          hotC, startAng, endAng);
      drawAnalogDialInRect(
          4, 120, 154, 120, m1.value, m1.minVal, m1.maxVal, m1.label,
          m1.unitStr, m1.color, (m1.bitIndex == METRIC_BIT_RPM),
          (float)config.redlineRpm, (m1.bitIndex == METRIC_BIT_COOLANT), coldC,
          hotC, startAng, endAng);
      drawAnalogDialInRect(
          162, 120, 154, 120, m2.value, m2.minVal, m2.maxVal, m2.label,
          m2.unitStr, m2.color, (m2.bitIndex == METRIC_BIT_RPM),
          (float)config.redlineRpm, (m2.bitIndex == METRIC_BIT_COOLANT), coldC,
          hotC, startAng, endAng);
    } else {
      for (int i = 0; i < 3; i++) {
        int bx = 4 + (i * 105);
        MetricInfo &m = activeMetrics[i];
        drawDigitalBoxInRect(bx, 26, 102, 210, m.value, m.label, m.unitStr,
                             m.color);
      }
    }
  } else if (count == 4) {
    int coords[4][2] = {{4, 26}, {162, 26}, {4, 132}, {162, 132}};
    for (int i = 0; i < 4; i++) {
      int bx = coords[i][0];
      int by = coords[i][1];
      MetricInfo &m = activeMetrics[i];
      bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
      if (config.gaugeLayout == LAYOUT_ALL_ANALOG) {
        drawAnalogDialInRect(
            bx, by, 154, 103, m.value, m.minVal, m.maxVal, m.label, m.unitStr,
            m.color, (m.bitIndex == METRIC_BIT_RPM), (float)config.redlineRpm,
            isCoolant, coldC, hotC, startAng, endAng);
      } else {
        drawDigitalBoxInRect(bx, by, 154, 103, m.value, m.label, m.unitStr,
                             m.color);
      }
    }
  } else if (count == 5) {
    // Optimized 5 Dials Cluster (3 Top Row + 2 Centered Bottom Row)
    for (int i = 0; i < 3; i++) {
      int bx = 4 + (i * 105);
      MetricInfo &m = activeMetrics[i];
      bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
      if (config.gaugeLayout == LAYOUT_ALL_ANALOG) {
        drawAnalogDialInRect(
            bx, 26, 102, 103, m.value, m.minVal, m.maxVal, m.label, m.unitStr,
            m.color, (m.bitIndex == METRIC_BIT_RPM), (float)config.redlineRpm,
            isCoolant, coldC, hotC, startAng, endAng);
      } else {
        drawDigitalBoxInRect(bx, 26, 102, 103, m.value, m.label, m.unitStr,
                             m.color);
      }
    }
    for (int i = 3; i < 5; i++) {
      int bx = 56 + ((i - 3) * 105);
      MetricInfo &m = activeMetrics[i];
      bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
      if (config.gaugeLayout == LAYOUT_ALL_ANALOG) {
        drawAnalogDialInRect(
            bx, 131, 102, 103, m.value, m.minVal, m.maxVal, m.label, m.unitStr,
            m.color, (m.bitIndex == METRIC_BIT_RPM), (float)config.redlineRpm,
            isCoolant, coldC, hotC, startAng, endAng);
      } else {
        drawDigitalBoxInRect(bx, 131, 102, 103, m.value, m.label, m.unitStr,
                             m.color);
      }
    }
  } else {
    for (int i = 0; i < count && i < 6; i++) {
      int col = i % 3;
      int row = i / 3;
      int bx = 4 + (col * 105);
      int by = 26 + (row * 104);
      MetricInfo &m = activeMetrics[i];
      bool isCoolant = (m.bitIndex == METRIC_BIT_COOLANT);
      if (config.gaugeLayout == LAYOUT_ALL_ANALOG) {
        drawAnalogDialInRect(
            bx, by, 102, 101, m.value, m.minVal, m.maxVal, m.label, m.unitStr,
            m.color, (m.bitIndex == METRIC_BIT_RPM), (float)config.redlineRpm,
            isCoolant, coldC, hotC, startAng, endAng);
      } else {
        drawDigitalBoxInRect(bx, by, 102, 101, m.value, m.label, m.unitStr,
                             m.color);
      }
    }
  }
}

void GaugeUI::renderGaugeCluster(const VehicleData &data,
                                 const SystemConfig &config, bool forceRedraw) {
  if (config.colorTheme != prevTheme || config.gaugeLayout != prevLayout ||
      config.primaryMetricIndex != prevPrimaryMetric ||
      config.metricsBitmask != prevMetricsBitmask ||
      config.maxSpeed != prevMaxSpeed || config.maxRpm != prevMaxRpm ||
      config.gaugeStartAngle != prevGaugeStartAngle ||
      config.gaugeEndAngle != prevGaugeEndAngle) {
    updateThemeColors(config.colorTheme);
    prevTheme = config.colorTheme;
    prevLayout = config.gaugeLayout;
    prevPrimaryMetric = config.primaryMetricIndex;
    prevMetricsBitmask = config.metricsBitmask;
    prevMaxSpeed = config.maxSpeed;
    prevMaxRpm = config.maxRpm;
    prevGaugeStartAngle = config.gaugeStartAngle;
    prevGaugeEndAngle = config.gaugeEndAngle;
    forceRedraw = true;
  }

  if (useSprite) {
    spr.fillSprite(UI_COLOR_BLACK);
    drawHeader(data, config);
    renderDynamicCluster(data, config);
    spr.pushSprite(0, 0);
  } else {
    if (forceRedraw)
      tft.fillScreen(UI_COLOR_BLACK);
    drawHeader(data, config);
    renderDynamicCluster(data, config);
  }
}

// Settings Page 1 (General Settings & Telemetry Controls) with Spacious Buttons
void GaugeUI::renderSettingsMenu(const SystemConfig &config) {
  updateThemeColors(config.colorTheme);

  if (useSprite) {
    spr.fillSprite(UI_COLOR_BLACK);

    // Header Title with Arrow Navigation (Page 1 of 3)
    spr.fillRect(0, 0, 320, 26, darkAccentColor);
    spr.drawFastHLine(0, 26, 320, primaryColor);

    spr.fillRoundRect(4, 2, 24, 22, 4, darkAccentColor);
    spr.drawRoundRect(4, 2, 24, 22, 4, primaryColor);
    spr.fillTriangle(18, 8, 10, 13, 18, 18, primaryColor);

    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("SETTINGS (PAGE 1 / 3)", 160, 13, 2);

    spr.fillRoundRect(292, 2, 24, 22, 4, darkAccentColor);
    spr.drawRoundRect(292, 2, 24, 22, 4, primaryColor);
    spr.fillTriangle(298, 8, 306, 13, 298, 18, primaryColor);

    // Row 1: Speed Unit Button (Y: 28, H: 30)
    spr.fillRoundRect(8, 28, 304, 30, 6, darkAccentColor);
    spr.drawRoundRect(8, 28, 304, 30, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" Speed Unit", 16, 43, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(primaryColor, darkAccentColor);
    spr.drawString(config.speedUnit == UNIT_MPH ? "[ MPH ] " : "[ KM/H ] ", 304,
                   43, 2);

    // Row 2: Temp Unit Button (Y: 60, H: 30)
    spr.fillRoundRect(8, 60, 304, 30, 6, darkAccentColor);
    spr.drawRoundRect(8, 60, 304, 30, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" Temp Unit", 16, 75, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(primaryColor, darkAccentColor);
    spr.drawString(config.tempUnit == UNIT_FAHRENHEIT ? "[ deg F ] "
                                                      : "[ deg C ] ",
                   304, 75, 2);

    // Row 3: Primary Central Gauge Selector (Y: 92, H: 30)
    spr.fillRoundRect(8, 92, 304, 30, 6, darkAccentColor);
    spr.drawRoundRect(8, 92, 304, 30, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" Primary Gauge", 16, 107, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(primaryColor, darkAccentColor);
    const char *metricLabels[] = {"[ SPEED ] ",    "[ COOLANT ] ", "[ RPM ] ",
                                  "[ THROTTLE ] ", "[ INTAKE ] ",  "[ LOAD ] ",
                                  "[ BATTERY ] "};
    spr.drawString(metricLabels[config.primaryMetricIndex % 7], 304, 107, 2);

    // Row 4: Configure Telemetry PIDs Button (Y: 124, H: 30)
    spr.fillRoundRect(8, 124, 304, 30, 6, primaryColor);
    spr.setTextColor(UI_COLOR_BLACK, primaryColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("CONFIGURE ACTIVE OBD2 METRICS >", 160, 139, 2);

    // Row 5: OBD Mode & Color Theme Split Row (Y: 156, H: 30)
    spr.fillRoundRect(8, 156, 148, 30, 6, darkAccentColor);
    spr.drawRoundRect(8, 156, 148, 30, 6, primaryColor);
    spr.setTextColor(config.demoMode ? UI_COLOR_ORANGE : UI_COLOR_GREEN,
                     darkAccentColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawString(config.demoMode ? "MODE: DEMO" : "MODE: HARDWARE", 82, 171,
                   2);

    spr.fillRoundRect(164, 156, 148, 30, 6, darkAccentColor);
    spr.drawRoundRect(164, 156, 148, 30, 6, primaryColor);
    spr.setTextColor(primaryColor, darkAccentColor);
    spr.setTextDatum(MC_DATUM);
    const char *themeNames[] = {"THEME: CYAN", "THEME: RED", "THEME: AMBER",
                                "THEME: GREEN"};
    spr.drawString(themeNames[config.colorTheme % 4], 238, 171, 2);

    // Save & Exit Button at Bottom (Y: 194, H: 40)
    spr.fillRoundRect(8, 194, 304, 40, 8, primaryColor);
    spr.setTextColor(UI_COLOR_BLACK, primaryColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("SAVE & EXIT", 160, 214, 4);

    spr.pushSprite(0, 0);
  }
}

// Dedicated OBD2 Data Selection Sub-Menu Screen
void GaugeUI::renderMetricConfigMenu(const SystemConfig &config) {
  updateThemeColors(config.colorTheme);

  if (useSprite) {
    spr.fillSprite(UI_COLOR_BLACK);

    // Header Title
    spr.fillRect(0, 0, 320, 26, darkAccentColor);
    spr.drawFastHLine(0, 26, 320, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("SELECT ACTIVE TELEMETRY METRICS", 160, 13, 2);

    const char *metricNames[] = {
        "Vehicle Speed (0x0D)",   "Coolant Temp (0x05)",
        "Engine RPM (0x0C)",      "Throttle Position (0x11)",
        "Intake Air Temp (0x0F)", "Engine Load (0x04)"};

    for (uint8_t i = 0; i < 6; i++) {
      int yPos = 28 + (i * 27);
      bool enabled = config.isMetricEnabled(i);
      uint16_t btnColor = enabled ? UI_COLOR_GREEN : UI_COLOR_GRAY;

      spr.fillRoundRect(10, yPos, 300, 25, 4, darkAccentColor);
      spr.drawRoundRect(10, yPos, 300, 25, 4, btnColor);
      spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
      spr.setTextDatum(ML_DATUM);
      spr.drawString(metricNames[i], 20, yPos + 13, 2);

      spr.setTextDatum(MR_DATUM);
      spr.setTextColor(btnColor, darkAccentColor);
      spr.drawString(enabled ? "[ ENABLED ] " : "[ DISABLED ] ", 300, yPos + 13,
                     2);
    }

    // Back / Save Button
    spr.fillRoundRect(10, 194, 300, 40, 8, primaryColor);
    spr.setTextColor(UI_COLOR_BLACK, primaryColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("SAVE & BACK TO SETTINGS", 160, 214, 2);

    spr.pushSprite(0, 0);
  }
}

// Settings Page 2 (Speed & RPM Ranges) with Spacious Buttons
void GaugeUI::renderRangeConfigMenu(const SystemConfig &config) {
  updateThemeColors(config.colorTheme);

  if (useSprite) {
    spr.fillSprite(UI_COLOR_BLACK);

    // Header Title with Arrow Navigation (Page 2 of 3)
    spr.fillRect(0, 0, 320, 26, darkAccentColor);
    spr.drawFastHLine(0, 26, 320, primaryColor);

    spr.fillRoundRect(4, 2, 24, 22, 4, darkAccentColor);
    spr.drawRoundRect(4, 2, 24, 22, 4, primaryColor);
    spr.fillTriangle(18, 8, 10, 13, 18, 18, primaryColor);

    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("SPEED & ARCS (PAGE 2 / 3)", 160, 13, 2);

    spr.fillRoundRect(292, 2, 24, 22, 4, darkAccentColor);
    spr.drawRoundRect(292, 2, 24, 22, 4, primaryColor);
    spr.fillTriangle(298, 8, 306, 13, 298, 18, primaryColor);

    // Row 1: Max Speed Scale Range (Y: 28, H: 28)
    spr.fillRoundRect(10, 28, 300, 28, 6, darkAccentColor);
    spr.drawRoundRect(10, 28, 300, 28, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" Max Speed Scale", 20, 42, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(primaryColor, darkAccentColor);
    char spdBuf[24];
    snprintf(spdBuf, sizeof(spdBuf), "[ %d %s ] > ",
             config.speedUnit == UNIT_MPH
                 ? config.maxSpeed
                 : (int)round(config.maxSpeed * 1.609f),
             config.speedUnit == UNIT_MPH ? "MPH" : "KM/H");
    spr.drawString(spdBuf, 300, 42, 2);

    // Row 2: Max RPM Scale Range (Y: 58, H: 28)
    spr.fillRoundRect(10, 58, 300, 28, 6, darkAccentColor);
    spr.drawRoundRect(10, 58, 300, 28, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" Max Tachometer Scale", 20, 72, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(primaryColor, darkAccentColor);
    char rpmBuf[24];
    snprintf(rpmBuf, sizeof(rpmBuf), "[ %d RPM ] > ", config.maxRpm);
    spr.drawString(rpmBuf, 300, 72, 2);

    // Row 3: RPM Redline Start (Y: 88, H: 28)
    spr.fillRoundRect(10, 88, 300, 28, 6, darkAccentColor);
    spr.drawRoundRect(10, 88, 300, 28, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" RPM Redline Start", 20, 102, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(UI_COLOR_RED, darkAccentColor);
    char redBuf[24];
    snprintf(redBuf, sizeof(redBuf), "[ %d RPM ] > ", config.redlineRpm);
    spr.drawString(redBuf, 300, 102, 2);

    // Row 4: Gauge Arc Preset (Y: 118, H: 28)
    spr.fillRoundRect(10, 118, 300, 28, 6, darkAccentColor);
    spr.drawRoundRect(10, 118, 300, 28, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" Gauge Arc Preset", 20, 132, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(primaryColor, darkAccentColor);
    const char *arcPresetName = "[ 270 deg Standard ] > ";
    if (config.gaugeStartAngle == -90 && config.gaugeEndAngle == 90) {
      arcPresetName = "[ 180 deg Semicircle ] > ";
    } else if (config.gaugeStartAngle == -120 && config.gaugeEndAngle == 120) {
      arcPresetName = "[ 240 deg Wide ] > ";
    } else if (config.gaugeStartAngle != -135 || config.gaugeEndAngle != 135) {
      arcPresetName = "[ Custom Arc ] > ";
    }
    spr.drawString(arcPresetName, 300, 132, 2);

    // Row 5: Custom Angle Limits (Y: 148, H: 28)
    spr.fillRoundRect(10, 148, 300, 28, 6, darkAccentColor);
    spr.drawRoundRect(10, 148, 300, 28, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" Custom Angles (L/R)", 20, 162, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(primaryColor, darkAccentColor);
    char angBuf[32];
    snprintf(angBuf, sizeof(angBuf), "[ %d deg / %d deg ] > ",
             config.gaugeStartAngle, config.gaugeEndAngle);
    spr.drawString(angBuf, 300, 162, 2);

    // Save & Exit Button (Y: 194, H: 40)
    spr.fillRoundRect(10, 194, 300, 40, 8, primaryColor);
    spr.setTextColor(UI_COLOR_BLACK, primaryColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("SAVE & EXIT", 160, 214, 4);

    spr.pushSprite(0, 0);
  }
}

// Settings Page 3 (Coolant & Ambient Temperature Scale Ranges) with Spacious
// Buttons
void GaugeUI::renderTempRangeConfigMenu(const SystemConfig &config) {
  updateThemeColors(config.colorTheme);

  if (useSprite) {
    spr.fillSprite(UI_COLOR_BLACK);

    // Header Title with Arrow Navigation (Page 3 of 3)
    spr.fillRect(0, 0, 320, 26, darkAccentColor);
    spr.drawFastHLine(0, 26, 320, primaryColor);

    spr.fillRoundRect(4, 2, 24, 22, 4, darkAccentColor);
    spr.drawRoundRect(4, 2, 24, 22, 4, primaryColor);
    spr.fillTriangle(18, 8, 10, 13, 18, 18, primaryColor);

    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("TEMP RANGES (PAGE 3 / 3)", 160, 13, 2);

    spr.fillRoundRect(292, 2, 24, 22, 4, darkAccentColor);
    spr.drawRoundRect(292, 2, 24, 22, 4, primaryColor);
    spr.fillTriangle(298, 8, 306, 13, 298, 18, primaryColor);

    // Row 1: Coolant Temp Min Scale (Y: 30, H: 30)
    spr.fillRoundRect(10, 30, 300, 30, 6, darkAccentColor);
    spr.drawRoundRect(10, 30, 300, 30, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" Coolant Temp Min Scale", 20, 45, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(primaryColor, darkAccentColor);
    char cMinBuf[24];
    snprintf(cMinBuf, sizeof(cMinBuf), "[ %d deg C ] > ",
             config.minCoolantTemp);
    spr.drawString(cMinBuf, 300, 45, 2);

    // Row 2: Coolant Temp Max Scale (Y: 62, H: 30)
    spr.fillRoundRect(10, 62, 300, 30, 6, darkAccentColor);
    spr.drawRoundRect(10, 62, 300, 30, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" Coolant Temp Max Scale", 20, 77, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(primaryColor, darkAccentColor);
    char cMaxBuf[24];
    snprintf(cMaxBuf, sizeof(cMaxBuf), "[ %d deg C ] > ",
             config.maxCoolantTemp);
    spr.drawString(cMaxBuf, 300, 77, 2);

    // Row 3: Cold Engine Region Limit (Y: 94, H: 30)
    spr.fillRoundRect(10, 94, 300, 30, 6, darkAccentColor);
    spr.drawRoundRect(10, 94, 300, 30, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" Cold Region Limit", 20, 109, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(UI_COLOR_BLUE, darkAccentColor);
    char coldBuf[24];
    snprintf(coldBuf, sizeof(coldBuf), "[ %d deg C ] > ",
             config.coldTempThreshold);
    spr.drawString(coldBuf, 300, 109, 2);

    // Row 4: High Temp Overheat Alarm (Y: 126, H: 30)
    spr.fillRoundRect(10, 126, 300, 30, 6, darkAccentColor);
    spr.drawRoundRect(10, 126, 300, 30, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(ML_DATUM);
    spr.drawString(" Overheat Alarm Threshold", 20, 141, 2);
    spr.setTextDatum(MR_DATUM);
    spr.setTextColor(UI_COLOR_RED, darkAccentColor);
    char hotBuf[24];
    snprintf(hotBuf, sizeof(hotBuf), "[ %d deg C ] > ",
             config.highTempThreshold);
    spr.drawString(hotBuf, 300, 141, 2);

    // Row 5: Reset Ranges to Factory Defaults (Y: 158, H: 30)
    spr.fillRoundRect(10, 158, 300, 30, 6, darkAccentColor);
    spr.drawRoundRect(10, 158, 300, 30, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("RESET TEMP DEFAULTS", 160, 173, 2);

    // Save & Exit Button (Y: 194, H: 40)
    spr.fillRoundRect(10, 194, 300, 40, 8, primaryColor);
    spr.setTextColor(UI_COLOR_BLACK, primaryColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawString("SAVE & EXIT", 160, 214, 4);

    spr.pushSprite(0, 0);
  }
}

// Open On-Screen Touch Numpad Popup
void GaugeUI::openNumpad(NumpadTarget target, int initialVal) {
  numpadTarget = target;
  snprintf(numpadBuffer, sizeof(numpadBuffer), "%d", initialVal);
  numpadLen = strlen(numpadBuffer);
}

// Render Interactive On-Screen Touch Numpad
void GaugeUI::renderNumpad(const SystemConfig &config) {
  updateThemeColors(config.colorTheme);

  if (useSprite) {
    spr.fillSprite(UI_COLOR_BLACK);

    // Title Header
    spr.fillRect(0, 0, 320, 24, darkAccentColor);
    spr.drawFastHLine(0, 24, 320, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(MC_DATUM);

    const char *titles[] = {"ENTER MAX SPEED SCALE",
                            "ENTER MAX TACHOMETER SCALE (RPM)",
                            "ENTER RPM REDLINE START LEVEL",
                            "ENTER HIGH TEMP OVERHEAT ALARM (C)",
                            "ENTER COOLANT MIN TEMP SCALE (C)",
                            "ENTER COOLANT MAX TEMP SCALE (C)",
                            "ENTER COLD ENGINE REGION LIMIT (C)",
                            "ENTER INTAKE/AMBIENT MIN TEMP (C)",
                            "ENTER INTAKE/AMBIENT MAX TEMP (C)",
                            "ENTER GAUGE START ANGLE (DEG)",
                            "ENTER GAUGE END ANGLE (DEG)"};
    spr.drawString(titles[numpadTarget % 11], 160, 12, 2);

    // Typed Value Display Box (Y: 28, H: 34)
    spr.fillRoundRect(10, 28, 300, 34, 6, darkAccentColor);
    spr.drawRoundRect(10, 28, 300, 34, 6, primaryColor);
    spr.setTextColor(UI_COLOR_WHITE, darkAccentColor);
    spr.setTextDatum(MC_DATUM);
    spr.drawNumber(numpadLen > 0 ? atoi(numpadBuffer) : 0, 160, 45, 6);

    // Numpad Keys Grid (Y: 66..236)
    const char *keyLabels[4][3] = {
        {"1", "2", "3"}, {"4", "5", "6"}, {"7", "8", "9"}, {"CLR", "0", "OK"}};

    for (int row = 0; row < 4; row++) {
      for (int col = 0; col < 3; col++) {
        int kx = 10 + (col * 102);
        int ky = 66 + (row * 42);
        uint16_t btnBg =
            (row == 3 && col == 2) ? primaryColor : darkAccentColor;
        uint16_t textCol =
            (row == 3 && col == 2) ? UI_COLOR_BLACK : UI_COLOR_WHITE;

        spr.fillRoundRect(kx, ky, 96, 38, 6, btnBg);
        spr.drawRoundRect(kx, ky, 96, 38, 6, primaryColor);
        spr.setTextColor(textCol, btnBg);
        spr.setTextDatum(MC_DATUM);
        spr.drawString(keyLabels[row][col], kx + 48, ky + 19, 4);
      }
    }

    spr.pushSprite(0, 0);
  }
}

UIState GaugeUI::handleTouchInGaugeState(const TouchPoint &tp,
                                         SystemConfig &config,
                                         ConfigManager &configMgr) {
  if (!tp.isPressed)
    return STATE_GAUGE_CLUSTER;

  // Compact Settings Icon Target in Top Right (X >= 270, Y <= 36)
  if (tp.x >= 270 && tp.y <= 36) {
    Serial.println(
        ">>> SETTINGS ICON CLICKED! Switching to Configuration Screen.");
    return STATE_SETTINGS_MENU;
  }

  // Top Header Left Arrow Button (<) (X: 75..110, Y <= 36)
  if (tp.x >= 75 && tp.x <= 110 && tp.y <= 36) {
    config.gaugeLayout = (config.gaugeLayout + 5) % 6;
    configMgr.save();
    Serial.print(">>> Header Left Arrow Clicked! Switched to Gauge Mode: ");
    Serial.println(config.gaugeLayout);
    return STATE_GAUGE_CLUSTER;
  }

  // Top Header Right Arrow Button (>) (X: 210..245, Y <= 36)
  if (tp.x >= 210 && tp.x <= 245 && tp.y <= 36) {
    config.gaugeLayout = (config.gaugeLayout + 1) % 6;
    configMgr.save();
    Serial.print(">>> Header Right Arrow Clicked! Switched to Gauge Mode: ");
    Serial.println(config.gaugeLayout);
    return STATE_GAUGE_CLUSTER;
  }

  return STATE_GAUGE_CLUSTER;
}

UIState GaugeUI::handleTouchInSettingsState(const TouchPoint &tp,
                                            SystemConfig &config,
                                            ConfigManager &configMgr,
                                            OBD2Manager &obd2) {
  if (!tp.isPressed)
    return STATE_SETTINGS_MENU;

  if (tp.y <= 26) {
    if (tp.x <= 40)
      return STATE_TEMP_RANGE_CONFIG;
    if (tp.x >= 280)
      return STATE_RANGE_CONFIG;
  }

  if (tp.y >= 28 && tp.y < 58) {
    config.speedUnit = (config.speedUnit == UNIT_MPH) ? UNIT_KMH : UNIT_MPH;
    Serial.println(">>> Option Clicked: Speed Unit");
  } else if (tp.y >= 60 && tp.y < 90) {
    config.tempUnit =
        (config.tempUnit == UNIT_CELSIUS) ? UNIT_FAHRENHEIT : UNIT_CELSIUS;
    Serial.println(">>> Option Clicked: Temp Unit");
  } else if (tp.y >= 92 && tp.y < 122) {
    config.primaryMetricIndex = (config.primaryMetricIndex + 1) % 7;
    Serial.print(">>> Option Clicked: Primary Central Gauge Changed to ");
    Serial.println(config.primaryMetricIndex);
  } else if (tp.y >= 124 && tp.y < 154) {
    Serial.println(">>> Entering Telemetry Metric Selection Menu...");
    return STATE_METRIC_CONFIG;
  } else if (tp.y >= 156 && tp.y < 186) {
    if (tp.x < 160) {
      config.demoMode = !config.demoMode;
      if (!config.demoMode)
        obd2.resetSearchTimeout();
      Serial.println(">>> Option Clicked: OBD Mode");
    } else {
      config.colorTheme = (config.colorTheme + 1) % 4;
      Serial.println(">>> Option Clicked: Color Theme");
    }
  } else if (tp.y >= 190) {
    configMgr.save();
    Serial.println(">>> Settings Saved to NVS! Returning to Gauge Cluster...");
    return STATE_GAUGE_CLUSTER;
  }

  return STATE_SETTINGS_MENU;
}

UIState GaugeUI::handleTouchInRangeConfigState(const TouchPoint &tp,
                                               SystemConfig &config,
                                               ConfigManager &configMgr) {
  if (!tp.isPressed)
    return STATE_RANGE_CONFIG;

  if (tp.y <= 26) {
    if (tp.x <= 40)
      return STATE_SETTINGS_MENU;
    if (tp.x >= 280)
      return STATE_TEMP_RANGE_CONFIG;
  }

  if (tp.y >= 28 && tp.y < 56) {
    openNumpad(NUMPAD_TARGET_MAX_SPEED, config.maxSpeed);
    return STATE_NUMPAD_INPUT;
  } else if (tp.y >= 58 && tp.y < 86) {
    openNumpad(NUMPAD_TARGET_MAX_RPM, config.maxRpm);
    return STATE_NUMPAD_INPUT;
  } else if (tp.y >= 88 && tp.y < 116) {
    openNumpad(NUMPAD_TARGET_REDLINE_RPM, config.redlineRpm);
    return STATE_NUMPAD_INPUT;
  } else if (tp.y >= 118 && tp.y < 146) {
    if (config.gaugeStartAngle == -90 && config.gaugeEndAngle == 90) {
      config.gaugeStartAngle = -135;
      config.gaugeEndAngle = 135;
    } else if (config.gaugeStartAngle == -135 && config.gaugeEndAngle == 135) {
      config.gaugeStartAngle = -120;
      config.gaugeEndAngle = 120;
    } else {
      config.gaugeStartAngle = -90;
      config.gaugeEndAngle = 90;
    }
    configMgr.save();
    Serial.println(">>> Option Clicked: Gauge Arc Preset Changed");
  } else if (tp.y >= 148 && tp.y < 176) {
    if (tp.x < 160) {
      openNumpad(NUMPAD_TARGET_GAUGE_START_ANG, abs(config.gaugeStartAngle));
    } else {
      openNumpad(NUMPAD_TARGET_GAUGE_END_ANG, config.gaugeEndAngle);
    }
    return STATE_NUMPAD_INPUT;
  } else if (tp.y >= 184) {
    configMgr.save();
    Serial.println(">>> Settings Saved to NVS! Returning to Gauge Cluster...");
    return STATE_GAUGE_CLUSTER;
  }

  return STATE_RANGE_CONFIG;
}

UIState GaugeUI::handleTouchInTempRangeConfigState(const TouchPoint &tp,
                                                   SystemConfig &config,
                                                   ConfigManager &configMgr) {
  if (!tp.isPressed)
    return STATE_TEMP_RANGE_CONFIG;

  if (tp.y <= 26) {
    if (tp.x <= 40)
      return STATE_RANGE_CONFIG;
    if (tp.x >= 280)
      return STATE_SETTINGS_MENU;
  }

  if (tp.y >= 30 && tp.y < 60) {
    openNumpad(NUMPAD_TARGET_MIN_COOLANT, config.minCoolantTemp);
    return STATE_NUMPAD_INPUT;
  } else if (tp.y >= 62 && tp.y < 92) {
    openNumpad(NUMPAD_TARGET_MAX_COOLANT, config.maxCoolantTemp);
    return STATE_NUMPAD_INPUT;
  } else if (tp.y >= 94 && tp.y < 124) {
    openNumpad(NUMPAD_TARGET_COLD_TEMP, config.coldTempThreshold);
    return STATE_NUMPAD_INPUT;
  } else if (tp.y >= 126 && tp.y < 156) {
    openNumpad(NUMPAD_TARGET_HIGH_TEMP, config.highTempThreshold);
    return STATE_NUMPAD_INPUT;
  } else if (tp.y >= 158 && tp.y < 188) {
    config.minCoolantTemp = 40;
    config.maxCoolantTemp = 120;
    config.coldTempThreshold = 65;
    config.highTempThreshold = 105;
    config.minIntakeTemp = 0;
    config.maxIntakeTemp = 80;
    configMgr.save();
    Serial.println(">>> Temperature Ranges Reset to Factory Defaults!");
  } else if (tp.y >= 190) {
    configMgr.save();
    Serial.println(">>> Settings Saved to NVS! Returning to Gauge Cluster...");
    return STATE_GAUGE_CLUSTER;
  }

  return STATE_TEMP_RANGE_CONFIG;
}

UIState GaugeUI::handleTouchInMetricConfigState(const TouchPoint &tp,
                                                SystemConfig &config,
                                                ConfigManager &configMgr) {
  if (!tp.isPressed)
    return STATE_METRIC_CONFIG;

  for (uint8_t i = 0; i < 6; i++) {
    int minY = 28 + (i * 27);
    int maxY = minY + 25;
    if (tp.y >= minY && tp.y < maxY) {
      config.toggleMetric(i);
      Serial.print(">>> Toggled Telemetry Metric Bit: ");
      Serial.println(i);
      return STATE_METRIC_CONFIG;
    }
  }

  if (tp.y >= 190) {
    configMgr.save();
    Serial.println(">>> Metrics Saved to NVS! Returning to Settings Menu...");
    return STATE_SETTINGS_MENU;
  }

  return STATE_METRIC_CONFIG;
}

UIState GaugeUI::handleTouchInNumpadState(const TouchPoint &tp,
                                          SystemConfig &config,
                                          ConfigManager &configMgr) {
  if (!tp.isPressed)
    return STATE_NUMPAD_INPUT;

  int targetCol = -1;
  int targetRow = -1;

  for (int col = 0; col < 3; col++) {
    int kx = 10 + (col * 102);
    if (tp.x >= (kx - 4) && tp.x <= (kx + 96 + 4)) {
      targetCol = col;
      break;
    }
  }

  for (int row = 0; row < 4; row++) {
    int ky = 66 + (row * 42);
    if (tp.y >= (ky - 3) && tp.y <= (ky + 38 + 3)) {
      targetRow = row;
      break;
    }
  }

  if (targetCol != -1 && targetRow != -1) {
    if (targetRow < 3) {
      int digit = (targetRow * 3) + targetCol + 1;
      if (numpadLen < 5) {
        numpadBuffer[numpadLen++] = '0' + digit;
        numpadBuffer[numpadLen] = '\0';
      }
    } else {
      if (targetCol == 0) {
        numpadBuffer[0] = '\0';
        numpadLen = 0;
      } else if (targetCol == 1) {
        if (numpadLen < 5) {
          if (numpadLen > 0) {
            numpadBuffer[numpadLen++] = '0';
            numpadBuffer[numpadLen] = '\0';
          } else {
            numpadBuffer[0] = '0';
            numpadBuffer[1] = '\0';
            numpadLen = 1;
          }
        }
      } else if (targetCol == 2) {
        int val = atoi(numpadBuffer);
        UIState returnPage = STATE_RANGE_CONFIG;

        switch (numpadTarget) {
        case NUMPAD_TARGET_MAX_SPEED:
          config.maxSpeed = constrain(val, 40, 300);
          returnPage = STATE_RANGE_CONFIG;
          break;
        case NUMPAD_TARGET_MAX_RPM:
          config.maxRpm = constrain((val / 500) * 500, 3000, 12000);
          if (config.redlineRpm >= config.maxRpm)
            config.redlineRpm = config.maxRpm - 1000;
          returnPage = STATE_RANGE_CONFIG;
          break;
        case NUMPAD_TARGET_REDLINE_RPM:
          config.redlineRpm = constrain(val, 1000, config.maxRpm);
          returnPage = STATE_RANGE_CONFIG;
          break;
        case NUMPAD_TARGET_MIN_COOLANT:
          config.minCoolantTemp = constrain(val, 0, config.maxCoolantTemp - 10);
          returnPage = STATE_TEMP_RANGE_CONFIG;
          break;
        case NUMPAD_TARGET_MAX_COOLANT:
          config.maxCoolantTemp =
              constrain(val, config.minCoolantTemp + 10, 160);
          returnPage = STATE_TEMP_RANGE_CONFIG;
          break;
        case NUMPAD_TARGET_COLD_TEMP:
          config.coldTempThreshold = constrain(val, config.minCoolantTemp + 5,
                                               config.highTempThreshold - 5);
          returnPage = STATE_TEMP_RANGE_CONFIG;
          break;
        case NUMPAD_TARGET_HIGH_TEMP:
          config.highTempThreshold = constrain(
              val, config.coldTempThreshold + 5, config.maxCoolantTemp);
          returnPage = STATE_TEMP_RANGE_CONFIG;
          break;
        case NUMPAD_TARGET_MIN_INTAKE:
          config.minIntakeTemp = constrain(val, -30, config.maxIntakeTemp - 10);
          returnPage = STATE_TEMP_RANGE_CONFIG;
          break;
        case NUMPAD_TARGET_MAX_INTAKE:
          config.maxIntakeTemp = constrain(val, config.minIntakeTemp + 10, 130);
          returnPage = STATE_TEMP_RANGE_CONFIG;
          break;
        case NUMPAD_TARGET_GAUGE_START_ANG:
          config.gaugeStartAngle = -constrain(val, 0, 180);
          if (config.gaugeStartAngle == 0)
            config.gaugeStartAngle = -90;
          returnPage = STATE_RANGE_CONFIG;
          break;
        case NUMPAD_TARGET_GAUGE_END_ANG:
          config.gaugeEndAngle = constrain(val, 0, 180);
          if (config.gaugeEndAngle == 0)
            config.gaugeEndAngle = 90;
          returnPage = STATE_RANGE_CONFIG;
          break;
        }
        configMgr.save();
        Serial.println(">>> Numpad Input Confirmed & Saved to NVS!");
        return returnPage;
      }
    }
  }

  return STATE_NUMPAD_INPUT;
}
