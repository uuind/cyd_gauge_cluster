#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

enum SpeedUnit {
    UNIT_MPH = 0,
    UNIT_KMH = 1
};

enum TempUnit {
    UNIT_CELSIUS = 0,
    UNIT_FAHRENHEIT = 1
};

enum ColorTheme {
    THEME_CYAN = 0,
    THEME_RED = 1,
    THEME_AMBER = 2,
    THEME_GREEN = 3
};

enum GaugeLayout {
    LAYOUT_ALL_ANALOG = 0,  // All Active Metrics as Sweeping Analog Dials
    LAYOUT_ALL_DIGITAL = 1, // All Active Metrics as Digital Level Bar Sweeps
    LAYOUT_ALL_NUMBERS = 2, // All Active Metrics as Pure Bold Number Cards
    LAYOUT_FOCUS_CENTER = 3, // Focused Central Dial + Peripheral Side Gauges
    LAYOUT_SINGLE_GAUGE = 4, // Single Primary Gauge Only
    LAYOUT_HYBRID_BARS = 5   // C4 Corvette Hybrid (Segmented Bargraphs + Big Digital)
};

// OBD2 Metric Bitmask Indices
enum MetricBitIndex {
    METRIC_BIT_SPEED = 0,     // PID 0x0D: Vehicle Speed
    METRIC_BIT_COOLANT = 1,   // PID 0x05: Coolant Temp
    METRIC_BIT_RPM = 2,       // PID 0x0C: Engine RPM
    METRIC_BIT_THROTTLE = 3,  // PID 0x11: Throttle Position (0-100%)
    METRIC_BIT_INTAKE = 4,    // PID 0x0F: Intake Air Temp
    METRIC_BIT_LOAD = 5,      // PID 0x04: Engine Load (0-100%)
    METRIC_BIT_BATTERY = 6    // Battery Voltage (10-15V)
};

struct SystemConfig {
    uint8_t speedUnit;          // 0: MPH, 1: KM/H
    uint8_t tempUnit;           // 0: Celsius, 1: Fahrenheit
    uint8_t colorTheme;         // 0: Cyan, 1: Red, 2: Amber, 3: Green
    uint8_t gaugeLayout;        // 0: All Analog, 1: All Digital, 2: All Numbers, 3: Focus Center, 4: Single Gauge
    uint8_t primaryMetricIndex; // 0..6: Metric selected as central focal gauge
    uint8_t metricsBitmask;     // Active telemetry PIDs bitmask
    bool demoMode;              // true: Demo simulation mode, false: OBD2 hardware
    
    // Custom Gauge Limits & Warning Thresholds
    int16_t maxSpeed;           // Maximum Speedometer Dial Scale (e.g. 120 MPH / 200 KM/H)
    int16_t maxRpm;             // Maximum Tachometer Scale (e.g. 6000 / 8000 RPM)
    int16_t redlineRpm;          // RPM Redline Warning Start Level (e.g. 5000 RPM)
    int16_t gaugeStartAngle;    // Analog Gauge Start Angle (e.g. -135 deg standard, -90 deg semicircle)
    int16_t gaugeEndAngle;      // Analog Gauge End Angle (e.g. 135 deg standard, 90 deg semicircle)
    
    // Temperature Scale Ranges & Configurable Cold/Hot Regions
    int16_t minCoolantTemp;     // Minimum Coolant Temp Scale (e.g. 40 C)
    int16_t maxCoolantTemp;     // Maximum Coolant Temp Scale (e.g. 120 C)
    int16_t coldTempThreshold;  // Cold Engine Warning Region Boundary (e.g. 65 C)
    int16_t highTempThreshold;  // Overheat Warning Region Boundary in Celsius (e.g. 105 C)

    int16_t minIntakeTemp;      // Minimum Ambient/Intake Air Temp Scale (e.g. 0 C)
    int16_t maxIntakeTemp;      // Maximum Ambient/Intake Air Temp Scale (e.g. 80 C)

    bool isMetricEnabled(uint8_t bitIndex) const {
        return (metricsBitmask & (1 << bitIndex)) != 0;
    }

    void toggleMetric(uint8_t bitIndex) {
        metricsBitmask ^= (1 << bitIndex);
        if (metricsBitmask == 0) {
            metricsBitmask = (1 << METRIC_BIT_SPEED);
        }
    }

    uint8_t countActiveMetrics() const {
        uint8_t count = 0;
        for (uint8_t i = 0; i < 7; i++) {
            if (isMetricEnabled(i)) count++;
        }
        return (count == 0) ? 1 : count;
    }
};

class ConfigManager {
private:
    Preferences preferences;
    const char* NAMESPACE = "gauge_cfg";

public:
    SystemConfig config;

    ConfigManager() {
        // Default Configuration
        config.speedUnit = UNIT_MPH;
        config.tempUnit = UNIT_FAHRENHEIT;
        config.colorTheme = THEME_CYAN;
        config.gaugeLayout = LAYOUT_FOCUS_CENTER;
        config.primaryMetricIndex = METRIC_BIT_SPEED;
        config.metricsBitmask = (1 << METRIC_BIT_SPEED) | (1 << METRIC_BIT_COOLANT) | (1 << METRIC_BIT_RPM);
        config.demoMode = true;
        config.maxSpeed = 120;
        config.maxRpm = 6000;
        config.redlineRpm = 5000;
        config.gaugeStartAngle = -135;
        config.gaugeEndAngle = 135;
        config.minCoolantTemp = 40;
        config.maxCoolantTemp = 120;
        config.coldTempThreshold = 65;
        config.highTempThreshold = 105;
        config.minIntakeTemp = 0;
        config.maxIntakeTemp = 80;
    }

    void begin() {
        preferences.begin(NAMESPACE, false);
        config.speedUnit = preferences.getUChar("spdUnit", config.speedUnit);
        config.tempUnit = preferences.getUChar("tmpUnit", config.tempUnit);
        config.colorTheme = preferences.getUChar("theme", config.colorTheme);
        config.gaugeLayout = preferences.getUChar("layout", config.gaugeLayout);
        config.primaryMetricIndex = preferences.getUChar("primMetric", config.primaryMetricIndex);
        config.metricsBitmask = preferences.getUChar("metrics", config.metricsBitmask);
        config.demoMode = preferences.getBool("demo", config.demoMode);
        config.maxSpeed = preferences.getShort("maxSpd", config.maxSpeed);
        config.maxRpm = preferences.getShort("maxRpm", config.maxRpm);
        config.redlineRpm = preferences.getShort("redline", config.redlineRpm);
        config.gaugeStartAngle = preferences.getShort("angStart", config.gaugeStartAngle);
        config.gaugeEndAngle = preferences.getShort("angEnd", config.gaugeEndAngle);
        config.minCoolantTemp = preferences.getShort("minCool", config.minCoolantTemp);
        config.maxCoolantTemp = preferences.getShort("maxCool", config.maxCoolantTemp);
        config.coldTempThreshold = preferences.getShort("coldTmp", config.coldTempThreshold);
        config.highTempThreshold = preferences.getShort("highTmp", config.highTempThreshold);
        config.minIntakeTemp = preferences.getShort("minInT", config.minIntakeTemp);
        config.maxIntakeTemp = preferences.getShort("maxInT", config.maxIntakeTemp);
        preferences.end();

        if (config.metricsBitmask == 0) {
            config.metricsBitmask = (1 << METRIC_BIT_SPEED) | (1 << METRIC_BIT_COOLANT) | (1 << METRIC_BIT_RPM);
        }
        if (config.gaugeLayout > 5) {
            config.gaugeLayout = LAYOUT_FOCUS_CENTER;
        }
        if (config.primaryMetricIndex > 6) {
            config.primaryMetricIndex = METRIC_BIT_SPEED;
        }
        if (config.maxSpeed < 40 || config.maxSpeed > 300) config.maxSpeed = 120;
        if (config.maxRpm < 3000 || config.maxRpm > 12000) config.maxRpm = 6000;
        if (config.redlineRpm < 2000 || config.redlineRpm > config.maxRpm) config.redlineRpm = 5000;
        if (config.gaugeStartAngle < -180 || config.gaugeStartAngle > 0) config.gaugeStartAngle = -135;
        if (config.gaugeEndAngle < 0 || config.gaugeEndAngle > 180) config.gaugeEndAngle = 135;
        if (config.gaugeEndAngle <= config.gaugeStartAngle) {
            config.gaugeStartAngle = -135;
            config.gaugeEndAngle = 135;
        }
        if (config.minCoolantTemp < 0 || config.minCoolantTemp >= config.maxCoolantTemp) config.minCoolantTemp = 40;
        if (config.maxCoolantTemp <= config.minCoolantTemp || config.maxCoolantTemp > 150) config.maxCoolantTemp = 120;
        if (config.coldTempThreshold <= config.minCoolantTemp || config.coldTempThreshold >= config.highTempThreshold) config.coldTempThreshold = 65;
        if (config.highTempThreshold <= config.coldTempThreshold || config.highTempThreshold > config.maxCoolantTemp) config.highTempThreshold = 105;
        if (config.minIntakeTemp < -30 || config.minIntakeTemp >= config.maxIntakeTemp) config.minIntakeTemp = 0;
        if (config.maxIntakeTemp <= config.minIntakeTemp || config.maxIntakeTemp > 120) config.maxIntakeTemp = 80;
    }

    void save() {
        preferences.begin(NAMESPACE, false);
        preferences.putUChar("spdUnit", config.speedUnit);
        preferences.putUChar("tmpUnit", config.tempUnit);
        preferences.putUChar("theme", config.colorTheme);
        preferences.putUChar("layout", config.gaugeLayout);
        preferences.putUChar("primMetric", config.primaryMetricIndex);
        preferences.putUChar("metrics", config.metricsBitmask);
        preferences.putBool("demo", config.demoMode);
        preferences.putShort("maxSpd", config.maxSpeed);
        preferences.putShort("maxRpm", config.maxRpm);
        preferences.putShort("redline", config.redlineRpm);
        preferences.putShort("angStart", config.gaugeStartAngle);
        preferences.putShort("angEnd", config.gaugeEndAngle);
        preferences.putShort("minCool", config.minCoolantTemp);
        preferences.putShort("maxCool", config.maxCoolantTemp);
        preferences.putShort("coldTmp", config.coldTempThreshold);
        preferences.putShort("highTmp", config.highTempThreshold);
        preferences.putShort("minInT", config.minIntakeTemp);
        preferences.putShort("maxInT", config.maxIntakeTemp);
        preferences.end();
    }

    void resetToDefaults() {
        config.speedUnit = UNIT_MPH;
        config.tempUnit = UNIT_FAHRENHEIT;
        config.colorTheme = THEME_CYAN;
        config.gaugeLayout = LAYOUT_FOCUS_CENTER;
        config.primaryMetricIndex = METRIC_BIT_SPEED;
        config.metricsBitmask = (1 << METRIC_BIT_SPEED) | (1 << METRIC_BIT_COOLANT) | (1 << METRIC_BIT_RPM);
        config.demoMode = true;
        config.maxSpeed = 120;
        config.maxRpm = 6000;
        config.redlineRpm = 5000;
        config.gaugeStartAngle = -135;
        config.gaugeEndAngle = 135;
        config.minCoolantTemp = 40;
        config.maxCoolantTemp = 120;
        config.coldTempThreshold = 65;
        config.highTempThreshold = 105;
        config.minIntakeTemp = 0;
        config.maxIntakeTemp = 80;
        save();
    }
};

#endif // CONFIG_MANAGER_H
