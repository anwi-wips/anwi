
#include "packet_capture.h"
#include <Arduino.h>

struct captured_packet_info pkt_info;

char bssid_mac[MAC_LEN_FMT];

/**
 * @brief Initializes the WiFi sniffer.
 *
 * Sets the WiFi mode to Station, sets the channel, and enables promiscuous mode
 * with the defined callback function.
 */
void init_sniffing() {
  wifi_set_opmode(STATION_MODE);
  wifi_set_channel(set_channel);
  wifi_promiscuous_enable(DISABLE);
  delay(10);
  wifi_set_promiscuous_rx_cb(sniffer_wifi_promiscuous_rx);
  delay(10);
  wifi_promiscuous_enable(ENABLE);
}

/**
 * @brief Callback function for processing captured packets.
 *
 * Called by the ESP8266 SDK when a packet is received in promiscuous mode.
 * Filters and analyzes management frames (Beacon, Deauth, Disassoc) to detect
 * attacks like Evil Twin and Deauthentication flooding.
 *
 * @param buf Pointer to the packet buffer.
 * @param buf_len Length of the packet buffer.
 */
void sniffer_wifi_promiscuous_rx(uint8 *buf, uint16 buf_len) {
  uint8_t *data;
  uint16_t i;
  uint16_t len;
  uint16_t cnt = 0;

  // MGMT Packet == buf2
  if (buf_len == sizeof(struct sniffer_buf2)) {
    struct sniffer_buf2 *ptr_buf2 = (struct sniffer_buf2 *)buf;
    struct ieee80211_frame_header *ptr_frame =
        (struct ieee80211_frame_header *)ptr_buf2->buf;
    if (ptr_frame->frame_control.type == IEEE80211_FTYPE_MGMT) {
      pkt_info.type = IEEE80211_FTYPE_MGMT;
      pkt_info.rssi = ptr_buf2->rx_ctrl.rssi;
      pkt_info.channel = wifi_get_channel();
      pkt_info.attack_type = -1;

      if (ptr_frame->frame_control.frame_control_flags.to_ds == 0 &&
          ptr_frame->frame_control.frame_control_flags.from_ds == 0) {
        memcpy(pkt_info.frame_hdr.destination_address,
               ptr_frame->destination_address, MAC_LEN);
        memcpy(pkt_info.frame_hdr.source_address, ptr_frame->source_address,
               MAC_LEN);
        memcpy(pkt_info.frame_hdr.bssid_address, ptr_frame->bssid_address,
               MAC_LEN);
      }
      if (ptr_frame->frame_control.frame_control_flags.to_ds == 0 &&
          ptr_frame->frame_control.frame_control_flags.from_ds == 1) {
        memcpy(pkt_info.frame_hdr.destination_address,
               ptr_frame->destination_address, MAC_LEN);
        memcpy(pkt_info.frame_hdr.bssid_address, ptr_frame->source_address,
               MAC_LEN);
        memcpy(pkt_info.frame_hdr.source_address, ptr_frame->bssid_address,
               MAC_LEN);
      }
      if (ptr_frame->frame_control.frame_control_flags.to_ds == 1 &&
          ptr_frame->frame_control.frame_control_flags.from_ds == 0) {
        memcpy(pkt_info.frame_hdr.bssid_address, ptr_frame->destination_address,
               MAC_LEN);
        memcpy(pkt_info.frame_hdr.source_address, ptr_frame->source_address,
               MAC_LEN);
        memcpy(pkt_info.frame_hdr.destination_address, ptr_frame->bssid_address,
               MAC_LEN);
      }
      // if ( to_ds == 1 && from_ds ==1 ) // No support for WDS presently

      switch (ptr_frame->frame_control.sub_type) {
      case IEEE80211_STYPE_DISASSOC: // Both disassociation and deauthentication
                                     // have same struct
                                     // pkt_info.is_disassoc_detected = true;
      case IEEE80211_STYPE_DEAUTH:

        sprintf(bssid_mac, MACSTR, MAC2STR(pkt_info.frame_hdr.bssid_address));

        if ((strcmp(sensor_config.protect_ap_info.BSSID_lower, bssid_mac) ==
             0) ||
            (strcmp(sensor_config.protect_ap_info.BSSID_upper, bssid_mac) ==
             0)) {
          if (pkt_info.frame_hdr.deauth.reason_code != 4) {
            pkt_info.is_deauth_detected = true;
            deauth_pkt_counter++;
            pkt_info.frame_hdr.deauth.reason_code =
                ptr_frame->deauth.reason_code;
          }
        }

        break;

      case IEEE80211_STYPE_BEACON:
        // reset all flags here
        pkt_info.has_ie_vendor_specific = false;
        pkt_info.has_ie_rsn = false;
        pkt_info.is_beacon_detected = false;

        struct ieee80211_beacon *curr_ptr =
            (struct ieee80211_beacon *)&ptr_frame->beacon;
        uint8_t *next_ptr;
        uint8_t *tmp_ptr;
        uint8_t offset_of_next_ie = 0;

        for (offset_of_next_ie = 0; offset_of_next_ie < SNIFF_BUF2_MAX_SIZE;
             offset_of_next_ie = curr_ptr->ie_type.len + 2) {
          switch (curr_ptr->ie_type.id) {
          case IEEE80211_IE_SSID:

            pkt_info.is_ssid_hidden == false;
            // Check if AP is OPEN or using encryption using Privacy Bit
            pkt_info.is_encrypted =
                (ptr_frame->beacon.capab_info >> 4) & 0X0001;
            if (ptr_frame->beacon.ie_type.len > 32) {
              break; // Do not proces SSID > 32
            }
            if (ptr_frame->beacon.ie_type.len == 0) {
              pkt_info.is_ssid_hidden == true;
              break;
            }
            memset((&pkt_info.frame_hdr.beacon.ssid_ie.ssid), 0,
                   MAX_SSID_LEN); // Clear det_ssid
            memcpy(pkt_info.frame_hdr.beacon.ssid_ie.ssid,
                   ptr_frame->beacon.ssid_ie.ssid,
                   ptr_frame->beacon.ie_type.len);
            if (!pkt_info.is_encrypted) {
              // Serial.println(sensor_config.protect_ap_info.SSID);

              if (strcmp(sensor_config.protect_ap_info.SSID,
                         pkt_info.frame_hdr.beacon.ssid_ie.ssid) == 0) {
                pkt_info.attack_type = IS_EVILTWIN_ATTACK;
              }
            }
            break;

            // http://lists.shmoo.com/pipermail/hostap/2008-July/018074.html
            // WPA OUI = 00:50:F2
            // https://msdn.microsoft.com/en-us/library/windows/hardware/ff565906(v=vs.85).aspx

          case IEEE80211_IE_VENDOR_SPECIFIC:
            pkt_info.has_ie_vendor_specific = true;
            memcpy(pkt_info.frame_hdr.beacon.ie_vendor_specific.oui,
                   curr_ptr->ie_vendor_specific.oui,
                   sizeof(curr_ptr->ie_vendor_specific.oui));
            break;

          case IEEE80211_IE_RSN:

            pkt_info.has_ie_rsn = true;
            memcpy(pkt_info.frame_hdr.beacon.rsn_ie.group_cipher,
                   curr_ptr->rsn_ie.group_cipher,
                   sizeof(curr_ptr->rsn_ie.group_cipher));
            pkt_info.frame_hdr.beacon.rsn_ie.pairwise_count =
                curr_ptr->rsn_ie.pairwise_count;

            uint8_t cntr2 = 0;
            cntr2 = curr_ptr->rsn_ie.pairwise_count;
            for (cntr = 0; cntr < cntr2; cntr++) {
              memcpy(pkt_info.frame_hdr.beacon.rsn_ie.pairwise_cipher[cntr]
                         .pairwise_cipher,
                     curr_ptr->rsn_ie.pairwise_cipher,
                     sizeof(curr_ptr->rsn_ie.pairwise_cipher));
              tmp_ptr = (uint8_t *)curr_ptr +
                        sizeof(ieee80211_ie_rsn_pairwise_cipher);
              curr_ptr = (struct ieee80211_beacon *)tmp_ptr;
            }
            break;
          }
          next_ptr = (uint8_t *)curr_ptr + offset_of_next_ie;
          curr_ptr = (struct ieee80211_beacon *)next_ptr;
        }
        break;
      }
    }
    return;
  }
}
