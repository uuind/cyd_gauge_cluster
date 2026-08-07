#ifndef TOUCH_MANAGER_H
#define TOUCH_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ESP32 Cheap Yellow Display (CYD / ESP32-2432S028R) Dedicated Touch SPI Pins
#define CYD_TOUCH_CLK  25
#define CYD_TOUCH_MISO 39
#define CYD_TOUCH_MOSI 32
#define CYD_TOUCH_CS   33
#define CYD_TOUCH_IRQ  36

struct TouchPoint {
    int16_t x;
    int16_t y;
    bool isPressed;
};

class TouchManager {
private:
    uint8_t csPin;
    uint8_t irqPin;
    unsigned long lastTouchTime;
    const unsigned long DEBOUNCE_MS = 150;
    
    SPIClass touchSPIBus;
    XPT2046_Touchscreen touchSPI;
    TFT_eSPI* tftRef;

    // CYD 2.8" Landscape Calibration (320x240)
    int16_t touchMinX = 250;
    int16_t touchMaxX = 3700;
    int16_t touchMinY = 250;
    int16_t touchMaxY = 3700;

public:
    TouchManager(uint8_t cs = CYD_TOUCH_CS, uint8_t irq = CYD_TOUCH_IRQ);
    void begin(TFT_eSPI* tftInstance = nullptr);
    TouchPoint getTouchPoint();
    
    static bool isTouchedInRect(const TouchPoint& tp, int16_t x, int16_t y, int16_t w, int16_t h);
};

#endif // TOUCH_MANAGER_H
