#include <..\lib\global_vars.h>

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <esp8266httpclient.h>


#include "packet_capture.h"
#include "debug_print.h"
#include "config.h"
#include "alerts.h"

uint8_t isConfiguredflag = -1;


void hop_channel()
{
    if (sensor_config.protection_config.is_hop_channel_enabled)
    {
        if (set_channel == MAX_CHANNEL)
        {
            set_channel = INIT_CHANNEL;
        }
        else
        {
            set_channel++;
        }
        if(DEBUG_PRINT)
        {
            Serial.print(" CHAN SET TO : ");
            Serial.println(set_channel);
        }
       wifi_set_channel(set_channel);
    }
}


void loop() 
{
    heartbeat();
    if(isWebConfig)
    {
        serve_clients();
    }

    curTime = millis();
    if(curTime - prevTime >= SCAN_FREQ)
    {    
        if(pkt_info.is_deauth_detected)
        {
            if(deauth_pkt_counter >= MAX_DEAUTH_PKT)
            {
                pkt_info.attack_type = IS_DEAUTH_ATTACK;
            }
            pkt_info.is_deauth_detected = false;
            //pkt_info.is_disassoc_detected = false;
        }
        hop_channel();
    }
    else
    {
        prevTime = curTime;
        deauth_pkt_counter = 0;
    }

    
    if (pkt_info.attack_type == IS_EVILTWIN_ATTACK ||  pkt_info.attack_type == IS_DEAUTH_ATTACK )
    {
        send_alert();
        pkt_info.attack_type = -1;
    }
}

void setup() 
{
    
    isConfiguredflag = get_configuration_status();
    Serial.begin(115200);
    
    if(Serial)
    {
        Serial.println("\nANWI - All New Wireless IDS\n ");
    
            Serial.println("Press (d) to delete configuration");
            delay(5000);
    
            if(Serial.read() == 'd')
            {
                Serial.println("Clearing Config");
                clear_configuration();
                ESP.restart();
                Serial.println("Restart failed");
            }
        if(isConfiguredflag == 0)
        {
            Serial.println("No configuration found");
            
            Serial.println("Press (c) to configure using Serial");
            delay(5000);
            if(Serial.read() == 'c')
            {
                config_sensor_manually();
            }
            else
            {
                Serial.println("Connect to ANWI-Sensor AP to configure");
                config_sensor_web();
            }
        }    
         //save_config_settings();
        if(isConfiguredflag == 1)
        {
            void print_config();
            Serial.println("Press (r) to re-configure.");
            delay(5000);
            if(Serial.read() == 'r')
            {
                config_sensor_manually();
            }

            curr_channel = 1;

            //get_config_settings();
            Serial.println("ANWI Protection Activated..");
            init_sniffing();
        }
    }
}
