/*
 * ============================================================================
 * ESP32 Digital Touchscreen Gauge Cluster for Cars (K-Line OBD2 & Demo Mode)
 * Replicates and expands functions from VamosGauge.ino
 * ============================================================================
 */

#include <Arduino.h>
#include "ConfigManager.h"
#include "OBD2Manager.h"
#include "TouchManager.h"
#include "GaugeUI.h"

// Hardware Pin Configuration for ESP32 K-Line OBD2 & Touchscreen (Serial1: RX=GPIO 35, TX=GPIO 22)
#define KLINE_RX_PIN 35 // RX (GPIO 35)
#define KLINE_TX_PIN 22 // TX (GPIO 22)
#define TOUCH_CS_PIN 33
#define TOUCH_IRQ_PIN 36

// Subsystem Instances
ConfigManager configMgr;
OBD2Manager   obd2(KLINE_RX_PIN, KLINE_TX_PIN);
TouchManager  touch(TOUCH_CS_PIN, TOUCH_IRQ_PIN);
GaugeUI       ui;

// System State & Data Structures
VehicleData vehicleData;
UIState     currentState = STATE_GAUGE_CLUSTER;
UIState     prevState = STATE_SETTINGS_MENU; // Force initial gauge render

unsigned long lastUpdate = 0;
const unsigned long REFRESH_INTERVAL_MS = 200; // 5Hz Dashboard Refresh Rate

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n--------------------------------------------------");
    Serial.println(" ESP32 Digital Touchscreen Gauge Cluster Initializing ");
    Serial.println(" Replicating & Upgrading VamosGauge.ino ");
    Serial.println("--------------------------------------------------");

    // 1. Load configuration from Non-Volatile Storage (Preferences / NVS)
    configMgr.begin();
    Serial.println("[Config] Settings loaded from ESP32 NVS.");

    // 2. Initialize OBD2 K-Line Subsystem
    obd2.begin();
    Serial.println("[OBD2] HardwareSerial (Serial1 K-Line ISO9141) Initialized on RX:35 TX:22.");

    // 3. Initialize Digital Display & UI Graphics Engine
    ui.begin();
    Serial.println("[UI] Digital Gauge UI Engine Initialized.");

    // 4. Initialize Touchscreen Input Subsystem (CYD HSPI Bus)
    touch.begin(ui.getTFTInstance());
    Serial.println("[Touch] CYD Touch Controller Initialized.");

    Serial.println("--------------------------------------------------");
    Serial.println(" Ready! Tap '<' or '>' arrows at top to switch modes.");
    Serial.println("--------------------------------------------------\n");
}

void loop() {
    // 1. Read Touchscreen Input
    TouchPoint tp = touch.getTouchPoint();

    // 2. Process Touches & State Transitions
    if (tp.isPressed) {
        if (currentState == STATE_GAUGE_CLUSTER) {
            UIState newState = ui.handleTouchInGaugeState(tp, configMgr.config, configMgr);
            if (newState != currentState) {
                currentState = newState;
                ui.renderSettingsMenu(configMgr.config);
                delay(300); // Debounce transition delay
            }
        } 
        else if (currentState == STATE_SETTINGS_MENU) {
            UIState newState = ui.handleTouchInSettingsState(tp, configMgr.config, configMgr, obd2);
            if (newState != currentState) {
                currentState = newState;
                if (currentState == STATE_METRIC_CONFIG) {
                    ui.renderMetricConfigMenu(configMgr.config);
                } else if (currentState == STATE_RANGE_CONFIG) {
                    ui.renderRangeConfigMenu(configMgr.config);
                } else if (currentState == STATE_TEMP_RANGE_CONFIG) {
                    ui.renderTempRangeConfigMenu(configMgr.config);
                } else if (currentState == STATE_GAUGE_CLUSTER) {
                    prevState = STATE_SETTINGS_MENU; // Force gauge redraw on exit
                }
                delay(300); // Debounce transition delay
            } else {
                ui.renderSettingsMenu(configMgr.config);
                delay(250); // Debounce menu option toggle
            }
        }
        else if (currentState == STATE_RANGE_CONFIG) {
            UIState newState = ui.handleTouchInRangeConfigState(tp, configMgr.config, configMgr);
            if (newState != currentState) {
                currentState = newState;
                if (currentState == STATE_SETTINGS_MENU) {
                    ui.renderSettingsMenu(configMgr.config);
                } else if (currentState == STATE_TEMP_RANGE_CONFIG) {
                    ui.renderTempRangeConfigMenu(configMgr.config);
                } else if (currentState == STATE_NUMPAD_INPUT) {
                    ui.renderNumpad(configMgr.config);
                } else if (currentState == STATE_GAUGE_CLUSTER) {
                    prevState = STATE_RANGE_CONFIG;
                }
                delay(300); // Debounce transition delay
            } else {
                ui.renderRangeConfigMenu(configMgr.config);
                delay(250); // Debounce range toggle
            }
        }
        else if (currentState == STATE_TEMP_RANGE_CONFIG) {
            UIState newState = ui.handleTouchInTempRangeConfigState(tp, configMgr.config, configMgr);
            if (newState != currentState) {
                currentState = newState;
                if (currentState == STATE_SETTINGS_MENU) {
                    ui.renderSettingsMenu(configMgr.config);
                } else if (currentState == STATE_RANGE_CONFIG) {
                    ui.renderRangeConfigMenu(configMgr.config);
                } else if (currentState == STATE_NUMPAD_INPUT) {
                    ui.renderNumpad(configMgr.config);
                } else if (currentState == STATE_GAUGE_CLUSTER) {
                    prevState = STATE_TEMP_RANGE_CONFIG;
                }
                delay(300); // Debounce transition delay
            } else {
                ui.renderTempRangeConfigMenu(configMgr.config);
                delay(250); // Debounce range toggle
            }
        }
        else if (currentState == STATE_METRIC_CONFIG) {
            UIState newState = ui.handleTouchInMetricConfigState(tp, configMgr.config, configMgr);
            if (newState != currentState) {
                currentState = newState;
                if (currentState == STATE_SETTINGS_MENU) {
                    ui.renderSettingsMenu(configMgr.config);
                }
                delay(300); // Debounce transition delay
            } else {
                ui.renderMetricConfigMenu(configMgr.config);
                delay(250); // Debounce toggle option
            }
        }
        else if (currentState == STATE_NUMPAD_INPUT) {
            UIState newState = ui.handleTouchInNumpadState(tp, configMgr.config, configMgr);
            if (newState != currentState) {
                currentState = newState;
                if (currentState == STATE_RANGE_CONFIG) {
                    ui.renderRangeConfigMenu(configMgr.config);
                } else if (currentState == STATE_TEMP_RANGE_CONFIG) {
                    ui.renderTempRangeConfigMenu(configMgr.config);
                }
                delay(300); // Debounce transition delay
            } else {
                ui.renderNumpad(configMgr.config);
                delay(200); // Debounce numpad keypress
            }
        }
    }

    // 3. Render/Update Dashboard UI in Gauge Cluster State
    if (currentState == STATE_GAUGE_CLUSTER) {
        if (millis() - lastUpdate >= REFRESH_INTERVAL_MS || currentState != prevState) {
            lastUpdate = millis();

            // Sync active config flags to Core 0 background task
            obd2.setDemoMode(configMgr.config.demoMode);
            obd2.setMetricsBitmask(configMgr.config.metricsBitmask);

            // Fetch latest thread-safe vehicle telemetry snapshot from Core 0 background task
            obd2.getVehicleData(vehicleData);

            // Render updated gauge cluster dashboard instantly at 60 FPS on Core 1
            bool forceRedraw = (currentState != prevState);
            ui.renderGaugeCluster(vehicleData, configMgr.config, forceRedraw);
            prevState = currentState;
        }
    }

    delay(15); // Loop pacing
}
