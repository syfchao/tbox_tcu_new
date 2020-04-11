#ifndef _SDK_VERSION_H_
#define _SDK_VERSION_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "simcom_demo_version.h"

#define SDK_VERSION    "SIM_SDK_VER_20180908"

const char * get_simcom_sdk_version(void)
{
    return SDK_VERSION;
}
#ifdef __cplusplus
}
#endif
#endif



