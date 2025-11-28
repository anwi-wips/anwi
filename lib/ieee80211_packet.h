#ifndef _PACKET_H_
#define _PACKET_H_

//#include <Arduino.h>

#define SNIFF_BUF2_MAX_SIZE 112
#define SNIFF_BUF_MAX_SIZE 36
#define MAX_SSID_LEN 32
#define MAC_LEN 6
#define MAC_LEN_FMT 18


#define IEEE80211_FTYPE_MGMT		0x0000

/* management packet subtype flags converted to little endian */

#define IEEE80211_STYPE_ASSOC_REQ	0x0000
#define IEEE80211_STYPE_ASSOC_RESP	0x0001
#define IEEE80211_STYPE_REASSOC_REQ	0x0002
#define IEEE80211_STYPE_REASSOC_RESP	0x0003
#define IEEE80211_STYPE_PROBE_REQ	0x0004
#define IEEE80211_STYPE_PROBE_RESP	0x0005
#define IEEE80211_STYPE_BEACON		0x0008
#define IEEE80211_STYPE_ATIM		0x0009
#define IEEE80211_STYPE_DISASSOC	0x000A
#define IEEE80211_STYPE_AUTH		0x000B
#define IEEE80211_STYPE_DEAUTH		0x000C
#define IEEE80211_STYPE_ACTION		0x000D


//http://docs.ros.org/diamondback/api/wpa_supplicant/html/ieee802__11__defs_8h_source.html

#define WLAN_CIPHER_SUITE_USE_GROUP     0x000FAC00
#define WLAN_CIPHER_SUITE_WEP40         0x000FAC01
#define WLAN_CIPHER_SUITE_TKIP          0x000FAC02
#define WLAN_CIPHER_SUITE_CCMP          0x000FAC04
#define WLAN_CIPHER_SUITE_WEP104        0x000FAC05
#define WLAN_CIPHER_SUITE_AES_CMAC      0x000FAC06
#define WLAN_AKM_SUITE_8021X            0x000FAC01
#define WLAN_AKM_SUITE_PSK              0x000FAC02


struct RxControl 
{
	signed rssi : 8;
	unsigned rate : 4;
	unsigned is_group : 1;
	unsigned : 1;
	unsigned sig_mode : 2;
	unsigned legacy_length : 12;
	unsigned damatch0 : 1;
	unsigned damatch1 : 1;
	unsigned bssidmatch0 : 1;
	unsigned bssidmatch1 : 1;
	unsigned MCS : 7;
	unsigned CWB : 1;
	unsigned HT_length : 16;
	unsigned Smoothing : 1;
	unsigned Not_Sounding : 1;
	unsigned : 1;
	unsigned Aggregation : 1;
	unsigned STBC : 2;
	unsigned FEC_CODING : 1;
	unsigned SGI : 1;
	unsigned rxend_state : 8;
	unsigned ampdu_cnt : 8;
	unsigned channel : 4;
	unsigned : 12;
};

struct Ampdu_Info
{
  uint16_t length;
  uint16_t seq;
  uint8_t  address3[6];
}__attribute__ ((packed));

struct sniffer_buf 
{
    struct RxControl rx_ctrl;
    uint8_t  buf[SNIFF_BUF_MAX_SIZE];
    uint16_t cnt;
    struct Ampdu_Info ampdu_info[1];
};

// MGMT PACKET esp8266-technical_reference_en.pdf, #108

struct sniffer_buf2
{
    struct RxControl rx_ctrl;
    uint8_t buf[SNIFF_BUF2_MAX_SIZE];
    uint16_t cnt;
    uint16_t len; //length of packet
}__attribute__ ((packed));


struct ieee80211_frame_control_flags
{
	unsigned	to_ds : 1;
	unsigned	from_ds : 1;
	unsigned	more_frag : 1;
	unsigned	retry : 1;
	unsigned	power_mng : 1;
	unsigned	more_data : 1;
	unsigned	protected_frame : 1;
	unsigned	order : 1;
}__attribute__ ((packed));


//802.11-2016.pdf, #670
struct ieee80211__frame_control
{
	unsigned	protocol_version : 2;
	unsigned	type : 2;
	unsigned	sub_type : 4;
	struct ieee80211_frame_control_flags frame_control_flags;
}__attribute__ ((packed));


struct ieee80211_sequence_control
{
	unsigned	fragment_number : 4;
	unsigned	sequence_number : 12; 
}__attribute__ ((packed));


struct ieee80211_deauth_or_disassoc
{
	uint16_t reason_code; // __le16
}__attribute__ ((packed));



/** 802.11 Robust Security Network ("WPA") information element
 * Many fields reference a cipher or authentication-type ID; this is a
 * three-byte OUI followed by one byte identifying the cipher with
 * respect to that OUI. For all standard ciphers the OUI is 00:0F:AC,
 * except in old-style WPA IEs encapsulated in vendor-specific IEs,
 * where it's 00:50:F2.
 */

 struct ieee80211_ie_rsn_pairwise_cipher
 {
	uint8_t pairwise_cipher[4];
 }__attribute__ ((packed));

 
struct ieee80211_ie_vendor_specific
{
	uint8_t oui[4];		/**< OUI and vendor-specific type byte */
	uint8_t data[0];
}__attribute__ ((packed));

struct ieee80211_ie_rsn 
{
	uint16_t version;
	uint8_t group_cipher[4];
	uint16_t pairwise_count;
	struct ieee80211_ie_rsn_pairwise_cipher pairwise_cipher[2];
	uint16_t akm_count;
	uint32_t akm_list[1];
	uint16_t rsn_capab;
	uint16_t pmkid_count;
	uint8_t pmkid_list[0];
}__attribute__((packed));

/** Information element ID for Robust Security Network information element */
#define IEEE80211_IE_RSN	48

#define IEEE80211_IE_VENDOR_SPECIFIC	221

/** 802.11 SSID information element */
struct ieee80211_ie_ssid 
{
	char ssid[0];		/**< SSID data, not NUL-terminated */
}__attribute__ ((packed));

/** Information element ID for SSID information element */
#define IEEE80211_IE_SSID	0

struct ieee80211_ie_type 
{
	uint8_t id;
	uint8_t len;
}__attribute__ ((packed));

struct ieee80211_beacon //https://mrncciew.com/2014/10/08/802-11-mgmt-beacon-frame/
{	
	uint64_t timestamp; // __le64
	uint16_t beacon_interval; // __le16
	uint16_t capab_info; // __le16 //today
	//struct ieee80211_capab_info capab_info;
	struct ieee80211_ie_type ie_type;
	//union 
	//{
		struct ieee80211_ie_ssid ssid_ie;
		struct ieee80211_ie_rsn rsn_ie;
		struct ieee80211_ie_vendor_specific ie_vendor_specific;
		
	//}__attribute__ ((packed));  
	
}__attribute__ ((packed));

//ieee80211 Managment Packet  802.11-2016.pdf, #692
struct ieee80211_frame_header
{
	
	struct ieee80211__frame_control frame_control;
	uint16_t	duration_id;
	uint8_t		destination_address[MAC_LEN];
	uint8_t		source_address[MAC_LEN];
	uint8_t		bssid_address[MAC_LEN];

	//Sequence Control,802.11-2016.pdf #
	// Instead of using struct, using uint8_t to bypass extra byte bug causing issues with ssid_len
	//struct ieee80211_sequence_control sequence_control; //// making it 4 instead of 12 to bypass bug of extra one 8 causing issue with beacon
	uint16_t seq_ctrl;
	union 
	{	
			struct ieee80211_deauth_or_disassoc deauth;
			struct ieee80211_beacon beacon;
	}__attribute__ ((packed));
}__attribute__((__packed__));

#endif