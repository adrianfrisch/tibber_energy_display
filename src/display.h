#pragma once

#include <Arduino.h>
#include "price_store.h"

/// Display renderer for the TFT screen.
namespace Display {

/// Initialize TFT display and set landscape orientation.
void init();

/// Set display backlight brightness (0–100).
void setBrightness(uint8_t percent);

/// Draw the full screen: header, bar chart, stats, nav bar.
/// @param prices     The daily prices to render.
/// @param currentHour Current hour for highlight (-1 if not applicable, e.g. tomorrow view).
/// @param isTodayView true = "Today" tab active, false = "Tomorrow" tab active.
/// @param wifiConnected Whether WiFi is connected (for status icon).
void drawFullScreen(const DailyPrices& prices, int currentHour,
                    bool isTodayView, bool wifiConnected);

/// Update only the current-hour highlight bar (minimal redraw).
void updateCurrentHour(const DailyPrices& prices, int previousHour, int currentHour);

/// Update the clock in the header without full redraw.
void updateClock(const char* timeStr);

/// Draw an error/status overlay message.
void drawStatusMessage(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr);

/// Draw "not yet available" placeholder for tomorrow.
void drawNotAvailable();

}  // namespace Display

