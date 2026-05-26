#include "touch_handler.h"
#include "config.h"
#include <TFT_eSPI.h>

// Shared TFT instance defined in display.cpp
extern TFT_eSPI tft;

namespace TouchHandler {

static unsigned long _lastTouchMs = 0;
static int _lastX = 0;
static int _lastY = 0;

// Calibration data for CYD in landscape (rotation 1).
// Format: { xMin, xMax, yMin, yMax, rotationFlag }
// These map raw ADC values to pixel coordinates.
// Adjust per-unit if touches are offset — the serial log prints raw values.
static uint16_t calData[5] = {300, 3600, 300, 3600, 7};

void init() {
    tft.setTouch(calData);
    Serial.println("[Touch] Initialized (landscape calibration)");
}

TouchRegion poll() {
    uint16_t tx, ty;
    bool pressed = tft.getTouch(&tx, &ty, 40);

    if (!pressed) {
        return TOUCH_NONE;
    }

    // Debounce — reject rapid repeated events
    unsigned long now = millis();
    if (now - _lastTouchMs < TOUCH_DEBOUNCE_MS) {
        return TOUCH_NONE;
    }
    _lastTouchMs = now;

    _lastX = (int)tx;
    _lastY = (int)ty;

    Serial.printf("[Touch] Tap at (%d, %d)\n", _lastX, _lastY);

    // Validate coordinates are within screen bounds
    if (_lastX < 0 || _lastX >= SCREEN_WIDTH ||
        _lastY < 0 || _lastY >= SCREEN_HEIGHT) {
        Serial.println("[Touch] Out of bounds, ignoring");
        return TOUCH_NONE;
    }

    // --- Check navigation bar (bottom strip) ---
    if (_lastY >= NAV_TOP) {
        // Today button: x 40–140
        if (_lastX >= 40 && _lastX <= 140) {
            Serial.println("[Touch] -> TODAY button");
            return TOUCH_TODAY_BUTTON;
        }
        // Tomorrow button: x 180–280
        if (_lastX >= 180 && _lastX <= 280) {
            Serial.println("[Touch] -> TOMORROW button");
            return TOUCH_TOMORROW_BUTTON;
        }
    }

    // --- Left / Right half as fallback gesture ---
    // Makes it easy to switch even if the user doesn't hit the small button
    if (_lastY >= STATS_TOP) {
        if (_lastX < SCREEN_WIDTH / 2) {
            Serial.println("[Touch] -> LEFT half (Today)");
            return TOUCH_TODAY_BUTTON;
        } else {
            Serial.println("[Touch] -> RIGHT half (Tomorrow)");
            return TOUCH_TOMORROW_BUTTON;
        }
    }

    // --- Chart area ---
    if (_lastY >= CHART_TOP && _lastY < STATS_TOP) {
        return TOUCH_CHART_AREA;
    }

    return TOUCH_NONE;
}

void getLastTouchPos(int& x, int& y) {
    x = _lastX;
    y = _lastY;
}

}  // namespace TouchHandler
