#include "wifiutil.h"
//#include <stdio.h>


#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>

#include <NASControl.h>
#include <iomanip>

wifiUtil::wifiUtil() {
  //增加配置文件

//    uint8_t wifi_status;
//    get_wifi_status(&wifi_status);
//    logTrace << "wifi_status" << wifi_status << endl;
//    if(wifi_status)
//    {
//        _wifi_switch_status = WIFI_SET_S_S;
//    }
//    else
//    {
//        _wifi_switch_status = WIFI_SET_S_E;
//    }
//
//



//    if(DEFAULT_WIFI)
//    {
//        _wifi_switch_status = WIFI_SET_S_S;
//    }


  WifiDefaultControl();

  _wifi_open_count = WIFI_WAIT_INIT_TIME;
  _wifi_close_count = WIFI_WAIT_INIT_TIME;

  _wifi_ap_ssid = Wifi_Get_LocalSsid(AP_INDEX_STA);
  _wifi_ap_password = Wifi_Get_LocalPasswork(AP_INDEX_STA);
  _wifi_sta_ssid = Wifi_Get_STA_Ssid();
  _wifi_sta_password = Wifi_Get_STA_Passwork();

  _wifi_netcard_count = WIFI_RE_TIME;

  _authType = WPA_WPA2_PSK;
  _encryptMode = AES_TKIP;

  std::thread wifiMonitorThread([this]() {
    Wifi_Monitor();
  });
  wifiMonitorThread.detach();
}

string wifiUtil::getWifiInfo(int type, int info) {
  logTrace << "getWifiInfo,type=" << type << " info=" << info << std::endl;

  /* test */
  return "192.168.100.100";
}

bool wifiUtil::wifiCtrl(bool on) {
  bool r = false;
  logTrace << "wifiCtl,on=" << on << endl;

  if (on) r = wifi_power(1);
  else r = wifi_power(0);

  if (!r) {
    logError << COLOR(red) << "wifi_power error" << COLOR(none) << endl;
  }

  return r;
}

int wifiUtil::wifiGetStatus(void) {
  bool r;
  uint8_t status = 0;
  r = get_wifi_status(&status);

  if (!r) {
    logError << COLOR(red) << "get_wifi_status error" << COLOR(none) << endl;
    status = 99;
  }

  logTrace << "wifiGetStatus status:" << (int) status << endl;

  return status;
}

/**
 * @brief get ip address of specified network interface
 *        Note: only support IPv4
 * @param interface
 *          1: wifi AP mode address (bridge0)
 *          2: wifi STA mode address (wlan1)
 * @return address of NIC or empty string if not found
 */
string wifiUtil::getIPAddress(int interface) {
  struct ifaddrs *ifAddrStruct = NULL;
  struct ifaddrs *ifa = NULL;
  void *tmpAddrPtr = NULL;
  string ipAddress = string("");
  string nicName = string("");

  switch (interface) {
    case 1:nicName = "bridge0";
      break;
    case 2:nicName = "wlan1";
      break;
    default:nicName = "unknown";
      break;
  }

  logTrace << "getIPAddress, interface:" << interface
           << " nicName:" << nicName << endl;

  getifaddrs(&ifAddrStruct);

  for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr) continue;

    if (ifa->ifa_addr->sa_family == AF_INET &&
        nicName.compare(ifa->ifa_name) == 0) {

      tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
      char addressBuffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
      printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
      ipAddress = string(addressBuffer);
    }
  }
  if (ifAddrStruct != NULL) freeifaddrs(ifAddrStruct);

  return ipAddress;
}

string wifiUtil::getNetmask(int interface) {
  string nicName;
  switch (interface) {
    case 1:nicName = "bridge0";
      break;
    case 2:nicName = "wlan1";
      break;
    default:nicName = "unknown";
      break;
  }

  return nicName;
}

string wifiUtil::getApSsid() {
  char ssid[20] = {0};
  bool r;
  r = get_ssid(ssid, AP_INDEX_STA);

  if (r) {
    return string(ssid);
  } else {
    return string("");
  }

}

string wifiUtil::getApPassword() {
  int authType;
  int encryptMode;
  char password[20] = {0};
  bool r;

  r = get_auth(&authType, &encryptMode, password, AP_INDEX_STA);

  if (r) {
    return string(password);
  } else {
    return string("");
  }
}

/**
 *
 *
 */

string wifiUtil::Wifi_Get_LocalSsid(int mode) {
  logTrace << "Wifi_Get_LocalSsid ==" << mode << endl;

  char ssid[20] = {0};
  bool r;

  r = get_ssid(ssid, (ap_index_type) mode);

  cout << " r == " << r << endl;

  if (r) {
    return string(ssid);
  } else {
    logError << COLOR(red) << "Wifi_Get_LocalSsid -- error" << COLOR(none) << endl;
    return string("");
  }

}

string wifiUtil::Wifi_Get_LocalPasswork(int mode) {
  logTrace << "Wifi_Get_LocalPasswork ==" << endl;

  int authType;
  int encryptMode;
  char password[20] = {0};
  bool r;
  ap_index_type modeType;

  r = get_auth(&authType, &encryptMode, password, (ap_index_type) mode);

  if (r) {
    return string(password);
  } else {
    logError << COLOR(red) << "Wifi_Get_LocalPasswork -- error" << COLOR(none) << endl;
    return string("");
  }
}

bool wifiUtil::Wifi_Set_Switch(int isOpen) {
  logTrace << "Wifi_Set_Switch == isOpen == " << isOpen << endl;

//    int s = isOpen ? 1 : 0;
  bool r = false;

  if (wifi_power(isOpen)) {
    r = true;
  } else {
    logError << COLOR(red) << "Wifi_Set_Switch -- error" << COLOR(none) << endl;
    r = false;
  }

  return r;
}

bool wifiUtil::Wifi_Set_Mode(int mode) {
  logTrace << "Wifi_Set_Mode == mode == " << mode << endl;

  bool r = false;

  r = set_wifi_mode((wifi_mode_type) mode);

  if (!r) {
    logError << COLOR(red) << "Wifi_Set_Mode -- error" << COLOR(none) << endl;
  }

  return r;

}

bool wifiUtil::Wifi_Set_LocalSsid(string ssid, int mode) {
  logTrace << "Wifi_Set_LocalSsid == ssid == " << ssid << endl;

  char cssid[20] = {0};
  bool r = false;

  bzero(cssid, sizeof cssid);
  memcpy(&cssid, ssid.data(), strlen(ssid.data()));

  r = set_ssid(cssid, (ap_index_type) mode);

  logTrace << " r == " << r << endl;
  if (!r) {
    logError << COLOR(red) << "Wifi_Set_LocalSsid -- error" << COLOR(none) << endl;
  }

  return r;
}

bool wifiUtil::Wifi_Set_LocalPasswork(const string &password, int auth_type, int encrypt_mode, int mode) {
  logTrace << "Wifi_Set_LocalPasswork == password == " << password << " == auth_type == " << auth_type
           << " ==encrypt_mode== " << encrypt_mode << endl;

  char pwd[20] = {0};
  bool r = false;

  bzero(&pwd, sizeof pwd);
  memcpy(&pwd, password.data(), strlen(password.data()));

  logTrace << "before set_auth " << endl;

  r = set_auth(pwd, auth_type, encrypt_mode, (ap_index_type) mode);
//    r = set_auth(pwd, auth_type,encrypt_mode,AP_INDEX_STA);

  logTrace << "after set_auth " << endl;

  if (!r) {
    logError << COLOR(red) << "Wifi_Set_LocalPasswork -- error" << COLOR(none) << endl;
  }

  return r;
}

string wifiUtil::Wifi_Get_MacAddr(int mode) {
  logTrace << "Wifi_Get_MacAddr ==" << endl;

  char mac[40] = {0};
  bool r;
  r = get_mac_addr(mac, (ap_index_type) mode);

  if (r) {
    return string(mac).substr(2);
  } else {
    logError << COLOR(red) << "Wifi_Get_MacAddr -- error" << COLOR(none) << endl;
    return string("NULL");
  }

}

bool wifiUtil::Wifi_Set_STA_Config(string ssid, string password) {
  logTrace << "Wifi_Set_STA_Config ==" << endl;

  char cssid[20] = {0};
  char cpassword[20] = {0};
  bool r = false;

  bzero(cpassword, sizeof cpassword);
  bzero(cssid, sizeof cssid);
  memcpy(&cssid, ssid.data(), strlen(ssid.data()));
  memcpy(&cpassword, password.data(), strlen(password.data()));

  r = set_sta_cfg(cssid, cpassword);

  if (!r) {
    logError << COLOR(red) << "Wifi_Set_STA_Config -- error" << COLOR(none) << endl;
  }

  return r;
}

string wifiUtil::Wifi_Get_STA_IP(void) {
  logTrace << "Wifi_Get_STA_IP ==" << endl;

  char sta_ip[30] = {0};
  bool r;

  cout << "--------- " << sizeof(sta_ip) << endl;
  r = get_sta_ip(sta_ip, sizeof(sta_ip));

  cout << "========= " << sta_ip << endl;

  if (r) {
    return string(sta_ip);
  } else {
    logError << COLOR(red) << "Wifi_Get_STA_IP -- error" << COLOR(none) << endl;
    return string("NULL");
  }

}

string wifiUtil::Wifi_Get_STA_Ssid(void) {

  logTrace << "Wifi_Get_STA_Ssid ==" << endl;

  char cssid[20] = {0};
  char cpassword[20] = {0};
  bool r;

  r = get_sta_cfg(cssid, cpassword);

  if (r) {
    return string(cssid);
  } else {
    logError << COLOR(red) << "Wifi_Get_STA_Ssid -- error" << COLOR(none) << endl;
    return string("NULL");
  }

}

string wifiUtil::Wifi_Get_STA_Passwork(void) {
  logTrace << "Wifi_Get_STA_Passwork ==" << endl;

  char cssid[20] = {0};
  char cpassword[20] = {0};
  bool r;

  r = get_sta_cfg(cssid, cpassword);

  if (r) {
    return string(cpassword);
  } else {
    logError << COLOR(red) << "Wifi_Get_STA_Ssid -- error" << COLOR(none) << endl;
    return string("NULL");
  }

}

string wifiUtil::Wifi_Get_Auth_Type(int mode) {
  logTrace << "Wifi_Get_Auth_Type " << endl;

  char cpassword[20] = {0};
  int auth_type;
  int encrypt_mode;

  bool r;

  if (!netcard_Valid("wlan1")) {
    return string("NULL");
  }
  r = get_auth(&auth_type, &encrypt_mode, cpassword, (ap_index_type) mode);

  if (r) {
    return to_string(auth_type);
  } else {
    logError << COLOR(red) << "Wifi_Get_Auth_Type -- error" << COLOR(none) << endl;
    return string("NULL");
  }
}

int wifiUtil::Wifi_Get_Encrypt_Mode(int interface) {
  logTrace << "Wifi_Get_Encrypt_Mode == " << endl;

  char cpassword[WIFI_SSID_MAX_LEN] = {0};
  int auth_type;
  int encrypt_mode;
  wifi_mode_type modeType;
  bool r;

  if (!netcard_Valid("wlan1")) {
    return NOREADY;
  }

#if 1
//    modeType = get_wifi_mode();

//    logTrace << "Wifi_Get_Encrypt_Mode == modeType == " << modeType << endl;

  logTrace << "get_auth before" << endl;
  r = get_auth(&auth_type, &encrypt_mode, cpassword, AP_INDEX_STA);
//    r = get_auth(&auth_type, &encrypt_mode, cpassword, AP_INDEX_STA);
  logTrace << "get_auth after" << endl;

  if (r) {
    logTrace << "encrypt_mode == " << encrypt_mode << endl;
    return encrypt_mode;
  } else {
    logError << COLOR(red) << "Wifi_Get_Encrypt_Mode -- error" << COLOR(none) << endl;
    return FAIL;
  }

#endif
//    return 99;
}

string wifiUtil::Wifi_Get_STA_Netmask(void) {

  int socket_fd;
  struct sockaddr_in *sin;
  struct ifreq ifr;

  char *netmask = new char(16);

  if (!access("/sys/class/net/wlan0", F_OK)) {
    logInfo << "waln0 is exists == " << endl;

    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {

      perror("socket \n");
      return string("NULL");

    } else {
      perror("ok \n");

      memset(&ifr, 0, sizeof(ifr));

      strcpy(ifr.ifr_name, "wlan0");

      memset(&sin, 0, sizeof(sin));

      if (ioctl(socket_fd, SIOCGIFNETMASK, &ifr) != -1) {

        sin = (struct sockaddr_in *) &ifr.ifr_broadaddr;

        strcpy(netmask, inet_ntoa(sin->sin_addr));

        logInfo << "Netmask == " << netmask << endl;

        return string(netmask);
      } else {
        logError << COLOR(red) << "no ioctl -- " << COLOR(none) << endl;
        return string("NULL");
      }

    }

  } else {
    logError << COLOR(red) << "no wlan0 -- " << COLOR(none) << endl;
    return string("NULL");
  }

}

string wifiUtil::Wifi_Get_Default_Gateway(string netcard) {

#define TEMP_MAX_LENGTH 1024
#define READ_LENGTH 128
  FILE *gw_fd = NULL;//==null
  char temp[TEMP_MAX_LENGTH] = {0};//宏
  char szNetGate[TEMP_MAX_LENGTH] = {0};

  if (!netcard_Valid(netcard)) {
    return string("NULL");
  }

//    gw_fd = popen("route -n | grep 'UG'", "r");

  string command = "route -n | grep UG | grep " + netcard;

  logTrace << "command====" << command << endl;

  gw_fd = popen(command.c_str(), "r");

  logTrace << "aaaa" << endl;

  if (gw_fd != NULL) {
    logTrace << "aaaa" << endl;

    size_t rsize = fread(temp, 1, READ_LENGTH, gw_fd);
//        fread(temp,1,READ_LENGTH, gw_fd); //宏

    logTrace << "aaaa" << endl;

    logDebug << rsize << endl;

    if (rsize == 0) {
      logTrace << "bbbb" << endl;
      pclose(gw_fd);

      return string("NULL");
    }
    logTrace << "cccc" << endl;

    sscanf(temp, "%*s%s", szNetGate);
    logTrace << "dddd" << endl;

//        printf("Gateway is %s\n", szNetGate);
    logInfo << "Gateway == " << szNetGate << endl;

    if (isIPAddressValid(szNetGate)) {
      pclose(gw_fd);
      return string(szNetGate);
    } else {
//            return string("NO DEFAULT GATEWAY");
      pclose(gw_fd);
      return string("NULL");
    }
  } else {
    logError << COLOR(red) << "no geteway -- " << COLOR(none) << endl;
    pclose(gw_fd);
    return string("NULL");
  }
}

string wifiUtil::Wifi_Get_Default_DNS(void) {
#define TEMP_MAX_LENGTH 1024
#define READ_LENGTH 128
  FILE *dns_fd = NULL;
  char temp[TEMP_MAX_LENGTH] = {0};
  char szDNS1[TEMP_MAX_LENGTH] = {0};

  if (dns_fd = popen("cat /etc/resolv.conf | grep 'nameserver'", "r")) {

//        fread(temp,1,READ_LENGTH, dns_fd);

    size_t rsize = fread(temp, 1, READ_LENGTH, dns_fd); //宏

    if (rsize == 0) {
      pclose(dns_fd);
      return string("NULL");
    }

    sscanf(temp, "%*s%s", szDNS1);

//        printf("DNS1 is %s",szDNS1);
    cout << "DNS1 == " << szDNS1 << endl;
    pclose(dns_fd);

    return string(szDNS1);
  } else {
    logError << COLOR(red) << "no dns -- " << COLOR(none) << endl;
    pclose(dns_fd);

    return string("NULL");
  }
}

int wifiUtil::Wifi_Get_Mode(void) {
  logTrace << "Wifi_Get_Mode == " << endl;

  wifi_mode_type modeType;

  modeType = get_wifi_mode();

  return modeType;
}

int wifiUtil::Wifi_Get_Switch_Status(void) {

  logTrace << "Wifi_Get_Switch_Status" << endl;

  bool r;
  uint8_t flag;

  r = get_wifi_status(&flag);

  if (r) {
    return flag;
  } else {
    logError << COLOR(red) << "Wifi_Get_Switch_Status -- error" << COLOR(none) << endl;
    return -1;
  }
}

map<string, string> wifiUtil::Wifi_Get_Auth(int mode) {
  logTrace << "Wifi_Set_LocalPasswork == passwork == " << endl;

  map<string, string> auth_map;

  char cpassword[20] = {0};
  int auth_type;
  int encrypt_mode;

  bool r = false;

  bzero(&cpassword, sizeof cpassword);

  r = get_auth(&auth_type, &encrypt_mode, cpassword, (ap_index_type) mode);

  if (!r) {
    logError << COLOR(red) << "Wifi_Set_LocalPasswork -- error" << COLOR(none) << endl;
    auth_map.clear();
  } else {
    auth_map["password"] = string(cpassword);
    auth_map["auth_type"] = to_string(auth_type);
    auth_map["encrypt_mode"] = to_string(encrypt_mode);
  }

  return auth_map;
}

bool wifiUtil::Wifi_Set_Config(string ap_ssid, int auth_type, int encrypt_mode, string ap_password, string sta_ssid,
                               string sta_password, int mode) {

  bool r = false;

  return r;
}

string wifiUtil::Wifi_Get_Echo() {
  logTrace << "Wifi_Get_Echo==DING" << endl;
  return string("DING");
}

/**
 * 20200225 cmd
 *
 */


string wifiUtil::Get_4GModule_ICCID(void) {

  uint8_t i;
  int retval;
  char strResult[100];
  string r;

  if (sim_Valid() <= 0) {

    logError << "SIM ERROR" << endl;
    return string("NULL");
  }

  retval = getICCID(strResult, sizeof(strResult));

  if (retval == 0)//宏
  {
    printf("ICCID:%s\n", strResult);
    cout << "ICCID----------" << strResult << endl;
    logTrace << "Get_4GModule_ICCID==" << strResult << endl;
    r = string(strResult);
    trim(r);
//        return string(strResult);
    return r;
  } else {
    logError << COLOR(red) << "no ICCID -- " << COLOR(none) << endl;

    return string("NULL");
  }

}

string wifiUtil::Get_4GModule_SigLevel(void) {
  int r;
//  TBoxNetworkStatus networkStatus; //网络连接状态信息
  int ret;
  int retval;
  uint8_t rssi;

  int sim = sim_Valid();

  logTrace << "sim  " << sim << endl;

  if (sim <= 0) {
    logError << "SIM ERROR" << endl;
    return string("NULL");
  }

  retval = getCSQ(&rssi);
//    r = get_SignalStrength((int *)&networkStatus.signalStrength, (int *)&ret);

//    logTrace << retval << endl;

  logTrace << "+++++++++++++" << to_string(rssi) << endl;

  if (retval > 0) {
    logTrace << "Get_4GModule_SigLevel==" << rssi << endl;
    if (rssi >= 0 && rssi <= 31) {

      int csq = rssi * 2 - 113;
      logTrace << csq << endl;
      logTrace << to_string(csq) << endl;
      return to_string(csq);
    } else {
      logError << COLOR(red) << "signalStrength OVER -- " << COLOR(none) << endl;
      return string("NULL");
    }

  } else {
    logError << COLOR(red) << "no signalStrength -- " << COLOR(none) << endl;
    return string("NULL");
  }
}

//util
int wifiUtil::get_addr(char *addr, int flag, char *netcard) //传 addr 长度
{
  logTrace << addr << "===" << flag << "===" << netcard << endl;

  int sockfd;
  struct sockaddr_in *sin;
  struct ifreq ifr;

  if (addr == nullptr || netcard == nullptr) {
    return PARM_INVALID;
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("socket error!\n");
    return FALSE;
  }

  memset(&ifr, 0, sizeof(ifr));
  snprintf(ifr.ifr_name, (sizeof(ifr.ifr_name) - 1), "%s", netcard);

  if (ioctl(sockfd, flag, &ifr) < 0) {
    perror("ioctl error!\n");
    close(sockfd);
    return FALSE;
  }
  close(sockfd);

  if (SIOCGIFHWADDR == flag) {
    memcpy((void *) addr, (const void *) &ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
    /*printf("mac address: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);*/
  } else {
    sin = (struct sockaddr_in *) &ifr.ifr_addr;
    snprintf((char *) addr, IP_LENGTH, "%s", inet_ntoa(sin->sin_addr));
  }

  return TRUE;
}

string wifiUtil::Wifi_Get_AP_Wlan1_IP(void) {

  logTrace << " Wifi_Get_AP_Wlan1_IP" << endl;

  if (!netcard_Valid("wlan1")) {
    return string("NULL");
  }

  char netcard[NETCARD_LENGTH] = "wlan1";
  char ip[IPV4_LENGTH];
  bool r;

  bzero(ip, sizeof(ip));
  r = get_addr(ip, SIOCGIFADDR, netcard);
  if (r) {
    logInfo << "Wifi_Get_AP_Wlan1_IP==" << ip << endl;
    return string(ip);
  } else {
    logError << COLOR(red) << "no Wifi_Get_AP_Wlan1_IP -- " << COLOR(none) << endl;

    return string("NULL");
  }
}

string wifiUtil::Wifi_Get_STA_Wlan0_IP(void) {
  logTrace << " Wifi_Get_STA_Wlan0_IP" << endl;

  if (!netcard_Valid("wlan0")) {
    return string("NULL");
  }

  char netcard[NETCARD_LENGTH] = "wlan0";
  char ip[IPV4_LENGTH];
  bool r;

  bzero(ip, sizeof(ip));//0
  r = get_addr(ip, SIOCGIFADDR, netcard);
  if (r) {
    logInfo << "Wifi_Get_STA_Wlan0_IP==" << ip << endl;
    return string(ip);
  } else {
    logError << COLOR(red) << "no Wifi_Get_STA_Wlan0_IP -- " << COLOR(none) << endl;

    return string("NULL");
  }
}

string wifiUtil::Wifi_Get_AP_Wlan1_Netmask(void) {
  logTrace << " Wifi_Get_AP_Wlan1_Netmask" << endl;

  if (!netcard_Valid("wlan1")) {
    return string("NULL");
  }

  char netcard[NETCARD_LENGTH] = "wlan1";
  char ip[IPV4_LENGTH];
  bool r;

  bzero(ip, sizeof(ip));
  r = get_addr(ip, SIOCGIFNETMASK, netcard);
  if (r) {
    logInfo << "Wifi_Get_AP_Wlan1_Netmask==" << ip << endl;
    return string(ip);
  } else {
    logError << COLOR(red) << "no Wifi_Get_AP_Wlan1_Netmask -- " << COLOR(none) << endl;

    return string("NULL");
  }
}

string wifiUtil::Wifi_Get_STA_Wlan0_Netmask(void) {
  logTrace << " Wifi_Get_STA_Wlan0_Netmask" << endl;

  if (!netcard_Valid("wlan0")) {
    return string("NULL");
  }

  char netcard[NETCARD_LENGTH] = "wlan0";
  char ip[IPV4_LENGTH];
  bool r;

  bzero(ip, sizeof(ip));
  r = get_addr(ip, SIOCGIFNETMASK, netcard);
  if (r) {
    logInfo << "Wifi_Get_STA_Wlan0_Netmask==" << ip << endl;
    return string(ip);
  } else {
    logError << COLOR(red) << "no Wifi_Get_STA_Wlan0_Netmask -- " << COLOR(none) << endl;

    return string("NULL");
  }
}

string wifiUtil::Get_4GModule_IP(void) {
  logTrace << " Get_4GModule_IP" << endl;

  if ((!netcard_Valid("rmnet_data1")) && (!netcard_Valid("rmnet_data0"))) {
    return string("NULL");
  }

  char netcard0[NETCARD_LENGTH] = "rmnet_data0";
  char netcard[NETCARD_LENGTH] = "rmnet_data1";

  char ip0[IPV4_LENGTH];
  char ip[IPV4_LENGTH];
  bool r0, r;

  bzero(ip, sizeof(ip));
  r0 = get_addr(ip0, SIOCGIFADDR, netcard0);
  r = get_addr(ip, SIOCGIFADDR, netcard);

  if (r0) {
    logInfo << "Get_4GModule_IP==" << ip0 << endl;
    return string(ip0);
  } else if (r) {
    logInfo << "Get_4GModule_IP==" << ip << endl;
    return string(ip);
  } else {
    logError << COLOR(red) << "no Get_4GModule_IP -- " << COLOR(none) << endl;

    return string("NULL");
  }
}

string wifiUtil::Get_4GModule_Netmask(void) {
  logTrace << " Get_4GModule_Netmask" << endl;

  char netcard[NETCARD_LENGTH] = "rmnet_data1";
  char netcard0[NETCARD_LENGTH] = "rmnet_data0";

  char ip[IPV4_LENGTH];
  bool r;

  bzero(ip, sizeof(ip));

  if (netcard_Valid("rmnet_data0")) {
    r = get_addr(ip, SIOCGIFNETMASK, netcard0);

  } else if (netcard_Valid("rmnet_data1")) {
    r = get_addr(ip, SIOCGIFNETMASK, netcard);

  } else {
    return string("NULL");
  }

  if (r) {
    logInfo << "Get_4GModule_Netmask==" << ip << endl;
    return string(ip);
  } else {
    logError << COLOR(red) << "no Get_4GModule_Netmask -- " << COLOR(none) << endl;

    return string("NULL");
  }
}

string wifiUtil::Get_4GModule_ConnStatus(void) {

  logTrace << "Get_4GModule_ConnStatus" << endl;

  if (!netcard_Valid("rmnet_data1")) {
    return string("NULL");
  }

  char ModelLogSigBuff[512] = {0};
  int retval;
  retval = sendATCmd((char *) "AT+CPSI?", (char *) "OK", ModelLogSigBuff, sizeof(ModelLogSigBuff), 2000);

  if (retval > 0) {
    logInfo << "Get_4GModule_ConnStatus==" << ModelLogSigBuff << endl;
    return string(ModelLogSigBuff);
  } else {
    logError << COLOR(red) << "no Get_4GModule_ConnStatus -- " << COLOR(none) << endl;
    return string("NULL");
  }
}

bool wifiUtil::isIPAddressValid(const char *pszIPAddr) {
  if (!pszIPAddr) {
    logError << "IP is NULL" << endl;
    return false; //若pszIPAddr为空
  }

  char IP1[100], cIP[4];
  int len = strlen(pszIPAddr);
  int i = 0, j = len - 1;
  int k, m = 0, n = 0, num = 0;
  //去除首尾空格(取出从i-1到j+1之间的字符):
  while (pszIPAddr[i++] == ' ');
  while (pszIPAddr[j--] == ' ');

  for (k = i - 1; k <= j + 1; k++) {
    IP1[m++] = *(pszIPAddr + k);
  }
  IP1[m] = '\0';

  char *p = IP1;

  while (*p != '\0') {
    if (*p == ' ' || *p < '0' || *p > '9') return false;
    cIP[n++] = *p; //保存每个子段的第一个字符，用于之后判断该子段是否为0开头

    int sum = 0;  //sum为每一子段的数值，应在0到255之间
    while (*p != '.' && *p != '\0') {
      if (*p == ' ' || *p < '0' || *p > '9') return false;
      sum = sum * 10 + *p - 48;  //每一子段字符串转化为整数
      p++;
    }
    if (*p == '.') {
      if ((*(p - 1) >= '0' && *(p - 1) <= '9')
          && (*(p + 1) >= '0' && *(p + 1) <= '9'))//判断"."前后是否有数字，若无，则为无效IP，如“1.1.127.”
        num++;  //记录“.”出现的次数，不能大于3
      else
        return false;
    };
    if ((sum > 255) || (sum > 0 && cIP[0] == '0') || num > 3) return false;//若子段的值>255或为0开头的非0子段或“.”的数目>3，则为无效IP

    if (*p != '\0') p++;
    n = 0;
  }
  if (num != 3) return false;
  return true;
}

/*
 * 先验证是否为合法IP，然后将掩码转化成32无符号整型，取反为000...00111...1，
 * 然后再加1为00...01000...0，此时为2^n，如果满足就为合法掩码
 *
 * */
bool wifiUtil::isNetmaskValid(const char *netmask) {
  if (!netmask) {
    return false;
  }

  if (isIPAddressValid(netmask)) {
    unsigned int b = 0, i, n[4];
    sscanf(netmask, "%u.%u.%u.%u", &n[3], &n[2], &n[1], &n[0]);
    for (i = 0; i < 4; ++i) //将子网掩码存入32位无符号整型
      b += n[i] << (i * 8);
    b = ~b + 1;
    if ((b & (b - 1)) == 0) //判断是否为2^n
      return TRUE;
  }

  return FALSE;
}

//
//int set_ip_netmask(unsigned char ip[16])
//{
//    return set_addr(ip, SIOCSIFNETMASK);
//}
//
//int set_ip(unsigned char ip[16])
//{
//    return set_addr(ip, SIOCSIFADDR);
//}

int wifiUtil::Set_4GModule_IP(string ip) {
  if (netcard_Valid("rmnet_data0")) {
    char netcard[NETCARD_LENGTH] = "rmnet_data0";//網卡名字長度
    return set_addr(ip.c_str(), SIOCSIFADDR, netcard);
  } else if (netcard_Valid("rmnet_data1")) {
    char netcard[NETCARD_LENGTH] = "rmnet_data1";//網卡名字長度
    return set_addr(ip.c_str(), SIOCSIFADDR, netcard);
  } else {
    return NOREADY;
  }

//    char netcard[NETCARD_LENGTH] = "rmnet_data1";//網卡名字長度
//    return set_addr(ip.c_str(), SIOCSIFADDR, netcard);
}

int wifiUtil::Set_4GModule_Netmask(string ip) {
  if (netcard_Valid("rmnet_data0")) {
    char netcard[NETCARD_LENGTH] = "rmnet_data0";//網卡名字長度
    return set_addr(ip.c_str(), SIOCSIFNETMASK, netcard);
  } else if (netcard_Valid("rmnet_data1")) {
    char netcard[NETCARD_LENGTH] = "rmnet_data1";//網卡名字長度
    return set_addr(ip.c_str(), SIOCSIFNETMASK, netcard);
  } else {
    return NOREADY;
  }
//    char netcard[NETCARD_LENGTH] = "rmnet_data1";//網卡名字長度
//    return set_addr(ip.c_str(), SIOCSIFNETMASK, netcard);
}
int wifiUtil::Wifi_Set_AP_Wlan1_IP(string ip) {
  if (!netcard_Valid("wlan1")) {
    return NOREADY;
  }
  char netcard[NETCARD_LENGTH] = "wlan1";
  return set_addr(ip.c_str(), SIOCSIFADDR, netcard);
}

int wifiUtil::Wifi_Set_AP_Wlan1_Netmask(string ip) {
  if (!netcard_Valid("wlan1")) {
    return NOREADY;
  }
  char netcard[NETCARD_LENGTH] = "wlan1";
  return set_addr(ip.c_str(), SIOCSIFNETMASK, netcard);
}

int wifiUtil::Wifi_Set_STA_Wlan0_IP(string ip) {
  if (!netcard_Valid("wlan0")) {
    return NOREADY;
  }
  char netcard[NETCARD_LENGTH] = "wlan0";
  return set_addr(ip.c_str(), SIOCSIFADDR, netcard);
}

int wifiUtil::Wifi_Set_STA_Wlan0_Netmask(string ip) {
  if (!netcard_Valid("wlan0")) {
    return NOREADY;
  }
  char netcard[NETCARD_LENGTH] = "wlan0";
  return set_addr(ip.c_str(), SIOCSIFNETMASK, netcard);
}

int wifiUtil::set_addr(const char *ip, int flag, char *netcard) {
  struct ifreq ifr;
  struct sockaddr_in sin;
  int sockfd;

  if (!ip || !netcard) {
    return NOREADY;
  }

//
//    if(!netcard)
//    {
//        return FAIL;
//    }


  if (!isIPAddressValid(ip)) {
    printf("ip was invalid!\n");
    return NOREADY;
  }

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "Could not get socket.\n");
    perror("socket\n");
    return SOCK_ERROR;
  }

  snprintf(ifr.ifr_name, (sizeof(ifr.ifr_name) - 1), "%s", netcard);

  /* Read interface flags */
  if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
    fprintf(stderr, "ifdown: shutdown ");
    perror(ifr.ifr_name);
    return SOCK_ERROR;
  }

  memset(&sin, 0, sizeof(struct sockaddr));
  sin.sin_family = AF_INET;
  inet_aton(ip, &sin.sin_addr);
  memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
  if (ioctl(sockfd, flag, &ifr) < 0) {
    fprintf(stderr, "Cannot set IP address. ");
    perror(ifr.ifr_name);
    return SOCK_ERROR;
  }

  return OK;
}

int wifiUtil::Wifi_Set_Default_Gateway(string ip, string netcard) {

  logTrace << "Wifi_Set_Default_Gateway" << endl;

  if (isIPAddressValid(ip.c_str())) {
    logTrace << "ip====" << ip << endl;
  } else {
    return PARM_INVALID;
  }

  if (!netcard_Valid(netcard)) {
    return NOREADY;
  }

  char oip[IPV4_LENGTH] = {0};
  char omask[IPV4_LENGTH] = {0};

  get_addr(oip, SIOCGIFADDR, const_cast<char *>(netcard.c_str()));
  get_addr(omask, SIOCGIFNETMASK, const_cast<char *>(netcard.c_str()));

  if (!getIPMaskCompareResult(oip, const_cast<char *>(ip.c_str()), omask, omask)) {
    return PARM_INVALID;
  }

  FILE *gw_fd = nullptr;

  string command = "route add default gw " + ip + " " + netcard;

  logTrace << "command====" << command << endl;

  gw_fd = popen(command.c_str(), "r");
  logTrace << "gw_fd====" << gw_fd << endl;

  if (gw_fd != NULL) {
    logTrace << "gw_fd====" << gw_fd << endl;

    return OK;
  } else {
    logError << COLOR(red) << "no geteway -- " << COLOR(none) << endl;

    return FAIL;
  }

}

int wifiUtil::set_gateway(const char *ip) { //return bool
  int sockFd;
  struct sockaddr_in sockaddr;
  struct rtentry rt;

  if (!ip) {
    return NOREADY;
  }

  if (!isIPAddressValid(ip)) {
    printf("gateway was invalid!\n");
    return NOREADY;
  }

  sockFd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockFd < 0) {
    perror("Socket create error.\n");
    return SOCK_ERROR;
  }

  memset(&rt, 0, sizeof(struct rtentry));
  memset(&sockaddr, 0, sizeof(struct sockaddr_in));
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = 0;
  if (inet_aton(ip, &sockaddr.sin_addr) < 0) {
    perror("inet_aton error\n");
    close(sockFd);
    return SOCK_ERROR;
  }

  memcpy(&rt.rt_gateway, &sockaddr, sizeof(struct sockaddr_in));
  ((struct sockaddr_in *) &rt.rt_dst)->sin_family = AF_INET;
  ((struct sockaddr_in *) &rt.rt_genmask)->sin_family = AF_INET;
  rt.rt_flags = RTF_GATEWAY;
  if (ioctl(sockFd, SIOCADDRT, &rt) < 0) {
    perror("ioctl(SIOCADDRT) error in set_default_route\n");
    close(sockFd);
    return SOCK_ERROR;
  }

  return OK;
}

int wifiUtil::Wifi_Set_STA_Wlan0_CardInfo(string ip, string netmask) {

  if (!netcard_Valid(NETCARD_STA)) {
    return NOREADY;
  }

  if (!isIPAddressValid(ip.c_str()) || !isIPAddressValid(netmask.c_str())) {
    return PARM_INVALID;
  }

  string oip = Wifi_Get_STA_Wlan0_IP();
  string omask = Wifi_Get_STA_Wlan0_Netmask();

  if (!getIPMaskCompareResult(const_cast<char *>(oip.c_str()),
                              const_cast<char *>(ip.c_str()),
                              const_cast<char *>(omask.c_str()),
                              const_cast<char *>(netmask.c_str()))) {
    return PARM_INVALID;
  }

  int r1, r2;

  r1 = Wifi_Set_STA_Wlan0_IP(ip);
  r2 = Wifi_Set_STA_Wlan0_Netmask(netmask);

  if ((r1 == OK) && (r2 == OK)) {
    return OK;
  } else {
    logError << " Wifi_Set_STA_Wlan0_CardInfo " << endl;
    return FAIL;
  }

}

int wifiUtil::netcard_Valid(string netcard) {

  string path = "/sys/class/net/" + netcard;

  logTrace << "path====" << path << endl;

  if (!access(path.c_str(), F_OK))//判断文件存在
  {
    logTrace << netcard << " exist" << endl;
    return B_TURE;
  } else {
    logError << netcard << " not exist" << endl;
    return B_FALSE;
  }
}

int wifiUtil::sim_Valid() {

  int i;
  int retval = -1;
  char strResult[100];

  memset(strResult, 0, sizeof(strResult));

  for (i = 0; i < 5; i++) {
    memset(strResult, 0, sizeof(strResult));
    retval = sendATCmd((char *) "AT+CPIN?", (char *) "+CPIN: READY", strResult, sizeof(strResult), 5000);
    printf("111 %d\n", retval);
    if (retval > 0) {
      printf("strResult:%s,  retval:%d \n", strResult, retval);
      logInfo << "ssssss" << retval << "ssssss" << strResult << endl;
      break;
    }
  }

  //printf("strResult:%s,  retval:%d \n",strResult, retval);
  //for(int i = 0; i<(int)sizeof(strResult); i++)
  //printf("%02x ",*(strResult+i));
  //printf("\n");

  logInfo << "ssssss" << retval << "ssssss" << strResult << endl;

  return retval;
}

void wifiUtil::Wifi_Monitor() {
//
//#define  WIFI_SSID_MAX_LEN 32
//#define  WIFI_PASS_MAX_LEN 64
//#define  BUFF_MAX_LEN 64
  logTrace << "Wifi_Monitor" << endl;

//    uint8 wifi_power_flag;
//    char ssid[WIFI_SSID_MAX_LEN] = {0};
//    char password[WIFI_PASS_MAX_LEN] = {0};
//    char sta_ssid[WIFI_SSID_MAX_LEN] = {0};
//    char sta_pass[WIFI_PASS_MAX_LEN] = {0};
//    char buff[BUFF_MAX_LEN] = {0};

//    u_char uCount = 0;
//    u_char uWifiPowerOffCount = 0;
//    u_char uWifiPowerOnCount =0;

//    int authType = WPA_WPA2_PSK;// queding
//    int encryptMode = AES_TKIP; //// queding

#if 1
  while (1) {

    logDebug << COLOR(blue) << " Monitor Online " << COLOR(none) << endl;
//        logInfo<<"Wifi_Get_LocalSsid info:"<<Wifi_Get_LocalSsid(2)<<"\n";
//        logInfo<<"Wifi_Get_LocalPasswork info:"<<Wifi_Get_LocalPasswork(2)<<"\n";
//        logInfo<<"Wifi_Get_STA_Ssid info:"<<Wifi_Get_STA_Ssid()<<"\n";
//        logInfo<<"Wifi_Get_STA_Passwork info:"<<Wifi_Get_STA_Passwork()<<"\n";
//
//        logInfo<<"_wifi_switch_status ======== "<<_wifi_switch_status<<endl
//               <<"_wifi_ap_ssid ======== "<<_wifi_ap_ssid<<endl
//               <<"_wifi_ap_password ======== "<<_wifi_ap_password<<endl
//               <<"_wifi_sta_ssid ======== "<<_wifi_sta_ssid<<endl
//               <<"_wifi_sta_password ======== "<<_wifi_sta_password<<endl;



    if (_wifi_netcard_count == 0) {
      system("sys_reboot");
    }

    WifiModeProcess();

    if (netcard_Valid("wlan0")) {
      _wifi_netcard_count = WIFI_RE_TIME;
//            tryWifiCloseProcess();
//            tryWifiSetAP();
//            tryWifiSetSTA();
      WifiSwicthProcess();
      WifiAPProcess();
      WifiSTAProcess();
    } else {
//            tryWifiOpenProcess();
      WifiSwicthProcess();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
//    sleep(1);
  }

#endif

#if 0
  //添加状态变量
  while (1)
  {
      logTrace << "====" << _loop_count << endl;
      _loop_count++;
      logTrace << "++++" << _loop_count << endl;

      logInfo<<"Wifi_Get"<<Wifi_Get()<<endl;

      if(!access("/sys/class/net/wlan0", F_OK))
      {
          logTrace<<"access"<<endl;
          get_wifi_status(&wifi_power_flag);

          if(( _wifi_switch_status == WIFI_SET_S_E) && (wifi_power_flag != 0))
          {
              if(uWifiPowerOffCount == 0) {
                  logTrace<<"1wifi_power_flag "<< wifi_power_flag << endl;
                  wifi_switch(0);
                  uWifiPowerOffCount = 10;
                  logTrace<<"2wifi_power_flag "<< wifi_power_flag << endl;
              }
              else if(uWifiPowerOffCount > 0)
              {

                  uWifiPowerOffCount--;
              }
              uWifiPowerOnCount = 0;
          }
          else if(( _wifi_switch_status == WIFI_SET_S_S)&&(_wifi_wlan1_set_status == WIFI_SET_ON))
          {
              logTrace<<"wlan1"<<endl;

              bzero(ssid, sizeof(ssid));
              memcpy(ssid, _wifi_ap_ssid.data(), strlen(_wifi_ap_ssid.data()));
              bzero(buff, sizeof(buff));

              get_ssid(buff, AP_INDEX_STA);

              logTrace<<"__ssid  === "<<ssid<<endl;
              logTrace<<"__pass  === "<<buff<<endl;

              if(strcmp(buff, ssid) != 0 && ssid[0] != 0)
              {
                  set_ssid(ssid, AP_INDEX_STA);
              }
              bzero(buff, sizeof(buff));
              get_auth(&authType, &encryptMode, buff, AP_INDEX_STA);
              bzero(password, sizeof(password));
              memcpy(password, _wifi_ap_password.data(), strlen(_wifi_ap_password.data()));

              logTrace<<"__password  === "<<password<<endl;
              logTrace<<"__pass  === "<<buff<<endl;

              if(strcmp(buff, password) != 0 && password[0] != 0)
              {
                  set_auth(password,authType, encryptMode,  AP_INDEX_STA);
              }
              _wifi_wlan1_set_status = WIFI_SET_OFF;
          }
          else if((_wifi_switch_status == WIFI_SET_S_S)&&(_wifi_wlan0_set_status == WIFI_SET_ON))
          {
              logTrace<<"wlan0"<<endl;

              bzero(ssid, sizeof(ssid));
              bzero(password, sizeof(password));
              memcpy(ssid, _wifi_sta_ssid.data(), strlen(_wifi_sta_ssid.data()));
              memcpy(password, _wifi_sta_password.data(), strlen(_wifi_sta_password.data()));

              bzero(sta_ssid, sizeof(sta_ssid));
              bzero(sta_pass, sizeof(sta_pass));
              get_sta_cfg(sta_ssid,sta_pass);

              logTrace<<"__ssid  === "<<ssid<<endl;
              logTrace<<"__pass  === "<<password<<endl;
              logTrace<< "sta ssid  === "<<sta_ssid<<endl;
              logTrace<<"sta pass  === "<<sta_pass<<endl;

              if((strcmp(sta_ssid, ssid) != 0 && ssid[0] != 0)
                  || (strcmp(sta_pass, password) != 0 && password[0] != 0))
              {
                  logTrace << "set_sta_cfg" << endl;
                  set_sta_cfg(ssid,password);
              }

              _wifi_wlan0_set_status = WIFI_SET_OFF;

          }
      }
      else
      {
          if( _wifi_switch_status == WIFI_SET_S_S)
          {
              get_wifi_status(&wifi_power_flag);

              logTrace<<"3wifi_power_flag "<< to_string(wifi_power_flag) << endl;

              if((wifi_power_flag != 1) && (uWifiPowerOnCount == 0))
              {
                  logTrace<<"4wifi_power_flag "<< to_string(wifi_power_flag) << endl;

                  wifi_switch(1);
                  uWifiPowerOnCount = 30;
              }
              else if(uWifiPowerOnCount > 0)
              {
                  uWifiPowerOnCount-- ;
              }

              uWifiPowerOffCount = 0;

          }


      }

      if(uCount++ >= 5)
      {
          logTrace << "monitor running" << endl;
          logTrace << "status==== " << _wifi_switch_status <<" == "<< _wifi_wlan1_set_status << " == " <<_wifi_wlan0_set_status << endl;
          uCount = 0;
      }

      sleep(1);
  }
#endif

}
int
wifiUtil::wifi_switch(uint8_t isOpen) {

  logTrace << "wifi_OpenOrClose is call!" << endl;
  int ret = wifi_power(isOpen);
  if (ret) {
    logTrace << "wifi_OpenOrClose success!" << endl;
    return COMMAND_TURE;
  } else {
    logError << "wifi_OpenOrClose failed!" << endl;
    return COMMAND_FALSE;
  }

  return COMMAND_TURE;

}
string wifiUtil::Wifi_Get() {
  logTrace << "Wifi_Get()" << endl;
  string wifi_info = to_string(_wifi_switch_status) + "|" + _wifi_ap_ssid + "|" + _wifi_ap_password +
      "|" + _wifi_sta_ssid + "|" + _wifi_sta_password + "/r/n";
  return wifi_info;
}
int wifiUtil::Wifi_Set(int wifi_swicth, string ap_ssid, string ap_pass, string sta_ssid, string sta_pass) {

//                截取string长度符合标准

  logTrace << "ap_ssid.size()" << ap_ssid.size() << endl;
  logTrace << "ap_pass.size()" << ap_pass.size() << endl;
  logTrace << "sta_ssid.size()" << sta_ssid.size() << endl;
  logTrace << "sta_pass.size()" << sta_pass.size() << endl;

  logTrace << "ap_ssid.size()" << ap_ssid.substr(0, 32).size() << endl;
  logTrace << "ap_pass.size()" << ap_pass.substr(0, 64).size() << endl;
  logTrace << "sta_ssid.size()" << sta_ssid.substr(0, 32).size() << endl;
  logTrace << "sta_pass.size()" << sta_pass.substr(0, 64).size() << endl;

#if 0
  if(strlen((ap_ssid.data()))<0
      ||strlen(ap_pass.data())<8
      ||strlen(sta_ssid.data())<0
      ||strlen(sta_pass.data())<8
      ||strlen((ap_ssid.data()))>32
      ||strlen(ap_pass.data())>64
      ||strlen(sta_ssid.data())>32
      ||strlen(sta_pass.data())>64
      )
  {
      return PARM_INVALID;
  }
#endif
  if (strlen((ap_ssid.data())) < 0
      || strlen(ap_pass.data()) < 0
      || strlen(sta_ssid.data()) < 0
      || strlen(sta_pass.data()) < 0
      ) {
    return PARM_INVALID;
  }



//    if(wifi_swicth == WIFI_SET_S_E)
//    {
//        logTrace<< "wifiStartStatus close " << endl;
//
//        _wifi_switch_status = WIFI_SET_S_E;
//        writeConf(WIFI_SET_S_E);
//
//        _wifi_wlan1_set_status = WIFI_SET_OFF;
//        _wifi_wlan0_set_status = WIFI_SET_OFF;
//
//    }

  if (wifi_swicth == WIFI_SET_S_S) {
    logTrace << "wifiStartStatus open " << endl;
    _wifi_switch_status = WIFI_SET_S_S;
    writeConf(WIFI_SET_S_S);

    _wifi_wlan1_set_status = WIFI_SET_ON;
    _wifi_wlan0_set_status = WIFI_SET_ON;

    //ap set
    logTrace << "ap_ssid" << ap_ssid << endl;
    _wifi_ap_ssid = ap_ssid.substr(0, WIFI_SSID_MAX_LEN);
    logTrace << "ap_pass" << ap_pass << endl;
    _wifi_ap_password = ap_pass.substr(0, WIFI_PASS_MAX_LEN);

    //sta set
    logTrace << "sta_ssid" << sta_ssid << endl;
    _wifi_sta_ssid = sta_ssid.substr(0, WIFI_SSID_MAX_LEN);
    logTrace << "sta_pass" << sta_pass << endl;
    _wifi_sta_password = sta_pass.substr(0, WIFI_PASS_MAX_LEN);

  } else {
    logTrace << "wifiStartStatus close " << endl;

    _wifi_switch_status = WIFI_SET_S_E;
    writeConf(WIFI_SET_S_E);

    _wifi_wlan1_set_status = WIFI_SET_OFF;
    _wifi_wlan0_set_status = WIFI_SET_OFF;

  }

  return COMMAND_TURE;
}
int
wifiUtil::tryWifiOpenProcess() {
  uint8 wifi_power_flag;
  static uint8 step = 0;

  get_wifi_status(&wifi_power_flag);

#if  0

  if( _wifi_switch_status == WIFI_SET_S_S)
  {

      get_wifi_status(&wifi_power_flag);

      logTrace<<"3wifi_power_flag "<< to_string(wifi_power_flag) << endl;

      if((wifi_power_flag != 1) && (_wifi_open_count == 0))
      {
          logTrace<<"4wifi_power_flag "<< to_string(wifi_power_flag) << endl;

          wifi_switch(1);
          _wifi_open_count = 30;
          _wifi_work_status = WIFI_WORK_OPENING;
      }
      else if(_wifi_open_count > 0)
      {
          _wifi_open_count-- ;
      }

      _wifi_close_count = 0;
      _wifi_work_status = WIFI_WORK_FREE;
  }

#endif

  return COMMAND_TURE;

}
int
wifiUtil::tryWifiCloseProcess() {
  uint8 wifi_power_flag;

  get_wifi_status(&wifi_power_flag);

  if ((_wifi_switch_status == WIFI_SET_S_E) && (wifi_power_flag != 0)) {
    if (_wifi_close_count == 0) {
      logTrace << "1wifi_power_flag " << wifi_power_flag << endl;
      wifi_switch(0);
      _wifi_close_count = 10;
      _wifi_work_status = WIFI_WORK_CLOSING;
      logTrace << "2wifi_power_flag " << wifi_power_flag << endl;
    } else if (_wifi_close_count > 0) {
      _wifi_close_count--;
    }
    _wifi_open_count = 0;
    _wifi_work_status = WIFI_WORK_FREE;
  }

  return COMMAND_TURE;
}
int
wifiUtil::tryWifiSetAP() {
  char ssid[WIFI_SSID_MAX_LEN] = {0};
  char password[WIFI_PASS_MAX_LEN] = {0};
  char buff[BUFF_MAX_LEN] = {0};

  int cur_authType, cur_encryptMode;

  if ((_wifi_switch_status == WIFI_SET_S_S) && (_wifi_wlan1_set_status == WIFI_SET_ON)) {
    logTrace << "wlan1" << endl;

    bzero(ssid, sizeof(ssid));
    memcpy(ssid, _wifi_ap_ssid.data(), strlen(_wifi_ap_ssid.data()));
    bzero(buff, sizeof(buff));

    get_ssid(buff, AP_INDEX_STA);

    logTrace << "__ssid  === " << ssid << endl;
    logTrace << "__pass  === " << buff << endl;

    if (strcmp(buff, ssid) != 0 && ssid[0] != 0) {
      set_ssid(ssid, AP_INDEX_STA);
      _wifi_work_status = WIFI_WORK_SET_BUSY;
    }

    bzero(buff, sizeof(buff));
    get_auth(&cur_authType, &cur_encryptMode, buff, AP_INDEX_STA);
    bzero(password, sizeof(password));
    memcpy(password, _wifi_ap_password.data(), strlen(_wifi_ap_password.data()));

    logTrace << "__password  === " << password << endl;
    logTrace << "__pass  === " << buff << endl;

    if (strcmp(buff, password) != 0 && password[0] != 0) {
      set_auth(password, _authType, _encryptMode, AP_INDEX_STA);
      _wifi_work_status = WIFI_WORK_SET_BUSY;
    }
    _wifi_wlan1_set_status = WIFI_SET_OFF;
    _wifi_work_status = WIFI_WORK_FREE;

  }
  return COMMAND_TURE;
}
int
wifiUtil::tryWifiSetSTA() {
  char ssid[WIFI_SSID_MAX_LEN] = {0};
  char password[WIFI_PASS_MAX_LEN] = {0};
  char sta_ssid[WIFI_SSID_MAX_LEN] = {0};
  char sta_pass[WIFI_PASS_MAX_LEN] = {0};

  if ((_wifi_switch_status == WIFI_SET_S_S) && (_wifi_wlan0_set_status == WIFI_SET_ON)) {
    logTrace << "wlan0" << endl;

    bzero(ssid, sizeof(ssid));
    bzero(password, sizeof(password));
    memcpy(ssid, _wifi_sta_ssid.data(), strlen(_wifi_sta_ssid.data()));
    memcpy(password, _wifi_sta_password.data(), strlen(_wifi_sta_password.data()));

    bzero(sta_ssid, sizeof(sta_ssid));
    bzero(sta_pass, sizeof(sta_pass));
    get_sta_cfg(sta_ssid, sta_pass);

    logTrace << "__ssid  === " << ssid << endl;
    logTrace << "__pass  === " << password << endl;
    logTrace << "sta ssid  === " << sta_ssid << endl;
    logTrace << "sta pass  === " << sta_pass << endl;

    if ((strcmp(sta_ssid, ssid) != 0 && ssid[0] != 0)
        || (strcmp(sta_pass, password) != 0 && password[0] != 0)) {
      logTrace << "set_sta_cfg" << endl;
      set_sta_cfg(ssid, password);
      _wifi_work_status = WIFI_WORK_SET_BUSY;
    }

    _wifi_wlan0_set_status = WIFI_SET_OFF;
    _wifi_work_status = WIFI_WORK_FREE;

  }

  return COMMAND_TURE;
}
int
wifiUtil::Wifi_Get_Work_Status(void) {
  return _wifi_work_status;
}
int
wifiUtil::WifiSwicthProcess() {
  logTrace << "WifiSwicthProcess~~~~" << endl;

  uint8 wifi_power_flag;
  static uint8_t step = 0;
  get_wifi_status(&wifi_power_flag);

//  logTrace << " _wifi_switch_status == " << _wifi_switch_status << endl;

  switch (step) {
    case WORK_INIT:logTrace << "step == " << step << endl;
      _wifi_work_status = WIFI_SET_SWITCH_STARTING;
      if (_wifi_switch_status == WIFI_SET_S_S) {
        _wifi_netcard_count--;
        step = WORK_OPENING;
      } else if (_wifi_switch_status == WIFI_SET_S_E) {
        step = WORK_CLOSING;
      }
      break;
    case WORK_OPENING:logTrace << "step == " << step << endl;
//      logTrace << " wifi_power_flag == " << to_string(wifi_power_flag) << endl;
      if ((wifi_power_flag != 1) && (_wifi_open_count == 0)) {
//        logTrace << "4wifi_power_flag " << to_string(wifi_power_flag) << endl;
        _wifi_work_status = WIFI_WORK_OPENING;
        wifi_switch(1);
        _wifi_open_count = WIFI_OPEN_WAIT_TIME;
        _wifi_close_count = 0;
      } else if (_wifi_open_count > 0) {
        _wifi_work_status = WIFI_WORK_FREE;
        _wifi_open_count--;
      }
//      logTrace << "step1end" << endl;

      step = WORK_INIT;

      break;
    case WORK_CLOSING:logTrace << "step == " << step << endl;

//      logTrace << "closing before wifi_power_flag " << wifi_power_flag << endl;
//      logTrace << "_wifi_close_count " << _wifi_close_count << endl;

      if ((wifi_power_flag != 0) && (_wifi_close_count == 0)) {
        _wifi_work_status = WIFI_WORK_CLOSING;
//        logTrace << "1wifi_power_flag " << wifi_power_flag << endl;
        wifi_switch(0);
        _wifi_close_count = WIFI_CLOSE_WAIT_TIME;
//        logTrace << "2wifi_power_flag " << wifi_power_flag << endl;
        _wifi_open_count = 0;
      } else if (_wifi_close_count > 0) {
//        logTrace << "close wait----" << endl;
        _wifi_work_status = WIFI_WORK_FREE;
        _wifi_close_count--;
      }
//      logTrace << "step2end" << endl;
      step = WORK_INIT;

      break;
  }
  return COMMAND_TURE;
}
int
wifiUtil::WifiAPProcess() {
  logTrace << "WifiAPProcess~~~~" << endl;

  char ssid[WIFI_SSID_MAX_LEN] = {0};
  char password[WIFI_PASS_MAX_LEN] = {0};
  char buff[BUFF_MAX_LEN] = {0};

  int cur_authType, cur_encryptMode;
  static uint8_t step = 0;

  switch (step) {
    case WORK_AP_INIT:
      if ((_wifi_switch_status == WIFI_SET_S_S) && (_wifi_wlan1_set_status == WIFI_SET_ON)) {
        step = WORK_AP_SET_SSID;
      }
      break;
    case WORK_AP_SET_SSID:logTrace << "wlan1" << endl;

      bzero(ssid, sizeof(ssid));
      memcpy(ssid, _wifi_ap_ssid.data(), strlen(_wifi_ap_ssid.data()));
      bzero(buff, sizeof(buff));

      get_ssid(buff, AP_INDEX_STA);

      logTrace << "__ssid  === " << ssid << endl;
      logTrace << "__pass  === " << buff << endl;

      if (strcmp(buff, ssid) != 0 && ssid[0] != 0) {
        _wifi_work_status = WIFI_WORK_SET_AP_SSID;
        set_ssid(ssid, AP_INDEX_STA);
      } else {
        logTrace << " no set ap ssid " << endl;
      }
      step = WORK_AP_SET_PASS;
      break;
    case WORK_AP_SET_PASS:bzero(buff, sizeof(buff));
      get_auth(&cur_authType, &cur_encryptMode, buff, AP_INDEX_STA);
      bzero(password, sizeof(password));
      memcpy(password, _wifi_ap_password.data(), strlen(_wifi_ap_password.data()));
//            memcpy(password, _wifi_ap_password.c_str(), _wifi_ap_password.size());

      logTrace << "__password  === " << password << endl;
      logTrace << "__pass  === " << buff << endl;

      if (strcmp(buff, password) != 0 && password[0] != 0) {
        _wifi_work_status = WIFI_WORK_SET_AP_PASS;
        set_auth(password, _authType, _encryptMode, AP_INDEX_STA);
      }
      _wifi_wlan1_set_status = WIFI_SET_OFF;
      step = WORK_AP_INIT;
      break;
  }

  return COMMAND_TURE;
}

int
wifiUtil::WifiSTAProcess() {
  logTrace << "WifiSTAProcess~~~~" << endl;
  char ssid[WIFI_SSID_MAX_LEN] = {0};
  char password[WIFI_PASS_MAX_LEN] = {0};
  char sta_ssid[WIFI_SSID_MAX_LEN] = {0};
  char sta_pass[WIFI_PASS_MAX_LEN] = {0};

  static uint8_t step = 0;

  switch (step) {
    case WORK_STA_INIT:
      if ((_wifi_switch_status == WIFI_SET_S_S) && (_wifi_wlan0_set_status == WIFI_SET_ON)) {
        step = WORK_STA_NEXT;
      }
      break;

    case WORK_STA_NEXT:logTrace << "wlan0" << endl;

      bzero(ssid, sizeof(ssid));
      bzero(password, sizeof(password));
      memcpy(ssid, _wifi_sta_ssid.data(), strlen(_wifi_sta_ssid.data()));
      memcpy(password, _wifi_sta_password.data(), strlen(_wifi_sta_password.data()));

      bzero(sta_ssid, sizeof(sta_ssid));
      bzero(sta_pass, sizeof(sta_pass));
      get_sta_cfg(sta_ssid, sta_pass);

      logTrace << "__ssid  === " << ssid << endl;
      logTrace << "__pass  === " << password << endl;
      logTrace << "sta ssid  === " << sta_ssid << endl;
      logTrace << "sta pass  === " << sta_pass << endl;

      if ((strcmp(sta_ssid, ssid) != 0 && ssid[0] != 0)
          || (strcmp(sta_pass, password) != 0 && password[0] != 0)) {
        logTrace << "set_sta_cfg" << endl;
        set_sta_cfg(ssid, password);
        _wifi_work_status = WIFI_WORK_SET_STA;
      }

      _wifi_wlan0_set_status = WIFI_SET_OFF;
      step = WORK_STA_INIT;
      break;
  }

  return COMMAND_TURE;
}
void
wifiUtil::trim(string &s) {
  if (!s.empty()) {
    s.erase(0, s.find_first_not_of(' '));
    s.erase(s.find_last_not_of(' ') + 1);
  }
}
int
wifiUtil::WifiDefaultControl() {

  logTrace << "WifiDefaultControl-+-+-+++++++++++++++++++++++++++++++++++" << endl;

  string path = DEFAULT_WIFI_PATH;
  _wifi_switch_status = WIFI_SET_S_S;

  if (!access(path.c_str(), F_OK)) {
    logTrace << "wifi free " << endl;

    _wifi_switch_status = readConf();

    if (_wifi_switch_status != WIFI_SET_S_S) {
      _wifi_switch_status = WIFI_SET_S_E;
    }

    logTrace << "readConf() =====def=====" << readConf() << endl;
    logTrace << "_wifi_switch_status =====def=====" << _wifi_switch_status << endl;
  } else {
    writeConf(WIFI_SET_S_S);
  }

  return 0;
}
int
wifiUtil::writeConf(int wifis) {
  ofstream out(WIFI_CONF_PATH);
  if (out) {
    logTrace << "wifi out" << endl;
    out << wifis << endl;
    out.close();
    return COMMAND_TURE;
  } else {
    logTrace << "wifi ofs fail" << endl;
    return COMMAND_FALSE;

  }

}
int
wifiUtil::readConf() {
  ifstream in(WIFI_CONF_PATH);
  string r;

  if (in) {
    logTrace << "wifi in" << endl;

    in >> r;
    logTrace << " r====" << r << endl;

    in.close();
  } else {
    logTrace << "wifi ifs fail" << endl;
    return COMMAND_FALSE;

  }

  r = getTrimString(r);

  //FIXME:stoi error
  if (r.compare("1") != 0 && r.compare("0") != 0) {
    logTrace << " dwifi null " << endl;
    writeConf(WIFI_SET_S_E);
    r = "0";
  }
  int rint = stoi(r);

  if (rint != WIFI_SET_S_S) {
    rint = WIFI_SET_S_E;
  }

  logTrace << rint << endl;
  return rint;
}

int wifiUtil::getFilterMode() {

//  logTrace << __LINE__ << __FUNCTION__ << endl;
  //TODO:get macaddr_acl in /etc/sta_mode_hostapd.conf
  string path = HOSTAPD_CONF_PATH;
  string mode;
  string::size_type idx;
  string cmd = "cat " + path + "|grep macaddr_acl=";

  shellPopen(cmd.c_str(), mode);

  idx = mode.find("=");
  if (idx == string::npos) {
    logError << " no info " << endl;
  }
  mode = mode.substr(idx + 1, mode.length() - idx);

  logInfo << " getFilterMode == mode == " << mode << endl;

  if (!mode.empty()) {
    return stoi(mode);
  } else {
    return FAIL;
  }

}

int wifiUtil::setFilterMode(int mode) {

  //TODO:set macaddr_acl in /etc/sta_mode_hostapd.conf

  //TODO: mode filter
  logTrace << " setFilterMode " << mode << endl;

//  string tmod = getTrimString(mode);
//  if (!mod.compare("0") && !mod.compare("1") && !mod.compare("2")) {
  if (mode != 0 && mode != 1 && mode != 2) {
    return PARM_INVALID;
  }

  string path = HOSTAPD_CONF_PATH;
  int m = getFilterMode();
  string cur = to_string(m);
  string cmd = "sed -i 's/macaddr_acl=" + cur + "/" + "macaddr_acl=" + to_string(mode) + "/g' " + path;
  logTrace << " cmd == " << cmd << endl;
  int r = shellPopen(cmd.c_str());
  //FIXME
  if (r == 0) {
    r = shellPopen("sync");
  }

  return r;
}

vector<string> wifiUtil::getWhiteList() {
  logTrace << __LINE__ << __FUNCTION__ << endl;

  return readLineFromFile(WHITELIST_PATH);
}

vector<string> wifiUtil::getBlackList() {
  logTrace << __LINE__ << __FUNCTION__ << endl;

  return readLineFromFile(BLACKLIST_PATH);
}

vector<string> wifiUtil::readLineFromFile(string path) {
  logError << "11111111111111111111111" << endl;

#define LINE_LENGTH 128

  char line[LINE_LENGTH] = {0};
  vector<string> vStr;
  int num;

  ifstream in(path);

  if (!in) {
    logError << "in file error" << endl;
    vStr.clear();
  } else {

    while (!in.eof()) {
      in.getline(line, LINE_LENGTH);

//    num = in.gcount();
//    cout << line << "\t" << num << endl;
      string s = getTrimString(string(line));
      if (!s.empty()) {
        vStr.push_back(string(line));
      }
    }
  }

  in.close();
  logError << "2222222222222222222" << endl;

  return vStr;

}

int wifiUtil::writeToFile(vector<string> list, string path) {

  fstream f(path, ios::out);

  if (f.bad()) {
    logTrace << "open file fail" << endl;
    return FAIL;
  }

  logTrace << " sort s " << endl;

  sort(list.begin(), list.end());
  list.erase(unique(list.begin(), list.end()), list.end());

  for (int i = 0; i < list.size(); i++) {
    string ws = getTrimString(list[i]);
    if (!ws.empty()) {
      f << ws << endl;
    }
  }

  f.close();

  return OK;
}

int wifiUtil::addWhiteList(string mac) {
  return addList(mac, WHITELIST_PATH);
}
int wifiUtil::removeWhiteList(string mac) {
  return removeList(mac, WHITELIST_PATH);
}
int wifiUtil::clearWhiteList() {

  int r;

  r = clearFile(WHITELIST_PATH);

  return r;
}
int wifiUtil::addBlackList(string mac) {
  return addList(mac, BLACKLIST_PATH);
}

int wifiUtil::removeBlackList(string mac) {
  return removeList(mac, BLACKLIST_PATH);
}

int wifiUtil::clearBlackList() {
  int r;

  r = clearFile(BLACKLIST_PATH);

  return r;
}
int wifiUtil::clearFile(string path) {
  int ret;
  fstream file(path, fstream::out | ios_base::trunc);

  ret = file.bad() ? FAIL : OK;
  file.close();

  return ret;
}
int wifiUtil::restartHostapd() {

#define HOSTAPD_COMMAND "hostapd -B /etc/sta_mode_hostapd.conf -P /data/hostapd_ssid1.pid -e /etc/entropy_file"
#define HOSTAPD_CLI_COMMAND "hostapd_cli -i wlan1 -p /var/run/hostapd -B -a /usr/bin/QCMAP_StaInterface"
#define HOSTAPD_KILLALL_COMMAND "killall -9 hostapd"

  static uint8_t step = 0;

  while (step == 3) {
    switch (step) {
      case 0:step = shellPopen(HOSTAPD_KILLALL_COMMAND) == COMMAND_TURE ? 1 : 0;
        break;
      case 1:step = shellPopen(HOSTAPD_COMMAND) == COMMAND_TURE ? 2 : 0;
        break;
      case 2:step = shellPopen(HOSTAPD_CLI_COMMAND) == COMMAND_TURE ? 3 : 0;
        break;
      default:step = 0;
        break;
    }
  }
  return 0;
}
int wifiUtil::shellPopen(const char *cmd) {
  if (!cmd) {
    printf("shell_popen cmd is NULL!\n");
    return -1;
  }

  printf("shell_popen : %s\n", cmd);

  FILE *fp;
  int ret;
  char buf[1024];

  if ((fp = popen(cmd, "r")) == NULL) {
    printf("shell_popen error: %s/n", strerror(errno));
    return -1;
  } else {
    while (fgets(buf, sizeof(buf), fp)) {
      if (buf[strlen(buf) - 1] == '\n') {
        buf[strlen(buf) - 1] = '\0';
      }

      printf("%s\n", buf);
    }
    cout << "buf" << buf << endl;

    if ((ret = pclose(fp)) == -1) {
      printf("shell_popen fail = [%s]\n", strerror(errno));
      return -1;
    } else {
      if (WIFEXITED(ret)) {
        if (0 == WEXITSTATUS(ret)) {
          printf("shell_popen run shell script successfully, status = [%d]\n", WEXITSTATUS(ret));
          ret = 0;
        } else {
          printf("shell_popen run shell script fail, status = [%d]\n", WEXITSTATUS(ret));
          ret = -1;
        }
      } else if (WIFSIGNALED(ret)) {
        printf("shell_popen abnormal termination, signal number = [%d]\n", WTERMSIG(ret));
        ret = -1;
      } else if (WIFSTOPPED(ret)) {
        printf("shell_popen process stopped, signal number = [%d]\n", WSTOPSIG(ret));
        ret = -1;
      } else {
        printf("shell_popen run shell unknown error, ret = [%d]\n", ret);
        ret = -1;
      }
    }
  }

  return ret;
}
string wifiUtil::getTrimString(string res) {

  //delete lb
  int r = res.find("\r\n");
  while (r != string::npos) {
    if (r != string::npos) {
      res.replace(r, 1, "");
      r = res.find("\r\n");
    }
  }

  //delete space
//  res.erase(std::remove_if(res.begin(), res.end(), std::isspace), res.end());
  logTrace << res << endl;
  trim(res);
  logTrace << res << endl;

  return res;

}

void wifiUtil::setTrimString(string &res) {

  int r = res.find("\r\n");
  while (r != string::npos) {
    if (r != string::npos) {
      res.replace(r, 1, "");
      r = res.find("\r\n");
    }
  }

  trim(res);

}

int wifiUtil::shellPopen(const char *cmd, string &rs) {
  if (!cmd) {
    printf("shell_popen cmd is NULL!\n");
    return -1;
  }

  printf("shell_popen : %s\n", cmd);

  FILE *fp;
  int ret;
  char buf[1024];

  if ((fp = popen(cmd, "r")) == NULL) {
    printf("shell_popen error: %s/n", strerror(errno));
    return -1;
  } else {
    while (fgets(buf, sizeof(buf), fp)) {
      if (buf[strlen(buf) - 1] == '\n') {
        buf[strlen(buf) - 1] = '\0';
      }

      printf("%s\n", buf);

    }

    cout << "buf " << buf << endl;

    if ((ret = pclose(fp)) == -1) {
      printf("shell_popen fail = [%s]\n", strerror(errno));
      return -1;
    } else {
      if (WIFEXITED(ret)) {
        if (0 == WEXITSTATUS(ret)) {
          printf("shell_popen run shell script successfully, status = [%d]\n", WEXITSTATUS(ret));
          rs = string(buf);  //return string
          ret = 0;
        } else {
          printf("shell_popen run shell script fail, status = [%d]\n", WEXITSTATUS(ret));
          ret = -1;
        }
      } else if (WIFSIGNALED(ret)) {
        printf("shell_popen abnormal termination, signal number = [%d]\n", WTERMSIG(ret));
        ret = -1;
      } else if (WIFSTOPPED(ret)) {
        printf("shell_popen process stopped, signal number = [%d]\n", WSTOPSIG(ret));
        ret = -1;
      } else {
        printf("shell_popen run shell unknown error, ret = [%d]\n", ret);
        ret = -1;
      }
    }
  }

  return ret;
}
int wifiUtil::rebootSystem() {

  int r = shellPopen("sys_reboot");

  return r;
}
int wifiUtil::regexMAC(string mac) {
  regex r("^([0-9a-fA-F]{2})(([/\\s:-][0-9a-fA-F]{2}){5})$");

  if (regex_match(mac, r)) {
    return B_TURE;
  } else {
    return B_FALSE;
  }

}
string wifiUtil::getWhiteListByIndex(int index) {
  return getListByIndex(index, WHITELIST_PATH);
}
string wifiUtil::getBlackListByIndex(int index) {
  return getListByIndex(index, BLACKLIST_PATH);
}

int wifiUtil::getWhiteListCount() {
  return getListCount(WHITELIST_PATH);
}
int wifiUtil::getBlackListCount() {
  return getListCount(BLACKLIST_PATH);;
}
int wifiUtil::getWifiMaxCount() {
  logTrace << " getWifiMaxCount " << endl;

  string path = HOSTAPD_CONF_PATH;
  string count;
  string::size_type idx;
  string cmd = "cat " + path + "|grep max_num_sta=";

  shellPopen(cmd.c_str(), count);

  idx = count.find("=");
  if (idx == string::npos) {
    logError << " no info " << endl;
  }
  count = count.substr(idx + 1, count.length() - idx);

  logInfo << " getFilterMode == count == " << count << endl;

  if (!count.empty()) {
    return stoi(count);
  } else {
    return FAIL;
  }
}
int wifiUtil::setWifiMaxCount(int count) {

  logTrace << " setWifiMaxCount ==  " << count << endl;

  if (count < 0 || count > 256) {
    return PARM_INVALID;
  }

  string path = HOSTAPD_CONF_PATH;
  int m = getWifiMaxCount();
  string cur = to_string(m);
  string mod = to_string(count);

  string cmd = "sed -i 's/max_num_sta=" + cur + "/" + "max_num_sta=" + mod + "/g' " + path;
  logTrace << " cmd == " << cmd << endl;
  int r = shellPopen(cmd.c_str());
  //FIXME
  if (r == 0) {
    r = shellPopen("sync");
  }

  return r;
}

int wifiUtil::addList(string mac, string path) {
  logTrace << " addList == " << mac << " To " << path << endl;

  vector<string> vt;
  int r;

  setTrimString(mac);

  //TODO:mac pd
  if (!regexMAC(mac)) {
    logError << "mac error" << endl;
    return PARM_INVALID;
  }

  vt = readLineFromFile(path);
  vt.emplace_back(mac);

  r = writeToFile(vt, path);

  logTrace << path << " == return == " << r << endl;

  return r;
}
int wifiUtil::removeList(string mac, string path) {
  logTrace << " removeList == " << mac << " From " << path << endl;

  vector<string> vt;
  int r;
  vector<string>::iterator it;

  if (!regexMAC(mac)) {
    return PARM_INVALID;
  }

  vt = readLineFromFile(path);

  for (it = vt.begin(); it != vt.end();) {
    if (getTrimString(string(*it)).compare(mac) == 0) {
      logTrace << "delete compare" << string(*it) << endl;
      it = vt.erase(it);
    } else {
      ++it;
    }
  }

//  clearWhiteList();
  clearFile(path);
  r = writeToFile(vt, path);

  return r;
}
string wifiUtil::getListByIndex(int index, string path) {
  logTrace << " getListByIndex == " << index << " From " << path << endl;

  vector<string> vt;
  vt = readLineFromFile(path);

  int count = getListCount(path);

  if (index >= count) {
    return string("NULL");
  }

  string rs = vt[index].empty() ? string("NULL") : vt[index];

  logDebug << " getWhiteListByIndex return == " << rs << endl;

  return rs;
}
int wifiUtil::getListCount(string path) {
  logTrace << " getListCount " << endl;

  vector<string> vt;
  vt = readLineFromFile(path);

  int count = vt.size();
  logInfo << count << endl;

  return count;
}
int wifiUtil::WifiModeProcess() {

  static u_int8_t timer = WIFI_MODE_WAIT_TIME;

  logTrace << timer << endl;

  if (timer == 0) {

    int mode = Wifi_Get_Mode();
    logTrace << mode << endl;

    if (mode != APSTA_MODE) {
      Wifi_Set_Mode(APSTA_MODE);
    }

    timer = WIFI_MODE_WAIT_TIME;
  } else {
    timer--;
  }

  return COMMAND_TURE;
}

void wifiUtil::printIntArray(int *array) {
  for (int i = 0; i < IPLength; i++)
    cout << array[i] << "  ";
  cout << endl;
}

int wifiUtil::charToInt(char ch, int sum) {
  int result = sum * 10 + (ch - '0');
  return result;
}

void wifiUtil::getIntOfIp(char *ip, int *intArray) {
  int count = 0;
  int temp = 0;
  while (*ip != '\0') {

    if (*ip == '.') {
      intArray[count] = temp;
      count++;
      temp = 0;
    } else if (count == 0) {
      temp = charToInt(*ip, temp);
    } else if (count == 1) {
      temp = charToInt(*ip, temp);
    } else if (count == 2) {
      temp = charToInt(*ip, temp);
    } else if (count == 3) {
      temp = charToInt(*ip, temp);
    }
    ip++;
  }
  intArray[count] = temp;
}

void wifiUtil::ipAndSubIP(int *nums1, int *nums2, int *nums3) {
  for (int i = 0; i < IPLength; i++) {
    nums3[i] = nums1[i] & nums2[i];
  }
}

bool wifiUtil::compareInt(int *nums1, int *nums2) {
  for (int i = 0; i < IPLength; i++) {
    if (nums1[i] - nums2[i])
      return false;
  }
  return true;
}

void wifiUtil::getFlagToString(int *nums, char *ch) {
  int count = 0;
  for (int i = 0; i < IPLength; i++) {
    int num = nums[i];
    int getH = num / 100;
    int getM = num / 10 % 10;
    int getL = num % 10;
    if (getH) {
      ch[count++] = getH + '0';
      ch[count++] = getM + '0';
    } else if (getM) {
      ch[count++] = getM + '0';
    }
    ch[count++] = getL + '0';
    ch[count++] = '.';
  }
  ch[count - 1] = '\0';
}

bool wifiUtil::getIPMaskCompareResult(char *ip1, char *ip2, char *ip3, char *ip4) {

  if (ip1 == nullptr || ip2 == nullptr || ip3 == nullptr || ip4 == nullptr) {
    return B_FALSE;
  }

  // 取出ip的整数值
  int ip2Array[IPLength];
  int ip1Array[IPLength];
  int ip3Array[IPLength];
  int ip4Array[IPLength];
  int flag1Array[IPLength];
  int flag2Array[IPLength];
  char ch[IPCharLength];

  getIntOfIp(ip1, ip1Array);
  getIntOfIp(ip2, ip2Array);
  getIntOfIp(ip3, ip3Array);
  getIntOfIp(ip4, ip4Array);

  ipAndSubIP(ip1Array, ip3Array, flag1Array);
  ipAndSubIP(ip2Array, ip4Array, flag2Array);

  bool result = compareInt(flag1Array, flag2Array);

  if (result) {
    getFlagToString(flag1Array, ch);
  }

  return result;
}

