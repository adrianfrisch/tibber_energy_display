#pragma once

#include <Arduino.h>

/// NTP time synchronization.
namespace NtpSync {

/// Configure and start SNTP with the given POSIX timezone string.
/// @param tz POSIX timezone, e.g. "CET-1CEST,M3.5.0,M10.5.0/3"
void init(const char* tz);

/// Force an NTP re-sync. Non-blocking; result available after a few seconds.
void sync();

/// Periodic check — re-syncs every NTP_SYNC_INTERVAL_MS.
void loop();

/// @return true if time has been synced at least once.
bool isSynced();

/// @return current local hour (0–23), or -1 if not synced.
int getLocalHour();

/// @return current local minute (0–59), or -1 if not synced.
int getLocalMinute();

/// Get the current local time as a struct tm.
/// @return true on success.
bool getLocalTime(struct tm& timeinfo);

/// Format current time into a buffer, e.g. "14:32".
void getTimeString(char* buf, size_t len);

/// Format current date into a buffer, e.g. "2026-05-13".
void getDateString(char* buf, size_t len);

}  // namespace NtpSync

