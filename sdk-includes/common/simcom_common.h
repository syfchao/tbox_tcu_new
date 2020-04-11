#ifndef _SIMCOM_COMMON_H
#define _SIMCOM_COMMON_H
#include "qmi_client.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "VoiceCall.h"
#ifndef DEBUG_C
	#define DEBUG_C 1
#endif

#if DEBUG_C
    #define DEBUGLOG(format,...) printf("### SYSLOG ### %s,%s[%d] " format"\n",__FILE__,__FUNCTION__,__LINE__,##__VA_ARGS__)
	#define DEBUG_NO(format,...) printf(format,##__VA_ARGS__)
#else
	#define DEBUGLOG(format,...)
	#define DEBUG_NO(format,...)
#endif

#ifndef __int8_t_defined
# define __int8_t_defined
typedef signed char     int8_t;
typedef short int       int16_t;
typedef int             int32_t;
# if __WORDSIZE == 64
typedef long int        int64_t;
# else
__extension__
typedef long long int   int64_t;
# endif
#endif

/* Unsigned. */
typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
#ifndef __uint32_t_defined
typedef unsigned int        uint32_t;
# define __uint32_t_defined
#endif
#if __WORDSIZE == 64
typedef unsigned long int   uint64_t;
#else
__extension__
typedef unsigned long long int uint64_t;
#endif


typedef enum
{
    SIMCOM_EVENT_SERVING_SYSTEM_IND = 0,
    SIMCOM_EVENT_SMS_PP_IND,
    SIMCOM_EVENT_VOICE_CALL_IND,
    SIMCOM_EVENT_VOICE_RECORD_IND,
    SIMCOM_EVENT_NETWORK_IND,
    /*SIMCOM_EVENT_DATACALL_IND,*/
    SIMCOM_EVENT_DATACALL_CONNECTED_IND,
    SIMCOM_EVENT_DATACALL_DISCONNECTED_IND,
    SIMCOM_EVENT_NMEA_IND,
    SIMCOM_EVENT_LOC_IND,
    SIMCOM_EVENT_MAX
}simcom_event_e;

typedef enum {
    PHONE_TYPE_E = 1,
    PHONE_TYPE_B,
    PHONE_TYPE_I,    
}phone_type;

//#define SIMCOM_TEST_UI
int voiceCall(char *callingNumber, phone_type phoneType);

#define SIM_COM_LIB_USE //20200410

#ifdef SIM_COM_LIB_USE
extern "C" void process_simcom_ind_message(simcom_event_e event,void *cb_usr_data);
#else
void process_simcom_ind_message(simcom_event_e event,void *cb_usr_data);
#endif

#ifdef SIMCOM_TEST_UI
//void print_option_menu(const char* options_list[], int array_size);
void simcom_test_main();

#endif

#endif /* _SIMCOM_COMMON_H */
