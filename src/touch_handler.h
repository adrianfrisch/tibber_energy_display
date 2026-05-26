#pragma once

#include <Arduino.h>

/// Touch input handler for the CYD resistive touchscreen.
namespace TouchHandler {

/// Touch region identifiers.
enum TouchRegion {
    TOUCH_NONE,
    TOUCH_TODAY_BUTTON,
    TOUCH_TOMORROW_BUTTON,
    TOUCH_CHART_AREA
};

/// Initialize touch controller.
void init();

/// Poll for touch events. Call from main loop.
/// @return The region that was tapped, or TOUCH_NONE.
TouchRegion poll();

/// Get the raw X/Y of the last touch (display coordinates).
void getLastTouchPos(int& x, int& y);

}  // namespace TouchHandler

