#include "geofence.h"

// geofence Settings
String MAC_TO_PROTECT = "";
const char *WEAK_PASSWORD = "123456789";
float Anwi_Power = 20.5;
float Strength_Baseline = 40;

// get RSSI value for given ssid
/**
 * @brief Gets the RSSI value for a given SSID.
 *
 * Scans available networks and returns the RSSI of the target SSID.
 *
 * @param target_ssid The SSID to search for.
 * @return int32_t The RSSI value, or 0 if not found.
 */
int32_t get_RSSI(const char *target_ssid) {
  byte available_networks = WiFi.scanNetworks();
  for (int network = 0; network < available_networks; network++) {
    if (strcmp(WiFi.SSID(network).c_str(), target_ssid) == 0) {
      return WiFi.RSSI(network);
    }
  }
  return 0;
}

// Get mac address of given ssid
/**
 * @brief Gets the MAC address of a given SSID.
 *
 * Scans available networks and returns the BSSID (MAC) of the target SSID.
 *
 * @param target_ssid The SSID to search for.
 * @return String The MAC address as a string, or empty string if not found.
 */
String get_mac(const char *target_ssid) {
  byte available_networks = WiFi.scanNetworks();
  for (int network = 0; network < available_networks; network++) {
    if (strcmp(WiFi.SSID(network).c_str(), target_ssid) == 0) {
      return WiFi.BSSIDstr(network);
    }
  }
  return String("");
}

// Return RSSI if MAC is provided
/**
 * @brief Gets the RSSI for a given MAC address.
 *
 * Scans available networks and returns the RSSI of the target BSSID (MAC).
 *
 * @param target_mac The MAC address to search for.
 * @return int32_t The RSSI value, or 0 if not found.
 */
int32_t getRSSI_mac(const char *target_mac) {
  byte available_networks = WiFi.scanNetworks();
  for (int network = 0; network < available_networks; network++) {
    if (strcmp(WiFi.BSSIDstr(network).c_str(), target_mac) == 0) {
      return WiFi.RSSI(network);
    }
  }
  return 0;
}

// set transmission power for geofence
/**
 * @brief Sets the transmission power for geofencing.
 *
 * Adjusts the WiFi output power based on the target signal strength to create
 * a geofence boundary.
 *
 * @param strength The target signal strength (RSSI).
 */
void set_transmission_power(int32_t strength) {
  strength = strength * (-1);
  Anwi_Power = 20.5 - (strength - Strength_Baseline) /
                          2; // mapping 40 to 80 with 0 20.5.
  WiFi.setOutputPower(Anwi_Power);
  Serial.printf("Input signal strength = -%ddbm. Anwi power set to %f dBm\n",
                strength, Anwi_Power);
}

// recalibrate geofence transmission power
/**
 * @brief Recalibrates the geofence transmission power.
 *
 * Measures the current RSSI of the protected AP and adjusts the transmission
 * power accordingly.
 */
void recalibrate_transmission_power() {
  if (strcmp(MAC_TO_PROTECT.c_str(), "") != 0) {
    int32_t target_ap_strength = 0;
    target_ap_strength = getRSSI_mac(MAC_TO_PROTECT.c_str());
    set_transmission_power(target_ap_strength);
  }
}

// print signal strength value in dBm
/**
 * @brief Prints the signal strength value in dBm to Serial.
 *
 * @param rssi The RSSI value to print.
 */
void print_signal_strength(int32_t rssi) {
  Serial.print("Target Signal strength: ");
  Serial.print(rssi);
  Serial.println("dBm");
}

// setup anwi geofencing
/**
 * @brief Sets up the ANWI geofencing hotspot.
 *
 * Creates a SoftAP with the target SSID and a weak password to lure devices
 * within the geofence.
 *
 * @param SSID The SSID to broadcast.
 */
void setup_hotspot(const char *SSID) {
  boolean result = WiFi.softAP(SSID, WEAK_PASSWORD);
  if (result == true) {
    Serial.println("ANWI GeoFencing Ready");
  } else {
    Serial.println("ANWI GeoFencing Failed!");
  }
}

/**
 * @brief Initializes ANWI geofencing.
 *
 * Gets the MAC and RSSI of the target AP, calibrates the transmission power,
 * and sets up the hotspot.
 *
 * @param SSID The SSID to protect.
 */
void setup_geofence(const char *SSID) {
  int32_t target_ap_strength = 0;
  Serial.println("Starting ANWI GeoFence");

  MAC_TO_PROTECT = get_mac(SSID).c_str();
  Serial.printf("Protecting SSID = %s MAC = %s", SSID, MAC_TO_PROTECT.c_str());

  target_ap_strength = getRSSI_mac(MAC_TO_PROTECT.c_str());
  Serial.print("\nTarget Signal Strength is");
  print_signal_strength(target_ap_strength);
  delay(3000);

  Serial.printf("\nCalibrating signal strength for SSID %s\n", SSID);
  set_transmission_power(target_ap_strength);

  Serial.println("\nActivating Geo-Fencing");
  setup_hotspot(SSID);
  set_transmission_power(target_ap_strength);
}