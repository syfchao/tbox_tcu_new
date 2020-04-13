//
// Created by renhono on 2020/3/18.
//

#include "ATControl.h"
#include "TBoxDataPool.h"
#include "Util.h"

int apn_init() {

  int ret;
  int apn_index = 4;
  char apn[30] = {0};
  char username[64] = {0};
  char password[64] = {0};
  int pdp_type;

  //apn 初始化

  if (wds_init() == 0) {
    std::cout << "wds_qmi_init success!" << std::endl;
  }
//    DEBUGLOG("wds_qmi_init success!\n");

  //profile_index: 4-->Private network; 6-->Public Network
  ret = get_apnInfo(apn_index, &pdp_type, apn, username, password);
  if (ret == FALSE) {
    printf("wds_GetAPNInfo Fail\n");
  } else {
    printf(">>>>>> apn[%d]=%s, pdp_type = %d, username=%s, password=%s\n",
           apn_index,
           apn,
           pdp_type,
           username,
           password);
  }

  if (strcmp(apn, APN1) != 0)
    set_apnInfo(4, 0, APN1, NULL, NULL);

  apn_index = 6;
  memset(apn, 0, sizeof(apn));
  memset(username, 0, sizeof(username));
  memset(password, 0, sizeof(password));
  ret = get_apnInfo(apn_index, &pdp_type, apn, username, password);
  if (ret == FALSE) {
    printf("wds_GetAPNInfo Fail\n");
  } else {
    printf(">>>>>> apn[%d]=%s, pdp_type = %d, username=%s, password=%s\n",
           apn_index,
           apn,
           pdp_type,
           username,
           password);
  }

  if (APN2 != NULL) {
    if (strcmp(apn, APN2) != 0)
      set_apnInfo(6, 0, APN2, NULL, NULL);
  } else {
    set_apnInfo(6, 0, NULL, NULL, NULL);
  }

  return 0;

}

int switch9011() {
  int retval = -1;
  char strResult[100];
  int i;

//  for (i = 0; i < 3; i++) {
  memset(strResult, 0, sizeof(strResult));
  retval = sendATCmd((char *) "AT+CUSBPIDSWITCH=9011,1,1", (char *) "OK", strResult, sizeof(strResult), 20000);
  if (retval > 0) {
    printf("switch9011 strResult:%s,  retval:%d \n", strResult, retval);
    retval = 0;
//      break;
  }
//  }
  //TODO:retry 3

  return retval;
}

int switchAdb() {
  int retval = -1;
  char strResult[100];
  int i;

//  for (i = 0; i < 3; i++) {
  memset(strResult, 0, sizeof(strResult));
  retval = sendATCmd((char *) "AT+CUSBADB=1", (char *) "OK", strResult, sizeof(strResult), 20000);
  if (retval > 0) {
    printf("switchAdb strResult:%s,  retval:%d \n", strResult, retval);
    retval = 0;
//      break;
  }
//  }
  //TODO:retry 3

  return retval;
}

int atProcess() {
  bool st9011 = false;
  bool stAdb = false;

  while (1) {
    if (check9011() != -1) {
      printf("ok 9011\n");
      st9011 = true;
    } else {
      printf("no 9011\n");
      switch9011();
    }
    sleep(1);

    if (checkAdb() != -1) {
      printf("ok adb\n");
      stAdb = true;
    } else {
      printf("no adb\n");
      switchAdb();
    }

    sleep(1);
    if (st9011 && stAdb) {

      printf("break\n");
      break;
    }

    sleep(1);
  }

  return 0;
}

int check9011() {
  int retval = -1;
  char strResult[100];
  int i;

  memset(strResult, 0, sizeof(strResult));
  retval =
      sendATCmd((char *) "AT+CUSBPIDSWITCH?", (char *) "+CUSBPIDSWITCH: 9011", strResult, sizeof(strResult), 5000);
  printf("CUSBPIDSWITCH %d\n", retval);
  if (retval > 0) {
    printf("strResult:%s,  retval:%d \n", strResult, retval);
    retval = 0;
  }
  return retval;
}

int checkAdb() {
  int retval = -1;
  char strResult[100];
  int i;

  memset(strResult, 0, sizeof(strResult));
  retval =
      sendATCmd((char *) "AT+CUSBADB?", (char *) "+CUSBADB: 1", strResult, sizeof(strResult), 5000);
  printf("CUSBADB %d\n", retval);
  if (retval > 0) {
    printf("strResult:%s,  retval:%d \n", strResult, retval);
    retval = 0;
  }
  return retval;
}

int NetworkInit() {
  char buff[100];
  char imei[30];
  char cimi[30];
  char iccid[30];

  TBoxDataPool *dataPool = TBoxDataPool::GetInstance();

  // get IMEI
  memset(imei, 0, sizeof(imei));
  dataPool->getPara(IMEI_INFO, (void *) imei, sizeof(imei));
  printf("imei:%s\n", imei);

  memset(buff, 0, sizeof(buff));
  if (getIMEI(buff, sizeof(buff)) == 0) {
    printf("IMEI:%s\n", buff);

    if (memcmp(imei, buff, (strlen(buff))) != 0) {
      dataPool->setPara(IMEI_INFO, (void *) buff, strlen(buff));
    }
  }

  memset(cimi, 0, sizeof(cimi));
  dataPool->getPara(CIMI_INFO, (void *) cimi, sizeof(cimi));
  printf("cimi:%s\n", cimi);

  memset(buff, 0, sizeof(buff));
  if (getCIMI(buff, sizeof(buff)) == 0) {
    printf("CIMI:%s\n", buff);

    if (memcmp(cimi, buff, (strlen(buff))) != 0) {
      dataPool->setPara(CIMI_INFO, (void *) buff, strlen(buff));
    }
  }

  if (getCPIN() > 0) {
    tboxInfo.networkStatus.isSIMCardPulledOut = 1;
    //enableMobileNetwork(1*2, networkConnectionStatus);

    memset(iccid, 0, sizeof(iccid));
    dataPool->getPara(SIM_ICCID_INFO, (void *) iccid, sizeof(iccid));

    // get ICCID
    memset(buff, 0, sizeof(buff));
    if (getICCID(buff, sizeof(buff)) == 0) {
      printf("ICCID:%s\n", buff);
      if (memcmp(iccid, buff, (strlen(buff))) != 0) {
        dataPool->setPara(SIM_ICCID_INFO, (void *) buff, strlen(buff));
      }
    }
  } else {
    printf("no sim card\n");
    tboxInfo.networkStatus.isSIMCardPulledOut = 0;
  }

  return 0;
}

int switchVoiceChannel() {
  int i;
  int retval = -1;
  //char strResult[100];

  for (i = 0; i < 3; i++) {
    //memset(strResult, 0, sizeof(strResult));
    retval = sendATCmd((char *) "AT+CSDVC=3", (char *) "OK", NULL, 0, 5000);
    if (retval > 0) {
      //printf("strResult:%s,  retval:%d \n",strResult, retval);
      break;
    }
  }
  printf("\n\n\n!!!!!\n\n\nretval:%d \n", retval);
  sendATCmd((char *) "at+CFUN=1", (char *) "OK", NULL, 0, 5000);
  return retval;
}

int hangUp_answer(voice_call_option op, unsigned char callid) {
  if ((op != VOICE_CALL_HANDUP) && (op != VOICE_CALL_ANSWER)) {
    return -1;
  }

  return voice_mtcall_process(op, callid);
}