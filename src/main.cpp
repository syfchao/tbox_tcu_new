//
// Created by renhono on 2020/4/11.
//

#include <iostream>
#include <thread>

#include "TBoxDataPool.h"
#include "DataCall.h"
#include "NASControl.h"
#include "SMSControl.h"
#include "WiFiControl.h"
#include "network_access_service_v01.h"
#include "common.h"
#include "DnsResolv.h"

/**
 * rpc inc
 */
#include "server.h"
#include "tiolog.hpp"
#include "wifiutil.h"
#include "INIReader.h"
#include "ServerTcu.h"
#include <ren/Util.h>

using namespace std;

/**
 * RPC dep
 * 20200410
 * fpp
 */
#define DEFAULT_RPC_PORT    8089

#define MODE_WAIT_INIT_TIME_OUT   10  /*10s 超时机制*/
#define MODE_WAIT_PSCS_TIME_OUT  10 /*10s 超时机制*/
#define MODE_WAIT_DC_TIME_OUT   10  /*10S 超时机制*/
#define MODE_WAIT_ST_DC_TIME_OUT   2  /*2S 超时机制*/

#define MODE_DATACALL_MAX_COUNT    3  /*最大三次重新拨号*/

void initLog() {
  printf("initLog \n");

  auto sink_cout = make_shared<tioLog::SinkCout>(tioLog::Severity::debug,
                                                 tioLog::Type::normal,
                                                 "[LOG]%Y%m%d %H:%M:%S.#ms [#severity] (#tag_func) #message"
  );

  auto sink_file = make_shared<tioLog::SinkFile>(tioLog::Severity::trace,
                                                 tioLog::Type::all, "/data/rpcs.log");

  tioLog::Log::init({sink_cout, sink_file});

}

int getConfRpcPort() {
  printf("getConfRpcPort \n");
  int rpcPort = DEFAULT_RPC_PORT;

  INIReader reader("rpcserver.conf");

  if (reader.ParseError() != 0) {
    logError << "Can't load 'rpcserver.conf'\n";
  } else {
    rpcPort = reader.GetInteger("server", "port", DEFAULT_RPC_PORT);
  }

  return rpcPort;
}

#if 1
void process_simcom_ind_message(simcom_event_e event, void *cb_usr_data) {
  int i;
  char buffer4[128];
  //call_info_type *call_list = (call_info_type *)cb_usr_data;
  switch (event) {
    case SIMCOM_EVENT_VOICE_CALL_IND: {
      //唤醒系统
    }
      break;

    case SIMCOM_EVENT_SMS_PP_IND: {
      //唤醒系统
//      if (tboxInfo.operateionStatus.isGoToSleep == 0) {
//        printf("11111111111111 sms \n");
//        tboxInfo.operateionStatus.isGoToSleep = 1;
//        tboxInfo.operateionStatus.wakeupSource = 2;
//      }

      sms_info_type sms_info;
      memcpy((void *) &sms_info, cb_usr_data, sizeof(sms_info));

      printf("\n-----------receive message --------------------------\n");
      printf("address=%s\n", sms_info.source_address);
      for (i = 0; i < strlen(sms_info.message_content); i++) {
        printf("0x%02X ", sms_info.message_content[i]);
      }
      printf("\n");
      break;
    }

    case SIMCOM_EVENT_NETWORK_IND: {
      network_info_type network_info;
      memcpy((void *) &network_info, cb_usr_data, sizeof(network_info));
      //printf("\n---------network info---------------------------\n");
      /*printf("network_info: register=%d, cs=%d, ps=%d,radio_if=%d\n",
              network_info.registration_state,
              network_info.cs_attach_state,
              network_info.ps_attach_state,
              network_info.radio_if_type);*/
      if (network_info.registration_state == NAS_REGISTERED_V01) {
        if (tboxInfo.networkStatus.networkRegSts != 1)
          tboxInfo.networkStatus.networkRegSts = 1;

      }
      if (network_info.registration_state != NAS_REGISTERED_V01) {
        if (tboxInfo.networkStatus.networkRegSts == 1)
          tboxInfo.networkStatus.networkRegSts = 0;
      }
      sprintf(buffer4,
              "process_simcom_ind_message: networkRegSts =%d\nsignalStrength =%d\n",
              tboxInfo.networkStatus.networkRegSts,
              tboxInfo.networkStatus.signalStrength);
      break;
    }
    case SIMCOM_EVENT_DATACALL_CONNECTED_IND:
    case SIMCOM_EVENT_DATACALL_DISCONNECTED_IND: {
      int ret;
      char target_ip_new[16] = {0};
      datacall_info_type datacall_info;
      //get_datacall_info(&datacall_info);
      get_datacall_info_by_profile(4, &datacall_info);

      if (datacall_info.status == DATACALL_CONNECTED) {
        int use_dns = 1;
        printf("datacall_ind1: if_name=%s,ip=%s,mask=%d\n",
               datacall_info.if_name,
               datacall_info.ip_str,
               datacall_info.mask);
        printf("datacall_ind2: dns1=%s,dns2=%s,gw=%s\n",
               datacall_info.pri_dns_str,
               datacall_info.sec_dns_str,
               datacall_info.gw_str);
        if (use_dns) {
          //长春一汽:"znwl-uat-carhq.faw.cn"
          ret = query_ip_from_dns((unsigned char *) "znwl-uat-carhq.faw.cn",
                                  datacall_info.pri_dns_str,
                                  datacall_info.pri_dns_str,
                                  target_ip_new);
          if (ret != 0) {
            printf("query ip fail\n");
            break;
          }

          printf("target_ip:%s\n", target_ip_new);

        } else {

        }
      }
    }
      break;
    default:break;
  }
}

#endif

int main(int argc, char *argv[]) {

  cout << " ==== START ==== " << endl;

  initLog();
  logInfo << "[VERSION] " << VER << endl;
  logDebug << "[VERSION] " << VER << endl;

  TBoxDataPool *dp = TBoxDataPool::GetInstance();

  /**
 * 20200410 fpp
 */

  static uint8_t u8ModemWorkState = MODEM_STATE_INIT;

  datacall_info_type datacall_info;
  nas_serving_system_type_v01 nas_status;//网络连接状态;

  static int NasCsstate = 0;
  static int NasPsstate = 0;

  static unsigned char u8ModemWaitTime = 0;
  static unsigned char u8DataCallWaitTime = MODE_WAIT_ST_DC_TIME_OUT;
  static unsigned char u8ModemDataCallCount = 0;

  uint8_t wifi_status;
  int ret;

  atctrl_init();
//  switch9011();
//  switchAdb();

  std::thread([]() {
    logDebug << "atProcess() s " << endl;
    atProcess();
    logDebug << "atProcess() e " << endl;
  }).detach();

  std::thread([]() {

    int port = getConfRpcPort();
    logInfo << "[SERVER PORT] " << port << endl;

    ServerTcu srv(DEFAULT_RPC_PORT);
    srv.TcuInterfaceInit();

    logDebug << " rpc run ~~~~~~~~~~" << endl;
    srv.run();

  }).detach();


//  AT_SetMuteInitValue();

#if 1

  while (1) {

    printf(" MAIN WHILE \n");
    logDebug << COLOR(cyan) << "[TIMES] == " << u8ModemWaitTime << " ==" << COLOR(none) << endl;

    switch (u8ModemWorkState) {
      case MODEM_STATE_INIT: {

        logDebug << COLOR(MAGENTA) << "MODEM_STATE_INIT" << COLOR(none) << endl;

        if (NetworkInit() < 0) {
          u8ModemWaitTime++;
          if (u8ModemWaitTime >= MODE_WAIT_INIT_TIME_OUT) {
            u8ModemWaitTime = 0;
            u8ModemWorkState = MODEM_STATE_CHECK_PS_CS;
            //printf("jason add modem  222222222222222222222222222\r\n");
          } else {
            break;
          }
        }

        if (apn_init() == 0)
          DEBUGLOG("apn_init success!");

        if (datacall_init() == 0)
          DEBUGLOG("dataCall_init success!");

        if (voice_init() == 0)
          DEBUGLOG("voiceCall_init success!");

        if (sms_init((sms_ind_cb_fcn) process_simcom_ind_message) == 0)
          DEBUGLOG("message_init success!");

        if (nas_init() == 0)
          DEBUGLOG("nas_init success!");
        u8ModemWaitTime = 0;
        u8ModemWorkState = MODEM_STATE_CHECK_PS_CS;
      }

      case MODEM_STATE_CHECK_PS_CS: {
        logDebug << COLOR(MAGENTA) << "MODEM_STATE_CHECK_PS_CS" << COLOR(none) << endl;

        get_NetworkType(&nas_status);//new sdk interface
        if (nas_status.registration_state == NAS_REGISTERED_V01) /*判断网络是否已经注册*/
        {
          if (tboxInfo.networkStatus.networkRegSts != 1) {
            tboxInfo.networkStatus.networkRegSts = 1;
          } else {
            if (tboxInfo.networkStatus.networkRegSts == 1) {
              tboxInfo.networkStatus.networkRegSts = 0;
            }
          }
        }

        if (nas_status.cs_attach_state == NAS_CS_ATTACHED_V01) {
          if (NasCsstate != NAS_CS_ATTACHED_V01) {
            NasCsstate = NAS_CS_ATTACHED_V01;
            //printf("jason add NAS_CS_ATTACHED_V01 \r\n");
          }
        } else {

          if (NasCsstate != nas_status.cs_attach_state) {
            NasCsstate = nas_status.cs_attach_state;
//            printf("jason add NasCsstate =%d \r\n", NasCsstate);
          }

        }

        if (nas_status.ps_attach_state == NAS_PS_ATTACHED_V01) {
          if (NasPsstate != NAS_PS_ATTACHED_V01) {
            NasPsstate = NAS_PS_ATTACHED_V01;
            printf("jason add NAS_PS_ATTACHED_V01 \r\n");
          }
        } else {

          if (NasPsstate != nas_status.ps_attach_state) {

            NasPsstate = nas_status.ps_attach_state;
            printf("jason add NasPsstate =%d \r\n", NasPsstate);
          }

        }

        if ((NasPsstate == NAS_PS_ATTACHED_V01) && (NasCsstate == NAS_CS_ATTACHED_V01)
            && (nas_status.registration_state == NAS_REGISTERED_V01)) {
          u8ModemWaitTime = 0;
          u8ModemDataCallCount = 0;
          u8ModemWorkState = MODEM_STATE_DATA_CALL;

          //printf("jason add modem  55555555555555555555555\r\n");
        } else {
          u8ModemWaitTime++;
          if (u8ModemWaitTime >= MODE_WAIT_PSCS_TIME_OUT) {
            u8ModemWaitTime = 0;
            u8ModemWorkState = MODEM_STATE_DATA_CALL;

            //printf("jason add modem  6666666666666666666666666\r\n");
          }
          break;
        }

      }

      case MODEM_STATE_DATA_CALL: {
        logDebug << COLOR(MAGENTA) << "MODEM_STATE_DATA_CALL" << COLOR(none) << endl;

        start_dataCall(app_tech_auto, DSI_IP_VERSION_4, 4, APN1, NULL, NULL);//new sdk interface

        u8ModemWorkState = MODEM_STATE_DATA_CALL_CHECK;
        u8DataCallWaitTime = MODE_WAIT_ST_DC_TIME_OUT;
      }

      case MODEM_STATE_DATA_CALL_CHECK: {
        logDebug << COLOR(MAGENTA) << "MODEM_STATE_DATA_CALL_CHECK" << COLOR(none) << endl;

        if (u8DataCallWaitTime > 0) {
          u8DataCallWaitTime--;
          break;
        }
//        u8DataCallWaitTime = MODE_WAIT_ST_DC_TIME_OUT;

        get_datacall_info_by_profile(4, &datacall_info);//new sdk interface

        if (datacall_info.status == DATACALL_DISCONNECTED) {
          logNotice << "DATACALL_DISCONNECTED" << endl;
          if (u8ModemWaitTime++ >= MODE_WAIT_DC_TIME_OUT) /*等待10S，还没有连接上重新拨号*/
          {
            u8ModemWaitTime = 0;
            u8ModemDataCallCount++;

            if (u8ModemDataCallCount >= MODE_DATACALL_MAX_COUNT) {

              get_wifi_status(&wifi_status);
              logDebug << "wifi_status" << to_string(wifi_status) << endl;
              if (wifi_status != 1) {
                u8ModemWorkState = MODEM_STATE_REBOOT;
              } else {

                u8ModemDataCallCount = 0;
                u8ModemWorkState = MODEM_STATE_DATA_CALL;
              }

            } else {
              u8ModemWorkState = MODEM_STATE_DATA_CALL;

            }
          }
        } else {
          u8ModemDataCallCount = 0;
          u8ModemWorkState = MODEM_STATE_IDLE_CHECK;

        }
        break;
      }

      case MODEM_STATE_IDLE_CHECK: {
        logDebug << COLOR(MAGENTA) << "MODEM_STATE_IDLE_CHECK" << COLOR(none) << endl;

        if (u8ModemWaitTime == 0) {
          u8ModemWaitTime = 30;

          get_datacall_info_by_profile(4, &datacall_info);//new sdk interface

          if (datacall_info.status == DATACALL_DISCONNECTED) {
            logNotice << "DATACALL_DISCONNECTED" << endl;
            u8ModemWorkState = MODEM_STATE_DATA_CALL;
          }

        } else {
          get_SignalStrength((int *) &tboxInfo.networkStatus.signalStrength, &ret);//new sdk interface
          sprintf(VER, "network signal strength %d \r\n", tboxInfo.networkStatus.signalStrength);
        }

        if (u8ModemWaitTime > 0) {
          u8ModemWaitTime--;
        }

      }
        break;

      case MODEM_STATE_REBOOT: {
        logDebug << COLOR(MAGENTA) << "MODEM_STATE_REBOOT" << COLOR(none) << endl;

        sleep(1);
        system("sys_reboot");

        break;

      }

      default:break;
    }

    sleep(1);

  }

#endif

  while (1) {
    printf("================================================ \n ");
    sleep(5);
  }

}
