# Product Context

## Problem Statement
Wireless security solutions are often expensive and complex, making them inaccessible to small organizations or home users. Common attacks like "Evil Twin" (creating a fake AP to steal credentials) and "Deauthentication" (disconnecting users) are easy to execute but hard to detect without specialized hardware.

## Solution
ANWI provides a cost-effective WIDS that can be deployed at the physical perimeter of a coverage area. It acts as a sensor that continuously monitors the airwaves for malicious activity and reports it back to a central system or directly to the user.

## User Experience
1.  **Configuration**: Users configure the sensor via a captive portal (Web UI) or serial connection, setting up the target SSID, alert preferences, and operation mode.
2.  **Deployment**: The sensor is powered on and placed in the target area.
3.  **Monitoring**: The device runs autonomously.
4.  **Alerting**: When an attack is detected, the user receives a notification via their chosen channel (e.g., an IFTTT notification on their phone).

## Success Metrics
-   Reliable detection of Deauth and Evil Twin attacks.
-   Low false-positive rate for geofencing.
-   Successful delivery of alerts even in adverse conditions (using NRF24 fallback).
