#include "debug_print.h"
#include "..\global_vars.h"
#include "..\ieee80211_packet.h"
#include <Arduino.h>

extern "C" {
#include <user_interface.h>
}

/**
 * @brief Prints debug info for beacon frames.
 *
 * Checks if an Evil Twin attack flag is set and prints a message if so.
 */
void debug_print_beacon() {
  if (is_evil_twin_attack) {
    Serial.print("Evil Twin Detected ");
    is_evil_twin_attack = false;
  }
}

/**
 * @brief Prints debug info for deauth/disassoc frames.
 *
 * Prints details of the detected deauthentication or disassociation attack,
 * including channel, source MAC, destination MAC, BSSID, and reason code.
 */
void debug_print_deauth() {
  char mac[MAC_LEN_FMT];

  Serial.println("DEAUTH ATTACK");
  Serial.println("Channel ");
  Serial.println(pkt_info.channel);

  Serial.print("SRC MAC ");
  sprintf(mac, MACSTR, MAC2STR(pkt_info.frame_hdr.source_address));
  Serial.println(mac);

  Serial.print("DST MAC ");
  sprintf(mac, MACSTR, MAC2STR(pkt_info.frame_hdr.destination_address));
  Serial.println(mac);

  Serial.print("BSSID MAC ");
  sprintf(mac, MACSTR, MAC2STR(pkt_info.frame_hdr.bssid_address));
  Serial.println(mac);

  if (pkt_info.is_disassoc_detected) {
    Serial.print("Disassoc Reason ");
  }

  else {
    Serial.print("Deauth Reason ");
  }
  Serial.println(pkt_info.frame_hdr.deauth.reason_code);
}