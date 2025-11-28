#include "config.h"
#include <..\global_vars.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>

struct sensor_config_info sensor_config;

ESP8266WebServer server(80);

uint8_t curr_channel = 0;
uint8_t set_channel = INIT_CHANNEL;
uint16_t deauth_pkt_counter = 0;
uint8_t cntr = 0;

unsigned long prevTime = 0;
unsigned long curTime = 0;

bool isWebConfig = false;
String tmp_str = "";
unsigned long alertTimecurr = 0;
unsigned long alertTimeprev = 0;

unsigned long heartbeatTimecurr = 0;
unsigned long heartbeatTimeprev = 0;

int curChannel = INIT_CHANNEL;

bool is_deauth_or_disassoc_attack = false;
bool is_evil_twin_attack = false;
bool manual_config = false;
bool isConfigured = false;

int config_ROM_Address = 0; // ROM address where Config will be stored from

/**
 * @brief Clears the configuration from EEPROM.
 *
 * Overwrites the first 1024 bytes of EEPROM with zeros.
 */
void clear_configuration() {
  EEPROM.begin(1024);

  for (int i = 0; i < 1024; i++) {
    EEPROM.put(i, 0);
  }
  EEPROM.commit();
  EEPROM.end();
}

/**
 * @brief Saves the current configuration to EEPROM.
 *
 * Writes the `sensor_config` structure to the defined EEPROM address.
 */
void save_config_settings() {
  EEPROM.begin(1024);
  EEPROM.put(config_ROM_Address, sensor_config);
  EEPROM.commit();
  EEPROM.end();
}

/**
 * @brief Loads the configuration from EEPROM.
 *
 * Reads the `sensor_config` structure from the defined EEPROM address.
 */
void get_config_settings() {
  EEPROM.begin(1024);
  EEPROM.get(config_ROM_Address, sensor_config);
  EEPROM.end();
}

/**
 * @brief Checks if the device is configured.
 *
 * Loads settings from EEPROM and returns the configuration status flag.
 *
 * @return uint8_t 1 if configured, 0 otherwise.
 */
uint8_t get_configuration_status() {
  get_config_settings();
  return sensor_config.isConfigured;
}

/**
 * @brief Prints the current configuration to the Serial monitor.
 *
 * Displays all sensor settings including ID, location, protected SSID,
 * operation mode, and alert settings.
 */
void print_config() {
  Serial.print("Sensor ID : ");
  Serial.println(sensor_config.id);

  Serial.print("Sensor Location : ");
  switch (sensor_config.sensor_location) {
  case NORTH:
    Serial.println("NORTH");
    break;
  case SOUTH:
    Serial.println("SOUTH");
    break;
  case WEST:
    Serial.println("WEST");
    break;
  case EAST:
    Serial.println("EAST");
    break;
  }

  Serial.print("SSID To Protect : ");
  Serial.println(sensor_config.protect_ap_info.SSID);
  Serial.print("BSSID To Protect : ");
  Serial.println(sensor_config.protect_ap_info.BSSID_lower);

  Serial.print("Connection SSID : ");
  Serial.println(sensor_config.connect_ap_info.SSID);
  Serial.print("Operation Mode : ");
  if (sensor_config.operation_mode == OPERATION_DETECTION_MODE) {
    Serial.println("Detection Mode");
  } else if (sensor_config.operation_mode == OPERATION_PROTECTION_MODE) {
    Serial.println("Protection Mode");
  }

  Serial.print("Alert Mode : ");
  switch (sensor_config.alert_mode) {
  case ALERT_STANDALONE:
    Serial.println("StandAlone Mode");
    Serial.print("IFTTT KEY : ");
    Serial.println(sensor_config.ifttt_info.ifttt_key);
    if (sensor_config.operation_mode == OPERATION_DETECTION_MODE) {
      Serial.print("IFTTT Deauthentication Event Name : ");
      Serial.println(sensor_config.ifttt_info.ifttt_eventName_deauth);
      Serial.print("IFTTT Evil Twin Event Name : ");
      Serial.println(sensor_config.ifttt_info.ifttt_eventName_eviltwin);
    } else if (sensor_config.operation_mode == OPERATION_PROTECTION_MODE) {
      Serial.print("IFTTT GeoFencing Event Name : ");
      Serial.println(sensor_config.ifttt_info.ifttt_eventName_geofence);
    }

    break;

  case ALERT_WIFI_SERVER:
    Serial.println("Wifi Server Alert Mode");
    Serial.print("Server IP : ");
    Serial.println(sensor_config.alert_server_info.server_ip);
    break;

  case ALERT_NRF:
    Serial.println("Standalone Radio Mode");
    break;
  }
}

/**
 * @brief Reads a string input from the Serial monitor.
 *
 * Blocks until a valid string within the specified length range is received.
 *
 * @param msg Prompt message to display.
 * @param min_len Minimum allowed length.
 * @param max_len Maximum allowed length.
 * @return String The received input.
 */
String get_string_input(String msg, uint8_t min_len, uint8_t max_len) {
  bool input_provided = false;
  String reply = "";
  Serial.print(msg);
  while (!input_provided) {
    reply = Serial.readStringUntil('\0');
    if (min_len == max_len && reply.length() == min_len) {
      input_provided = true;
    }
    if (reply.length() >= min_len && reply.length() <= max_len)
      input_provided = true;
  }
  Serial.println(reply);
  return reply;
}

/**
 * @brief Reads an integer input from the Serial monitor.
 *
 * Blocks until a valid integer within the specified range is received.
 *
 * @param msg Prompt message to display.
 * @param min_val Minimum allowed value.
 * @param max_val Maximum allowed value.
 * @return uint8_t The received integer.
 */
uint8_t get_int_input(String msg, uint8_t min_val, uint8_t max_val) {
  bool input_provided = false;
  String reply = "";
  uint8_t reply_int = 0;
  Serial.print(msg);
  while (!input_provided) {
    reply = Serial.readStringUntil('\0');
    reply_int = reply.toInt();
    if (reply_int > min_val && reply_int < max_val)
      input_provided = true;
  }
  Serial.println(reply_int);
  return reply_int;
}

/**
 * @brief Configures the sensor using JSON input from Serial.
 *
 * Parses a JSON object received over Serial and updates the sensor
 * configuration.
 */
void config_sensor_json() {
  // const char * tmpchar;
  DynamicJsonBuffer jsonBuffer(1024);

  if (Serial.available()) {
    JsonObject &root = jsonBuffer.parseObject(Serial);

    tmp_str = root.get<String>("id");
    sensor_config.id = tmp_str.toInt();

    tmp_str = root.get<String>("sensor_location");
    sensor_config.sensor_location = tmp_str.toInt();

    tmp_str = root.get<String>("protect_SSID");
    tmp_str.toCharArray(sensor_config.protect_ap_info.SSID,
                        sizeof(sensor_config.protect_ap_info.SSID));

    tmp_str = root.get<String>("protect_BSSID");
    tmp_str.toLowerCase();
    tmp_str.toCharArray(sensor_config.protect_ap_info.BSSID_lower,
                        sizeof(sensor_config.protect_ap_info.BSSID_lower));
    tmp_str.toUpperCase();
    tmp_str.toCharArray(sensor_config.protect_ap_info.BSSID_upper,
                        sizeof(sensor_config.protect_ap_info.BSSID_upper));

    tmp_str = root.get<String>("connect_SSID");
    tmp_str.toCharArray(sensor_config.connect_ap_info.SSID,
                        sizeof(sensor_config.connect_ap_info.SSID));

    tmp_str = root.get<String>("connect_PASSWORD");
    tmp_str.toCharArray(sensor_config.connect_ap_info.PASSWORD,
                        sizeof(sensor_config.connect_ap_info.PASSWORD));

    tmp_str = root.get<String>("op_mode");
    sensor_config.operation_mode = tmp_str.toInt();
    tmp_str = root.get<String>("Alert_Mode");

    switch (tmp_str.toInt()) {
    case 1: // Alert_Mode-IFTTT
      tmp_str = root.get<String>("ifttt_key");
      tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_key,
                          sizeof(sensor_config.ifttt_info.ifttt_key));
      if (sensor_config.operation_mode == OPERATION_DETECTION_MODE) {
        tmp_str = root.get<String>("ifttt_eventName_eviltwin");
        tmp_str.toCharArray(
            sensor_config.ifttt_info.ifttt_eventName_eviltwin,
            sizeof(sensor_config.ifttt_info.ifttt_eventName_eviltwin));

        tmp_str = root.get<String>("ifttt_eventName_deauth");
        tmp_str.toCharArray(
            sensor_config.ifttt_info.ifttt_eventName_deauth,
            sizeof(sensor_config.ifttt_info.ifttt_eventName_deauth));
      } else if (sensor_config.operation_mode == OPERATION_PROTECTION_MODE) {
        tmp_str = root.get<String>("ifttt_eventName_geofence");
        tmp_str.toCharArray(
            sensor_config.ifttt_info.ifttt_eventName_geofence,
            sizeof(sensor_config.ifttt_info.ifttt_eventName_geofence));
      }
      sensor_config.alert_mode = ALERT_STANDALONE;
      break;
    case 2: // Alert_Mode-SERVER
      tmp_str = root.get<String>("server_ip");
      tmp_str.toCharArray(sensor_config.alert_server_info.server_ip,
                          sizeof(sensor_config.alert_server_info.server_ip));
      sensor_config.alert_mode = ALERT_WIFI_SERVER;
      break;
    case 3:
      sensor_config.alert_mode = ALERT_NRF;
      break;
    }
    sensor_config.isConfigured = 1;
    save_config_settings();
    print_config();
  }
}

/**
 * @brief Configures the sensor manually via interactive Serial prompts.
 *
 * Guides the user through setting up the sensor ID, protected AP details,
 * WiFi credentials, operation mode, and alert settings.
 */
void config_sensor_manually() {
  String reply = "";
  bool ssid_len_in_limit = false;
  bool bssid_len_in_limit = false;
  bool confirm_setting = false;
  uint8_t cho = -1;
  uint8_t alert_mode = 0, operation_mode = 0;
  bool id_set = false;
  bool alert_mode_set = false;
  bool alert_nrf_mode_selected = false;
  bool alert_standalone_mode_set = false;
  bool alert_server_mode_set = false;
  bool ifttt_key_set = false;
  bool ifttt_event_set = false;
  bool server_ip_set = false;
  bool operation_mode_detection_set = false;
  bool operation_mode_protection_set = false;

  while (!confirm_setting) {
    String msg = "Enter ID for Sensor [XX] : ";
    sensor_config.id = get_int_input(msg, 0, 100);
    msg = "Enter SSID of Access Point to Protect (MAX 32 Len ): ";

    tmp_str = get_string_input(msg, 1, MAX_SSID_LEN);
    tmp_str.toCharArray(sensor_config.protect_ap_info.SSID,
                        tmp_str.length() - 1);

    msg = "Enter BSSID ( MAC ) of Access Point to protect ( eg "
          "00:11:22:33:44:55) : ";
    tmp_str = get_string_input(msg, MAC_LEN_FMT + 1, MAC_LEN_FMT + 1);

    // Temp
    tmp_str.toLowerCase();
    tmp_str.toCharArray(sensor_config.protect_ap_info.BSSID_lower,
                        tmp_str.length() - 1);
    tmp_str.toUpperCase();
    tmp_str.toCharArray(sensor_config.protect_ap_info.BSSID_upper,
                        tmp_str.length() - 1);

    // Type of WiFi network to protect
    msg = "Enter Type Of Wifi Network To Protect OPEN/ENCRYPTED : ";
    tmp_str = get_string_input(msg, 4, MAX_NETWORK_TYPE_LEN);
    tmp_str.toCharArray(sensor_config.protect_ap_info.NETWORK_type,
                        tmp_str.length() - 1);

    msg = "Enter WIFI SSID : ";
    tmp_str = get_string_input(msg, 1, MAX_SSID_LEN);
    tmp_str.toCharArray(sensor_config.connect_ap_info.SSID,
                        tmp_str.length() - 1);

    msg = "Enter WIFI PASSWD : ";
    tmp_str = get_string_input(msg, 1, MAX_SSID_LEN);
    tmp_str.toCharArray(sensor_config.connect_ap_info.PASSWORD,
                        tmp_str.length() - 1);

    while (!alert_mode_set) {
      msg = "Select ANWI operation mode \n 1) Detection Mode \n 2) Protection "
            "Mode";
      operation_mode = get_int_input(msg, 0, 3);
      sensor_config.operation_mode = operation_mode;

      msg = "Select sensor alert mode \n 1) Standalone Mode (IFTTT) \n 2) Wifi "
            "Server Mode \n 3) Standalone Radio Mode";
      alert_mode = get_int_input(msg, 0, 4);

      switch (alert_mode) {
      case 1:
        msg = "Enter IFTTT key : ";
        tmp_str = get_string_input(msg, 1, 100);
        tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_key,
                            tmp_str.length() - 1);

        if (operation_mode == OPERATION_DETECTION_MODE) // DETECTION_MODE
        {
          msg = "Enter IFTTT event name for Evil Twin: ";
          tmp_str = get_string_input(msg, 1, 100);
          tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_eventName_eviltwin,
                              tmp_str.length() - 1);

          msg = "Enter IFTTT event name for Deauthentication : ";
          tmp_str = get_string_input(msg, 1, 100);
          tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_eventName_deauth,
                              tmp_str.length() - 1);
        } else if (operation_mode ==
                   OPERATION_PROTECTION_MODE) // PROTECTION_MODE
        {
          msg = "Enter IFTTT event name for GeoFencing : ";
          tmp_str = get_string_input(msg, 1, 100);
          tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_eventName_geofence,
                              tmp_str.length() - 1);
        }

        sensor_config.alert_mode = ALERT_STANDALONE;
        alert_mode_set = true;
        break;

      case 2:
        msg = "Enter Alert Server IP : ";
        tmp_str = get_string_input(msg, 7, 17);
        tmp_str.toCharArray(sensor_config.alert_server_info.server_ip,
                            tmp_str.length() - 1);
        sensor_config.alert_mode = ALERT_WIFI_SERVER;
        alert_mode_set = true;
        break;

      case 3:
        msg = "Separate Radio mode selected";
        sensor_config.alert_mode = ALERT_NRF;
        alert_mode_set = true;
        break;
      }
    }

    print_config();
    msg = " Do you want to save Yes[1], No[0] : ";
    cho = get_int_input(msg, 0, 3);
    if (cho == 1) {
      sensor_config.isConfigured = 1;
      save_config_settings();
      if (DEBUG_PRINT) {
        Serial.println("Settings Saved");
        get_config_settings();
        Serial.println("Saved setttings from ROM :");
        print_config();
      }
      confirm_setting = true;
      ESP.restart();
      break;
    }
  }
}

/**
 * @brief Saves settings received from the Web UI.
 *
 * Handler for the "/save_settings" endpoint. Parses arguments from the
 * web request, updates the configuration, saves it to EEPROM, and restarts.
 */
void save_settings() {

  uint8_t alert_mode = 1, operation_mode = 1;

  tmp_str = server.arg("id");
  sensor_config.id = tmp_str.toInt();

  // tmp_str =  server.arg("sensor_location");
  // sensor_config.sensor_location = tmp_str.toInt();

  tmp_str = server.arg("protect_SSID");
  tmp_str.toCharArray(sensor_config.protect_ap_info.SSID,
                      sizeof(sensor_config.protect_ap_info.SSID));

  tmp_str = server.arg("protect_BSSID");

  tmp_str.toLowerCase();
  tmp_str.toCharArray(sensor_config.protect_ap_info.BSSID_lower,
                      sizeof(sensor_config.protect_ap_info.BSSID_lower));

  tmp_str.toUpperCase();
  tmp_str.toCharArray(sensor_config.protect_ap_info.BSSID_upper,
                      sizeof(sensor_config.protect_ap_info.BSSID_upper));

  tmp_str = server.arg("connect_SSID");
  tmp_str.toCharArray(sensor_config.connect_ap_info.SSID,
                      sizeof(sensor_config.connect_ap_info.SSID));

  tmp_str = server.arg("connect_PASSWORD");
  tmp_str.toCharArray(sensor_config.connect_ap_info.PASSWORD,
                      sizeof(sensor_config.connect_ap_info.PASSWORD));

  tmp_str = server.arg("Alert_Mode");
  alert_mode = tmp_str.toInt();

  tmp_str = server.arg("Operation_Mode");
  operation_mode = tmp_str.toInt();
  sensor_config.operation_mode = operation_mode;

  switch (alert_mode) {
  case 1: // Alert_Mode-IFTTT
    tmp_str = server.arg("ifttt_key");
    tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_key,
                        sizeof(sensor_config.ifttt_info.ifttt_key));

    tmp_str = server.arg("ifttt_eventName_eviltwin");
    tmp_str.toCharArray(
        sensor_config.ifttt_info.ifttt_eventName_eviltwin,
        sizeof(sensor_config.ifttt_info.ifttt_eventName_eviltwin));

    tmp_str = server.arg("ifttt_eventName_deauth");
    tmp_str.toCharArray(
        sensor_config.ifttt_info.ifttt_eventName_deauth,
        sizeof(sensor_config.ifttt_info.ifttt_eventName_deauth));
    sensor_config.alert_mode = ALERT_STANDALONE;

    if (operation_mode == OPERATION_DETECTION_MODE) {
      tmp_str = server.arg("ifttt_eventName_eviltwin");
      tmp_str.toCharArray(
          sensor_config.ifttt_info.ifttt_eventName_eviltwin,
          sizeof(sensor_config.ifttt_info.ifttt_eventName_eviltwin));

      tmp_str = server.arg("ifttt_eventName_deauth");
      tmp_str.toCharArray(
          sensor_config.ifttt_info.ifttt_eventName_deauth,
          sizeof(sensor_config.ifttt_info.ifttt_eventName_deauth));
    } else if (operation_mode == OPERATION_PROTECTION_MODE) {
      tmp_str = server.arg("ifttt_eventName_geofence");
      tmp_str.toCharArray(
          sensor_config.ifttt_info.ifttt_eventName_geofence,
          sizeof(sensor_config.ifttt_info.ifttt_eventName_geofence));
    }

    break;
  case 2: // Alert_Mode-SERVER
    tmp_str = server.arg("server_ip");
    tmp_str.toCharArray(sensor_config.alert_server_info.server_ip,
                        sizeof(sensor_config.alert_server_info.server_ip));

    sensor_config.alert_mode = ALERT_WIFI_SERVER;
    break;
  case 3:
    sensor_config.alert_mode = ALERT_NRF;
    break;
  }
  sensor_config.isConfigured = 1;
  save_config_settings();
  //   save_configuration_status();

  server.send(200, "text/html", "Saved");

  print_config();
  ESP.restart();
}

/**
 * @brief Creates a SoftAP for configuration.
 *
 * Sets up an Access Point named "Anwi_Sensor" to allow users to connect
 * and configure the device via a web browser.
 */
void create_AP() {

  WiFi.mode(WIFI_AP);
  WiFi.softAP("Anwi_Sensor");       //,"anwi");
  IPAddress myIP = WiFi.softAPIP(); // Get IP address
  Serial.print("HotSpt IP:");
  Serial.println(myIP);
  Serial.println("WiFi AP Created");
  isWebConfig = true;
}

/**
 * @brief Starts the Web Server for configuration.
 *
 * Initializes the Access Point and sets up the web server routes.
 */
void config_sensor_web() {
  Serial.println("Starting Access Point");
  create_AP();
  // listen to port 80

  server.on("/", [&]() { server.send(200, "text/html", Sensor_config_web); });

  server.on("/save_settings", save_settings);
  server.begin(); // start the webserver
  Serial.println("Webserver started");
}

/**
 * @brief Handles incoming Web Server client requests.
 *
 * This function must be called frequently in the main loop when in
 * web configuration mode.
 */
void serve_clients() {
  server.handleClient(); // process all the requests for the Webserver
}
