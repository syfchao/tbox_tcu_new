//
// Created by renhono on 2020/3/18.
//

#ifndef TBOX_TCU_NEW_INC_UTIL_H_
#define TBOX_TCU_NEW_INC_UTIL_H_

#include <iostream>
#include <WDSControl.h>

#if 1
//天津一汽APN
//生产测试用APN
//#define APN1  "zwzgyq02.clfu.njm2mapn"
//#define APN2  "UNIM2M.NJM2MAPN"
//量产APN
#define APN1  "zwzgyq02.clfu.njm2mapn"
#define APN2  "zwzgyq03.clfu.njm2mapn"
#endif

class Util {
};

extern int apn_init();
extern int switch9011();
extern int switchAdb();
extern int atProcess();
extern int check9011();
extern int checkAdb();

extern int NetworkInit();
extern int hangUp_answer();

#endif //TBOX_TCU_NEW_INC_UTIL_H_
