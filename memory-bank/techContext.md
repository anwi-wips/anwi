# Technical Context

## Development Environment
-   **Platform**: PlatformIO Core 6.1.18
-   **Framework**: Arduino for ESP8266
-   **Board**: Wemos D1 Mini (`d1_mini`)
-   **Language**: C++

## Dependencies
-   `ESP8266WiFi`: Core WiFi functionality.
-   `ArduinoJson@5`: JSON parsing and serialization (Version 5 is strictly required due to legacy code usage).
-   `ESP8266WebServer`: For the configuration portal.
-   `RF24`: Driver for NRF24L01+ transceivers.
-   `RF24Network`: Network layer for NRF24.

## Compilation Notes
-   **Incompatibility**: The `nrf_to_nrf` library is explicitly ignored (`lib_ignore`) in `platformio.ini` as it causes conflicts.
-   **API Changes**: `HTTPClient` usage has been patched to include `WiFiClient` in `begin()` calls to support newer ESP8266 core versions.

## Project Structure
```
anwi-master/
├── lib/
│   ├── alerts/          # Alerting logic (IFTTT, Server, NRF)
│   ├── config/          # EEPROM and Web Config
│   ├── packet_capture/  # Sniffer logic
│   ├── protection/      # Geofencing logic
│   └── ...
├── src/
│   └── main.cpp         # Entry point and main loop
├── docs/                # Documentation and website assets
└── platformio.ini       # Build configuration
```
