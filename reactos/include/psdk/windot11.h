
#ifndef _NTDDNDIS_
#include <ntddndis.h>
#endif

/* Enumerations */

#if defined(__midl) || defined(__WIDL__)
typedef [v1_enum] enum _DOT11_BSS_TYPE {
#else
typedef enum _DOT11_BSS_TYPE {
#endif
    dot11_BSS_type_infrastructure = 1,
    dot11_BSS_type_independent,
    dot11_BSS_type_any
} DOT11_BSS_TYPE;

typedef enum _DOT11_PHY_TYPE {
    dot11_phy_type_unknown,
    dot11_phy_type_any,
    dot11_phy_type_fhss,
    dot11_phy_type_dsss,
    dot11_phy_type_irbaseband,
    dot11_phy_type_ofdm,
    dot11_phy_type_hrdsss,
    dot11_phy_type_erp,
    dot11_phy_type_ht,
    dot11_phy_type_IHV_start,
    dot11_phy_type_IHV_end 
} DOT11_PHY_TYPE;

typedef enum _DOT11_AUTH_ALGORITHM {
    DOT11_AUTH_ALGO_80211_OPEN         = 1,
    DOT11_AUTH_ALGO_80211_SHARED_KEY,
    DOT11_AUTH_ALGO_WPA,
    DOT11_AUTH_ALGO_WPA_PSK,
    DOT11_AUTH_ALGO_WPA_NONE,
    DOT11_AUTH_ALGO_RSNA,
    DOT11_AUTH_ALGO_RSNA_PSK,
    DOT11_AUTH_ALGO_IHV_START          = 0x80000000,
    DOT11_AUTH_ALGO_IHV_END            = 0xffffffff 
} DOT11_AUTH_ALGORITHM;

typedef enum _DOT11_CIPHER_ALGORITHM {
    DOT11_CIPHER_ALGO_NONE            = 0x00,
    DOT11_CIPHER_ALGO_WEP40           = 0x01,
    DOT11_CIPHER_ALGO_TKIP            = 0x02,
    DOT11_CIPHER_ALGO_CCMP            = 0x04,
    DOT11_CIPHER_ALGO_WEP104          = 0x05,
    DOT11_CIPHER_ALGO_WPA_USE_GROUP   = 0x100,
    DOT11_CIPHER_ALGO_RSN_USE_GROUP   = 0x100,
    DOT11_CIPHER_ALGO_WEP             = 0x101,
    DOT11_CIPHER_ALGO_IHV_START       = 0x80000000,
    DOT11_CIPHER_ALGO_IHV_END         = 0xffffffff
} DOT11_CIPHER_ALGORITHM;

/* Types */

typedef UCHAR DOT11_MAC_ADDRESS[6];
typedef DOT11_MAC_ADDRESS* PDOT11_MAC_ADDRESS;

typedef struct _DOT11_SSID {
    ULONG uSSIDLength;
    UCHAR ucSSID[32];
} DOT11_SSID, *PDOT11_SSID;

typedef struct _DOT11_NETWORK {
    DOT11_SSID dot11Ssid;
    DOT11_BSS_TYPE dot11BssType;
} DOT11_NETWORK, *PDOT11_NETWORK;

typedef struct _DOT11_NETWORK_LIST {
    DWORD dwNumberOfItems;
    DWORD dwIndex;
#if defined(__midl) || defined(__WIDL__)
    [size_is(dwNumberOfItems)] DOT11_NETWORK Network[];
#else
    DOT11_NETWORK Network[1];
#endif
} DOT11_NETWORK_LIST, *PDOT11_NETWORK_LIST;

typedef struct _DOT11_BSSID_LIST {
    NDIS_OBJECT_HEADER Header;
    ULONG uNumOfEntries;
    ULONG uTotalNumOfEntries;
#if defined(__midl) || defined(__WIDL__)
    [size_is(uTotalNumOfEntries)] DOT11_MAC_ADDRESS BSSIDs[];
#else
    DOT11_MAC_ADDRESS BSSIDs[1];
#endif
} DOT11_BSSID_LIST, *PDOT11_BSSID_LIST;

