#ifndef WIFIUTIL_H
#define WIFIUTIL_H

#include <string>
#include <map>
#include <set>
#include <regex>
#include <thread>
#include <chrono>

#include "unistd.h"
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/route.h>

#include "tiolog.hpp"
#include <RpcCommon.h>

extern "C" {
#include "WiFiControl.h"
#include "ATControl.h"

}
#include <UIMControl.h>

#define IP_LENGTH 30
#define IPV4_LENGTH 16
//#define SUCCESS 1
//#define FAIL 0
#define NETCARD_LENGTH 32

#define WIFI_DEFAULT_STATUS -1
#define WIFI_DEFAULT_AP_SSID "SIMCOM7600WIFI"
#define WIFI_DEFAULT_AP_PASSWORD "1234567890"
#define WIFI_DEFAULT_STA_SSID "renhono"
#define WIFI_DEFAULT_STA_PASSWORD "12345678"
#define COMMAND_TURE 0
#define COMMAND_FALSE -1
#define WIFI_SET_S_S 1
#define WIFI_SET_S_E 0
#define WIFI_SET_ON 1
#define WIFI_SET_OFF 0
#define B_TURE 1
#define B_FALSE 0

#define  WIFI_SSID_MAX_LEN 32
#define  WIFI_PASS_MAX_LEN 63
#define  BUFF_MAX_LEN 64
#define  IPLength 4
#define  IPCharLength 13


#define NETCARD_STA "wlan0"
#define NETCARD_AP "wlan1"

#define DEFAULT_WIFI 0
#define DEFAULT_WIFI_PATH "/data/dwifi"
#define WIFI_CONF_PATH "/data/dwifi"
#define WIFI_RE_TIME 120

#define WIFI_WAIT_INIT_TIME 2
#define WIFI_OPEN_WAIT_TIME 30
#define WIFI_CLOSE_WAIT_TIME 10
#define WIFI_MODE_WAIT_TIME 60
#define WHITELIST_PATH "/data/mifi/hostapd.accept"
#define BLACKLIST_PATH "/data/mifi/hostapd.deny"
#define HOSTAPD_CONF_PATH "/data/mifi/sta_mode_hostapd.conf"

using namespace std;

typedef struct {
  string wifiApSsid;
  string wifiApPassword;
  string wifiStaSsid;
  string wifiStaPassword;
  int auth_type;
  int encrypt_mode;
  int mode;

} wifiParameter;

//用于记录获取的时时状态信息
//typedef struct {
//  //SIM卡是否拔出
//  uint8_t isSIMCardPulledOut;
//  //4G是否连网 0:没有网络, 1: lte网络正常
//  uint8_t isLteNetworkAvailable;
//  //网络连接类型 0:没有网络, 1: lte连接网络上网, 2: wifi sta连接网络上网
//  uint8_t NetworkConnectionType;
//  //Wifi连接状态
//  uint8_t wifiConnectionType;
//  //LTE注网状态      0,未注册网络, 1:注册上网络
//  uint8_t networkRegSts;
//  //网络信号强度
//  uint8_t signalStrength;
//} TBoxNetworkStatus;

//typedef enum _wifi_state {
//  OK,
//  FAIL,
//  BUSY,
//  NOREADY,
//  PARM_INVALID,
//  RET_INVALID,
//  SOCK_ERROR
//} State;

//typedef enum _wifi_work_status
//{
//    WIFI_FAIL = -1,
//    WIFI_INIT,
//    WIFI_SUCCESS
//}WifiStatus;

typedef enum _auth {
  Auto,
  Open,
  Share,
  WPA_PSK,
  WPA2_PSK,
  WPA_WPA2_PSK
} AuthType;

//typedef enum _encrypt {
//  NUL,
//  WEP,
//  TKIP,
//  AES,
//  AES_TKIP
//} EncryptMode;

typedef enum _wifi_work {
  WIFI_SET_SWITCH_STARTING,
  WIFI_WORK_OPENING,
  WIFI_WORK_CLOSING,
  WIFI_WORK_SET_AP_SSID,
  WIFI_WORK_SET_AP_PASS,
  WIFI_WORK_SET_STA,
  WIFI_WORK_SET_BUSY,
  WIFI_WORK_SET_FAIL,
  WIFI_WORK_FREE
} WifiWork;

typedef enum _switch_work_step {
  WORK_INIT,
  WORK_OPENING,
  WORK_CLOSING,
  WORK_NEXT
} SwitchWorkStep;
typedef enum _ap_work_step {
  WORK_AP_INIT,
  WORK_AP_SET_SSID,
  WORK_AP_SET_PASS,
  WORK_AP_NEXT
} APWorkStep;
typedef enum _sta_work_step {
  WORK_STA_INIT,
  WORK_STA_NEXT
} STAWorkStep;

class wifiUtil {
 public:
  wifiUtil();
  static string getWifiInfo(int type, int info);
  bool wifiCtrl(bool on);
  int wifiGetStatus(void);
  string getIPAddress(int interface);
  string getNetmask(int interface);
  string getApSsid(void);
  string getApPassword(void);

  //fpp 200110
  bool Wifi_Set_Switch(int isOpen);
  bool Wifi_Set_Mode(int mode);
  bool Wifi_Set_LocalSsid(string ssid, int mode);
  static bool Wifi_Set_LocalPasswork(const string &password, int auth_type, int encrypt_mode, int mode);
  bool Wifi_Set_STA_Config(string ssid, string password);

  string Wifi_Get_LocalSsid(int mode);
  string Wifi_Get_LocalPasswork(int mode);
  string Wifi_Get_MacAddr(int mode);
  string Wifi_Get_STA_IP(void);
  string Wifi_Get_STA_Ssid(void);
  string Wifi_Get_STA_Passwork(void);
  string Wifi_Get_Auth_Type(int mode);
  int Wifi_Get_Encrypt_Mode(int interface);
  string Wifi_Get_STA_Netmask(void);
  string Wifi_Get_Default_Gateway(string netcard);
  string Wifi_Get_Default_DNS(void);
  int Wifi_Get_Mode(void);
  int Wifi_Get_Switch_Status(void);

  //fpp 200115
  map<string, string> Wifi_Get_Auth(int mode);
  bool Wifi_Set_Config(string ap_ssid,
                       int auth_type,
                       int encrypt_mode,
                       string ap_password,
                       string sta_ssid,
                       string sta_password,
                       int mode);

  string Wifi_Get_Echo();

  //fpp 20200225
  string Get_4GModule_ICCID(void);
  string Get_4GModule_SigLevel(void);

  int get_addr(char *addr, int flag, char *netcard);

  //fpp 20200226

  string Get_4GModule_IP(void);              //获取4G 的IP        o
  string Get_4GModule_Netmask(void);    //获取4G 的Netmask         o
//    string Get_4GModule_Gateway(string &gateway);    //获取4G 的Gateway
//    string Get_4GModule_DNS(string &dns);            //获取4G DNS字串
  string Get_4GModule_ConnStatus(void);     //获取4G 是否连接

  string Wifi_Get_AP_Wlan1_IP(void);              //获取AP模式下，wlan1的IP          o
  string Wifi_Get_AP_Wlan1_Netmask(void);    //获取AP模式下，wlan1的Netmask       o
//    string Wifi_Get_AP_Wlan1_Gateway(void);    //获取AP模式下，wlan1的Gateway
  string Wifi_Get_STA_Wlan0_IP(void);             //获取STA模式下，wlan0的IP              o
  string Wifi_Get_STA_Wlan0_Netmask(void);   //获取STA模式下，wlan0的Netmask           o
//    string Wifi_Get_STA_Wlan0_Gateway(void);   //获取STA模式下，wlan0的Gateway

  bool isIPAddressValid(const char *pszIPAddr);
  bool isNetmaskValid(const char *netmask);

  int Set_4GModule_IP(string ip);
  int Set_4GModule_Netmask(string ip);
  int Wifi_Set_AP_Wlan1_IP(string ip);
  int Wifi_Set_AP_Wlan1_Netmask(string ip);
  int Wifi_Set_STA_Wlan0_IP(string ip);
  int Wifi_Set_STA_Wlan0_Netmask(string ip);

  int set_addr(const char *ip, int flag, char *netcard);

  int Wifi_Set_Default_Gateway(string ip, string netcard);
  int set_gateway(const char *ip);

  int Wifi_Set_STA_Wlan0_CardInfo(string ip, string netmask);

  static int netcard_Valid(string netcard);
  static int sim_Valid(void);

  //monitor
  void Wifi_Monitor();
  int wifi_switch(uint8_t isOpen);

  int Wifi_Set(int wifi_swicth, string ap_ssid, string ap_pass, string sta_ssid, string sta_pass);
  string Wifi_Get();

  //process wifi
  int tryWifiOpenProcess();
  int tryWifiCloseProcess();
  int tryWifiSetAP();
  int tryWifiSetSTA();

  int Wifi_Get_Work_Status(void);              //获取WIFI设置工作状态

  int WifiSwicthProcess();
  int WifiAPProcess();
  int WifiSTAProcess();
  int WifiModeProcess();

  static void trim(string &s);
  int WifiDefaultControl();

  static int writeConf(int wifis);
  static int readConf();

  //20200312

  /**
   * @author RenHono.fpp
   * @date 20200324
   * @param mode : 0 blacklist, 1 whitelist, 2 radius server
   * @return cmd status
   */
  int setFilterMode(int mode);
  int getFilterMode();
  int setWhiteList();
  vector<string> getWhiteList();
  int addWhiteList(string mac);
  int removeWhiteList(string mac);
  int clearWhiteList();
  int setBlackList();
  vector<string> getBlackList();
  int addBlackList(string mac);
  int removeBlackList(string mac);
  int clearBlackList();
  vector<string> readLineFromFile(string path);
  int writeToFile(vector<string> list, string path);
  int clearFile(string path);
  int restartHostapd();
  string getWhiteListByIndex(int index);
  string getBlackListByIndex(int index);

  int addList(string mac, string path);
  int removeList(string mac, string path);
  string getListByIndex(int index, string path);
  int getListCount(string path);

  //TODO: rename
  int shellPopen(const char *cmd);
  int shellPopen(const char *cmd, string &rs);

  static string getTrimString(string res);
  void setTrimString(string &res);
  int rebootSystem();
  int regexMAC(string mac);
  int getWhiteListCount();
  int getBlackListCount();

  int getWifiMaxCount();
  int setWifiMaxCount(int count);

 private:

  int _loop_count;
  int _wifi_switch_status;
  string _wifi_ap_ssid;
  string _wifi_ap_password;
  string _wifi_sta_ssid;
  string _wifi_sta_password;

  int _wifi_wlan0_set_status;
  int _wifi_wlan1_set_status;

  int _wifi_work_status;
  int _wifi_open_count;
  int _wifi_close_count;

  int _authType;
  int _encryptMode;

  int _wifi_netcard_count;

  void printIntArray(int *array);
  int charToInt(char ch, int sum);
  void getIntOfIp(char *ip, int *intArray);
  void ipAndSubIP(int *nums1, int *nums2, int *nums3);
  bool compareInt(int *nums1, int *nums2);
  void getFlagToString(int *nums, char *ch);
  bool getIPMaskCompareResult(char *ip1, char *ip2, char *ip3, char *ip4);
};

#endif // WIFIUTIL_H
