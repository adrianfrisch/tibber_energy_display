#pragma once

// ============================================================
// Pin Definitions for Cheap Yellow Display (ESP32-2432S028R)
// ============================================================

// TFT Display (ILI9341) - configured via build_flags for TFT_eSPI
// Kept here for reference and non-TFT_eSPI usage
#define TFT_BL_PIN      21

// Touch (XPT2046)
#define TOUCH_CS_PIN    33
#define TOUCH_IRQ_PIN   36

// SD Card
#define SD_CS_PIN       5

// RGB LED
#define LED_R_PIN       4
#define LED_G_PIN       16
#define LED_B_PIN       17

// LDR (Light Dependent Resistor)
#define LDR_PIN         34

// ============================================================
// Display Dimensions (landscape mode)
// ============================================================
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240

// ============================================================
// UI Layout Constants
// ============================================================
#define HEADER_HEIGHT   20
#define CHART_TOP       22
#define CHART_HEIGHT    170
#define STATS_TOP       (CHART_TOP + CHART_HEIGHT + 2)
#define STATS_HEIGHT    26
#define NAV_TOP         (STATS_TOP + STATS_HEIGHT + 2)
#define NAV_HEIGHT      20

#define BAR_COUNT       24
#define BAR_WIDTH       11
#define BAR_GAP         1
#define CHART_LEFT      25   // leave room for Y-axis labels
#define CHART_RIGHT     (CHART_LEFT + BAR_COUNT * (BAR_WIDTH + BAR_GAP))

// ============================================================
// Color Definitions (RGB565)
// ============================================================
#define COLOR_VERY_CHEAP    0x07E0  // Bright Green
#define COLOR_CHEAP         0x0660  // Green
#define COLOR_NORMAL        0xFE60  // Yellow
#define COLOR_EXPENSIVE     0xFB20  // Orange
#define COLOR_VERY_EXPENSIVE 0xF800 // Red
#define COLOR_HIGHLIGHT     0xFFFF  // White (current hour border)
#define COLOR_BACKGROUND    0x0000  // Black
#define COLOR_TEXT          0xFFFF  // White
#define COLOR_GRID          0x4208  // Dark grey
#define COLOR_HEADER_BG     0x000F  // Dark blue
#define COLOR_NAV_ACTIVE    0x001F  // Blue
#define COLOR_NAV_INACTIVE  0x4208  // Grey

// ============================================================
// Timing Constants
// ============================================================
#define NTP_SYNC_INTERVAL_MS    (6UL * 60 * 60 * 1000)  // 6 hours
#define DISPLAY_REFRESH_MS      (60UL * 1000)            // 1 minute
#define TOUCH_DEBOUNCE_MS       200
#define WIFI_RETRY_BASE_MS      2000
#define WIFI_RETRY_MAX_MS       (5UL * 60 * 1000)       // 5 minutes
#define FETCH_RETRY_INTERVALS   {5*60*1000UL, 15*60*1000UL, 60*60*1000UL}

// ============================================================
// Schedule Times (local time, hour:minute)
// ============================================================
#define MORNING_FETCH_HOUR      0
#define MORNING_FETCH_MINUTE    15
#define AFTERNOON_FETCH_HOUR    13
#define AFTERNOON_FETCH_MINUTE  15

// ============================================================
// Tibber API
// ============================================================
#define TIBBER_API_HOST     "api.tibber.com"
#define TIBBER_API_PORT     443
#define TIBBER_API_PATH     "/v1-beta/gql"

// ============================================================
// File Paths (LittleFS)
// ============================================================
#define CONFIG_FILE_PATH        "/config.json"
#define PRICES_TODAY_PATH       "/prices_today.json"
#define PRICES_TOMORROW_PATH    "/prices_tomorrow.json"

// ============================================================
// Defaults
// ============================================================
#define DEFAULT_TIMEZONE        "CET-1CEST,M3.5.0,M10.5.0/3"
#define DEFAULT_PRICE_MULTIPLIER 100.0f
#define DEFAULT_CURRENCY_UNIT   "ct/kWh"
#define DEFAULT_BRIGHTNESS      80
#define DEFAULT_HOME_INDEX      0

// ============================================================
// Watchdog
// ============================================================
#define WDT_TIMEOUT_S           30

