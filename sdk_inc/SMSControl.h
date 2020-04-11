#ifndef _MESSAGE_H_
#define _MESSAGE_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "mcm_client.h"
#include "mcm_sms_v01.h"
#include "mcm_client_v01.h"
#include "simcom_common.h"

typedef struct {
  uint8 format;
  char message_content[256];
  char source_address[MCM_SMS_MAX_ADDR_LENGTH_V01 + 1];
}sms_info_type;

typedef void (*sms_ind_cb_fcn)(uint8 simcom_event, sms_info_type *sms_info);

int sms_init(sms_ind_cb_fcn sms_ind_cb);
void sms_deinit();
extern void mcm_sms_release();


/*****************************************************************************
* Function Name : send_message
* Description   : 发送短信
                  smsMode->1 只能发送英文
                  smsMode->2 可以发送UCS-2的文字（可发送中文只支持UCS2类型的文字）
* Input         : int smsMode, char *phoneNumber, char *content
* Output        : None
* Return        : id
* Auther        : ygg
* Date          : 2018.03.10
*****************************************************************************/	
int send_message(int smsMode, char *phoneNumber, unsigned char *content, int content_len);
//extern void mcm_sms_release();

#ifdef __cplusplus
}
#endif
#endif

