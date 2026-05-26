#pragma once

#include <Arduino.h>

/// Timed event scheduler for periodic tasks.
namespace Scheduler {

/// Initialize the scheduler.
void init();

/// Call from main loop. Checks all scheduled events and fires callbacks.
void loop();

/// @return true if it's time for the morning price fetch (00:15).
bool isMorningFetchDue();

/// @return true if it's time for the afternoon price fetch (13:15).
bool isAfternoonFetchDue();

/// @return true if the hour has changed since last check.
bool hasHourChanged();

/// @return true if an NTP re-sync is due.
bool isNtpSyncDue();

/// @return true if display should be refreshed (every minute).
bool isDisplayRefreshDue();

/// Mark a fetch as completed (resets retry timer).
void markFetchCompleted();

/// Mark a fetch as failed (starts retry timer).
void markFetchFailed();

/// @return true if a retry fetch is due after a failure.
bool isRetryDue();

/// Get the current hour (cached from last check).
int getCurrentHour();

/// Get the previous hour (before the last change).
int getPreviousHour();

}  // namespace Scheduler

