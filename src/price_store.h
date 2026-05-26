#pragma once

#include <Arduino.h>

/// Maximum number of hourly price entries per day.
#define MAX_PRICE_ENTRIES 24

/// A single hourly electricity price entry.
struct PriceEntry {
    uint8_t  hour;            // 0–23
    float    total;           // total price incl. tax (e.g. EUR/kWh from API)
    float    energy;          // energy component
    float    tax;             // tax component
    char     currency[4];     // "EUR", "SEK", "NOK"
    char     level[16];       // "VERY_CHEAP", "CHEAP", "NORMAL", "EXPENSIVE", "VERY_EXPENSIVE"
    bool     valid;           // true if entry was populated
};

/// Daily price data with summary stats.
struct DailyPrices {
    PriceEntry entries[MAX_PRICE_ENTRIES];
    uint8_t    count;         // Number of valid entries (0 or 24)
    float      minPrice;
    float      maxPrice;
    float      avgPrice;
    char       date[11];      // "YYYY-MM-DD"
    bool       available;     // true if data has been fetched
};

/// Price data storage: in-memory + LittleFS persistence.
namespace PriceStore {

/// Initialize LittleFS and load cached data.
void init();

/// Set today's price data (24 entries). Recalculates stats and persists to flash.
void setToday(const PriceEntry entries[], uint8_t count, const char* date);

/// Set tomorrow's price data. Pass count=0 to mark as unavailable.
void setTomorrow(const PriceEntry entries[], uint8_t count, const char* date);

/// Get today's price data.
const DailyPrices& getToday();

/// Get tomorrow's price data.
const DailyPrices& getTomorrow();

/// Get price entry for a specific hour today. Returns nullptr if not available.
const PriceEntry* getTodayEntry(uint8_t hour);

/// Clear tomorrow (e.g. at midnight rollover).
void clearTomorrow();

/// Rotate: move tomorrow → today, clear tomorrow. Called at midnight.
void rotateDays();

/// Persist current data to LittleFS.
bool persistToday();
bool persistTomorrow();

/// Load cached data from LittleFS. Called by init().
bool loadFromFlash();

}  // namespace PriceStore

