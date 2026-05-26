/**
 * Electricity Price Dashboard
 *
 * ESP32 Cheap Yellow Display (CYD) application that fetches day-ahead
 * electricity prices from the Tibber GraphQL API and displays them
 * as a color-coded bar chart.
 *
 * See docs/specification.md for full project documentation.
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "wifi_manager.h"
#include "ntp_sync.h"
#include "tibber_client.h"
#include "price_store.h"
#include "display.h"
#include "touch_handler.h"
#include "scheduler.h"

// ============================================================
// Application State Machine
// ============================================================
enum AppState {
    STATE_BOOT,
    STATE_WIFI_CONNECT,
    STATE_NTP_SYNC,
    STATE_FETCH_PRICES,
    STATE_DISPLAY,
    STATE_IDLE
};

static AppState state = STATE_BOOT;
static bool showingToday = true;

// Configuration loaded from LittleFS
static char cfgSsid[64] = {0};
static char cfgPassword[64] = {0};
static char cfgTibberToken[128] = {0};
static char cfgTimezone[64] = {0};
static uint8_t cfgHomeIndex = DEFAULT_HOME_INDEX;
static float cfgPriceMultiplier = DEFAULT_PRICE_MULTIPLIER;
static uint8_t cfgBrightness = DEFAULT_BRIGHTNESS;

// ============================================================
// Load configuration from LittleFS
// ============================================================
static bool loadConfig() {
    if (!LittleFS.begin(true)) {
        Serial.println("[Config] LittleFS mount failed");
        return false;
    }

    File f = LittleFS.open(CONFIG_FILE_PATH, "r");
    if (!f) {
        Serial.println("[Config] config.json not found");
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        Serial.printf("[Config] Parse error: %s\n", err.c_str());
        return false;
    }

    strlcpy(cfgSsid, doc["wifi_ssid"] | "", sizeof(cfgSsid));
    strlcpy(cfgPassword, doc["wifi_password"] | "", sizeof(cfgPassword));
    strlcpy(cfgTibberToken, doc["tibber_token"] | "", sizeof(cfgTibberToken));
    strlcpy(cfgTimezone, doc["timezone"] | DEFAULT_TIMEZONE, sizeof(cfgTimezone));
    cfgHomeIndex = doc["home_index"] | DEFAULT_HOME_INDEX;
    cfgPriceMultiplier = doc["price_multiplier"] | DEFAULT_PRICE_MULTIPLIER;
    cfgBrightness = doc["display_brightness"] | DEFAULT_BRIGHTNESS;

    Serial.printf("[Config] SSID: %s, Home: %d, TZ: %s\n", cfgSsid, cfgHomeIndex, cfgTimezone);
    return (strlen(cfgSsid) > 0 && strlen(cfgTibberToken) > 0);
}

// ============================================================
// Redraw display based on current view
// ============================================================
static void refreshDisplay() {
    int hour = Scheduler::getCurrentHour();
    bool wifi = WifiManager::isConnected();

    if (showingToday) {
        const DailyPrices& prices = PriceStore::getToday();
        Display::drawFullScreen(prices, hour, true, wifi);
    } else {
        const DailyPrices& tomorrow = PriceStore::getTomorrow();
        if (tomorrow.available) {
            Display::drawFullScreen(tomorrow, -1, false, wifi);
        } else {
            Display::drawFullScreen(PriceStore::getToday(), hour, false, wifi);
            Display::drawNotAvailable();
        }
    }
}

// ============================================================
// Fetch prices from Tibber API
// ============================================================
static bool doFetchPrices() {
    Serial.println("[Main] Fetching prices from Tibber...");
    Display::drawStatusMessage("Updating prices...");

    bool ok = TibberClient::fetchPrices();
    if (ok) {
        Scheduler::markFetchCompleted();
        Serial.println("[Main] Prices updated successfully");
    } else {
        Scheduler::markFetchFailed();
        Serial.printf("[Main] Fetch failed: %s\n", TibberClient::getLastError());
    }
    return ok;
}

// ============================================================
// setup()
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n========================================");
    Serial.println("  Electricity Price Dashboard v1.0");
    Serial.println("  Cheap Yellow Display (CYD)");
    Serial.println("========================================\n");

    // Enable watchdog timer
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);

    // Initialize display first (for visual feedback during boot)
    Display::init();
    Display::drawStatusMessage("Booting...", "Loading config");

    // Load configuration
    if (!loadConfig()) {
        Display::drawStatusMessage("CONFIG ERROR", "Upload config.json", "via LittleFS");
        Serial.println("[Main] FATAL: No valid configuration. Halting.");
        while (true) { delay(1000); esp_task_wdt_reset(); }
    }

    Display::setBrightness(cfgBrightness);

    // Initialize price store (loads cached data from flash)
    PriceStore::init();

    // Initialize Tibber client
    TibberClient::init(cfgTibberToken, cfgHomeIndex);

    // Connect to WiFi
    state = STATE_WIFI_CONNECT;
    Display::drawStatusMessage("Connecting WiFi...", cfgSsid);
    WifiManager::init();

    if (!WifiManager::connect(cfgSsid, cfgPassword, 15000)) {
        Display::drawStatusMessage("WiFi Failed", "Will retry...", "Showing cached data");
        delay(2000);
        // Continue with cached data if available
        if (PriceStore::getToday().available) {
            state = STATE_DISPLAY;
        }
    } else {
        state = STATE_NTP_SYNC;
    }

    // NTP sync
    if (state == STATE_NTP_SYNC) {
        Display::drawStatusMessage("Syncing time...");
        NtpSync::init(cfgTimezone);
        NtpSync::sync();

        if (NtpSync::isSynced()) {
            state = STATE_FETCH_PRICES;
        } else {
            Display::drawStatusMessage("NTP sync failed", "Retrying...");
            delay(1000);
            state = STATE_FETCH_PRICES;  // Try fetching anyway
        }
    }

    // Initial price fetch
    if (state == STATE_FETCH_PRICES) {
        doFetchPrices();
        state = STATE_DISPLAY;
    }

    // Initialize remaining modules
    TouchHandler::init();
    Scheduler::init();

    // Draw initial screen
    refreshDisplay();
    state = STATE_IDLE;

    Serial.println("[Main] Setup complete, entering main loop");
}

// ============================================================
// loop()
// ============================================================
void loop() {
    esp_task_wdt_reset();

    // WiFi keep-alive
    WifiManager::loop();

    // Scheduler tick
    Scheduler::loop();

    // NTP re-sync
    if (Scheduler::isNtpSyncDue()) {
        NtpSync::sync();
    }

    // Scheduled fetches
    if (WifiManager::isConnected()) {
        if (Scheduler::isMorningFetchDue() || Scheduler::isAfternoonFetchDue()) {
            doFetchPrices();
            refreshDisplay();
        }

        // Retry after failure
        if (Scheduler::isRetryDue()) {
            if (doFetchPrices()) {
                refreshDisplay();
            }
        }
    }

    // Hour change → update display
    if (Scheduler::hasHourChanged()) {
        int prev = Scheduler::getPreviousHour();
        int curr = Scheduler::getCurrentHour();
        Serial.printf("[Main] Hour changed: %d → %d\n", prev, curr);

        // Midnight: rotate days
        if (curr == 0) {
            PriceStore::rotateDays();
        }

        if (showingToday) {
            Display::updateCurrentHour(PriceStore::getToday(), prev, curr);
        }
    }

    // Periodic display refresh (clock update)
    if (Scheduler::isDisplayRefreshDue()) {
        char timeStr[6];
        NtpSync::getTimeString(timeStr, sizeof(timeStr));
        Display::updateClock(timeStr);
    }

    // Touch input
    TouchHandler::TouchRegion touch = TouchHandler::poll();
    switch (touch) {
        case TouchHandler::TOUCH_TODAY_BUTTON:
            if (!showingToday) {
                showingToday = true;
                refreshDisplay();
            }
            break;

        case TouchHandler::TOUCH_TOMORROW_BUTTON:
            if (showingToday) {
                showingToday = false;
                refreshDisplay();
            }
            break;

        case TouchHandler::TOUCH_CHART_AREA:
            // Future: show tooltip for tapped hour
            break;

        default:
            break;
    }

    delay(50);  // Small delay to reduce CPU usage
}

