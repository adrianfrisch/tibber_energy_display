#include "ntp_sync.h"
#include "config.h"
#include <time.h>
#include <esp_sntp.h>

namespace NtpSync {

static bool _synced = false;
static unsigned long _lastSyncMs = 0;

void init(const char* tz) {
    configTzTime(tz, "pool.ntp.org", "time.nist.gov");
    Serial.printf("[NTP] Timezone set to: %s\n", tz);
}

void sync() {
    Serial.println("[NTP] Requesting time sync...");
    // esp_sntp will sync in background; we poll getLocalTime()
    struct tm timeinfo;
    int attempts = 0;
    while (!::getLocalTime(&timeinfo, 1000) && attempts < 10) {
        attempts++;
        delay(500);
    }
    if (attempts < 10) {
        _synced = true;
        _lastSyncMs = millis();
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("[NTP] Synced: %s\n", buf);
    } else {
        Serial.println("[NTP] Sync failed");
    }
}

void loop() {
    unsigned long now = millis();
    if (now - _lastSyncMs >= NTP_SYNC_INTERVAL_MS) {
        sync();
    }
}

bool isSynced() {
    return _synced;
}

int getLocalHour() {
    struct tm timeinfo;
    if (!::getLocalTime(&timeinfo, 100)) return -1;
    return timeinfo.tm_hour;
}

int getLocalMinute() {
    struct tm timeinfo;
    if (!::getLocalTime(&timeinfo, 100)) return -1;
    return timeinfo.tm_min;
}

bool getLocalTime(struct tm& timeinfo) {
    return ::getLocalTime(&timeinfo, 100);
}

void getTimeString(char* buf, size_t len) {
    struct tm timeinfo;
    if (::getLocalTime(&timeinfo, 100)) {
        strftime(buf, len, "%H:%M", &timeinfo);
    } else {
        snprintf(buf, len, "--:--");
    }
}

void getDateString(char* buf, size_t len) {
    struct tm timeinfo;
    if (::getLocalTime(&timeinfo, 100)) {
        strftime(buf, len, "%Y-%m-%d", &timeinfo);
    } else {
        snprintf(buf, len, "----.--.--");
    }
}

}  // namespace NtpSync

