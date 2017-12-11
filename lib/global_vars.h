#ifndef _GLOBAL_VARS_H_
#define _GLOBAL_VARS_H_

#include <Arduino.h>

#include ".\config\config.h"
#include "ieee80211_packet.h"


extern "C" 
{
    #include <user_interface.h>
}

#define DEBUG_PRINT 0

#define DISABLE 0
#define ENABLE  1
#define MAX_IP_LEN 16
#define MAX_STRING_INPUT 100

#define IFTTT_ALERT_URL  "http://maker.ifttt.com/trigger/"


#define INIT_CHANNEL 6
#define MAX_CHANNEL 11
#define SCAN_FREQ 1000
#define MAX_DEAUTH_PKT 20
#define ALERT_FREQ 60000

#define HEARTBEAT_FREQ 300000

#define IS_EVILTWIN_ATTACK	1
#define IS_DEAUTH_ATTACK	2

#define ALERT_STANDALONE 1
#define ALERT_WIFI_SERVER 2
#define ALERT_NRF 3


extern uint8_t set_channel;
extern uint16_t deauth_pkt_counter;

extern uint8_t cntr;
extern uint8_t sensor_id;

extern int config_ROM_Address;   //ROM Address where the config is saved

extern unsigned long prevTime;
extern unsigned long curTime;

extern unsigned long alertTimecurr;
extern unsigned long alertTimeprev;
extern bool is_first_alert_sent;

extern unsigned long heartbeatTimecurr;
extern unsigned long heartbeatTimeprev;

extern int curChannel;
extern bool is_deauth_or_disassoc_attack;
extern bool is_evil_twin_attack;
extern bool isConfigured;
extern bool manual_config;

extern uint8_t alert_mode;

extern uint8_t curr_channel;
extern bool isWebConfig;


struct protection_config
{
    bool is_hop_channel_enabled = false;
    bool is_evil_twin_detect_enabled = true;
    bool is_deauth_or_disassoc_detect_enabled = true;
};

struct protect_ap_info
{
    char SSID[MAX_SSID_LEN];
    char BSSID_lower[MAC_LEN_FMT];
    char BSSID_upper[MAC_LEN_FMT];
};

extern struct captured_packet_info
{    
    int8_t rssi;
    uint8_t channel;
    uint8_t type;
    uint8_t attack_type = -1;
    bool is_beacon_detected = false;
    bool is_deauth_detected = false;
    bool is_disassoc_detected = false;    
    bool has_ie_vendor_specific = false;
    bool has_ie_rsn = false;
    bool is_ssid_hidden;
    bool is_encrypted;
    struct ieee80211_frame_header frame_hdr;
}pkt_info;


struct ifttt_info
{
    char ifttt_key[MAX_STRING_INPUT] = "";
    char ifttt_eventName_eviltwin[MAX_STRING_INPUT] = "";
    char ifttt_eventName_deauth[MAX_STRING_INPUT] = "";
};

struct alert_server_info
{
    char server_ip[MAX_IP_LEN] = "";
};

struct nrf_info 
{
    uint8_t nrf_channel = 1;
};

struct connect_ap_info
{
    char SSID[MAX_SSID_LEN];
    char PASSWORD[MAX_SSID_LEN];
};

extern struct sensor_config_info
{
    uint8_t isConfigured = -1;
    uint8_t id = 0;
    struct protect_ap_info protect_ap_info;
    uint8_t alert_mode = -1;
    struct connect_ap_info connect_ap_info;
    struct ifttt_info ifttt_info;
    struct alert_server_info alert_server_info;
    struct nrf_info nrf_info;
    struct protection_config protection_config;
    
}sensor_config;

const char Sensor_config_web[] PROGMEM = R"=====(
    <!DOCTYPE html>
    <html>
    <head>
    <style>
    body{background-color: #454743;
        padding:20px;
        line-height: 1em;
        font-family : Arial, sans-serif;
    }
    
    h1{
      text-align: center;
      font: italic bold 1.6em Arial, sans-serif;
      color:#f5ab35;
      text-shadow: 3px 2px black;
    }
    
    .form-horizontal{
        display: table;
        border-collapse:separate;
        border-spacing:5px;
    }
    
    .form-group{
      display: table-row;
    }
    
    label{
      display: table-cell;
      padding:8px;
    }
    
    input {
      display: table-cell;
      padding:8px;
      border-radius: 15px;
      background-color: #eeeeee
    }
    
    input:focus{
      border-radius: 15px;
      border: 2px solid blue;
      outline: 0 none;
    }
    
    input:hover 
    {
        background-color: #e8cfab;
    }
    
    #btn_submit_form {
      display: inline-block;
      padding: 15px 25px;
      font-size: 14px;
      cursor: pointer;
      text-align: center;
      text-decoration: none;
      outline: none;
      color: #fff;
      background-color: #4CAF50;
      border: none;
      border-radius: 15px;
      box-shadow: 0 6px #999;
    }
    
    #btn_submit_form:hover{
        background-color: #3e8e41;
    }
    
    #btn_submit_form:active{
      background-color: #3e8e41;
      box-shadow: 0 5px #666;
      transform: translateY(4px);
    }
    
    </style>
    
    <script type="text/javascript">
         window.onload = function() {
          document.getElementById('alert_server').style.display = 'none';
    }
    
    function alert_modeCheck() {
        if (document.getElementById('Alert_Mode-IFTTT').checked) {
            document.getElementById('alert_server').style.display = 'none';
    
            document.getElementById('difttt_key').style.display = 'table-row';
            document.getElementById('difttt_evilt').style.display = 'table-row';
            document.getElementById('difttt_deauth').style.display = 'table-row';
        }       
        else if(document.getElementById('Alert_Mode-SERVER').checked) {
            document.getElementById('alert_server').style.display = 'table-row';
            document.getElementById('difttt_key').style.display = 'none';
            document.getElementById('difttt_evilt').style.display = 'none';
            document.getElementById('difttt_deauth').style.display = 'none';
       }
    }
    
    </script>
    </head>
    <body>
    <h1>ANWI ( All New Wireless IDS) - Sensor Configuration</h1>
    <form class="form-horizontal" action="/save_settings" method="post">
    
    <div class="form-group">
      <label class="col-md-4" for="id">Sensor ID</label>  
      <input id="id" name="id" type="text" placeholder="1" required="" size="2" maxlength="2" min="1" max="99">
    </div>
    
    <div class="form-group">
      <label class="col-md-4" for="protect_BSSID">BSSID To Protect</label>  
      <input id="protect_BSSID" name="protect_BSSID" type="text" placeholder="00:11:22:33:44:55"  title="MAC ID can be found on Router configuration page" required="" size="17" maxlength="17" pattern="^([0-9A-Fa-f]{2}[:]){5}([0-9A-Fa-f]{2})$">
    </div>
    
    <div class="form-group">
      <label class="col-md-4" for="protect_SSID">SSID To Protect</label>  
      <input id="protect_SSID" name="protect_SSID" type="text" placeholder="Protect_SSID" title="SSID you want to protect" required="" size="32" maxlength="32">
    </div>
    
    <div class="form-group">
      <label class="col-md-4" for="connect_SSID">Connection SSID</label>  
      <input id="connect_SSID" name="connect_SSID" type="text" placeholder="Connect_SSID" title="SSID to use for sending alerts" required="" size="32" maxlength="32">
    </div>
    
    <div class="form-group">
       <label class="col-md-4" for="connect_PASSWORD">Connection SSID Password</label>
       <input id="connect_PASSWORD" name="connect_PASSWORD" type="password" placeholder="Connect_PASSWORD" title="Password of SSID to use for sending alerts" required="" size="32" maxlength="32">
    </div>
    
    <div class="form-group">
      <label class="col-md-4" for="Alert_Mode">Alert Mode</label>
      <div class="radio">
        <label for="Alert_Mode-0">
          <input type="radio" name="Alert_Mode" id="Alert_Mode-IFTTT" value="1" onclick="javascript:alert_modeCheck();" checked="checked" title="Email alerts sent using IFTTT service">
          Standalone Mode (IFTTT)
        </label>
        </div>
      <div class="radio">
        <label for="Alert_Mode-1">
          <input type="radio" name="Alert_Mode" id="Alert_Mode-SERVER" value="2" onclick="javascript:alert_modeCheck();" title="Alerts sent to server">
          Server Mode
        </label>
      </div>
    </div>
    
    <div class="form-group" id="difttt_key">
      <label class="col-md-4" for="ifttt_key">IFTTT Key</label>  
      <input id="ifttt_key" name="ifttt_key" type="text" placeholder="" title="IFTTT Key" size="32" maxlength="100">
    </div>
    
    <div class="form-group" id="difttt_evilt">
      <label class="col-md-4" for="ifttt_eventName_eviltwin">EvilTwin attack trigger name</label>  
      <input id="ifttt_eventName_eviltwin" name="ifttt_eventName_eviltwin" type="text" placeholder="" title="EvilTwin attack trigger name" size="32" maxlength="32">
    </div>
    
     <div class="form-group" id="difttt_deauth">
      <label class="col-md-4" for="ifttt_eventName_deauth">Deauth attack trigger name</label>  
      <input id="ifttt_eventName_deauth" name="ifttt_eventName_deauth" type="text" placeholder="" title="Deauthentication attack trigger name" size="32" maxlength="32">
     </div>
    
    <div class="form-group" id="alert_server">
      <label class="col-md-4" for="server_ip">Alert Server IP</label>  
      <input id="server_ip" name="server_ip" type="text" placeholder="0.0.0.0" title="Alert server IP" size="15" maxlength="15" pattern="\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$">
    </div>
    
    <div class="form-group">
        <input id="btn_submit_form" name="btn_submit_form"  type="Submit" value="Save Settings" class="btn btn-success">
    </div>
    </div>
    </form>
    </body> 
    </html>
  )=====";



#endif