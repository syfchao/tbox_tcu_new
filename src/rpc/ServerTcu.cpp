//
// Created by renhono on 2020/3/19.
//

#include "ServerTcu.h"

void ServerTcu::TcuInterfaceInit() {

  logDebug << " ++++ TcuInterfaceInit ++++ " << endl;

//  logInfo << "Wifi_Set:" << m_wifiutill->Wifi_Set(
//      1,
//      "abcdefghijk",
//      "0987654321",
//      "honoxxx",
//      "123456789")
//          << endl;

  bind("Wifi_Set_Switch", [this](int isOpen) {
    return m_wifiutill->Wifi_Set_Switch(isOpen);
  });

  bind("Wifi_Set_Mode", [this](int mode) {
    return m_wifiutill->Wifi_Set_Mode(mode);
  });

  bind("Wifi_Set_LocalSsid", [this](string ssid, int mode) {
    return m_wifiutill->Wifi_Set_LocalSsid(ssid, mode);
  });

  bind("Wifi_Set_LocalPasswork", [this](string passwork, int auth_type, int encrypt_mode, int mode) {
    return m_wifiutill->Wifi_Set_LocalPasswork(passwork, auth_type, encrypt_mode, mode);
  });

  bind("Wifi_Set_STA_Config", [this](string ssid, string password) {
    return m_wifiutill->Wifi_Set_STA_Config(ssid, password);
  });

  bind("Wifi_Get_LocalSsid", [this](int mode) {
    return m_wifiutill->Wifi_Get_LocalSsid(mode);
  });

  bind("Wifi_Get_LocalPasswork", [this](int mode) {
    return m_wifiutill->Wifi_Get_LocalPasswork(mode);
  });

  bind("Wifi_Get_MacAddr", [this](int mode) {
    return m_wifiutill->Wifi_Get_MacAddr(mode);
  });

  bind("Wifi_Get_STA_IP", [this]() {
    return m_wifiutill->Wifi_Get_STA_IP();
  });

  bind("Wifi_Get_STA_Ssid", [this]() {
    return m_wifiutill->Wifi_Get_STA_Ssid();
  });

  bind("Wifi_Get_STA_Passwork", [this]() {
    return m_wifiutill->Wifi_Get_STA_Passwork();
  });

  bind("Wifi_Get_Auth_Type", [this](int mode) {
    return m_wifiutill->Wifi_Get_Auth_Type(mode);
  });

  bind("Wifi_Get_Encrypt_Mode", [this](int interface) {
    return m_wifiutill->Wifi_Get_Encrypt_Mode(interface);
  });

  bind("Wifi_Get_STA_Netmask", [this]() {
    return m_wifiutill->Wifi_Get_STA_Netmask();
  });

  bind("Wifi_Get_Default_Gateway", [this](string netcard) {
    return m_wifiutill->Wifi_Get_Default_Gateway(netcard);
  });

  bind("Wifi_Get_Default_DNS", [this]() {
    return m_wifiutill->Wifi_Get_Default_DNS();
  });

  bind("Wifi_Get_Mode", [this]() {
    return m_wifiutill->Wifi_Get_Mode();
  });

  bind("Wifi_Get_Switch_Status", [this]() {
    return m_wifiutill->Wifi_Get_Switch_Status();
  });

  bind("Wifi_Get_Auth", [this](int mode) {
    return m_wifiutill->Wifi_Get_Auth(mode);
  });

  bind("Wifi_Get_Echo", [this]() {
    return m_wifiutill->Wifi_Get_Echo();
  });

  bind("Get_4GModule_ICCID", [this]() {
    return m_wifiutill->Get_4GModule_ICCID();
  });

  bind("Get_4GModule_SigLevel", [this]() {
    return m_wifiutill->Get_4GModule_SigLevel();
  });

  bind("Get_4GModule_IP", [this]() {
    return m_wifiutill->Get_4GModule_IP();
  });
  bind("Get_4GModule_Netmask", [this]() {
    return m_wifiutill->Get_4GModule_Netmask();
  });
  bind("Wifi_Get_AP_Wlan1_IP", [this]() {
    return m_wifiutill->Wifi_Get_AP_Wlan1_IP();
  });
  bind("Wifi_Get_AP_Wlan1_Netmask", [this]() {
    return m_wifiutill->Wifi_Get_AP_Wlan1_Netmask();
  });
  bind("Wifi_Get_STA_Wlan0_IP", [this]() {
    return m_wifiutill->Wifi_Get_STA_Wlan0_IP();
  });
  bind("Wifi_Get_STA_Wlan0_Netmask", [this]() {
    return m_wifiutill->Wifi_Get_STA_Wlan0_Netmask();
  });

  bind("Get_4GModule_ConnStatus", [this]() {
    return m_wifiutill->Get_4GModule_ConnStatus();
  });

  bind("Set_4GModule_IP", [this](string ip) {
    return m_wifiutill->Set_4GModule_IP(ip);
  });
  bind("Set_4GModule_Netmask", [this](string mask) {
    return m_wifiutill->Set_4GModule_Netmask(mask);
  });
  bind("Wifi_Set_STA_Wlan0_CardInfo", [this](string ip, string netmask) {
    return m_wifiutill->Wifi_Set_STA_Wlan0_CardInfo(ip, netmask);
  });
  bind("Wifi_Set_Default_Gateway", [this](string gateway, string netcard) {
    return m_wifiutill->Wifi_Set_Default_Gateway(gateway, netcard);
  });
  bind("Wifi_Get_Work_Status", [this]() {
    return m_wifiutill->Wifi_Get_Work_Status();
  });
  bind("Wifi_Get", [this]() {
    return m_wifiutill->Wifi_Get();
  });
  bind("Wifi_Set", [this](int wifi_swicth,
                          string ap_ssid,
                          string ap_pass,
                          string sta_ssid,
                          string sta_pass) {
    return m_wifiutill->Wifi_Set(wifi_swicth, ap_ssid, ap_pass, sta_ssid, sta_pass);
  });



  //20200312

  bind("getWhiteList", [this]() {
    return m_wifiutill->getWhiteList();
  });
  bind("getBlackList", [this]() {
    return m_wifiutill->getBlackList();
  });

  bind("clearBlackList", [this]() {
    return m_wifiutill->clearBlackList();
  });

  bind("clearWhiteList", [this]() {
    return m_wifiutill->clearWhiteList();
  });

  bind("getBlackListCount", [this]() {
    return m_wifiutill->getBlackListCount();
  });

  bind("getWhiteListCount", [this]() {
    return m_wifiutill->getWhiteListCount();
  });
  bind("getWhiteListByIndex", [this](int index) {
    return m_wifiutill->getWhiteListByIndex(index);
  });

  bind("getBlackListByIndex", [this](int index) {
    return m_wifiutill->getBlackListByIndex(index);
  });

  bind("addWhiteList", [this](string mac) {
    return m_wifiutill->addWhiteList(mac);
  });

  bind("addBlackList", [this](string mac) {
    return m_wifiutill->addBlackList(mac);
  });

  bind("removeWhiteList", [this](string mac) {
    return m_wifiutill->removeWhiteList(mac);
  });
  bind("removeBlackList", [this](string mac) {
    return m_wifiutill->removeBlackList(mac);
  });

  bind("getFilterMode", [this]() {
    logDebug << "getFilterMode" << endl;
    return m_wifiutill->getFilterMode();
  });

  bind("setFilterMode", [this](int mode) {
    logDebug << "setFilterMode" << endl;
    return m_wifiutill->setFilterMode(mode);
  });

  bind("getWifiMaxCount", [this]() {
    return m_wifiutill->getWifiMaxCount();
  });

  bind("setWifiMaxCount", [this](int count) {
    return m_wifiutill->setWifiMaxCount(count);
  });

  bind("rebootSystem", [this]() {
    return m_wifiutill->rebootSystem();
  });

}
ServerTcu::ServerTcu(uint16_t port) : server(port) {
  m_wifiutill = new wifiUtil;
}
ServerTcu::~ServerTcu() {
  delete m_wifiutill;
}

