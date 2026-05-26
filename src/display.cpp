#include "display.h"
#include "config.h"
#include <TFT_eSPI.h>

namespace Display {

static TFT_eSPI tft = TFT_eSPI();
static TFT_eSprite sprite = TFT_eSprite(&tft);

/// Map a Tibber price level string to an RGB565 color.
static uint16_t levelToColor(const char* level) {
    if (strcmp(level, "VERY_CHEAP") == 0)    return COLOR_VERY_CHEAP;
    if (strcmp(level, "CHEAP") == 0)         return COLOR_CHEAP;
    if (strcmp(level, "NORMAL") == 0)        return COLOR_NORMAL;
    if (strcmp(level, "EXPENSIVE") == 0)     return COLOR_EXPENSIVE;
    if (strcmp(level, "VERY_EXPENSIVE") == 0) return COLOR_VERY_EXPENSIVE;
    return COLOR_NORMAL;
}

void init() {
    tft.init();
    tft.setRotation(1);  // Landscape
    tft.fillScreen(COLOR_BACKGROUND);

    // Configure backlight
    ledcSetup(0, 5000, 8);         // Channel 0, 5 kHz, 8-bit
    ledcAttachPin(TFT_BL_PIN, 0);
    setBrightness(DEFAULT_BRIGHTNESS);

    Serial.println("[Display] Initialized (320x240 landscape)");
}

void setBrightness(uint8_t percent) {
    uint8_t duty = map(constrain(percent, 0, 100), 0, 100, 0, 255);
    ledcWrite(0, duty);
}

/// Draw the header bar with title, date label, time, and WiFi icon.
static void drawHeader(bool isTodayView, const char* timeStr, bool wifiConnected) {
    tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_HEADER_BG);
    tft.setTextColor(COLOR_TEXT, COLOR_HEADER_BG);
    tft.setTextSize(1);

    // Lightning bolt + title
    tft.setCursor(4, 5);
    tft.print("Electricity Prices");

    // Today/Tomorrow label
    tft.setCursor(160, 5);
    tft.print(isTodayView ? "Today" : "Tomorrow");

    // Time
    tft.setCursor(240, 5);
    tft.print(timeStr);

    // WiFi icon (simple text indicator)
    tft.setCursor(300, 5);
    tft.setTextColor(wifiConnected ? COLOR_VERY_CHEAP : COLOR_VERY_EXPENSIVE, COLOR_HEADER_BG);
    tft.print(wifiConnected ? "WiFi" : " -- ");
    tft.setTextColor(COLOR_TEXT, COLOR_HEADER_BG);
}

/// Draw the bar chart for the given daily prices.
static void drawBarChart(const DailyPrices& prices, int currentHour) {
    // Clear chart area
    tft.fillRect(0, CHART_TOP, SCREEN_WIDTH, CHART_HEIGHT, COLOR_BACKGROUND);

    if (!prices.available || prices.count == 0) return;

    float maxP = prices.maxPrice;
    float minP = prices.minPrice;
    float range = maxP - minP;
    if (range < 0.001f) range = 0.001f;  // Avoid division by zero

    int chartBottom = CHART_TOP + CHART_HEIGHT - 15;  // Leave space for hour labels
    int chartUsable = CHART_HEIGHT - 25;               // Usable bar height (minus labels + margin)

    // Draw Y-axis labels
    tft.setTextSize(1);
    tft.setTextColor(COLOR_GRID, COLOR_BACKGROUND);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", maxP * DEFAULT_PRICE_MULTIPLIER);
    tft.setCursor(0, CHART_TOP + 2);
    tft.print(buf);
    snprintf(buf, sizeof(buf), "%.0f", minP * DEFAULT_PRICE_MULTIPLIER);
    tft.setCursor(0, chartBottom - 10);
    tft.print(buf);

    // Draw bars
    for (uint8_t i = 0; i < prices.count && i < BAR_COUNT; i++) {
        int x = CHART_LEFT + i * (BAR_WIDTH + BAR_GAP);
        float normalized = (prices.entries[i].total - minP) / range;
        int barH = max((int)(normalized * chartUsable), 2);  // Minimum 2px visible
        int y = chartBottom - barH;

        uint16_t color = levelToColor(prices.entries[i].level);
        tft.fillRect(x, y, BAR_WIDTH, barH, color);

        // Highlight current hour
        if ((int)prices.entries[i].hour == currentHour) {
            tft.drawRect(x - 1, y - 1, BAR_WIDTH + 2, barH + 2, COLOR_HIGHLIGHT);
        }

        // Hour label (every 3 hours + current)
        if (i % 3 == 0 || (int)prices.entries[i].hour == currentHour) {
            tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
            snprintf(buf, sizeof(buf), "%02d", prices.entries[i].hour);
            tft.setCursor(x, chartBottom + 3);
            tft.print(buf);
        }
    }
}

/// Draw the stats bar: current price, min, max, average.
static void drawStatsBar(const DailyPrices& prices, int currentHour) {
    tft.fillRect(0, STATS_TOP, SCREEN_WIDTH, STATS_HEIGHT, COLOR_BACKGROUND);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);

    if (!prices.available) {
        tft.setCursor(4, STATS_TOP + 8);
        tft.print("No data available");
        return;
    }

    float mult = DEFAULT_PRICE_MULTIPLIER;
    char buf[80];

    // Current price
    float currentPrice = 0;
    const PriceEntry* curr = nullptr;
    for (uint8_t i = 0; i < prices.count; i++) {
        if ((int)prices.entries[i].hour == currentHour) {
            curr = &prices.entries[i];
            currentPrice = curr->total;
            break;
        }
    }

    if (curr) {
        snprintf(buf, sizeof(buf), "Now:%.1f  Min:%.1f  Max:%.1f  Avg:%.1f %s",
                 currentPrice * mult,
                 prices.minPrice * mult,
                 prices.maxPrice * mult,
                 prices.avgPrice * mult,
                 DEFAULT_CURRENCY_UNIT);
    } else {
        snprintf(buf, sizeof(buf), "Min:%.1f  Max:%.1f  Avg:%.1f %s",
                 prices.minPrice * mult,
                 prices.maxPrice * mult,
                 prices.avgPrice * mult,
                 DEFAULT_CURRENCY_UNIT);
    }

    tft.setCursor(4, STATS_TOP + 8);
    tft.print(buf);
}

/// Draw the bottom navigation bar with Today/Tomorrow tabs.
static void drawNavBar(bool isTodayView, bool tomorrowAvailable) {
    tft.fillRect(0, NAV_TOP, SCREEN_WIDTH, NAV_HEIGHT, COLOR_BACKGROUND);

    // Today button
    uint16_t todayColor = isTodayView ? COLOR_NAV_ACTIVE : COLOR_NAV_INACTIVE;
    tft.fillRoundRect(40, NAV_TOP + 2, 100, NAV_HEIGHT - 4, 4, todayColor);
    tft.setTextColor(COLOR_TEXT, todayColor);
    tft.setTextSize(1);
    tft.setCursor(68, NAV_TOP + 6);
    tft.print("TODAY");

    // Tomorrow button
    uint16_t tmrwColor = (!isTodayView && tomorrowAvailable) ? COLOR_NAV_ACTIVE : COLOR_NAV_INACTIVE;
    tft.fillRoundRect(180, NAV_TOP + 2, 100, NAV_HEIGHT - 4, 4, tmrwColor);
    tft.setTextColor(tomorrowAvailable ? COLOR_TEXT : COLOR_GRID, tmrwColor);
    tft.setCursor(196, NAV_TOP + 6);
    tft.print("TOMORROW");
}

void drawFullScreen(const DailyPrices& prices, int currentHour,
                    bool isTodayView, bool wifiConnected) {
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "--:--");
    // Caller should use NtpSync::getTimeString, but we accept what we have
    struct tm ti;
    if (getLocalTime(&ti, 100)) {
        strftime(timeStr, sizeof(timeStr), "%H:%M", &ti);
    }

    drawHeader(isTodayView, timeStr, wifiConnected);
    drawBarChart(prices, currentHour);
    drawStatsBar(prices, currentHour);
    drawNavBar(isTodayView, true);  // TODO: check actual tomorrow availability

    Serial.println("[Display] Full screen drawn");
}

void updateCurrentHour(const DailyPrices& prices, int previousHour, int currentHour) {
    // Minimal redraw: just update the two affected bars and the stats
    drawBarChart(prices, currentHour);  // For now, redraw whole chart
    drawStatsBar(prices, currentHour);
}

void updateClock(const char* timeStr) {
    tft.fillRect(240, 0, 50, HEADER_HEIGHT, COLOR_HEADER_BG);
    tft.setTextColor(COLOR_TEXT, COLOR_HEADER_BG);
    tft.setTextSize(1);
    tft.setCursor(240, 5);
    tft.print(timeStr);
}

void drawStatusMessage(const char* line1, const char* line2, const char* line3) {
    // Draw a centered overlay
    int boxW = 260, boxH = 80;
    int boxX = (SCREEN_WIDTH - boxW) / 2;
    int boxY = (SCREEN_HEIGHT - boxH) / 2;

    tft.fillRect(boxX, boxY, boxW, boxH, COLOR_HEADER_BG);
    tft.drawRect(boxX, boxY, boxW, boxH, COLOR_EXPENSIVE);
    tft.setTextColor(COLOR_TEXT, COLOR_HEADER_BG);
    tft.setTextSize(1);

    int y = boxY + 12;
    if (line1) { tft.setCursor(boxX + 10, y); tft.print(line1); y += 18; }
    if (line2) { tft.setCursor(boxX + 10, y); tft.print(line2); y += 18; }
    if (line3) { tft.setCursor(boxX + 10, y); tft.print(line3); }
}

void drawNotAvailable() {
    drawStatusMessage("Tomorrow's prices", "not yet available", "Check after 13:00");
}

}  // namespace Display

