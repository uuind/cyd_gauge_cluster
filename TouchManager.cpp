#include "TouchManager.h"

TouchManager::TouchManager(uint8_t cs, uint8_t irq) : 
    csPin(cs), irqPin(irq), lastTouchTime(0), touchSPIBus(HSPI), touchSPI(cs, irq), tftRef(nullptr) {}

void TouchManager::begin(TFT_eSPI* tftInstance) {
    tftRef = tftInstance;
    
    // Initialize dedicated SPI bus for ESP32 CYD Touchscreen (CLK:25, MISO:39, MOSI:32, CS:33)
    touchSPIBus.begin(CYD_TOUCH_CLK, CYD_TOUCH_MISO, CYD_TOUCH_MOSI, CYD_TOUCH_CS);
    touchSPI.begin(touchSPIBus);
    touchSPI.setRotation(1); // CYD Landscape orientation (320x240)
}

TouchPoint TouchManager::getTouchPoint() {
    TouchPoint tp;
    tp.x = 0;
    tp.y = 0;
    tp.isPressed = false;

    // 1. Primary Sampler: TFT_eSPI Touch Engine
    if (tftRef != nullptr) {
        uint16_t tftX = 0, tftY = 0;
        bool isTouched = tftRef->getTouch(&tftX, &tftY);
        if (isTouched && (tftX > 0 || tftY > 0)) {
            if (millis() - lastTouchTime >= DEBOUNCE_MS) {
                tp.x = constrain((int16_t)tftX, 0, 320);
                tp.y = constrain((int16_t)tftY, 0, 240);
                tp.isPressed = true;
                lastTouchTime = millis();

                Serial.print("[CYD TFT TOUCH] Pixel (X:");
                Serial.print(tp.x);
                Serial.print(", Y:");
                Serial.print(tp.y);
                Serial.println(")");
                return tp;
            }
        }
    }

    // 2. Hardware Sampler: CYD Dedicated XPT2046 SPI Bus
    if (touchSPI.touched()) {
        TS_Point p = touchSPI.getPoint();

        // Valid touch pressure for CYD XPT2046 resistive screen
        if (p.z > 200) { 
            if (millis() - lastTouchTime >= DEBOUNCE_MS) {
                // Map CYD raw touchscreen ADC values to 320 x 240 landscape coordinates
                int16_t mappedX = map(p.x, touchMinX, touchMaxX, 0, 320);
                int16_t mappedY = map(p.y, touchMinY, touchMaxY, 0, 240);

                tp.x = constrain(mappedX, 0, 320);
                tp.y = constrain(mappedY, 0, 240);
                tp.isPressed = true;

                lastTouchTime = millis();

                Serial.print("[CYD SPI TOUCH] Raw (X:");
                Serial.print(p.x);
                Serial.print(", Y:");
                Serial.print(p.y);
                Serial.print(", Z:");
                Serial.print(p.z);
                Serial.print(") -> Screen Pixel (X:");
                Serial.print(tp.x);
                Serial.print(", Y:");
                Serial.print(tp.y);
                Serial.println(")");
            }
        }
    }

    return tp;
}

bool TouchManager::isTouchedInRect(const TouchPoint& tp, int16_t x, int16_t y, int16_t w, int16_t h) {
    if (!tp.isPressed) return false;
    return (tp.x >= x && tp.x <= (x + w) && tp.y >= y && tp.y <= (y + h));
}
