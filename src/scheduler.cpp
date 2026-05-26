#include "scheduler.h"
#include "config.h"
#include "ntp_sync.h"

namespace Scheduler {

static int _currentHour = -1;
static int _previousHour = -1;
static int _currentMinute = -1;

static bool _morningFetchDone = false;
static bool _afternoonFetchDone = false;

static unsigned long _lastDisplayRefreshMs = 0;
static unsigned long _lastNtpSyncMs = 0;

// Retry logic
static bool _retryActive = false;
static uint8_t _retryIndex = 0;
static unsigned long _retryStartMs = 0;
static const unsigned long _retryIntervals[] = FETCH_RETRY_INTERVALS;
static const uint8_t _retryCount = sizeof(_retryIntervals) / sizeof(_retryIntervals[0]);

void init() {
    _currentHour = NtpSync::getLocalHour();
    _previousHour = _currentHour;
    _currentMinute = NtpSync::getLocalMinute();
    _lastDisplayRefreshMs = millis();
    _lastNtpSyncMs = millis();
    _morningFetchDone = false;
    _afternoonFetchDone = false;
    _retryActive = false;
    Serial.printf("[Scheduler] Initialized (hour: %d)\n", _currentHour);
}

void loop() {
    int newHour = NtpSync::getLocalHour();
    int newMinute = NtpSync::getLocalMinute();

    if (newHour != _currentHour && newHour >= 0) {
        _previousHour = _currentHour;
        _currentHour = newHour;

        // Reset daily flags at midnight
        if (_currentHour == 0) {
            _morningFetchDone = false;
            _afternoonFetchDone = false;
        }
    }
    _currentMinute = newMinute;
}

bool isMorningFetchDue() {
    if (_morningFetchDone) return false;
    if (_currentHour == MORNING_FETCH_HOUR && _currentMinute >= MORNING_FETCH_MINUTE) {
        return true;
    }
    return false;
}

bool isAfternoonFetchDue() {
    if (_afternoonFetchDone) return false;
    if (_currentHour == AFTERNOON_FETCH_HOUR && _currentMinute >= AFTERNOON_FETCH_MINUTE) {
        return true;
    }
    // Also fetch if we missed the window (e.g., device was off)
    if (_currentHour > AFTERNOON_FETCH_HOUR) {
        return true;
    }
    return false;
}

bool hasHourChanged() {
    return _currentHour != _previousHour && _currentHour >= 0;
}

bool isNtpSyncDue() {
    return (millis() - _lastNtpSyncMs) >= NTP_SYNC_INTERVAL_MS;
}

bool isDisplayRefreshDue() {
    if ((millis() - _lastDisplayRefreshMs) >= DISPLAY_REFRESH_MS) {
        _lastDisplayRefreshMs = millis();
        return true;
    }
    return false;
}

void markFetchCompleted() {
    // Determine which fetch was completed based on current time
    if (_currentHour <= AFTERNOON_FETCH_HOUR) {
        _morningFetchDone = true;
    } else {
        _afternoonFetchDone = true;
    }
    _retryActive = false;
    _retryIndex = 0;
    Serial.println("[Scheduler] Fetch completed");
}

void markFetchFailed() {
    if (!_retryActive) {
        _retryActive = true;
        _retryIndex = 0;
        _retryStartMs = millis();
        Serial.println("[Scheduler] Fetch failed, starting retry timer");
    }
}

bool isRetryDue() {
    if (!_retryActive) return false;
    if (_retryIndex >= _retryCount) {
        // All retries exhausted; reset and wait for next scheduled fetch
        _retryActive = false;
        _retryIndex = 0;
        return false;
    }
    if ((millis() - _retryStartMs) >= _retryIntervals[_retryIndex]) {
        _retryIndex++;
        _retryStartMs = millis();
        Serial.printf("[Scheduler] Retry %d/%d\n", _retryIndex, _retryCount);
        return true;
    }
    return false;
}

int getCurrentHour() { return _currentHour; }
int getPreviousHour() { return _previousHour; }

}  // namespace Scheduler

