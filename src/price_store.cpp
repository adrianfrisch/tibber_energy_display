#include "price_store.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

namespace PriceStore {

static DailyPrices _today;
static DailyPrices _tomorrow;

static void recalcStats(DailyPrices& dp) {
    if (dp.count == 0) {
        dp.minPrice = 0;
        dp.maxPrice = 0;
        dp.avgPrice = 0;
        return;
    }
    float sum = 0;
    dp.minPrice = dp.entries[0].total;
    dp.maxPrice = dp.entries[0].total;
    for (uint8_t i = 0; i < dp.count; i++) {
        float p = dp.entries[i].total;
        sum += p;
        if (p < dp.minPrice) dp.minPrice = p;
        if (p > dp.maxPrice) dp.maxPrice = p;
    }
    dp.avgPrice = sum / dp.count;
}

static bool persistDailyPrices(const char* path, const DailyPrices& dp) {
    JsonDocument doc;
    doc["date"] = dp.date;
    doc["count"] = dp.count;
    doc["available"] = dp.available;
    JsonArray arr = doc["entries"].to<JsonArray>();
    for (uint8_t i = 0; i < dp.count; i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["hour"] = dp.entries[i].hour;
        obj["total"] = dp.entries[i].total;
        obj["energy"] = dp.entries[i].energy;
        obj["tax"] = dp.entries[i].tax;
        obj["currency"] = dp.entries[i].currency;
        obj["level"] = dp.entries[i].level;
    }

    File f = LittleFS.open(path, "w");
    if (!f) {
        Serial.printf("[PriceStore] Failed to open %s for writing\n", path);
        return false;
    }
    serializeJson(doc, f);
    f.close();
    Serial.printf("[PriceStore] Persisted %d entries to %s\n", dp.count, path);
    return true;
}

static bool loadDailyPrices(const char* path, DailyPrices& dp) {
    if (!LittleFS.exists(path)) return false;
    File f = LittleFS.open(path, "r");
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        Serial.printf("[PriceStore] JSON parse error in %s: %s\n", path, err.c_str());
        return false;
    }

    strlcpy(dp.date, doc["date"] | "", sizeof(dp.date));
    dp.count = doc["count"] | 0;
    dp.available = doc["available"] | false;

    JsonArray arr = doc["entries"].as<JsonArray>();
    uint8_t i = 0;
    for (JsonObject obj : arr) {
        if (i >= MAX_PRICE_ENTRIES) break;
        dp.entries[i].hour = obj["hour"] | 0;
        dp.entries[i].total = obj["total"] | 0.0f;
        dp.entries[i].energy = obj["energy"] | 0.0f;
        dp.entries[i].tax = obj["tax"] | 0.0f;
        strlcpy(dp.entries[i].currency, obj["currency"] | "EUR", sizeof(dp.entries[i].currency));
        strlcpy(dp.entries[i].level, obj["level"] | "NORMAL", sizeof(dp.entries[i].level));
        dp.entries[i].valid = true;
        i++;
    }
    recalcStats(dp);
    Serial.printf("[PriceStore] Loaded %d entries from %s (date: %s)\n", dp.count, path, dp.date);
    return true;
}

void init() {
    if (!LittleFS.begin(true)) {
        Serial.println("[PriceStore] LittleFS mount failed!");
        return;
    }
    Serial.println("[PriceStore] LittleFS mounted");
    memset(&_today, 0, sizeof(_today));
    memset(&_tomorrow, 0, sizeof(_tomorrow));
    loadFromFlash();
}

void setToday(const PriceEntry entries[], uint8_t count, const char* date) {
    _today.count = min(count, (uint8_t)MAX_PRICE_ENTRIES);
    _today.available = (_today.count > 0);
    strlcpy(_today.date, date, sizeof(_today.date));
    for (uint8_t i = 0; i < _today.count; i++) {
        _today.entries[i] = entries[i];
        _today.entries[i].valid = true;
    }
    recalcStats(_today);
    persistToday();
}

void setTomorrow(const PriceEntry entries[], uint8_t count, const char* date) {
    _tomorrow.count = min(count, (uint8_t)MAX_PRICE_ENTRIES);
    _tomorrow.available = (_tomorrow.count > 0);
    strlcpy(_tomorrow.date, date, sizeof(_tomorrow.date));
    for (uint8_t i = 0; i < _tomorrow.count; i++) {
        _tomorrow.entries[i] = entries[i];
        _tomorrow.entries[i].valid = true;
    }
    recalcStats(_tomorrow);
    persistTomorrow();
}

const DailyPrices& getToday() { return _today; }
const DailyPrices& getTomorrow() { return _tomorrow; }

const PriceEntry* getTodayEntry(uint8_t hour) {
    if (!_today.available || hour >= _today.count) return nullptr;
    for (uint8_t i = 0; i < _today.count; i++) {
        if (_today.entries[i].hour == hour) return &_today.entries[i];
    }
    return nullptr;
}

void clearTomorrow() {
    memset(&_tomorrow, 0, sizeof(_tomorrow));
    LittleFS.remove(PRICES_TOMORROW_PATH);
}

void rotateDays() {
    _today = _tomorrow;
    memset(&_tomorrow, 0, sizeof(_tomorrow));
    persistToday();
    LittleFS.remove(PRICES_TOMORROW_PATH);
    Serial.println("[PriceStore] Rotated tomorrow → today");
}

bool persistToday() { return persistDailyPrices(PRICES_TODAY_PATH, _today); }
bool persistTomorrow() { return persistDailyPrices(PRICES_TOMORROW_PATH, _tomorrow); }

bool loadFromFlash() {
    bool t = loadDailyPrices(PRICES_TODAY_PATH, _today);
    bool m = loadDailyPrices(PRICES_TOMORROW_PATH, _tomorrow);
    return t || m;
}

}  // namespace PriceStore

