/**
  @file
  app_data_call.c

  @brief
  This file provides a demo to inplement a data call in mdm9607 embedded linux
  plane.

*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "DataCall.h"
#include <arpa/inet.h>
#include <signal.h>
#include "ds_util.h"

#include <sys/ioctl.h>

#include <net/if.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#define SA_FAMILY(addr)         (addr).sa_family
#define SA_DATA(addr)           (addr).sa_data
#define SASTORAGE_FAMILY(addr)  (addr).ss_family
#define SASTORAGE_DATA(addr)    (addr).__ss_padding

#define RMNET0_NAME "rmnet_data0"
#define RMNET1_NAME "rmnet_data1"
#define DSI_PROFILE_3GPP2_OFFSET (1000)


typedef struct {
  app_tech_e tech;
  unsigned int dsi_tech_val;
} app_tech_map_t;

app_tech_map_t tech_map[] = {
  {app_tech_umts, DSI_RADIO_TECH_UMTS},
  {app_tech_cdma, DSI_RADIO_TECH_CDMA},
  {app_tech_1x, DSI_RADIO_TECH_1X},
  {app_tech_do, DSI_RADIO_TECH_DO},
  {app_tech_lte, DSI_RADIO_TECH_LTE},
  {app_tech_auto, DSI_RADIO_TECH_UNKNOWN}
};


struct event_strings_s
{
  dsi_net_evt_t evt;
  char * str;
};

pthread_t route_setting_thread;
pthread_t datacall_disconnect_thread;

struct event_strings_s event_string_tbl[DSI_EVT_MAX] =
{
  { DSI_EVT_INVALID, "DSI_EVT_INVALID" },
  { DSI_EVT_NET_IS_CONN, "DSI_EVT_NET_IS_CONN" },
  { DSI_EVT_NET_NO_NET, "DSI_EVT_NET_NO_NET" },
  { DSI_EVT_PHYSLINK_DOWN_STATE, "DSI_EVT_PHYSLINK_DOWN_STATE" },
  { DSI_EVT_PHYSLINK_UP_STATE, "DSI_EVT_PHYSLINK_UP_STATE" },
  { DSI_EVT_NET_RECONFIGURED, "DSI_EVT_NET_RECONFIGURED" },
  { DSI_EVT_QOS_STATUS_IND, "DSI_EVT_QOS_STATUS_IND" },
  { DSI_EVT_NET_NEWADDR, "DSI_EVT_NET_NEWADDR" },
  { DSI_EVT_NET_DELADDR, "DSI_EVT_NET_DELADDR" },
  { DSI_NET_TMGI_ACTIVATED, "DSI_NET_TMGI_ACTIVATED" },
  { DSI_NET_TMGI_DEACTIVATED, "DSI_NET_TMGI_DEACTIVATED" },
  { DSI_NET_TMGI_LIST_CHANGED, "DSI_NET_TMGI_LIST_CHANGED" },
};

dsi_hndl_t handle = 0; 
int init_status = 0;


static pthread_mutex_t datacall_lock; 

static datacall_info_type s_datacall_info[MAX_DATACALL_NUM];

int check_ip_and_name_match(char *if_name, char *if_ip)
{
    FILE *fp;
    int i;
    char buff[16] = {0};
    memset(buff, 0, sizeof(buff));
    char command[512]={0};

    if(strlen(if_ip) < 7)
    {
        return FALSE;
    }
    sprintf(command,"ifconfig %s|sed -n '/inet addr/p' |awk '{print $2}'|awk -F: '{print $2}'|awk '{print $1}'",if_name);
    //sprintf(command,"hostapd_cli -i wlan%d all_sta | grep \"dot11RSNAStatsSTAAddress\" | awk -F= '{print $2}'", interface_num);
    printf("command:%s\n",command);
    fp = popen(command,"r");
    if(fp == NULL)
    {
        return FALSE;
    }
    
    if(fread(buff, sizeof(char), 15, fp) > 0)
    {
        if(strlen(buff) < 7)
        {
            pclose(fp);
            return FALSE;
        }
        
        for(i = 0;i < strlen(buff); i++)
        {
            if((buff[i] != '.') && !(buff[i] >= 0x30 && buff[i] <= 0x39))
            {
                buff[i] = 0;
            }
        }
        printf("if_ip: %s, len = %d\n", if_ip, strlen(if_ip));
        printf("buff: %s, len = %d\n", buff, strlen(buff));
        //printf("buff:%02X%02X%02X\n", buff[strlen(buff)-3],buff[strlen(buff)-2],buff[strlen(buff)-1]);

        if(strlen(buff) < 7)
        {
            pclose(fp);
            return FALSE;
        }

        if(strlen(buff) == strlen(if_ip) && strcmp(buff, if_ip) == 0)
        {
            printf("match buff = %s\n",buff);
            pclose(fp);
            return TRUE;
        }
    }
    pclose(fp);
    return FALSE;  
}

static boolean check_valid_ip(char* ip)
{
    int a=-1,b=-1,c=-1,d=-1;
    char s[16]={0};

    if(ip == NULL || strlen(ip) < 7 || strlen(ip) > 15)
    {
        return FALSE;
    }

    sscanf(ip,"%d.%d.%d.%d%s",&a,&b,&c,&d,s);
    if(a>255 || a<0 || b>255 || b<0 || c>255 || c<0 ||d>255 || d<0 ) 
        return FALSE;
    if(s[0]!=0) 
        return FALSE;
     
    return TRUE;
}

#if 0
int query_ip_from_dns(char *url, char *pri_dns_ip, char *sec_dns_ip, char *ip)
{
    FILE *fp;
    char buff[32];
    memset(buff, 0, sizeof(buff));
    char command[512];
    int query_result = 0;
    int i;
    
    if(url == NULL || strlen(url) < 2)
    {
        return -1;
    }
    if((pri_dns_ip == NULL || strlen(pri_dns_ip) < 7) &&
       (pri_dns_ip == NULL || strlen(sec_dns_ip) < 7))
    {
        return -1;
    }

    if(ip == NULL)
    {
        return -1;
    }

    printf("query_ip_from_dns: url=%s\n", url);
    printf("query_ip_from_dns: dns1=%s\n", pri_dns_ip);
    printf("query_ip_from_dns: dns2=%s\n", sec_dns_ip);
    if(pri_dns_ip != NULL)
    {
        memset(buff, 0, sizeof(buff));
        memset(command, 0, sizeof(command));
        sprintf(command,"nslookup %s %s|awk '{L[NR]=$0}END{for (i=5;i<=NR;i++){print L[i]}}'|grep Address|awk -F\": \" '{print $2}'",
                         url, pri_dns_ip);
        printf("cmd=%s", command);
        fp = popen(command,"r");

        if(fread(buff, sizeof(char), 15, fp) >= 7)
        {
            printf("dns1 buff:%s buf_len=%d, buff[len-1]=%d\n",buff,strlen(buff),buff[strlen(buff) - 1]);
            
            for(i = 0;i < strlen(buff); i++)
            {
                if((buff[i] != '.') && !(buff[i] >= 0x30 && buff[i] <= 0x39))
                {
                    buff[i] = 0;
                }
            }
            if(strlen(buff) >= 7 && check_valid_ip(buff))
            {
                strcpy(ip,buff);
                query_result = 1;
            }
        }
        pclose(fp);
    }

    if(query_result == 0 && sec_dns_ip != NULL)
    {
        memset(buff, 0, sizeof(buff));
        memset(command, 0, sizeof(command));
        sprintf(command,"nslookup %s %s|awk '{L[NR]=$0}END{for (i=5;i<=NR;i++){print L[i]}}'|grep Address|awk -F\": \" '{print $2}'",
                         url, sec_dns_ip);
        printf("cmd=%s", command);                 
        fp = popen(command,"r");
    
        if(fread(buff, sizeof(char), 15, fp) >= 7)
        {
            printf("dns2 buff:%s buf_len=%d, buff[len-1]=%d\n",buff,strlen(buff),buff[strlen(buff) - 1]);
            
            for(i = 0;i < strlen(buff); i++)
            {
                if((buff[i] != '.') && !(buff[i] >= 0x30 && buff[i] <= 0x39))
                {
                    buff[i] = 0;
                }
            }
            if(strlen(buff) >= 7 && check_valid_ip(buff))
            {
                strcpy(ip,buff);
                query_result = 1;
            }
        }
        pclose(fp);

    }

    if(query_result)
        return 0;
    else
        return -1;
}
#endif

int set_host_route(char *old_ip, char *new_ip, char *if_name)
{
    FILE *fp;
    char buff[16];
    char command[512];
    int old_ip_route_exist = 0;
    memset(buff, 0, sizeof(buff));
    memset(command, 0, sizeof(command));

    if(old_ip != NULL && strlen(old_ip) >= 7)
    {
        sprintf(command,"route -n|grep %s|grep UH|grep %s|awk '{print $1}'",old_ip, if_name);

        fp = popen(command,"r"); 
        if(fread(buff, sizeof(char), 15, fp) >= 7)
        {     
            if(strlen(buff) == strlen(old_ip))
            {
                old_ip_route_exist = 1;
            }
        }
        pclose(fp);
        if(old_ip_route_exist)
        {
            memset(command, 0, sizeof(command));
            sprintf(command,"route del -host %s dev %s",old_ip, if_name);
            ds_system_call(command, strlen(command));
        }
    }
    
    if(new_ip != NULL && strlen(new_ip) >= 7 && strlen(new_ip) <= 15)
    {
        if(check_valid_ip(new_ip))
        {
            memset(command, 0,sizeof(command));
            sprintf(command,"route add -host %s dev %s",new_ip, if_name);
            ds_system_call(command, strlen(command));        
        }
    }   
}

static void set_datacall_info(dsi_hndl_t handle)
{
    char device_name[DSI_CALL_INFO_DEVICE_NAME_MAX_LEN + 2] = {0};
    int index = -1;
    int ret;
    dsi_addr_info_t addr_info;
    char cmd[128] = {0};
    datacall_info_type datacall_info;
    datacall_info_type *pcallinfo = NULL;
    int i;
	printf("jason add ===========set_datacall_info \r\n");

    for(i=0; i<MAX_DATACALL_NUM; i++)
    {
        if(handle == s_datacall_info[i].handle)
        {
            pcallinfo = &s_datacall_info[i];
            break;
        }
    }

    if(pcallinfo == NULL)
    {
        return;
    }
    
    memset(&datacall_info, 0, sizeof(datacall_info_type));

    memset(&addr_info, 0, sizeof(dsi_addr_info_t));
    
    //ret = dsi_get_device_name(handle,device_name,DSI_CALL_INFO_DEVICE_NAME_MAX_LEN);
    //printf("get device name: ret = %d, device_name=%s\n",ret, device_name);
  
    ret = dsi_get_ip_addr(handle,&addr_info,1);
    printf("get ip addr ret = %d\n",ret);

    
    if (addr_info.iface_addr_s.valid_addr)
    {
        if (SASTORAGE_FAMILY(addr_info.iface_addr_s.addr) == AF_INET)
        {
            sprintf(datacall_info.ip_str,  "%d.%d.%d.%d", SASTORAGE_DATA(addr_info.iface_addr_s.addr)[0], SASTORAGE_DATA(addr_info.iface_addr_s.addr)[1], SASTORAGE_DATA(addr_info.iface_addr_s.addr)[2], SASTORAGE_DATA(addr_info.iface_addr_s.addr)[3]);
            printf("ip_str = %s\n", datacall_info.ip_str);        
        }
    }
    if (addr_info.dnsp_addr_s.valid_addr)
    {
        sprintf(datacall_info.pri_dns_str,"%d.%d.%d.%d", SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[0], SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[1], SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[2], SASTORAGE_DATA(addr_info.dnsp_addr_s.addr)[3]);
        printf("pri_dns_str = %s\n", datacall_info.pri_dns_str);
    }
    if (addr_info.dnss_addr_s.valid_addr)
    {
        sprintf(datacall_info.sec_dns_str, "%d.%d.%d.%d", SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[0], SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[1], SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[2], SASTORAGE_DATA(addr_info.dnss_addr_s.addr)[3]);
        printf("sec_dns_str = %s\n", datacall_info.sec_dns_str);
    }
    if (addr_info.gtwy_addr_s.valid_addr)
    {
        sprintf(datacall_info.gw_str, "%d.%d.%d.%d", SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[0], SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[1], SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[2], SASTORAGE_DATA(addr_info.gtwy_addr_s.addr)[3]);
        printf("gw = %s\n", datacall_info.gw_str);
    }

    datacall_info.mask = addr_info.iface_mask;
    
    printf("ip netmask: %d\n", datacall_info.mask);
    printf("gw netmask: %d\n", addr_info.gtwy_mask);

    //ret = get_current_device_info(ip_str, if_name);

    if(check_ip_and_name_match(RMNET1_NAME,datacall_info.ip_str))
    {
        index = 1;
        strcpy(datacall_info.if_name, RMNET1_NAME);
    }
    else if(check_ip_and_name_match(RMNET0_NAME,datacall_info.ip_str))
    {   
        index = 0;
        strcpy(datacall_info.if_name, RMNET0_NAME);
    }
    else
    {
        printf("no match\n");
        return;
    }

    printf("index = %d\n", index);

    memset(pcallinfo->if_name, 0, sizeof(pcallinfo->if_name));
    strcpy(pcallinfo->if_name,  datacall_info.if_name);

    if(strlen(datacall_info.ip_str) >= 7)
    {
        memset(pcallinfo->ip_str,0,sizeof(pcallinfo->ip_str));
        strcpy(pcallinfo->ip_str, datacall_info.ip_str);
    }
    
    if(strlen(datacall_info.gw_str) >= 7)
    {
        memset(pcallinfo->gw_str,0,sizeof(pcallinfo->gw_str));
        strcpy(pcallinfo->gw_str, datacall_info.gw_str);
    }

    pcallinfo->mask = datacall_info.mask;

    if(strlen(pcallinfo->pri_dns_str) >= 7)
    {
        memset(cmd, 0,sizeof(cmd));
        sprintf(cmd, "route del -host %s dev %s%d",pcallinfo->pri_dns_str, "rmnet_data", index);
        printf("cmd=%s\n", cmd);
        ds_system_call(cmd, strlen(cmd));        
    }
    if(strlen(datacall_info.pri_dns_str) >= 7)
    {
        memset(cmd, 0,sizeof(cmd));
        sprintf(cmd, "route add -host %s dev %s%d",datacall_info.pri_dns_str, "rmnet_data", index);
        printf("cmd=%s\n", cmd);
        ds_system_call(cmd, strlen(cmd));
        strcpy(pcallinfo->pri_dns_str,datacall_info.pri_dns_str);
    }
    else
    {
        memset(pcallinfo->pri_dns_str, 0, sizeof(pcallinfo->pri_dns_str));
    }
    if(strlen(pcallinfo->sec_dns_str) >=7)
    {
        memset(cmd, 0,sizeof(cmd));
        sprintf(cmd, "route del -host %s dev %s%d",pcallinfo->sec_dns_str, "rmnet_data", index);
        printf("cmd=%s\n", cmd);
        ds_system_call(cmd, strlen(cmd));        
    }
    if(strlen(datacall_info.sec_dns_str) >= 7)
    {
        memset(cmd, 0,sizeof(cmd));
        sprintf(cmd, "route add -host %s dev %s%d",datacall_info.sec_dns_str, "rmnet_data", index);
        printf("cmd=%s\n", cmd);
        ds_system_call(cmd, strlen(cmd));
        strcpy(pcallinfo->sec_dns_str,datacall_info.sec_dns_str);
    }
    else
    {
        memset(pcallinfo->sec_dns_str, 0, sizeof(pcallinfo->sec_dns_str));
    }

    pcallinfo->status = DATACALL_CONNECTED;
}

void *datacall_connected_fun(void *pData)
{
	printf("jason add ===========datacall_connected_fun \r\n");
    dsi_hndl_t handle = (dsi_hndl_t)pData;
    pthread_detach(pthread_self());
    pthread_mutex_lock(&datacall_lock);
    set_datacall_info(handle);
    pthread_mutex_unlock(&datacall_lock);
    process_simcom_ind_message(SIMCOM_EVENT_DATACALL_CONNECTED_IND, handle);
}

void datacall_disconnected_fun(dsi_hndl_t handle)
{
    dsi_ce_reason_t end_reason;
    datacall_info_type *pcallinfo = NULL;
    char cmd[256]={0};
    int i;
	printf("jason add ======== datacall_disconnected_fun\r\n");
    pthread_mutex_lock(&datacall_lock);
    for(i = 0; i < MAX_DATACALL_NUM; i++)
    {
        if(handle == s_datacall_info[i].handle)
        {
            pcallinfo = &s_datacall_info[i];
            break;
        }
    }

    if(pcallinfo == NULL)
    {
        return;
    }

    if(strlen(pcallinfo->ip_str) > 0)
    {
        memset(pcallinfo->ip_str,0,sizeof(pcallinfo->ip_str));
    }
    
    if(strlen(pcallinfo->gw_str) > 0)
    {
        memset(pcallinfo->gw_str,0,sizeof(pcallinfo->gw_str));
    }

    if(strlen(pcallinfo->pri_dns_str) >=7)
    {
        sprintf(cmd, "route del -host %s dev %s%",pcallinfo->pri_dns_str, pcallinfo->if_name);
        ds_system_call(cmd, strlen(cmd));
        memset(pcallinfo->pri_dns_str,0,sizeof(pcallinfo->pri_dns_str));
        
    }
    if(strlen(pcallinfo->sec_dns_str) >=7)
    {
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, "route del -host %s dev %s%",pcallinfo->sec_dns_str, pcallinfo->if_name);
        ds_system_call(cmd, strlen(cmd)); 
        memset(pcallinfo->pri_dns_str,0,sizeof(pcallinfo->pri_dns_str));
    }

    memset(pcallinfo, 0, sizeof(datacall_info_type));
    
    pthread_mutex_unlock(&datacall_lock);

    //get_datacall_end_reason_by_handle(handle, &pcallinfo->end_reason, DSI_IP_FAMILY_V4);
   
    dsi_get_call_end_reason(handle, &end_reason, DSI_IP_FAMILY_V4);
    process_simcom_ind_message(SIMCOM_EVENT_DATACALL_DISCONNECTED_IND, &end_reason);
}



static void app_net_ev_cb( dsi_hndl_t hndl,
                     void * user_data,
                     dsi_net_evt_t evt,
                     dsi_evt_payload_t *payload_ptr)
{
    int i;
    char suffix[256];

    if (evt > DSI_EVT_INVALID && evt < DSI_EVT_MAX)
    {
        app_call_info_t* call = (app_call_info_t*) user_data;
        memset( suffix, 0x0, sizeof(suffix) );

        /* Update call state */
        switch( evt )
        {
            case DSI_EVT_NET_NO_NET:
                {
                    call->call_status =  app_call_status_idle;
                    pthread_create(&datacall_disconnect_thread, NULL, datacall_disconnected_fun, (void*)hndl);
                    //datacall_disconnected_fun(hndl); 
                }
                break;

            case DSI_EVT_NET_IS_CONN:
                {
                    call->call_status = app_call_status_connected;
                    pthread_create(&route_setting_thread, NULL, datacall_connected_fun, (void*)hndl);
                }
                break;

            case DSI_EVT_QOS_STATUS_IND:
            case DSI_EVT_PHYSLINK_DOWN_STATE:
            case DSI_EVT_PHYSLINK_UP_STATE:
            case DSI_EVT_NET_RECONFIGURED:
            case DSI_EVT_NET_NEWADDR:
            case DSI_EVT_NET_DELADDR:
            case DSI_NET_TMGI_ACTIVATED:
            case DSI_NET_TMGI_DEACTIVATED:
            case DSI_NET_TMGI_LIST_CHANGED:
                break;

            default:
                printf("<<< callback received unknown event, evt= %d\n",evt);
                return;
        }

        /* Debug message */
        for (i=0; i < DSI_EVT_MAX; i++)
        {
            if (event_string_tbl[i].evt == evt)
            {
                printf("<<< callback received event [%s]%s\n",event_string_tbl[i].str, suffix);         
                break;
            }
        }  
    }
}

int get_datacall_info_by_profile(int profile, datacall_info_type *pcallinfo)
{
    int ret = -1;
    int i;
    pthread_mutex_lock(&datacall_lock);

    if(pcallinfo != NULL)
    {
        for(i = 0; i < MAX_DATACALL_NUM; i++)
        {
            if(profile == s_datacall_info[i].profile)
            {
                memcpy(pcallinfo, &s_datacall_info[i], sizeof(datacall_info_type));
                ret = 0;
                break;
            }
        }
    }
    pthread_mutex_unlock(&datacall_lock);
    return ret;
}

int get_datacall_info_by_handle(dsi_hndl_t handle, datacall_info_type *pcallinfo)
{
    int ret = -1;
    int i;
    pthread_mutex_lock(&datacall_lock);

    if(pcallinfo != NULL)
    {
        for(i = 0; i < MAX_DATACALL_NUM; i++)
        {
            if(handle == s_datacall_info[i].handle)
            {
                memcpy(pcallinfo, &s_datacall_info[i], sizeof(datacall_info_type));
                ret = 0;
                break;
            }
        }
    }
    pthread_mutex_unlock(&datacall_lock);
    return ret;
}

int app_add_datacall_info(dsi_hndl_t handle, int ip_family, app_tech_e tech, int profile)
{
    int ret = 0;
    int i = 0;

    pthread_mutex_lock(&datacall_lock);
    for(i = 0; i < MAX_DATACALL_NUM; i++)
    {
        if(s_datacall_info[i].handle != NULL)
        {
            continue;
        }

        s_datacall_info[i].handle = handle;
        s_datacall_info[i].family = ip_family;
        s_datacall_info[i].tech = tech;
        s_datacall_info[i].profile = profile;

        ret = 1;
        break;
    }  
    pthread_mutex_unlock(&datacall_lock);

    return ret;
}

int app_del_datacall_info(int profile)
{
    int ret = 0;
    int i = 0;

    pthread_mutex_lock(&datacall_lock);

    for(i = 0; i < MAX_DATACALL_NUM; i++)
    {
        if(profile == s_datacall_info[i].profile) {
            memset(&s_datacall_info[i], 0, sizeof(datacall_info_type));
            ret = 1;
            break;
        }
    }

    pthread_mutex_unlock(&datacall_lock);
    return ret;
}

int start_dataCall(app_tech_e tech, int ip_family, int profile, char *apn, char *username, char *password)
{
    int                    ret = -1;
    int                    i;
    dsi_call_param_value_t param_info;
    app_call_info_t        app_call_info;
    datacall_info_type     datacall_info;
    char                   username_val[QMI_WDS_USER_NAME_MAX_V01 + 1] = {0};
    char                   password_val[QMI_WDS_PASSWORD_MAX_V01 + 1] = {0};
    printf("===============jason add for start_dataCall====================== \r\n");

    memset(&datacall_info, 0, sizeof(datacall_info_type));
    get_datacall_info_by_profile(profile, &datacall_info);

    if(datacall_info.handle != 0){
    printf("The datacall has been created.[profile = %d]\n", profile);
        return ret;
    }
    
    handle = dsi_get_data_srvc_hndl(app_net_ev_cb, (void*) &app_call_info);
    printf("app_call_info.handle 0x%x\n",app_call_info.handle );


    param_info.buf_val = NULL;
    param_info.num_val = (int)tech_map[tech].dsi_tech_val;
    printf("Setting tech to %d\n", param_info.num_val);
    dsi_set_data_call_param(handle, DSI_CALL_INFO_TECH_PREF, &param_info);


    if (profile > 0)
    {
        param_info.buf_val = NULL;
        param_info.num_val = 0;
        printf("Setting 3GPP2 PROFILE to %d\n", param_info.num_val);
        dsi_set_data_call_param(handle, DSI_CALL_INFO_CDMA_PROFILE_IDX, &param_info);

        param_info.buf_val = NULL;
        param_info.num_val = profile;
        printf("Setting 3GPP PROFILE to %d\n", param_info.num_val);
        dsi_set_data_call_param(handle, DSI_CALL_INFO_UMTS_PROFILE_IDX, &param_info);
    }

    
    switch (tech)
    {
        case app_tech_umts:
        case app_tech_lte:
        case app_tech_auto:
            if(apn != NULL && strlen(apn) > 0 && strlen(apn) <= QMI_WDS_APN_NAME_MAX_V01)
            {
                param_info.buf_val = apn;
                param_info.num_val = strlen(apn);
                printf("Setting APN to %s\n", apn);
                dsi_set_data_call_param(handle, DSI_CALL_INFO_APN_NAME, &param_info);
            }
            break;

        case app_tech_cdma:
            param_info.buf_val = NULL;
            param_info.num_val = DSI_AUTH_PREF_PAP_CHAP_BOTH_ALLOWED;
            printf("Setting auth pref to both allowed");
            dsi_set_data_call_param(handle, DSI_CALL_INFO_AUTH_PREF, &param_info);
            break;

        default:
            break;
    }
    

    if(username != NULL && strlen(username) > 0 && strlen(username) <= QMI_WDS_USER_NAME_MAX_V01)
    {
        //memcpy(username_val, username, strlen(username));
        param_info.buf_val = username;
        param_info.num_val = strlen(username);   
        printf("Setting username: %s\n", param_info.buf_val);
        dsi_set_data_call_param(handle, DSI_CALL_INFO_USERNAME, &param_info);
    }
    if(password != NULL && strlen(password) > 0 && strlen(password) <= QMI_WDS_PASSWORD_MAX_V01)
    {
        //memcpy(password_val, password, strlen(password));
        param_info.buf_val = password;
        param_info.num_val = strlen(password);  
        printf("Setting password: %s\n", param_info.buf_val);
        dsi_set_data_call_param(handle, DSI_CALL_INFO_PASSWORD, &param_info);
    }
    


    //------ DSI_IP_VERSION_4:  DSI_IP_VERSION_6  DSI_IP_VERSION_4_6
    param_info.buf_val = NULL;
    param_info.num_val = ip_family;
    printf("Setting family to %d\n", ip_family);
    dsi_set_data_call_param(handle, DSI_CALL_INFO_IP_VERSION, &param_info);


    ret = dsi_start_data_call(handle);
    if(ret != DSI_SUCCESS)
    {
        return ret;
    }

    app_add_datacall_info(handle, ip_family, tech, profile);

    return ret;
}

int stop_dataCall(int profile)
{
    int ret = -1;
    datacall_info_type  datacall_info;

    //check the profile 
    memset(&datacall_info, 0, sizeof(datacall_info_type));
    get_datacall_info_by_profile(profile, &datacall_info);


    if(datacall_info.handle != 0){
        dsi_stop_data_call(datacall_info.handle);

        ret = 0;
    }

    return ret;
}

int datacall_init()
{
    if(init_status)
    {
        // already init
        return 0;
    }

    memset(s_datacall_info, 0, sizeof(s_datacall_info));
	printf("jason add for s_datacall_info[4].status = %d \r\n",s_datacall_info[4].status);
    if (DSI_SUCCESS !=  dsi_init(DSI_MODE_GENERAL))
    {
        printf("%s", "dsi_init failed !!\n");
        return -1;
    }
    init_status = 1;
    return 0;
}

int datacall_deinit()
{
    int i = 0;
    if(init_status == 1)
    {
        dsi_release(DSI_MODE_GENERAL);
    }
    init_status = 0;
}


