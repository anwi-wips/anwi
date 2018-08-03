#include "geofence.h"

//geofence Settings
String MAC_TO_PROTECT = "";
const char* WEAK_PASSWORD = "123456789";
float Anwi_Power = 20.5;
float Strength_Baseline = 40;

//get RSSI value for given ssid
int32_t get_RSSI(const char* target_ssid) 
{
  byte available_networks = WiFi.scanNetworks();
  for (int network = 0; network < available_networks; network++) 
  {
    if (strcmp(WiFi.SSID(network).c_str(), target_ssid) == 0) 
    {
      return WiFi.RSSI(network);
    }
  }
  return 0;
}

// Get mac address of given ssid
String get_mac(const char* target_ssid) 
{
  byte available_networks = WiFi.scanNetworks();
  for (int network = 0; network < available_networks; network++) 
  {
    if (strcmp(WiFi.SSID(network).c_str(), target_ssid) == 0) 
    {
      return WiFi.BSSIDstr(network);
    }
  }
  return String("");
}

//Return RSSI if MAC is provided
int32_t getRSSI_mac(const char* target_mac) 
{
  byte available_networks = WiFi.scanNetworks();
  for (int network = 0; network < available_networks; network++) 
  {
    if (strcmp(WiFi.BSSIDstr(network).c_str(), target_mac) == 0) 
    {
      return WiFi.RSSI(network);
    }
  }
  return 0;
}

//set transmission power for geofence
void set_transmission_power(int32_t strength)
{
  strength = strength *(-1);
  Anwi_Power = 20.5 - (strength-Strength_Baseline)/2; // mapping 40 to 80 with 0 20.5.
  WiFi.setOutputPower(Anwi_Power);
  Serial.printf("Input signal strength = -%ddbm. Anwi power set to %f dBm\n",strength,Anwi_Power);
}

// recalibrate geofence transmission power
void recalibrate_transmission_power()
{
    if(strcmp(MAC_TO_PROTECT.c_str(),"")!= 0)
    {
        int32_t target_ap_strength = 0;
        target_ap_strength = getRSSI_mac(MAC_TO_PROTECT.c_str());
        set_transmission_power(target_ap_strength);
    }
}

// print signal strength value in dBm
void print_signal_strength(int32_t rssi)
{
  Serial.print("Target Signal strength: ");
  Serial.print(rssi);
  Serial.println("dBm");
}

//setup anwi geofencing
void setup_hotspot(const char * SSID)
{
  boolean result = WiFi.softAP(SSID, WEAK_PASSWORD);
  if(result == true)
  {
    Serial.println("ANWI GeoFencing Ready");
  }
  else
  {
    Serial.println("ANWI GeoFencing Failed!");
  }
}

void setup_geofence(const char * SSID)
{
  int32_t target_ap_strength = 0;
  Serial.println("Starting ANWI GeoFence");

  MAC_TO_PROTECT = get_mac(SSID).c_str();
  Serial.printf("Protecting SSID = %s MAC = %s",SSID,MAC_TO_PROTECT.c_str());

  target_ap_strength = getRSSI_mac(MAC_TO_PROTECT.c_str());
  Serial.print("\nTarget Signal Strength is");
  print_signal_strength(target_ap_strength);
  delay(3000);

  Serial.printf("\nCalibrating signal strength for SSID %s\n",SSID);
  set_transmission_power(target_ap_strength);

  Serial.println("\nActivating Geo-Fencing");
  setup_hotspot(SSID);
  set_transmission_power(target_ap_strength);
}