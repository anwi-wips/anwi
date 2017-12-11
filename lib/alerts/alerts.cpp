#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <esp8266httpclient.h>
#include "alerts.h"
#include "..\global_vars.h"
#include "config.h"
#include "packet_capture.h"

bool is_first_alert_sent = false;

void connect_Wifi()
{
    delay(10);
    yield();
    WiFi.mode(WIFI_STA);
    if(DEBUG_PRINT) 
    {
        Serial.print("Connecting to : ");
        Serial.println(sensor_config.connect_ap_info.SSID);
    }
    WiFi.begin(sensor_config.connect_ap_info.SSID, sensor_config.connect_ap_info.PASSWORD);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(1000);
      Serial.print(".");
    }
    if(DEBUG_PRINT) 
    {
        Serial.println("WiFi connected");  
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
}

void alert_ifttt()
{
    connect_Wifi();
    HTTPClient http;
    if(DEBUG_PRINT) Serial.println("Sending alert to IFTTT");
    //https://bblanchon.github.io/ArduinoJson/example/generator/

    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["value1"] = pkt_info.frame_hdr.bssid_address;
    root["value2"] = pkt_info.rssi;

    char data[256];
    size_t n = root.printTo(data, sizeof(data));
    String ifttt_triggerName="";
    String alert_url = "";
    switch(pkt_info.attack_type)
    {
        case IS_EVILTWIN_ATTACK:
            ifttt_triggerName=sensor_config.ifttt_info.ifttt_eventName_eviltwin;
        break;
        case IS_DEAUTH_ATTACK:
            ifttt_triggerName=sensor_config.ifttt_info.ifttt_eventName_deauth;
        break;
    }
    alert_url = IFTTT_ALERT_URL+ ifttt_triggerName+"/with/key/"+sensor_config.ifttt_info.ifttt_key;
    if(DEBUG_PRINT) 
    {
        Serial.println(alert_url);
    }
    http.begin(alert_url);
    http.addHeader("Content-Type", "application/json");
    http.POST(data);
    http.end();

}

void alert_server()
{
    connect_Wifi();
    HTTPClient http;

    //https://bblanchon.github.io/ArduinoJson/example/generator/

    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    if(DEBUG_PRINT) 
    {
        Serial.println("Sending Alert to server");
    }
    char bssid_mac[18],dest_address[18],src_address[18];
    root["id"] = sensor_config.id;
    sprintf(bssid_mac,MACSTR , MAC2STR(pkt_info.frame_hdr.bssid_address));
    root["bssid_mac"] = bssid_mac;
    root["rssi"] = pkt_info.rssi;
    root["attack_type"] = pkt_info.attack_type;
    root["channel"] =pkt_info.channel;
    sprintf(dest_address,MACSTR , MAC2STR(pkt_info.frame_hdr.destination_address));
    root["destination_address"] = dest_address;
//    sprintf(src_address,MACSTR , MAC2STR(pkt_info.frame_hdr.source_address));
  //  root["source_address"] = src_address;
    

    if(pkt_info.attack_type == IS_DEAUTH_ATTACK)
    {
        root["deauth_reason"] = pkt_info.frame_hdr.deauth.reason_code;
    }

    char data[256];
    size_t n = root.printTo(data, sizeof(data));
    String server_ip(sensor_config.alert_server_info.server_ip);
    

    String alert_url = "";
    alert_url = "http://"+server_ip + ":1880/AttackDetected";
    if(DEBUG_PRINT) 
    {
        Serial.println(alert_url);
    }
    http.begin(alert_url);
    http.addHeader("Content-Type", "application/json");
    http.POST(data);
    http.end();
}

void alert_nrf()
{

}

void heartbeat()
{
    heartbeatTimecurr = millis();
    if(heartbeatTimecurr - heartbeatTimeprev >= HEARTBEAT_FREQ)
    {
        if(DEBUG_PRINT) 
        {
            Serial.println("Sending Heartbeat");
        }

        wifi_promiscuous_enable(DISABLE);
        connect_Wifi();
            
        HTTPClient http;
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root["id"] = sensor_config.id;
        char data[256];
        size_t n = root.printTo(data, sizeof(data));
        String server_ip(sensor_config.alert_server_info.server_ip);
        String heartbeat_url = "";
        heartbeat_url = "http://"+server_ip + ":1880/HeartBeatDetected";
        if(DEBUG_PRINT) 
        {
            Serial.println(heartbeat_url);
        }

        http.begin(heartbeat_url);
        http.addHeader("Content-Type", "application/json");
        http.POST(data);
        http.end();
        heartbeatTimeprev = heartbeatTimecurr;
        if(DEBUG_PRINT) 
        {
            Serial.println("HeartBeatSent");
            Serial.println("Re-enabling protection mode");
        }
        init_sniffing();
        //wifi_set_opmode(STATION_MODE);
        //wifi_promiscuous_enable(ENABLE);
    }
        
}

void send_alert()
{
    alertTimecurr = millis();

    if ( alertTimecurr - alertTimeprev >= ALERT_FREQ)// || !is_first_alert_sent)
    {
        wifi_promiscuous_enable(DISABLE);
        if(DEBUG_PRINT) 
        {
            Serial.println("Sending Alert");
        }

        switch(sensor_config.alert_mode)
        {   
            case ALERT_STANDALONE:
                alert_ifttt();
            break;
            case ALERT_WIFI_SERVER:
                alert_server();
            break;
            case ALERT_NRF:
                alert_nrf();
            break;
        }
        if(DEBUG_PRINT) 
        {
            Serial.println("Re-enabling protection mode");
        }

        is_first_alert_sent = true;
        alertTimeprev = alertTimecurr;
        init_sniffing();
        //wifi_set_opmode(STATION_MODE);
        //wifi_promiscuous_enable(ENABLE);
        
    }
}
