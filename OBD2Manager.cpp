#include "OBD2Manager.h"

OBD2Manager::OBD2Manager(uint8_t rx, uint8_t tx) : 
    kline(Serial1, 10400, rx, tx), rxPin(rx), txPin(tx), isInitialized(false), queryIndex(0), lastQueryTime(0),
    obdTaskHandle(nullptr), dataMutex(nullptr), currentDemoMode(true), currentMetricsBitmask(0xFF),
    searchStartTime(0), lastInitAttemptTime(0), searchTimedOut(false),
    filteredSpeed(0.0f), filteredRpm(800.0f), filteredTemp(65.0f),
    filteredThrottle(15.0f), filteredIntake(25.0f), filteredLoad(20.0f),
    lastSimStepTime(0), simSpeed(0.0f), simTemp(65.0f), simRpm(800.0f), simThrottle(15.0f),
    simIntakeTemp(25.0f), simLoad(20.0f), simBattery(13.8f), simAccelerating(true) {}

void OBD2Manager::begin() {
    kline.setProtocol("ISO9141");
    kline.setByteWriteInterval(5);
    kline.setInterByteTimeout(60);
    kline.setReadTimeout(150); // Reduced read timeout to prevent blocking display loop
    resetSearchTimeout();

    if (dataMutex == nullptr) {
        dataMutex = xSemaphoreCreateMutex();
    }

    if (obdTaskHandle == nullptr) {
        xTaskCreatePinnedToCore(
            OBD2Manager::obdTaskLoop,
            "OBD2_Core0_Task",
            4096,
            this,
            1,
            &obdTaskHandle,
            0 // Dedicated to ESP32 Core 0!
        );
        Serial.println("[OBD2] Dedicated FreeRTOS background task launched on ESP32 Core 0.");
    }
}

void OBD2Manager::resetSearchTimeout() {
    searchStartTime = millis();
    lastInitAttemptTime = 0; // Trigger immediate 1st attempt
    searchTimedOut = false;
    isInitialized = false;
}

bool OBD2Manager::initISO9141() {
    if (kline.initOBD2()) {
        isInitialized = true;
        searchTimedOut = false;
        return true;
    }

    isInitialized = false;
    return false;
}

int OBD2Manager::readPIDValue(uint8_t pid) {
    if (!isInitialized) return -1;

    float val = kline.getLiveData(pid);
    if (val >= 0.0f || pid == 0x05 || pid == 0x0F) {
        return (int)round(val);
    }

    return -1;
}

bool OBD2Manager::checkConnection() {
    return isInitialized;
}

void OBD2Manager::obdTaskLoop(void* param) {
    OBD2Manager* manager = static_cast<OBD2Manager*>(param);
    manager->runTaskLoop();
}

void OBD2Manager::runTaskLoop() {
    while (true) {
        VehicleData tempFrame;
        bool demoModeCopy = currentDemoMode;
        uint8_t bitmaskCopy = currentMetricsBitmask;

        // Perform hardware OBD2 queries or simulation updates on Core 0
        update(tempFrame, demoModeCopy, bitmaskCopy);

        // Thread-safe update of latestData snapshot for Core 1 UI reader
        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            latestData = tempFrame;
            currentDemoMode = demoModeCopy;
            xSemaphoreGive(dataMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(15)); // Pacing for Core 0 loop
    }
}

void OBD2Manager::getVehicleData(VehicleData &outData) {
    if (dataMutex != nullptr && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        outData = latestData;
        xSemaphoreGive(dataMutex);
    } else {
        outData = latestData;
    }
}

void OBD2Manager::setDemoMode(bool enable) {
    if (dataMutex != nullptr && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        currentDemoMode = enable;
        if (!enable) resetSearchTimeout();
        xSemaphoreGive(dataMutex);
    } else {
        currentDemoMode = enable;
        if (!enable) resetSearchTimeout();
    }
}

void OBD2Manager::setMetricsBitmask(uint8_t bitmask) {
    if (dataMutex != nullptr && xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        currentMetricsBitmask = bitmask;
        xSemaphoreGive(dataMutex);
    } else {
        currentMetricsBitmask = bitmask;
    }
}

void OBD2Manager::update(VehicleData &data, bool &demoMode, uint8_t metricsBitmask) {
    data.searchTimedOut = searchTimedOut;

    if (demoMode) {
        if (millis() - lastSimStepTime >= 80) {
            lastSimStepTime = millis();
            if (simAccelerating) {
                simSpeed += 1.2f;
                simRpm += 45.0f;
                simTemp += 0.05f;
                simThrottle += 0.8f;
                simLoad += 0.6f;
                if (simSpeed >= 95.0f) {
                    simAccelerating = false;
                }
            } else {
                simSpeed -= 0.8f;
                simRpm -= 30.0f;
                if (simTemp > 88.0f) simTemp -= 0.02f;
                simThrottle -= 0.6f;
                simLoad -= 0.4f;
                if (simSpeed <= 0.0f) {
                    simSpeed = 0.0f;
                    simAccelerating = true;
                }
            }

            if (simRpm > 4200.0f) simRpm = 1800.0f;
            if (simRpm < 800.0f) simRpm = 800.0f;
            if (simTemp > 110.0f) simTemp = 110.0f;
            if (simThrottle > 90.0f) simThrottle = 90.0f;
            if (simThrottle < 10.0f) simThrottle = 10.0f;
            if (simLoad > 85.0f) simLoad = 85.0f;
            if (simLoad < 15.0f) simLoad = 15.0f;
        }

        data.speedKmh = (int)simSpeed;
        data.coolantTempC = (int)simTemp;
        data.rpm = (int)simRpm;
        data.throttlePct = (int)simThrottle;
        data.intakeTempC = 28;
        data.engineLoadPct = (int)simLoad;
        data.batteryVolts = 14.1f;
        data.isConnected = true;
    } else {
        // Hardware OBD2 Mode
        if (!isInitialized) {
            if (searchStartTime == 0) searchStartTime = millis();

            if (millis() - searchStartTime >= SEARCH_TIMEOUT_MS) {
                if (!searchTimedOut) {
                    searchTimedOut = true;
                    data.searchTimedOut = true;
                    data.isConnected = false;
                    demoMode = true;
                    Serial.println("\n[OBD2 TIMEOUT] Couldn't connect after 10s! Falling back to Demo Mode.\n");
                }
                return;
            }

            // Pace connection init attempts so screen renders instantly & UI stays responsive
            if (lastInitAttemptTime == 0 || millis() - lastInitAttemptTime >= INIT_RETRY_INTERVAL_MS) {
                lastInitAttemptTime = millis();
                initISO9141();
            }
        }

        if (millis() - lastQueryTime >= 50) {
            lastQueryTime = millis();
            if (isInitialized) {
                // Non-Blocking Round-Robin PID Scheduler: Queries 1 active PID per update cycle
                static const struct { uint8_t bit; uint8_t pid; } pidMap[] = {
                    { 0x01, 0x0D }, // SPEED
                    { 0x04, 0x0C }, // RPM
                    { 0x02, 0x05 }, // COOLANT
                    { 0x08, 0x11 }, // THROTTLE
                    { 0x10, 0x0F }, // INTAKE
                    { 0x20, 0x04 }  // ENG LOAD
                };

                for (uint8_t i = 0; i < 6; i++) {
                    uint8_t idx = (queryIndex + i) % 6;
                    if (metricsBitmask & pidMap[idx].bit) {
                        uint8_t p = pidMap[idx].pid;
                        int val = readPIDValue(p);
                        if (val >= 0 || (p == 0x05 || p == 0x0F)) {
                            if (p == 0x0D) data.speedKmh = val;
                            else if (p == 0x05) data.coolantTempC = val;
                            else if (p == 0x0C) data.rpm = val;
                            else if (p == 0x11) data.throttlePct = val;
                            else if (p == 0x0F) data.intakeTempC = val;
                            else if (p == 0x04) data.engineLoadPct = val;
                        }
                        data.batteryVolts = 13.9f;
                        data.isConnected = true;
                        searchTimedOut = false;
                        queryIndex = (idx + 1) % 6;
                        break;
                    }
                }
            } else {
                data.isConnected = false;
            }
        }
    }

    // Exponential Moving Average (EMA) Low-Pass Damping Filter (alpha = 0.22f for fluid continuous speed & RPM sweeps)
    const float alpha = 0.22f;
    filteredSpeed += ((float)data.speedKmh - filteredSpeed) * alpha;
    filteredRpm += ((float)data.rpm - filteredRpm) * alpha;
    filteredTemp += ((float)data.coolantTempC - filteredTemp) * alpha;
    filteredThrottle += ((float)data.throttlePct - filteredThrottle) * alpha;
    filteredIntake += ((float)data.intakeTempC - filteredIntake) * alpha;
    filteredLoad += ((float)data.engineLoadPct - filteredLoad) * alpha;

    data.smoothedSpeedKmh = filteredSpeed;
    data.smoothedRpm = filteredRpm;
    data.smoothedCoolantTempC = filteredTemp;
    data.smoothedThrottlePct = filteredThrottle;
    data.smoothedIntakeTempC = filteredIntake;
    data.smoothedEngineLoadPct = filteredLoad;
}
