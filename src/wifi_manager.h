#pragma once

#include <Arduino.h>

/// WiFi connection manager with exponential back-off retry logic.
namespace WifiManager {

/// Initialize WiFi in station mode.
void init();

/// Attempt to connect to the configured SSID.
/// Blocks up to `timeoutMs` milliseconds.
/// @return true if connected.
bool connect(const char* ssid, const char* password, unsigned long timeoutMs = 10000);

/// Non-blocking check / reconnect. Call from main loop.
/// Implements exponential back-off internally.
void loop();

/// @return true if currently connected to WiFi.
bool isConnected();

/// @return WiFi RSSI (signal strength) or 0 if not connected.
int getSignalStrength();

}  // namespace WifiManager

