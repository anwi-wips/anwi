#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include "config.h"
#include "..\global_vars.h"

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


int config_ROM_Address = 0;   //ROM address where Config will be stored from

void clear_configuration()
{
    EEPROM.begin(512);
    
    for(int i = 0; i < 512;i++)
    {
        EEPROM.put(i, 0);
    }
    EEPROM.end();
    
}

void save_config_settings()
{
    EEPROM.begin(512);
    EEPROM.put(config_ROM_Address, sensor_config);
    EEPROM.commit();
    EEPROM.end();
}

void get_config_settings()
{
    EEPROM.begin(512);
    EEPROM.get(config_ROM_Address,sensor_config);
    EEPROM.end();
}

uint8_t get_configuration_status()
{
    get_config_settings();
    return sensor_config.isConfigured;
}
    

void print_config()
{
    Serial.print("SSID To Protect : ");
    Serial.println(sensor_config.protect_ap_info.SSID);
    Serial.print("BSSID To Protect : ");
    Serial.println(sensor_config.protect_ap_info.BSSID_lower);
    Serial.print("Sensor ID : ");
    Serial.println(sensor_config.id);
    Serial.print("Connection SSID : ");
    Serial.println(sensor_config.connect_ap_info.SSID);
    Serial.print("Alert Mode : ");
    switch(sensor_config.alert_mode)
    {
        case ALERT_STANDALONE :
            Serial.println("StandAlone Mode");
            Serial.print("IFTTT KEY : ");
            Serial.println(sensor_config.ifttt_info.ifttt_key);
            Serial.print("IFTTT Deauthentication Event Name : ");
            Serial.println(sensor_config.ifttt_info.ifttt_eventName_deauth);
            Serial.print("IFTTT Evil Twin Event Name : ");
            Serial.println(sensor_config.ifttt_info.ifttt_eventName_eviltwin);
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

String get_string_input(String msg,uint8_t min_len,uint8_t max_len)
{
    bool input_provided = false;
    String reply = "";
    Serial.print(msg);
    while(!input_provided)
    {
        reply = Serial.readStringUntil('\0');
        if ( min_len == max_len && reply.length() == min_len) 
        {
            input_provided = true;
        }
        if(reply.length() >= min_len && reply.length() <= max_len )
            input_provided = true;
    }
    Serial.println(reply);
    return reply;
}

uint8_t get_int_input(String msg,uint8_t min_val,uint8_t max_val)
{
    bool input_provided = false;
    String reply = "";
    uint8_t reply_int = 0;
    Serial.print(msg);
    while(!input_provided)
    {
        reply = Serial.readStringUntil('\0');
        reply_int = reply.toInt();
        if(reply_int > min_val && reply_int < max_val )
        input_provided = true;
    }
    Serial.println(reply_int);
    return reply_int;
}

void config_sensor_manually()
{
    
    String reply = "";
    bool ssid_len_in_limit = false;
    bool bssid_len_in_limit = false;
    bool confirm_setting = false;
    uint8_t cho = 0;
    bool id_set = false;
    bool alert_mode_set = false;
    bool alert_nrf_mode_selected  = false;
    bool alert_standalone_mode_set = false;
    bool alert_server_mode_set = false;
    bool ifttt_key_set = false;
    bool ifttt_event_set = false;
    bool server_ip_set = false;

    while(!confirm_setting)
    {
        String msg = "Enter ID for Sensor [XX] : ";
        sensor_config.id = get_int_input(msg,0,100);
        msg = "Enter SSID of Access Point to Protect (MAX 32 Len ): ";

        tmp_str = get_string_input(msg,1,MAX_SSID_LEN);
        tmp_str.toCharArray(sensor_config.protect_ap_info.SSID,tmp_str.length()-1);
        
        msg = "Enter BSSID ( MAC ) of Access Point to protect ( eg 00:11:22:33:44:55) : ";
        tmp_str = get_string_input(msg,MAC_LEN_FMT+1,MAC_LEN_FMT+1);
        // Temp
        tmp_str.toLowerCase();
        tmp_str.toCharArray(sensor_config.protect_ap_info.BSSID_lower,tmp_str.length() -1 ); 
        tmp_str.toUpperCase();
        tmp_str.toCharArray(sensor_config.protect_ap_info.BSSID_upper,tmp_str.length() -1 ); 

        msg = "Enter WIFI SSID : ";
        tmp_str = get_string_input(msg,1,MAX_SSID_LEN);
        tmp_str.toCharArray(sensor_config.connect_ap_info.SSID,tmp_str.length()-1);

        msg = "Enter WIFI PASSWD : ";
        tmp_str = get_string_input(msg,1,MAX_SSID_LEN);
        tmp_str.toCharArray(sensor_config.connect_ap_info.PASSWORD,tmp_str.length()-1);


        while(!alert_mode_set)
        {
            msg = "Select sensor alert mode \n 1) Standalone Mode (IFTTT) \n 2) Wifi Server Mode";// \n 3) Standalone Radio Mode";
            cho = get_int_input(msg,0,3);
            switch (cho)
            {
                case 1:
                    msg= "Enter IFTTT key : ";
                    tmp_str = get_string_input(msg,1,100);
                    tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_key,tmp_str.length()-1); 
                    msg = "Enter IFTTT event name for Evil Twin: ";
                    tmp_str = get_string_input(msg,1,100);
                    tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_eventName_eviltwin,tmp_str.length()-1); 
                    
                    msg = "Enter IFTTT event name for Deauthentication : ";
                    tmp_str = get_string_input(msg,1,100);
                    tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_eventName_deauth,tmp_str.length()-1); 
                    
                    sensor_config.alert_mode = ALERT_STANDALONE;
                    alert_mode_set = true;
                break;
                
                case 2:
                    msg = "Enter Alert Server IP : ";
                    tmp_str  = get_string_input(msg,7,17);
                    tmp_str.toCharArray(sensor_config.alert_server_info.server_ip,tmp_str.length()-1);
                    sensor_config.alert_mode = ALERT_WIFI_SERVER;
                    alert_mode_set = true;
                break;
                /*
                case 3:
                    msg = "Separate Radio mode selected";
                    sensor_config.alert_mode = ALERT_NRF;
                    alert_mode_set = true;                    
                break;
                */
            }
        }
    
        print_config();
        msg = " Do you want to save Yes[1], No[0] : ";
        cho = get_int_input(msg,0,3);
        if( cho == 1)
        {
            sensor_config.isConfigured = 1;   
            save_config_settings();
            if(DEBUG_PRINT) 
            {
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

void save_settings()
{

    tmp_str =  server.arg("id");
    sensor_config.id = tmp_str.toInt();

    tmp_str =  server.arg("protect_SSID");
    tmp_str.toCharArray(sensor_config.protect_ap_info.SSID,tmp_str.length() + 1);

    tmp_str =  server.arg("protect_BSSID");

    tmp_str.toLowerCase();
    tmp_str.toCharArray(sensor_config.protect_ap_info.BSSID_lower,tmp_str.length() + 1); 

    tmp_str.toUpperCase();
    tmp_str.toCharArray(sensor_config.protect_ap_info.BSSID_upper,tmp_str.length() + 1); 


    tmp_str =  server.arg("connect_SSID");
    tmp_str.toCharArray(sensor_config.connect_ap_info.SSID,tmp_str.length() + 1);

    tmp_str =  server.arg("connect_PASSWORD");
    tmp_str.toCharArray(sensor_config.connect_ap_info.PASSWORD,tmp_str.length() + 1);

    tmp_str =  server.arg("Alert_Mode");
    
    switch (tmp_str.toInt())
    {
        case 1: // Alert_Mode-IFTTT
            tmp_str =  server.arg("ifttt_key");
            tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_key,tmp_str.length() + 1); 

            tmp_str =  server.arg("ifttt_eventName_eviltwin");
            tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_eventName_eviltwin,tmp_str.length() + 1); 

            tmp_str =  server.arg("ifttt_eventName_deauth");
            tmp_str.toCharArray(sensor_config.ifttt_info.ifttt_eventName_deauth,tmp_str.length() + 1); 
            sensor_config.alert_mode = ALERT_STANDALONE;

        break;
        case 2: //Alert_Mode-SERVER
            tmp_str =  server.arg("server_ip");
            tmp_str.toCharArray(sensor_config.alert_server_info.server_ip,tmp_str.length() + 1);
            sensor_config.alert_mode = ALERT_WIFI_SERVER;
        break;
    }
    sensor_config.isConfigured = 1;
    save_config_settings();
 //   save_configuration_status();
    
    server.send ( 200, "text/html", "Saved");
    
    print_config();
    //ESP.restart();
    
}

void create_AP()
{

    WiFi.mode(WIFI_AP);    
    WiFi.softAP("Anwi_Sensor");//,"anwi");
    IPAddress myIP = WiFi.softAPIP(); //Get IP address
    Serial.print("HotSpt IP:");
    Serial.println(myIP);
    Serial.println("WiFi AP Created");  
    isWebConfig = true;

}

void config_sensor_web() 
{
    Serial.println("Starting Access Point");
    create_AP();
      //listen to port 80
    
    server.on ( "/", [&](){
        server.send ( 200, "text/html", Sensor_config_web);
    });

    server.on ("/save_settings",save_settings);
    server.begin(); //start the webserver
    Serial.println("Webserver started");
}


void serve_clients()
{
    server.handleClient();  //process all the requests for the Webserver
}
