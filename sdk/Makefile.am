DIR_SRC = ./src

AM_CFLAGS = -Wall \
    -Wundef \
    -Wno-trigraphs \
    -g -O0 \
    $(QMIFRAMEWORK_CFLAGS) \
    $(QMI_CFLAGS) \
    -DFEATURE_DSS_LINUX \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/inc \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/common/inc \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/qmi \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/qmi-framework \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/mcm \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/dsutils \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/dsi \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/data \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/common \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/gps
  
 
AM_CXXFLAGS = \
    $(AM_CFLAGS) \
    -fpermissive

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -D__packed__= \
        -DIMAGE_APPS_PROC \
        -DFEATURE_Q_SINGLE_LINK \
        -DFEATURE_Q_NO_SELF_QPTR \
        -DFEATURE_DSS_LINUX \
        -DFEATURE_LINUX \
        -DFEATURE_NATIVELINUX \
        -DFEATURE_DSM_DUP_ITEMS \
        -DFEATURE_LE_DIAG \
        -DFEATURE_MULTIPROCESSOR \
        -DAEEINTERFACE_CPLUSPLUS \
        -DFEATURE_SMD \
        -DFEATURE_DIAG_MP_MASTER_APPS \
        -DFEATURE_DS_LINUX_ANDROID \
        -DFEATURE_DSS_LINUX \
        -DFEATURE_DSUTILS_OFFTARGET \
        -DFEATURE_NO_REX \
        -DFEATURE_XMLLIB \
        $(DIAG_CFLAGS) \
        $(DSUTILS_CFLAGS) \
        $(XMLLIB_CFLAGS) \
        $(CFLAGS) \
        $(MCM_CORE_CFLAGS) \
        -I./inc \
        -Dstrlcpy=g_strlcpy \
        -Dstrlcat=g_strlcat \
        -DBUILD_VERSION_WIFI_RTL \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/inc \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/common/inc \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/qmi \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/qmi-framework \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/mcm \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/dsutils \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/dsi \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/data \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/common \
    -I$(WORKSPACE)/simcom-daemons/simcom-demo/sdk-includes/gps
    
library_includedir = $(pkgincludedir)
libsdk_sources = \
    $(DIR_SRC)/DMSControl.c  \
    $(DIR_SRC)/WDSControl.c \
    $(DIR_SRC)/DnsResolv.c   \ 
    $(DIR_SRC)/ATControl.c  \
    $(DIR_SRC)/VoiceCall.c  \ 
    $(DIR_SRC)/UIMControl.c  \
    $(DIR_SRC)/NASControl.c  \                                           
    $(DIR_SRC)/DataCall.c  \
    $(DIR_SRC)/WiFiControl.c  \
    $(DIR_SRC)/SMSControl.c \
    $(DIR_SRC)/sdkVersion.c \
    $(DIR_SRC)/TTSControl.c \
    $(DIR_SRC)/ALSAControl.c \
    $(DIR_SRC)/DEVControl.c

    
lib_LTLIBRARIES = libsdk.la
libsdk_la_CC = @CC@
libsdk_la_SOURCES = $(libsdk_sources)
libsdk_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) -fPIC  @GLIB_CFLAGS@
libsdk_la_CPPFLAGS = $(AM_CPPFLAGS)  -DUSE_GLIB  @GLIB_CFLAGS@
libsdk_la_LDFLAGS = -lpthread -lrt -lssl -lcrypto -ldl $(QMIFRAMEWORK_LIBS) $(QMI_LIBS) $(DSUTILS_LIBS) $(MCM_CORE_LIBS)
