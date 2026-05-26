#include "touch_handler.h"
#include "config.h"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

// ============================================================
// CYD Touch SPI Pins (HSPI – separate from display VSPI)
// ============================================================
#define TOUCH_CLK   25
#define TOUCH_DIN   32   // MOSI to touch controller
#define TOUCH_DOUT  39   // MISO from touch controller
#define TOUCH_CS    33
#define TOUCH_IRQ   36

// HSPI bus instance for touch (display uses default VSPI)
static SPIClass hspi(HSPI);
static XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

namespace TouchHandler {

static unsigned long _lastTouchMs = 0;
static int _lastX = 0;
static int _lastY = 0;

void init() {
    hspi.begin(TOUCH_CLK, TOUCH_DOUT, TOUCH_DIN, TOUCH_CS);
    ts.begin(hspi);
    ts.setRotation(1);  // landscape, matching display rotation
    Serial.println("[Touch] XPT2046 initialized on HSPI (CLK=25, DIN=32, DOUT=39, CS=33)");
}

TouchRegion poll() {
    if (!ts.touched()) {
        return TOUCH_NONE;
    }

    // Debounce
    unsigned long now = millis();
    if (now - _lastTouchMs < TOUCH_DEBOUNCE_MS) {
        return TOUCH_NONE;
    }
    _lastTouchMs = now;

    TS_Point p = ts.getPoint();

    // Map raw touch ADC values (typically 0–4095) to screen coordinates
    _lastX = map(p.x, 200, 3800, 0, SCREEN_WIDTH);
    _lastY = map(p.y, 200, 3800, 0, SCREEN_HEIGHT);
    _lastX = constrain(_lastX, 0, SCREEN_WIDTH - 1);
    _lastY = constrain(_lastY, 0, SCREEN_HEIGHT - 1);

    Serial.printf("[Touch] raw(%d,%d) -> screen(%d,%d) z=%d\n",
                  p.x, p.y, _lastX, _lastY, p.z);

    // --- Nav bar buttons (bottom strip) ---
    if (_lastY >= NAV_TOP) {
        if (_lastX >= 40 && _lastX <= 140) {
            Serial.println("[Touch] -> TODAY");
            return TOUCH_TODAY_BUTTON;
        }
        if (_lastX >= 180 && _lastX <= 280) {
            Serial.println("[Touch] -> TOMORROW");
            return TOUCH_TOMORROW_BUTTON;
        }
    }

    // --- Left/Right half fallback for stats + nav area ---
    if (_lastY >= STATS_TOP) {
        if (_lastX < SCREEN_WIDTH / 2) {
            Serial.println("[Touch] -> left half (Today)");
            return TOUCH_TODAY_BUTTON;
        } else {
            Serial.println("[Touch] -> right half (Tomorrow)");
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
