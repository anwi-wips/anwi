#include "alerts.h"
#include "..\global_vars.h"
#include "config.h"
#include "packet_capture.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <esp8266httpclient.h>

#define NRF_CHANNEL 8

#define NRF24_HEARTBEAT 1
#define WIFI_HEARTBEAT 2

bool is_first_alert_sent = false;

// 4,15 for NodeMCU
// 7,8 for Arduino
RF24 radio(4, 15);
RF24Network network(radio);

const uint16_t this_node_address = 01;   // Address of this node
const uint16_t server_node_address = 00; // Address of the server node

const unsigned long interval = 20;

unsigned long last_sent;
unsigned long packets_sent;

uint16_t heartbeat_type = 1;

/**
 * @brief Updates the NRF24 network.
 *
 * This function must be called regularly to keep the network layer active.
 */
void radio_update() { network.update(); }

/**
 * @brief Initializes the NRF24 radio module.
 *
 * Sets up SPI, initializes the radio, sets PA level, and starts the network.
 */
void init_radio() {
  SPI.begin();
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  network.begin(NRF_CHANNEL, this_node_address);
}

/**
 * @brief Sends an alert via NRF24 radio.
 *
 * Constructs a JSON payload with attack details and sends it to the server node
 * using the NRF24 network.
 */
void alert_nrf() {
  Serial.println("Sending RADIO Alert ");
  char data[256];
  char bssid_mac[18], dest_address[18], src_address[18];
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  if (DEBUG_PRINT) {
    Serial.println("Sending RADIO Alert ");
  }

  root["id"] = sensor_config.id;

  // root["sensor_location"] = sensor_config.sensor_location;

  snprintf(bssid_mac, sizeof(bssid_mac), MACSTR,
           MAC2STR(pkt_info.frame_hdr.bssid_address));

  // root["bssid_mac"] = bssid_mac;
  root["rssi"] = pkt_info.rssi;
  root["attack_type"] = pkt_info.attack_type;
  root["channel"] = pkt_info.channel;
  snprintf(dest_address, sizeof(dest_address), MACSTR,
           MAC2STR(pkt_info.frame_hdr.destination_address));
  // root["destination_address"] = dest_address;
  snprintf(src_address, sizeof(src_address), MACSTR,
           MAC2STR(pkt_info.frame_hdr.source_address));
  // root["source_address"] = src_address;
  root["sensor_location"] = "CORE";

  if (pkt_info.attack_type == IS_EVILTWIN_ATTACK) {
    root["bssid_mac"] = bssid_mac;
    root["attackers_mac"] = src_address;
    root["victims_mac"] = bssid_mac;
    root["deauth_reason"] = 4;
  }

  if (pkt_info.attack_type == IS_DEAUTH_ATTACK) {
    root["bssid_mac"] = bssid_mac;
    root["attackers_mac"] = "NA";
    root["victims_mac"] = bssid_mac;
    root["deauth_reason"] = pkt_info.frame_hdr.deauth.reason_code;
  }

  if (pkt_info.attack_type == IS_GEOFENCE_ATTACK) {
    root["bssid_mac"] = bssid_mac;
    root["attackers_mac"] = src_address;
    root["victims_mac"] = dest_address;
    root["deauth_reason"] = 4;
  }

  size_t n = root.printTo(data, sizeof(data));
  RF24NetworkHeader header(server_node_address);
  bool msg_status = network.write(header, data, n);
  Serial.println(data);
  if (DEBUG_PRINT) {
    if (msg_status) {
      Serial.println("Alert Sent using nRF24 successfully");
    } else {
      Serial.println("Alert sending nRF24 failed");
    }
    Serial.println("Alert Sent");
  }
}

/**
 * @brief Connects to the configured WiFi network.
 *
 * Puts the device in Station mode and attempts to connect to the SSID
 * specified in the configuration. Blocks until connected.
 */
void connect_Wifi() {
  delay(10);
  yield();
  WiFi.mode(WIFI_STA);
  if (DEBUG_PRINT) {
    Serial.print("Connecting to : ");
    Serial.println(sensor_config.connect_ap_info.SSID);
  }
  WiFi.begin(sensor_config.connect_ap_info.SSID,
             sensor_config.connect_ap_info.PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  if (DEBUG_PRINT) {
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

/**
 * @brief Sends an alert to IFTTT.
 *
 * Connects to WiFi and sends a JSON payload to the configured IFTTT Webhook
 * URL.
 */
void alert_ifttt() {
  connect_Wifi();
  WiFiClient client;
  HTTPClient http;
  if (DEBUG_PRINT)
    Serial.println("Sending alert to IFTTT");
  // https://bblanchon.github.io/ArduinoJson/example/generator/

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["value1"] = pkt_info.frame_hdr.bssid_address;
  root["value2"] = pkt_info.rssi;

  char data[256];
  size_t n = root.printTo(data, sizeof(data));
  String ifttt_triggerName = "";
  String alert_url = "";
  switch (pkt_info.attack_type) {
  case IS_EVILTWIN_ATTACK:
    ifttt_triggerName = sensor_config.ifttt_info.ifttt_eventName_eviltwin;
    break;
  case IS_DEAUTH_ATTACK:
    ifttt_triggerName = sensor_config.ifttt_info.ifttt_eventName_deauth;
    break;
  case IS_GEOFENCE_ATTACK:
    ifttt_triggerName = sensor_config.ifttt_info.ifttt_eventName_geofence;
    break;
  }
  alert_url = IFTTT_ALERT_URL + ifttt_triggerName + "/with/key/" +
              sensor_config.ifttt_info.ifttt_key;
  if (DEBUG_PRINT) {
    Serial.println(alert_url);
  }
  http.begin(client, alert_url);
  http.addHeader("Content-Type", "application/json");
  http.POST(data);
  http.end();
}

/**
 * @brief Sends an alert to a local server.
 *
 * Connects to WiFi and sends a JSON payload to the configured local server IP.
 */
void alert_server() {
  connect_Wifi();
  WiFiClient client;
  HTTPClient http;
  // https://bblanchon.github.io/ArduinoJson/example/generator/
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  if (DEBUG_PRINT) {
    Serial.println("Sending Alert to server");
  }

  char bssid_mac[18], dest_address[18], src_address[18];
  root["id"] = sensor_config.id;
  root["sensor_location"] = sensor_config.sensor_location;

  snprintf(bssid_mac, sizeof(bssid_mac), MACSTR,
           MAC2STR(pkt_info.frame_hdr.bssid_address));
  root["bssid_mac"] = bssid_mac;
  root["rssi"] = pkt_info.rssi;
  root["attack_type"] = pkt_info.attack_type;
  root["channel"] = pkt_info.channel;
  snprintf(dest_address, sizeof(dest_address), MACSTR,
           MAC2STR(pkt_info.frame_hdr.destination_address));
  root["destination_address"] = dest_address;
  snprintf(src_address, sizeof(src_address), MACSTR,
           MAC2STR(pkt_info.frame_hdr.source_address));
  root["source_address"] = src_address;

  if (pkt_info.attack_type == IS_EVILTWIN_ATTACK) {
    root["sensor_location"] = "CORE";
    root["attackers_mac"] = src_address;
    root["victims_mac"] = bssid_mac;
  }

  if (pkt_info.attack_type == IS_DEAUTH_ATTACK) {
    root["sensor_location"] = "CORE";
    root["attackers_mac"] = "NA";
    root["victims_mac"] = bssid_mac;
    root["deauth_reason"] = pkt_info.frame_hdr.deauth.reason_code;
  }

  if (pkt_info.attack_type == IS_GEOFENCE_ATTACK) {
    root["sensor_location"] = "NORTH";
    root["attackers_mac"] = src_address;
    root["victims_mac"] = bssid_mac;
  }

  char data[256];
  size_t n = root.printTo(data, sizeof(data));
  String server_ip(sensor_config.alert_server_info.server_ip);

  String alert_url = "";
  alert_url = "http://" + server_ip + ":1880/AttackDetected";
  if (DEBUG_PRINT) {
    Serial.println(alert_url);
  }
  http.begin(client, alert_url);
  http.addHeader("Content-Type", "application/json");
  http.POST(data);
  http.end();
}

/**
 * @brief Sends a heartbeat signal.
 *
 * Periodically sends a signal to indicate the sensor is active.
 * Supports both WiFi (HTTP) and NRF24 heartbeat mechanisms.
 */
void heartbeat() {

  heartbeatTimecurr = millis();
  if (heartbeatTimecurr - heartbeatTimeprev >= HEARTBEAT_FREQ) {
    if (DEBUG_PRINT) {
      Serial.println("Sending Heartbeat");
    }

    if (heartbeat_type == WIFI_HEARTBEAT) // 2
    {
      wifi_promiscuous_enable(DISABLE);
      connect_Wifi();
      WiFiClient client;
      HTTPClient http;
      StaticJsonBuffer<32> jsonBuffer;
      JsonObject &root = jsonBuffer.createObject();
      root["id"] = sensor_config.id;
      char data[32];
      size_t n = root.printTo(data, sizeof(data));
      String server_ip(sensor_config.alert_server_info.server_ip);
      String heartbeat_url = "";
      heartbeat_url = "http://" + server_ip + ":1880/HeartBeatDetected";
      if (DEBUG_PRINT) {
        Serial.println(heartbeat_url);
      }
      http.begin(client, heartbeat_url);
      http.addHeader("Content-Type", "application/json");
      http.POST(data);
      http.end();
      heartbeatTimeprev = heartbeatTimecurr;
      if (DEBUG_PRINT) {
        Serial.println("HeartBeatSent");
        Serial.println("Re-enabling protection mode");
      }
      init_sniffing();
    } else // 1
    {
      StaticJsonBuffer<32> jsonBuffer;
      JsonObject &root = jsonBuffer.createObject();
      root["id"] = sensor_config.id;
      char data[32];
      size_t n = root.printTo(data, sizeof(data));
      RF24NetworkHeader header(server_node_address);
      bool msg_status = network.write(header, data, n);
      if (DEBUG_PRINT) {
        if (msg_status) {
          Serial.println("HeartBeat Sent successfully");
        } else {
          Serial.println("HeartBeat sending failed");
        }
      }
      heartbeatTimeprev = heartbeatTimecurr;
    }
  }
}

/**
 * @brief Dispatches an alert based on the configured mode.
 *
 * Checks if enough time has passed since the last alert (ALERT_FREQ).
 * If so, it disables promiscuous mode (if necessary) and calls the appropriate
 * alert function (IFTTT, Server, or NRF). Re-enables sniffing afterwards.
 */
void send_alert() {
  alertTimecurr = millis();
  if (alertTimecurr - alertTimeprev >= ALERT_FREQ) // || !is_first_alert_sent)
  {
    wifi_promiscuous_enable(DISABLE);
    if (DEBUG_PRINT) {
      Serial.println("Sending Alert");
    }

    switch (sensor_config.alert_mode) {
    case ALERT_STANDALONE:
      // wifi_promiscuous_enable(DISABLE);
      alert_ifttt();

      break;
    case ALERT_WIFI_SERVER:
      // wifi_promiscuous_enable(DISABLE);
      alert_server();
      break;
    case ALERT_NRF:
      alert_nrf();
      break;
    }
    if (DEBUG_PRINT) {
      if (sensor_config.operation_mode == OPERATION_DETECTION_MODE) {
        Serial.println("Re-enabling detection mode");
      } else {
        Serial.println("Re-enabling protection mode");
      }
    }
    is_first_alert_sent = true;
    alertTimeprev = alertTimecurr;
    if (sensor_config.operation_mode == OPERATION_DETECTION_MODE) {
      init_sniffing();
    }
  }
}
