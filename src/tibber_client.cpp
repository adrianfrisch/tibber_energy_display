#include "tibber_client.h"
#include "config.h"
#include "price_store.h"
#include "certs/tibber_ca.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace TibberClient {

static char _token[128] = {0};
static uint8_t _homeIndex = 0;
static int _lastHttpCode = 0;
static char _lastError[128] = {0};

static const char TIBBER_QUERY[] PROGMEM = R"GQL(
{"query":"{ viewer { homes { currentSubscription { priceInfo { current { total energy tax startsAt currency level } today { total energy tax startsAt currency level } tomorrow { total energy tax startsAt currency level } } } } } }"}
)GQL";

void init(const char* token, uint8_t homeIndex) {
    strlcpy(_token, token, sizeof(_token));
    _homeIndex = homeIndex;
    Serial.printf("[Tibber] Initialized (home index: %d)\n", _homeIndex);
}

/// Parse an ISO8601 timestamp and extract the hour.
static uint8_t parseHourFromISO(const char* startsAt) {
    // Format: "2026-05-13T14:00:00+02:00" or "2026-05-13T14:00:00.000+02:00"
    // Hour is at position 11-12
    if (strlen(startsAt) < 13) return 0;
    return (uint8_t)atoi(startsAt + 11);
}

/// Extract date string "YYYY-MM-DD" from ISO timestamp.
static void parseDateFromISO(const char* startsAt, char* dateBuf, size_t len) {
    if (strlen(startsAt) >= 10) {
        strlcpy(dateBuf, startsAt, min(len, (size_t)11));
    } else {
        dateBuf[0] = '\0';
    }
}

/// Parse a JSON array of price entries into a PriceEntry array.
static uint8_t parsePriceArray(JsonArray arr, PriceEntry* entries, char* dateBuf, size_t dateLen) {
    uint8_t count = 0;
    for (JsonObject obj : arr) {
        if (count >= MAX_PRICE_ENTRIES) break;

        const char* startsAt = obj["startsAt"] | "";
        entries[count].hour = parseHourFromISO(startsAt);
        entries[count].total = obj["total"] | 0.0f;
        entries[count].energy = obj["energy"] | 0.0f;
        entries[count].tax = obj["tax"] | 0.0f;
        strlcpy(entries[count].currency, obj["currency"] | "EUR", sizeof(entries[count].currency));
        strlcpy(entries[count].level, obj["level"] | "NORMAL", sizeof(entries[count].level));
        entries[count].valid = true;

        // Extract date from first entry
        if (count == 0 && dateBuf) {
            parseDateFromISO(startsAt, dateBuf, dateLen);
        }
        count++;
    }
    return count;
}

bool fetchPrices() {
    _lastHttpCode = 0;
    _lastError[0] = '\0';

    if (strlen(_token) == 0) {
        strlcpy(_lastError, "No API token configured", sizeof(_lastError));
        Serial.printf("[Tibber] Error: %s\n", _lastError);
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    String url = String("https://") + TIBBER_API_HOST + TIBBER_API_PATH;

    if (!http.begin(client, url)) {
        strlcpy(_lastError, "HTTP begin failed", sizeof(_lastError));
        Serial.printf("[Tibber] Error: %s\n", _lastError);
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + _token);
    http.setTimeout(15000);

    Serial.println("[Tibber] Sending GraphQL query...");
    _lastHttpCode = http.POST(TIBBER_QUERY);

    if (_lastHttpCode != 200) {
        snprintf(_lastError, sizeof(_lastError), "HTTP %d", _lastHttpCode);
        Serial.printf("[Tibber] Error: %s\n", _lastError);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    Serial.printf("[Tibber] Response: %d bytes\n", payload.length());

    // Parse JSON
    JsonDocument doc;
    DeserializationError jsonErr = deserializeJson(doc, payload);
    if (jsonErr) {
        snprintf(_lastError, sizeof(_lastError), "JSON: %s", jsonErr.c_str());
        Serial.printf("[Tibber] Error: %s\n", _lastError);
        return false;
    }

    // Navigate to the home's price info
    JsonObject home = doc["data"]["viewer"]["homes"][_homeIndex];
    if (home.isNull()) {
        snprintf(_lastError, sizeof(_lastError), "Home[%d] not found", _homeIndex);
        Serial.printf("[Tibber] Error: %s\n", _lastError);
        return false;
    }

    JsonObject priceInfo = home["currentSubscription"]["priceInfo"];
    if (priceInfo.isNull()) {
        strlcpy(_lastError, "No priceInfo in response", sizeof(_lastError));
        Serial.printf("[Tibber] Error: %s\n", _lastError);
        return false;
    }

    // Parse today's prices
    JsonArray todayArr = priceInfo["today"].as<JsonArray>();
    if (todayArr) {
        PriceEntry todayEntries[MAX_PRICE_ENTRIES];
        memset(todayEntries, 0, sizeof(todayEntries));
        char todayDate[11] = {0};
        uint8_t todayCount = parsePriceArray(todayArr, todayEntries, todayDate, sizeof(todayDate));
        if (todayCount > 0) {
            PriceStore::setToday(todayEntries, todayCount, todayDate);
            Serial.printf("[Tibber] Today: %d prices for %s\n", todayCount, todayDate);
        }
    }

    // Parse tomorrow's prices (may be empty)
    JsonArray tomorrowArr = priceInfo["tomorrow"].as<JsonArray>();
    if (tomorrowArr && tomorrowArr.size() > 0) {
        PriceEntry tomorrowEntries[MAX_PRICE_ENTRIES];
        memset(tomorrowEntries, 0, sizeof(tomorrowEntries));
        char tomorrowDate[11] = {0};
        uint8_t tomorrowCount = parsePriceArray(tomorrowArr, tomorrowEntries, tomorrowDate, sizeof(tomorrowDate));
        if (tomorrowCount > 0) {
            PriceStore::setTomorrow(tomorrowEntries, tomorrowCount, tomorrowDate);
            Serial.printf("[Tibber] Tomorrow: %d prices for %s\n", tomorrowCount, tomorrowDate);
        }
    } else {
        Serial.println("[Tibber] Tomorrow's prices not yet available");
    }

    return true;
}

int getLastHttpCode() { return _lastHttpCode; }
const char* getLastError() { return _lastError; }

}  // namespace TibberClient

