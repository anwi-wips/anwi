# Project Brief: ANWI (All New Wireless IDS)

ANWI is a low-cost, open-source Wireless Intrusion Detection System (WIDS) designed to run on the ESP8266 platform (specifically Wemos D1 Mini). It aims to democratize wireless security by providing an affordable solution for detecting common WiFi attacks and securing physical perimeters.

## Core Philosophy
-   **Affordability**: Uses cheap, readily available hardware ($5 ESP8266).
-   **Ease of Use**: "Fire and forget" deployment principle.
-   **Versatility**: Supports multiple alerting mechanisms (IFTTT, Local Server, NRF24).

## Key Features
-   **Attack Detection**: Identifies Evil Twin APs and Deauthentication/Disassociation attacks.
-   **Geofencing**: Monitors signal strength (RSSI) to detect unauthorized devices or APs outside a designated physical perimeter.
-   **Multi-Mode Alerting**: Sends alerts via WiFi (IFTTT, HTTP Server) or a separate radio channel (NRF24L01+) for resiliency against WiFi jamming.
-   **Dual Operation Modes**:
    -   **Detection Mode**: Actively sniffs for attacks.
    -   **Protection Mode**: Monitors specific APs and clients for geofencing violations.
