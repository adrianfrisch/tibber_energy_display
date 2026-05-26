#include "touch_handler.h"
#include "config.h"
#include <TFT_eSPI.h>

// The TFT_eSPI instance is shared; we use an extern reference
// In a real build, consider a shared display singleton.
extern TFT_eSPI tft;

namespace TouchHandler {

static unsigned long _lastTouchMs = 0;
static int _lastX = 0;
static int _lastY = 0;

void init() {
    // Touch calibration values for CYD - may need adjustment per unit
    uint16_t calData[5] = {300, 3600, 300, 3600, 1};
    tft.setTouch(calData);
    Serial.println("[Touch] Initialized");
}

TouchRegion poll() {
    uint16_t tx, ty;
    if (!tft.getTouch(&tx, &ty, 40)) {
        return TOUCH_NONE;
    }

    // Debounce
    unsigned long now = millis();
    if (now - _lastTouchMs < TOUCH_DEBOUNCE_MS) {
        return TOUCH_NONE;
    }
    _lastTouchMs = now;
    _lastX = (int)tx;
    _lastY = (int)ty;

    Serial.printf("[Touch] Tap at (%d, %d)\n", _lastX, _lastY);

    // Check nav bar region (bottom of screen)
    if (_lastY >= NAV_TOP) {
        if (_lastX >= 40 && _lastX <= 140) {
            return TOUCH_TODAY_BUTTON;
        }
        if (_lastX >= 180 && _lastX <= 280) {
            return TOUCH_TOMORROW_BUTTON;
        }
    }

    // Check chart area
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

