# Electricity Price Dashboard

ESP32-based electricity price display using the **Cheap Yellow Display (CYD)** board, fetching day-ahead prices from the **Tibber GraphQL API**.

![Board: ESP32-2432S028R](https://img.shields.io/badge/Board-CYD%20ESP32--2432S028R-yellow)
![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-blue)
![Build: PlatformIO](https://img.shields.io/badge/Build-PlatformIO-orange)

## Features

- 📊 24-hour bar chart of electricity prices
- 🎨 Color-coded by price level (green → red)
- ⏰ Auto-updates at midnight and 13:15 (when tomorrow's prices are published)
- 📱 Touch-based navigation between Today / Tomorrow views
- 💾 Cached data survives reboots
- 📶 Auto WiFi reconnection with exponential back-off

## Hardware

- [Cheap Yellow Display (ESP32-2432S028R)](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
- 2.8" ILI9341 TFT (320×240), resistive touch, WiFi

## Quick Start

### Prerequisites
- [PlatformIO](https://platformio.org/) (CLI or IDE extension)
- A [Tibber account](https://tibber.com/) with an API token from [developer.tibber.com](https://developer.tibber.com/)

### 1. Configure
Edit `data/config.json` with your credentials:
```json
{
  "wifi_ssid": "YourNetwork",
  "wifi_password": "YourPassword",
  "tibber_token": "your-tibber-api-token",
  "timezone": "CET-1CEST,M3.5.0,M10.5.0/3",
  "home_index": 0,
  "price_multiplier": 100.0,
  "display_brightness": 80
}
```

### 2. Upload Filesystem
```bash
pio run --target uploadfs
```

### 3. Build & Flash
```bash
pio run --target upload
```

### 4. Monitor
```bash
pio device monitor
```

## Project Structure

```
├── platformio.ini              # Build configuration
├── src/
│   ├── main.cpp                # Application entry point & state machine
│   ├── config.h                # Pin definitions & constants
│   ├── wifi_manager.h/.cpp     # WiFi connection management
│   ├── ntp_sync.h/.cpp         # NTP time synchronization
│   ├── tibber_client.h/.cpp    # Tibber GraphQL API client
│   ├── price_store.h/.cpp      # Price data storage & persistence
│   ├── display.h/.cpp          # TFT display rendering
│   ├── touch_handler.h/.cpp    # Touch input handling
│   ├── scheduler.h/.cpp        # Timed event scheduler
│   └── certs/
│       └── tibber_ca.h         # TLS root CA certificate
├── data/
│   └── config.json             # User configuration
└── docs/
    └── specification.md        # Full specification & design document
```

## 3D Printable Enclosure

A parametric OpenSCAD enclosure is included in [`enclosure/`](enclosure/). It features a two-part snap-fit design with a 15° angled stand for shelf placement and accessible USB port. See [enclosure/README.md](enclosure/README.md) for print instructions.

## Documentation

See [docs/specification.md](docs/specification.md) for the full specification and technical design document.

## License

MIT

