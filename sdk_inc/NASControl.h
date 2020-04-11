#ifndef _NAS_CONTROL_H_
#define _NAS_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qmi_client.h"
#include "network_access_service_v01.h"
#include "simcom_common.h"
#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#define NAS_QMI_TIMEOUT_VALUE  10000

#define LTE_DB_MAX   -44
#define LTE_DB_MIN   -140

#define GSM_WCDMA_DB_MAX   -51
#define GSM_WCDMA_DB_MIN   -113
typedef unsigned char boolean;
typedef unsigned char uint8;

typedef enum{
  RADIO_IF_NO_SVC = 0x00, 
  RADIO_IF_CDMA_1X = 0x01,
  RADIO_IF_CDMA_1XEVDO = 0x02,
  RADIO_IF_AMPS = 0x03, 
  RADIO_IF_GSM = 0x04, 
  RADIO_IF_UMTS = 0x05,
  RADIO_IF_LTE = 0x08,
  RADIO_IF_TDSCDMA = 0x09,
  RADIO_IF_1XLTE = 0x0A
}radio_if_type_enum;

typedef struct {
  nas_registration_state_enum_v01 registration_state;
  nas_cs_attach_state_enum_v01 cs_attach_state;
  nas_ps_attach_state_enum_v01 ps_attach_state;
  radio_if_type_enum radio_if_type;
}network_info_type;

int nas_init();
void nas_dinit();
int get_SignalStrength(int *level, int* mode);
int get_NetworkType(nas_serving_system_type_v01 *serving_system);

int nas_get_signal_strength(nas_get_signal_strength_resp_msg_v01 *signal_strength);
int nas_get_serving_system(nas_get_serving_system_resp_msg_v01 *serving_system);
int nas_get_preference_network(mode_pref_mask_type_v01 *mode_pref);
int nas_set_preference_network(mode_pref_mask_type_v01 mode_pref);

void nas_qmi_release(void);


#ifdef __cplusplus
}
#endif

#endif


