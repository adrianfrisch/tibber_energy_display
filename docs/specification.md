# Electricity Price Dashboard – Specification & Technical Design

## 1. Project Overview

An ESP32-based electricity price dashboard using the **Cheap Yellow Display (CYD)** board (ESP32-2432S028R) that fetches day-ahead electricity prices from the **Tibber GraphQL API** and presents them on the built-in 2.8" TFT display. The device operates autonomously, refreshing data once daily, and provides an at-a-glance view of current and upcoming electricity prices.

---

## 2. Goals & Objectives

| # | Goal | Priority |
|---|------|----------|
| G1 | Display today's hourly electricity prices as a 24-hour bar chart | Must |
| G2 | Highlight the current hour's price | Must |
| G3 | Show tomorrow's prices once available (typically after 13:00 CET) | Must |
| G4 | Display current price, daily min, max, and average | Must |
| G5 | Indicate price level with color coding (cheap / normal / expensive) | Must |
| G6 | Auto-connect to WiFi and recover from connectivity loss | Must |
| G7 | Synchronize time via NTP | Must |
| G8 | Low power idle between refreshes | Should |
| G9 | Touch-based UI to toggle between today / tomorrow view | Should |
| G10 | OTA firmware update support | Could |
| G11 | Display currency and unit (e.g., ct/kWh) configurable | Should |

---

## 3. Hardware Specification

### 3.1 Board: Cheap Yellow Display (CYD) – ESP32-2432S028R

| Component | Specification |
|-----------|---------------|
| MCU | ESP32-WROOM-32 (dual-core 240 MHz, 520 KB SRAM, 4 MB Flash) |
| Display | 2.8" ILI9341 TFT, 320×240 pixels, 65K colors, SPI interface |
| Touch | XPT2046 resistive touch controller (SPI) |
| Connectivity | WiFi 802.11 b/g/n, Bluetooth 4.2 |
| Power | Micro-USB 5V (some revisions USB-C) |
| Storage | 4 MB flash (SPIFFS/LittleFS partition) |
| GPIO | SD card slot, RGB LED, LDR (light sensor), speaker header |
| Dimensions | ~86 × 50 mm |

### 3.2 Pin Mapping (CYD Rev 2.x)

| Function | Pin(s) |
|----------|--------|
| TFT MOSI | GPIO 13 |
| TFT SCLK | GPIO 14 |
| TFT CS | GPIO 15 |
| TFT DC | GPIO 2 |
| TFT RST | GPIO -1 (tied to EN) |
| TFT Backlight | GPIO 21 |
| Touch CS | GPIO 33 |
| Touch IRQ | GPIO 36 |
| SD CS | GPIO 5 |
| RGB LED (R/G/B) | GPIO 4 / 16 / 17 |
| LDR | GPIO 34 (ADC) |

> **Note:** Pin assignments vary between CYD hardware revisions. Verify against the actual board silkscreen.

---

## 4. Software Architecture

### 4.1 Technology Stack

| Layer | Technology |
|-------|-----------|
| Framework | Arduino (via PlatformIO) or ESP-IDF |
| Graphics | TFT_eSPI (or LovyanGFX) library |
| Touch | TFT_eSPI touch driver / XPT2046_Touchscreen |
| HTTP Client | WiFiClientSecure + HTTPClient (ESP32 Arduino) |
| JSON Parser | ArduinoJson v7 |
| Time Sync | ESP32 built-in SNTP (`configTime()`) |
| File System | LittleFS (for config & cached price data) |
| Build System | PlatformIO |

### 4.2 High-Level Architecture

```
┌─────────────────────────────────────────────────────┐
│                   ESP32 (CYD)                       │
│                                                     │
│  ┌──────────┐  ┌────────────┐  ┌────────────────┐  │
│  │  WiFi    │  │  NTP Time  │  │  Tibber API    │  │
│  │  Manager │  │  Sync      │  │  Client        │  │
│  └────┬─────┘  └─────┬──────┘  └───────┬────────┘  │
│       │              │                  │           │
│       ▼              ▼                  ▼           │
│  ┌─────────────────────────────────────────────┐    │
│  │            Application Controller           │    │
│  │  - Scheduler (fetch timing logic)           │    │
│  │  - State machine (startup/run/error/sleep)  │    │
│  └──────────────────┬──────────────────────────┘    │
│                     │                               │
│       ┌─────────────┴─────────────┐                 │
│       ▼                           ▼                 │
│  ┌──────────┐            ┌──────────────┐           │
│  │  Price   │            │  Display     │           │
│  │  Data    │            │  Renderer    │           │
│  │  Store   │            │  (TFT_eSPI)  │           │
│  │ LittleFS │            │  + Touch     │           │
│  └──────────┘            └──────────────┘           │
│                                                     │
└─────────────────────────────────────────────────────┘
           │                        ▲
           ▼                        │
    ┌─────────────┐          ┌──────────┐
    │ Tibber API  │          │  User    │
    │ (HTTPS/GQL) │          │  (Touch) │
    └─────────────┘          └──────────┘
```

### 4.3 Module Breakdown

#### 4.3.1 WiFi Manager
- Connect to configured SSID/password (stored in LittleFS as `config.json`).
- Retry with exponential back-off on failure.
- Optional: AP mode for initial provisioning (captive portal).

#### 4.3.2 NTP Time Sync
- Sync with `pool.ntp.org` on boot and every 6 hours.
- Timezone: configurable, default `CET-1CEST,M3.5.0,M10.5.0/3`.
- All scheduling logic uses local time.

#### 4.3.3 Tibber API Client
- **Endpoint:** `https://api.tibber.com/v1-beta/gql`
- **Auth:** Bearer token (Personal Access Token) in `Authorization` header.
- **Transport:** HTTPS (TLS 1.2) via `WiFiClientSecure`; use Tibber's root CA cert or `setInsecure()` for development.
- **Query:**

```graphql
{
  viewer {
    homes {
      currentSubscription {
        priceInfo {
          current {
            total
            energy
            tax
            startsAt
            currency
            level
          }
          today {
            total
            energy
            tax
            startsAt
            currency
            level
          }
          tomorrow {
            total
            energy
            tax
            startsAt
            currency
            level
          }
        }
      }
    }
  }
}
```

- **Response Handling:** Parse JSON with ArduinoJson; extract arrays of 24 price entries for today and (optionally) tomorrow.
- **Error Handling:** HTTP errors, JSON parse errors, empty `tomorrow` array (not yet published) → log and retry later.

#### 4.3.4 Price Data Store
- In-memory struct arrays for today (24 entries) and tomorrow (0 or 24 entries).
- Persist last-fetched data to LittleFS (`/prices_today.json`, `/prices_tomorrow.json`) to survive reboots.
- Data structure per entry:

```cpp
struct PriceEntry {
    uint8_t  hour;        // 0–23
    float    total;       // total price incl. tax (ct/kWh or öre/kWh)
    float    energy;      // energy component
    float    tax;         // tax component
    char     currency[4]; // "EUR", "SEK", "NOK"
    char     level[16];   // "VERY_CHEAP", "CHEAP", "NORMAL", "EXPENSIVE", "VERY_EXPENSIVE"
};
```

#### 4.3.5 Display Renderer
- **Library:** TFT_eSPI configured for ILI9341 + CYD pin mapping.
- **Layout (320×240, landscape):**

```
┌──────────────────────────────────────────────┐
│  ⚡ Electricity Prices    Today  14:32  WiFi │  ← Header (20 px)
├──────────────────────────────────────────────┤
│                                              │
│  ██                                          │
│  ██       ██                          ██     │
│  ██  ██   ██  ██                 ██   ██     │
│  ██  ██   ██  ██  ██  ██  ██    ██   ██  ██ │  ← Bar chart area
│  ██  ██   ██  ██  ██  ██  ██    ██   ██  ██ │    (170 px)
│  ██  ██   ██  ██  ██  ██  ██    ██   ██  ██ │
│  00  03   06  09  12  15  18    21   23     │
├──────────────────────────────────────────────┤
│  Now: 28.5 ct  Min: 12.1  Max: 45.2  Ø 24.8│  ← Stats bar (30 px)
├──────────────────────────────────────────────┤
│  [  TODAY  ]  [ TOMORROW ]                   │  ← Touch nav (20 px)
└──────────────────────────────────────────────┘
```

- **Bar Chart:**
  - 24 bars, each ≈11 px wide with 1 px gap → fits in ~290 px.
  - Bar height proportional to price; Y-axis auto-scaled to daily min/max.
  - Current hour bar has a distinct highlight border / pulsing effect.
- **Color Coding:**

| Tibber Level | Color | Hex |
|-------------|-------|-----|
| VERY_CHEAP | Bright Green | `#00FF00` |
| CHEAP | Green | `#00CC00` |
| NORMAL | Yellow | `#FFCC00` |
| EXPENSIVE | Orange | `#FF6600` |
| VERY_EXPENSIVE | Red | `#FF0000` |

- **Partial screen updates** using sprites (TFT_eSPI sprite buffer) to avoid flicker.
- Redraw bar chart on hour change; full redraw on data fetch.

#### 4.3.6 Touch Handler
- Detect tap regions:
  - "TODAY" button → show today's prices.
  - "TOMORROW" button → show tomorrow's prices (if available; greyed out otherwise).
- Debounce touch input (200 ms minimum interval).

#### 4.3.7 Application Controller / Scheduler

**State Machine:**

```
          ┌──────┐
          │ BOOT │
          └──┬───┘
             ▼
      ┌──────────────┐
      │ WIFI_CONNECT  │◄──── retry (backoff)
      └──────┬───────┘
             ▼
      ┌──────────────┐
      │  NTP_SYNC    │
      └──────┬───────┘
             ▼
      ┌──────────────┐
      │ FETCH_PRICES │◄──── scheduled trigger
      └──────┬───────┘
             ▼
      ┌──────────────┐
      │   DISPLAY    │◄──── hourly refresh / touch event
      └──────┬───────┘
             ▼
      ┌──────────────┐
      │    IDLE      │ ─── loop: check hour change, touch, schedule
      └──────────────┘
```

**Fetch Schedule:**
| Event | Time | Action |
|-------|------|--------|
| Boot fetch | On startup | Fetch today + tomorrow prices |
| Morning refresh | 00:15 local time | Fetch today's (now new day) prices |
| Afternoon fetch | 13:15 local time | Fetch tomorrow's prices (day-ahead published ~13:00 CET) |
| Retry on failure | +5 min, +15 min, +60 min | Exponential back-off |

---

## 5. Configuration

Configuration stored in `/config.json` on LittleFS:

```json
{
  "wifi_ssid": "MyNetwork",
  "wifi_password": "secret",
  "tibber_token": "d1007ead2dc84a2b82f0de19451c5fb22112f7ae11d19bf2bedb224a003ff74a",
  "timezone": "CET-1CEST,M3.5.0,M10.5.0/3",
  "home_index": 0,
  "currency_unit": "ct/kWh",
  "price_multiplier": 100.0,
  "display_brightness": 80
}
```

- `home_index`: Index into the Tibber `homes[]` array (for users with multiple homes).
- `price_multiplier`: Tibber returns EUR/kWh; multiply by 100 for ct/kWh display.
- Initial provisioning: upload `config.json` via PlatformIO filesystem upload, or implement a setup AP/captive portal.

---

## 6. Data Flow

```
1. Device boots → connect WiFi → sync NTP
2. Read cached prices from LittleFS (show immediately if valid for today)
3. HTTPS POST to Tibber GraphQL API
4. Parse JSON response → populate PriceEntry arrays
5. Persist to LittleFS
6. Render bar chart + stats on TFT display
7. Enter idle loop:
   a. Every 60s: check if hour changed → update highlight bar + current price
   b. At 00:15: fetch new "today" prices
   c. At 13:15: fetch "tomorrow" prices
   d. On touch: toggle today/tomorrow view
   e. Every 6h: re-sync NTP
```

---

## 7. Error Handling & Resilience

| Scenario | Handling |
|----------|----------|
| WiFi unreachable | Retry with exponential back-off (2s → 4s → 8s … max 5 min). Show "No WiFi" icon. |
| Tibber API error (HTTP 4xx/5xx) | Retry schedule. Display stale cached data with "last updated" timestamp. |
| Tibber API token invalid (401) | Display "Auth Error – check token" message. |
| Empty tomorrow prices | Normal before ~13:00 CET; show "Tomorrow: not yet available". |
| JSON parse failure | Log error, use cached data, retry at next interval. |
| NTP sync failure | Retry every 60s; if no time, display "Time not synced". |
| Flash write failure | Continue with in-memory data only; log warning. |
| Watchdog | Enable ESP32 task watchdog (30s timeout) to auto-reset on hang. |

---

## 8. Security Considerations

- **Tibber API Token:** Stored in flash (LittleFS). Not a high-security context, but avoid logging the token to serial.
- **TLS:** Use certificate pinning or at minimum validate the Tibber API server certificate. Bundle the DigiCert root CA used by `api.tibber.com`.
- **OTA Updates (if implemented):** Password-protected; HTTPS preferred.
- **WiFi Password:** Stored in plaintext in flash (standard for ESP32 projects; hardware access = game over anyway).

---

## 9. Project Structure

```
electricity-price-dashboard/
├── platformio.ini              # PlatformIO build config
├── src/
│   ├── main.cpp                # Setup + main loop, state machine
│   ├── config.h                # Compile-time defaults, pin definitions
│   ├── wifi_manager.h / .cpp   # WiFi connection & reconnection
│   ├── ntp_sync.h / .cpp       # NTP time synchronization
│   ├── tibber_client.h / .cpp  # Tibber GraphQL API client
│   ├── price_store.h / .cpp    # Price data structures, LittleFS persistence
│   ├── display.h / .cpp        # TFT rendering (bar chart, stats, UI)
│   ├── touch_handler.h / .cpp  # Touch input processing
│   └── scheduler.h / .cpp      # Timed event scheduler
├── data/
│   └── config.json             # User configuration (uploaded via LittleFS)
├── certs/
│   └── tibber_ca.h             # Root CA certificate for api.tibber.com
├── docs/
│   └── specification.md        # This document
└── README.md
```

---

## 10. PlatformIO Configuration

```ini
[env:cyd]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
board_build.partitions = min_spiffs.csv
lib_deps =
    bodmer/TFT_eSPI@^2.5.0
    bblanchon/ArduinoJson@^7.0.0
build_flags =
    -DUSER_SETUP_LOADED=1
    ; TFT_eSPI ILI9341 CYD configuration
    -DILI9341_DRIVER=1
    -DTFT_WIDTH=240
    -DTFT_HEIGHT=320
    -DTFT_MOSI=13
    -DTFT_SCLK=14
    -DTFT_CS=15
    -DTFT_DC=2
    -DTFT_RST=-1
    -DTFT_BL=21
    -DTOUCH_CS=33
    -DSPI_FREQUENCY=55000000
    -DSPI_READ_FREQUENCY=20000000
    -DSPI_TOUCH_FREQUENCY=2500000
```

---

## 11. API Reference (Tibber)

- **Docs:** https://developer.tibber.com/docs/overview
- **Explorer:** https://developer.tibber.com/explorer
- **Endpoint:** `https://api.tibber.com/v1-beta/gql`
- **Rate Limits:** ~100 requests/day for personal tokens (our usage: 2–5/day).
- **Auth:** HTTP header `Authorization: Bearer <token>`
- **Response (excerpt):**

```json
{
  "data": {
    "viewer": {
      "homes": [{
        "currentSubscription": {
          "priceInfo": {
            "current": {
              "total": 0.2851,
              "startsAt": "2026-05-13T14:00:00+02:00",
              "level": "NORMAL"
            },
            "today": [
              { "total": 0.2105, "startsAt": "2026-05-13T00:00:00+02:00", "level": "CHEAP" },
              { "total": 0.1982, "startsAt": "2026-05-13T01:00:00+02:00", "level": "VERY_CHEAP" }
              // ... 24 entries
            ],
            "tomorrow": [
              // 24 entries or empty array if not yet published
            ]
          }
        }
      }]
    }
  }
}
```

---

## 12. Display Mockups

### 12.1 Main View (Today)

```
┌──────────────────────────────────────────────────┐
│ ⚡ Electricity Prices       Today 14:32     📶  │
├──────────────────────────────────────────────────┤
│  45 ┤                                            │
│     │        ▓▓                                  │
│  35 ┤     ▓▓ ▓▓ ▓▓                              │
│     │  ▓▓ ▓▓ ▓▓ ▓▓ ▓▓          ▓▓              │
│  25 ┤  ▓▓ ▓▓ ▓▓ ▓▓ ▓▓ ██ ▓▓   ▓▓ ▓▓           │
│     │  ▓▓ ▓▓ ▓▓ ▓▓ ▓▓ ██ ▓▓ ▓▓▓▓ ▓▓ ▓▓        │
│  15 ┤  ▓▓ ▓▓ ▓▓ ▓▓ ▓▓ ██ ▓▓ ▓▓▓▓ ▓▓ ▓▓ ▓▓     │
│     │▓▓▓▓ ▓▓ ▓▓ ▓▓ ▓▓ ██ ▓▓ ▓▓▓▓ ▓▓ ▓▓ ▓▓ ▓▓  │
│   5 ┤▓▓▓▓ ▓▓ ▓▓ ▓▓ ▓▓ ██ ▓▓ ▓▓▓▓ ▓▓ ▓▓ ▓▓ ▓▓  │
│     └──────────────────────────────────────────  │
│      00  02  04  06  08  10  12  14  16  18  22  │
├──────────────────────────────────────────────────┤
│  Now: 28.5 ct/kWh  Min: 5.2  Max: 42.1  Ø 22.3 │
├──────────────────────────────────────────────────┤
│     [ ● TODAY ]         [ ○ TOMORROW ]           │
└──────────────────────────────────────────────────┘
```

`██` = current hour (highlighted), `▓▓` = other hours (color-coded by level)

### 12.2 Error State

```
┌──────────────────────────────────────────────────┐
│ ⚡ Electricity Prices                       ✕   │
├──────────────────────────────────────────────────┤
│                                                  │
│              ⚠ WiFi Disconnected                │
│                                                  │
│            Showing cached data from              │
│              2026-05-12 23:15                     │
│                                                  │
│             Retrying in 45s ...                   │
│                                                  │
├──────────────────────────────────────────────────┤
│  (stale bar chart still visible)                 │
└──────────────────────────────────────────────────┘
```

---

## 13. Testing Strategy

| Test Type | Approach |
|-----------|----------|
| Unit (data parsing) | Run ArduinoJson parsing on host (PlatformIO native test) with sample API responses |
| Integration (API) | Test against Tibber API with a demo token (`d1007ead...`) in Tibber's sandbox |
| Display (visual) | Manual verification on hardware; screenshot via TFT_eSPI BMP export |
| Resilience | Simulate WiFi loss, invalid JSON, expired token, empty tomorrow array |
| Long-running | 72h soak test to verify memory stability (heap monitoring via serial) |

---

## 14. Future Enhancements (Backlog)

- **Web-based configuration portal** (captive portal AP mode for WiFi + token setup).
- **Tibber Pulse / real-time consumption** overlay on the chart.
- **Multiple tariff support** (grid fees, markups) for total cost display.
- **MQTT integration** for Home Assistant / other smart home systems.
- **Ambient light auto-brightness** using the on-board LDR.
- **Deep sleep** between hours for battery-powered operation.
- **Localization** (language, date format, currency).
- **3D-printable enclosure** design.

---

## 15. References

- [Cheap Yellow Display (CYD) Documentation](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
- [Tibber Developer API](https://developer.tibber.com/)
- [ArduinoJson](https://arduinojson.org/)
- [PlatformIO ESP32](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
- [ESP32 Arduino SNTP](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html)

---

*Document version: 1.0 — 2026-05-13*

