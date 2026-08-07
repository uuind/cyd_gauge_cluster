#ifndef GAUGE_UI_H
#define GAUGE_UI_H

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "ConfigManager.h"
#include "OBD2Manager.h"

// Forward declaration
struct TouchPoint;

// 16-bit RGB565 Palette Definitions for TFT_eSPI
#define UI_COLOR_BLACK       TFT_BLACK
#define UI_COLOR_NAVY        0x000F
#define UI_COLOR_DARK_GRAY   0x18C3
#define UI_COLOR_GRAY        0x39E7
#define UI_COLOR_LIGHT_GRAY  0x9D7C
#define UI_COLOR_WHITE       TFT_WHITE
#define UI_COLOR_RED         TFT_RED
#define UI_COLOR_BRIGHT_RED  0xF800
#define UI_COLOR_ORANGE      TFT_ORANGE
#define UI_COLOR_YELLOW      TFT_YELLOW
#define UI_COLOR_CYAN        TFT_CYAN
#define UI_COLOR_GREEN       TFT_GREEN
#define UI_COLOR_BLUE        TFT_BLUE

enum UIState {
    STATE_GAUGE_CLUSTER      = 0,
    STATE_SETTINGS_MENU      = 1, // Page 1 Settings (General & Telemetry)
    STATE_METRIC_CONFIG      = 2,
    STATE_RANGE_CONFIG       = 3, // Page 2 Settings (Speed & RPM Ranges)
    STATE_TEMP_RANGE_CONFIG  = 4, // Page 3 Settings (Temperature Ranges)
    STATE_NUMPAD_INPUT       = 5  // Touch Numpad Popup
};

enum NumpadTarget {
    NUMPAD_TARGET_MAX_SPEED       = 0,
    NUMPAD_TARGET_MAX_RPM         = 1,
    NUMPAD_TARGET_REDLINE_RPM     = 2,
    NUMPAD_TARGET_HIGH_TEMP       = 3,
    NUMPAD_TARGET_MIN_COOLANT     = 4,
    NUMPAD_TARGET_MAX_COOLANT     = 5,
    NUMPAD_TARGET_COLD_TEMP       = 6,
    NUMPAD_TARGET_MIN_INTAKE      = 7,
    NUMPAD_TARGET_MAX_INTAKE      = 8,
    NUMPAD_TARGET_GAUGE_START_ANG = 9,
    NUMPAD_TARGET_GAUGE_END_ANG   = 10
};

struct MetricInfo {
    uint8_t bitIndex;
    const char* label;
    const char* unitStr;
    float value;
    float minVal;
    float maxVal;
    uint16_t color;
};

class GaugeUI {
private:
    TFT_eSPI tft;
    TFT_eSprite spr; // Sprite for double-buffered rendering
    bool useSprite;

    uint16_t primaryColor;
    uint16_t darkAccentColor;
    uint16_t backgroundColor;
    
    // Cached values to prevent unnecessary redraws
    int prevSpeed;
    int prevTemp;
    int prevRpm;
    uint8_t prevTheme;
    uint8_t prevLayout;
    uint8_t prevPrimaryMetric;
    uint8_t prevMetricsBitmask;
    int16_t prevMaxSpeed;
    int16_t prevMaxRpm;
    int16_t prevGaugeStartAngle;
    int16_t prevGaugeEndAngle;
    bool prevConnected;
    bool prevDemo;
    bool alertState;
    unsigned long lastAlertFlash;

    // Numpad State Variables
    NumpadTarget numpadTarget;
    char numpadBuffer[10];
    uint8_t numpadLen;

    void updateThemeColors(uint8_t theme);
    void drawHeader(const VehicleData &data, const SystemConfig &config);
    void drawSettingsButton();

    // Adaptive Component Drawing Helpers with Major/Minor Ticks & Cold/Hot Regions
    void drawAnalogDialInRect(int x, int y, int w, int h, float val, float minV, float maxV, const char* label, const char* unitStr, uint16_t accentColor, bool showRedline = false, float redlineVal = 5000.0f, bool isCoolantGauge = false, float coldVal = 65.0f, float hotVal = 105.0f, float startAngleDeg = -135.0f, float endAngleDeg = 135.0f);
    void drawDigitalBoxInRect(int x, int y, int w, int h, float val, const char* label, const char* unitStr, uint16_t accentColor);
    void drawLevelBarInRect(int x, int y, int w, int h, float val, float minV, float maxV, const char* label, const char* unitStr, uint16_t accentColor, bool isCoolant = false, float coldVal = 65.0f, float hotVal = 105.0f);
    void drawSegmentedBarInRect(int x, int y, int w, int h, float val, float minV, float maxV, const char* label, const char* unitStr, uint16_t accentColor, bool showRedline = false, float redlineVal = 5000.0f, bool isCoolantGauge = false, float coldVal = 65.0f, float hotVal = 105.0f);
    void drawSegmentedArcGauge(int x, int y, int w, int h, float val, float minV, float maxV, const char* label, const char* unitStr, uint16_t accentColor, bool showRedline = false, float redlineVal = 5000.0f, bool isCoolantGauge = false, float coldVal = 65.0f, float hotVal = 105.0f);

    // Streamlined Layout Engine Renderers
    void renderDynamicCluster(const VehicleData &data, const SystemConfig &config);

public:
    GaugeUI();
    void begin();
    TFT_eSPI* getTFTInstance() { return &tft; }
    void renderGaugeCluster(const VehicleData &data, const SystemConfig &config, bool forceRedraw = false);
    
    // Multi-Page Arrow Navigational Settings System
    void renderSettingsMenu(const SystemConfig &config);      // Page 1
    void renderRangeConfigMenu(const SystemConfig &config);   // Page 2
    void renderTempRangeConfigMenu(const SystemConfig &config); // Page 3
    void renderMetricConfigMenu(const SystemConfig &config);

    // Numpad Popup Engine
    void openNumpad(NumpadTarget target, int initialVal);
    void renderNumpad(const SystemConfig &config);

    UIState handleTouchInGaugeState(const TouchPoint &tp, SystemConfig &config, ConfigManager &configMgr);
    UIState handleTouchInSettingsState(const TouchPoint &tp, SystemConfig &config, ConfigManager &configMgr, OBD2Manager &obd2);
    UIState handleTouchInRangeConfigState(const TouchPoint &tp, SystemConfig &config, ConfigManager &configMgr);
    UIState handleTouchInTempRangeConfigState(const TouchPoint &tp, SystemConfig &config, ConfigManager &configMgr);
    UIState handleTouchInMetricConfigState(const TouchPoint &tp, SystemConfig &config, ConfigManager &configMgr);
    UIState handleTouchInNumpadState(const TouchPoint &tp, SystemConfig &config, ConfigManager &configMgr);
};

#endif // GAUGE_UI_H
