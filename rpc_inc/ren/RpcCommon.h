#ifndef COMMON_H
#define COMMON_H

#define  FUNC_GET_4GMODULE_IP                "Get_4GModule_IP"
#define  FUNC_GET_4GMODULE_NETMASK           "Get_4GModule_Netmask"
#define  FUNC_GET_4GMODULE_GATEWAY           "Wifi_Get_Default_Gateway"
#define  FUNC_GET_4GMODULE_DNS               "Wifi_Get_Default_DNS"
#define  FUNC_GET_4GMODULE_ICCID             "Get_4GModule_ICCID"
#define  FUNC_GET_4GMODULE_SIGLEVEL          "Get_4GModule_SigLevel"
#define  FUNC_WIFI_SET_ENABLE                "Wifi_Set"
#define  FUNC_WIFI_SET_STA_WLAN0_CARDINFO    "Wifi_Set_STA_Wlan0_CardInfo"
#define  FUNC_WIFI_SET_GATEWAY               "Wifi_Set_Default_Gateway"
#define  FUNC_WIFI_GET_MACADDR               "Wifi_Get_MacAddr"
#define  FUNC_WIFI_GET_STA_WLAN0_IP          "Wifi_Get_STA_Wlan0_IP"
#define  FUNC_WIFI_GET_STA_WLAN0_NETMASK     "Wifi_Get_STA_Wlan0_Netmask"
#define  FUNC_WIFI_GET_STA_WLAN0_GATEWAY     "Wifi_Get_Default_Gateway"
#define  FUNC_WIFI_GET_STA_DNS               "Wifi_Get_Default_DNS"
#define  FUNC_WIFI_GET_AP_ENCRYPT_Mode       "Wifi_Get_Encrypt_Mode"
#define  FUNC_WIFI_GET_AP_WLAN1_IP           "Wifi_Get_AP_Wlan1_IP"
#define  FUNC_WIFI_GET_AP_WLAN1_NETMASK      "Wifi_Get_AP_Wlan1_Netmask"
#define  FUNC_WIFI_GET_AP_WLAN1_GATEWAY      "Wifi_Get_Default_Gateway"

#define FUNC_WIFI_GET_WHITELIST  "getWhiteList"
#define FUNC_WIFI_GET_BLACKLIST "getBlackList"
#define FUNC_DELETE_BLACKLIST  "clearBlackList"
#define FUNC_DELETE_WHITELIST  "clearWhiteList"
#define FUNC_GET_BLACKLIST_COUNT  "getBlackListCount"
#define FUNC_GET_WHITELIST_COUNT  "getWhiteListCount"
#define FUNC_GET_BLACKLIST_BY_INDEX  "getBlackListByIndex"
#define FUNC_GET_WHITELIST_BY_INDEX  "getWhiteListByIndex"
#define FUNC_ADD_BLACKLIST  "addBlackList"
#define FUNC_ADD_WHITELIST  "addWhiteList"
#define FUNC_DELETE_BLACKLIST_MAC "removeBlackList"
#define FUNC_DELETE_WHITELIST_MAC "removeWhiteList"
#define FUNC_GET_FILTER_MODE  "getFilterMode"
#define FUNC_SET_FILTER_MODE  "setFilterMode"
#define FUNC_GET_WIFI_MAX_COUNT  "getWifiMaxCount"
#define FUNC_SET_WIFI_MAX_COUNT  "setWifiMaxCount"
#define FUNC_REBOOT_SYSTEM  "rebootSystem"

typedef enum _connect_status {
  CONNECTED,
  DISCONNECTED
} Conn_Status;

typedef enum _wifi_state {
  FAIL = -6,
  BUSY,
  NOREADY,
  PARM_INVALID,
  RET_INVALID,
  SOCK_ERROR,
  OK
} State;

//typedef enum _wifi_mode_type {
//  AP,     //AP模式
//  AP_AP,  //双AP模式
//  AP_STA  //AP+STA模式
//} WIFI_MODE_TYPE;
//
//typedef enum _wifi_enable {
//  DISABLE, //关闭
//  ENABLE  //打开
//} WIFI_ENABLE;

typedef enum _netcard_type {
  WLAN0,//wifi sta
  WLAN1,//wifi ap
  BRIDGE0,//4g
  RMNET_DATA0,//4g
  RMNET_DATA1//4g
} Netcard;

typedef enum _wifi_encrypt_type {
  NOMAL,
  WEP,
  TKIP,
  AES,
  AES_TKIP
} WIFI_ENCRYPT_TYPE;

#endif // COMMON_H
