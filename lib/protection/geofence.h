#ifndef _GEOFENCE_H_
#define _GEOFENCE_H_

#include <Arduino.h>
#include <ESP8266WiFi.h>

//get RSSI value for given ssid
int32_t get_RSSI(const char* target_ssid);

// Get mac address of given ssid
String get_mac(const char* target_ssid);

//Return RSSI if MAC is provided
int32_t getRSSI_mac(const char* target_mac);

//set transmission power for geofence
void set_transmission_power(int32_t strength);

// recalibrate geofence transmission power
void recalibrate_transmission_power();
// print signal strength value in dBm
void print_signal_strength(int32_t rssi);

//setup hotspot
void setup_hotspot(const char * SSID);

//setup anwi geofencing
void setup_geofence(const char * SSID);

#endif