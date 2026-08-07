#ifndef OBD2_MANAGER_H
#define OBD2_MANAGER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "OBD2_KLine.h"

struct VehicleData {
    int speedKmh;       // PID 0x0D: Vehicle speed in km/h
    int coolantTempC;   // PID 0x05: Coolant temperature in °C
    int rpm;            // PID 0x0C: Engine RPM
    int throttlePct;    // PID 0x11: Throttle Position (0-100%)
    int intakeTempC;    // PID 0x0F: Intake Air Temperature in °C
    int engineLoadPct;  // PID 0x04: Engine Load (0-100%)
    float batteryVolts; // Battery Voltage (10.0-15.0V)
    bool isConnected;   // K-Line OBD2 connection status
    bool searchTimedOut;// True if 10-second connection search timed out

    // Continuous Low-Pass Filter Damped Values for Fluid Displays
    float smoothedSpeedKmh;
    float smoothedCoolantTempC;
    float smoothedRpm;
    float smoothedThrottlePct;
    float smoothedIntakeTempC;
    float smoothedEngineLoadPct;
};

class OBD2Manager {
private:
    OBD2_KLine kline;
    uint8_t rxPin;
    uint8_t txPin;
    bool isInitialized;
    uint8_t queryIndex;
    unsigned long lastQueryTime;
    
    // FreeRTOS Core 0 Task & Mutex Data Sync
    TaskHandle_t obdTaskHandle;
    SemaphoreHandle_t dataMutex;
    VehicleData latestData;
    bool currentDemoMode;
    uint8_t currentMetricsBitmask;

    // 10-second OBD Connection Timeout parameters
    unsigned long searchStartTime;
    unsigned long lastInitAttemptTime;
    bool searchTimedOut;
    const unsigned long SEARCH_TIMEOUT_MS = 10000; // 10 seconds
    const unsigned long INIT_RETRY_INTERVAL_MS = 3000; // Paced 3s retry interval

    // Exponential Moving Average (EMA) Damping Filter State
    float filteredSpeed;
    float filteredRpm;
    float filteredTemp;
    float filteredThrottle;
    float filteredIntake;
    float filteredLoad;

    // Simulation / Demo state parameters
    unsigned long lastSimStepTime;
    float simSpeed;
    float simTemp;
    float simRpm;
    float simThrottle;
    float simIntakeTemp;
    float simLoad;
    float simBattery;
    bool simAccelerating;

    bool initISO9141();
    int readPIDValue(uint8_t pid);

    static void obdTaskLoop(void* param);
    void runTaskLoop();

public:
    OBD2Manager(uint8_t rx = 35, uint8_t tx = 22);
    void begin();
    void resetSearchTimeout();
    void update(VehicleData &data, bool &demoMode, uint8_t metricsBitmask = 0xFF);
    void getVehicleData(VehicleData &outData);
    void setDemoMode(bool enable);
    void setMetricsBitmask(uint8_t bitmask);
    bool checkConnection();
};

#endif // OBD2_MANAGER_H
