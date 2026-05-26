#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>

namespace WifiManager {

static unsigned long _lastRetryMs = 0;
static unsigned long _retryIntervalMs = WIFI_RETRY_BASE_MS;
static bool _initialized = false;

void init() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    _initialized = true;
    Serial.println("[WiFi] Initialized in STA mode");
}

bool connect(const char* ssid, const char* password, unsigned long timeoutMs) {
    if (!_initialized) init();

    Serial.printf("[WiFi] Connecting to %s ...\n", ssid);
    WiFi.begin(ssid, password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] Connected! IP: %s  RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        _retryIntervalMs = WIFI_RETRY_BASE_MS;
        return true;
    }

    Serial.println("[WiFi] Connection failed");
    return false;
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        _retryIntervalMs = WIFI_RETRY_BASE_MS;
        return;
    }

    unsigned long now = millis();
    if (now - _lastRetryMs >= _retryIntervalMs) {
        _lastRetryMs = now;
        Serial.printf("[WiFi] Reconnecting (interval %lu ms)...\n", _retryIntervalMs);
        WiFi.reconnect();
        // Exponential back-off
        _retryIntervalMs = min(_retryIntervalMs * 2, (unsigned long)WIFI_RETRY_MAX_MS);
    }
}

bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int getSignalStrength() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.RSSI();
    }
    return 0;
}

}  // namespace WifiManager

