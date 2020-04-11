#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <cutils/properties.h>
#include <linux/reboot.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ds_util.h"
//#include <glib.h>
#include "qcmap_client_const.h"
#include "simcom_common.h"
#include "WiFiControl.h"

#define WIFIDAEMON_WEBCLIENT_SOCK "/data/etc/qcmap_webclient_wifidaemon_file"
#define WEBCLIENT_WIFIDAEMON_SOCK "/data/etc/qcmap_wifidaemon_webclient_file"

//Time to wait for socket responce in secs.
#define SOCK_TIMEOUT 20
#define MAX_BUFFER_SIZE 10240

#define SOCK_OPEN_ERROR "{\"commit\":\"Socket Open Error\"}"
#define SOCK_OPEN_ERROR_LEN 30
#define SOCK_FD_ERROR "{\"commit\":\"Socket FD Error\"}"
#define SOCK_FD_ERROR_LEN 28
#define SOCK_BIND_ERROR "{\"commit\":\"Socket Bind Error\"}"
#define SOCK_BIND_ERROR_LEN 30

#define SOCK_SEND_ERROR "{\"commit\":\"Socket Send Error\"}"
#define SOCK_SEND_ERROR_LEN 30
#define SOCK_SEND_COMPLETE_ERROR "{\"commit\":\"Unable to Send Complete "\
	"data through socket\"}"
#define SOCK_SEND_COMPLETE_ERROR_LEN 56
#define SOCK_TIMEOUT_ERROR "{\"commit\":\"Socket TimeOut\"}"
#define SOCK_TIMEOUT_ERROR_LEN 27
#define SOCK_RESPONSE_ERROR "{\"commit\":\"Socket Responce Error\"}"
#define SOCK_RESPONSE_ERROR_LEN 34
#define SOCK_RECEIVE_ERROR "{\"commit\":\"Socket Receive Error\"}"
#define SOCK_RECEIVE_ERROR_LEN 33
#define SUCCESS 0
#define FAIL    -1

#define MAX_ELEMENT_LENGTH 300       //Max Size of any element value
#define MAX_ELEMENT_COUNT  40       //Max Elements can be processed

#define SCAN_TEMP_FILE "/data/mifi/scan_results_temp"

#define TRUE 1
#define FALSE 0


//typedef unsigned char boolean;
//typedef unsigned char uint8;

const static char *country_name[] = {
	"AL","DZ","AR","AM","AU","AT","AZ","BH","BY","BE",
	"BZ","BO","BR","BN","BG","CA","CL","CN","CO","CR",
	"HR","CY","CZ","DK","DO","EC","EG","SV","EE","FI",
	"FR","DE","GE","GR","GT","HN","HK","HU","IS","IN",
	"ID","IR","IE","IL","IT","JO","KZ","KR","KP","KW",
	"LB","LV","LI","LT","LU","MA","MK","MY","MO","MX",
	"MC","NL","NZ","NO","OM","PK","PA","PE","PH","PL",
	"PT","PR","QA","RO","RU","SA","SG","SK","SI","ZA",
	"ES","SE","CH","SY","TW","TH","TT","TN","TR","UA",
	"AE","GB","US","UY","UZ","VE","VN","YE","ZW","JP"
};
const static int channel_number[] = {
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	10, 11, 12, 13, 36, 40, 44, 48, 52, 56,
	60, 64, 100,104,108,112,116,120,124,128,
	132,136,140,149,153
};

const static int CN_5G_channel_list[] ={
	149,153,157,161,165
};

const char *p_auth[] = { 
	"Auto",
	"Open",
	"Share",
	"WPA-PSK",
	"WPA2-PSK",
	"WPA-WPA2-PSK"
};

const char *p_encrypt[] = {
	"NULL",
	"WEP",
	"TKIP",
	"AES",
	"AES-TKIP"
};

static ap_index_type ap_index;

static wifi_mode_type get_ap_cfg();


//static int wifidaemon_mifi_configselect = 0;

static void WIFI_LOG(const char *format, ...)
{
#if 0
	printf(format, ...);
#endif
}

static int wifi_daemon_init_send_string(char** send_str, char** recv_str)
{
	*send_str = (char *)malloc(MAX_ELEMENT_COUNT * MAX_ELEMENT_LENGTH);
	if(*send_str == NULL)
	{
		return FAIL;
	}

	*recv_str = (char *)malloc(MAX_ELEMENT_COUNT * MAX_ELEMENT_LENGTH);
	if(*recv_str == NULL)
	{
		free(*send_str);
		return FAIL;
	}

	memset(*send_str, 0, MAX_ELEMENT_COUNT * MAX_ELEMENT_LENGTH);
	memset(*recv_str, 0, MAX_ELEMENT_COUNT * MAX_ELEMENT_LENGTH);
	return SUCCESS;
}

static void wifi_daemon_free_send_string(char *send_str, char* recv_str)
{
	if(send_str)
	{
		free(send_str);
	}

	if(recv_str)
	{
		free(recv_str);
	}
}

static int wifi_daemon_send_to_webcli(char *wifidaemon_webcli_buff,
		int wifidaemon_webcli_buff_size,
		char *webcli_wifidaemon_buff,
		int *webcli_wifidaemon_buff_size,
		int *webcli_sockfd)
{
	//Socket Address to store address of webclient and atfwd sockets.
	struct sockaddr_un webcli_wifidaemon_sock;
	struct sockaddr_un wifidaemon_webcli_sock;
	//Variables to track data received and sent.
	int bytes_sent_to_cli = 0;
	int sock_addr_len = 0;


	memset(&webcli_wifidaemon_sock,0,sizeof(struct sockaddr_un));
	memset(&wifidaemon_webcli_sock,0,sizeof(struct sockaddr_un));

	if ((*webcli_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
	{
		*webcli_wifidaemon_buff_size = SOCK_OPEN_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_OPEN_ERROR, SOCK_OPEN_ERROR_LEN);
		return FAIL;
	}

	if (fcntl(*webcli_sockfd, F_SETFD, FD_CLOEXEC) < 0)
	{
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_FD_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_FD_ERROR, SOCK_FD_ERROR_LEN);
		return FAIL;
	}

	webcli_wifidaemon_sock.sun_family = AF_UNIX;
	memset(webcli_wifidaemon_sock.sun_path, 0, sizeof(webcli_wifidaemon_sock.sun_path));
	strncat(webcli_wifidaemon_sock.sun_path, WEBCLIENT_WIFIDAEMON_SOCK,strlen(WEBCLIENT_WIFIDAEMON_SOCK));
	unlink(webcli_wifidaemon_sock.sun_path);
	sock_addr_len = strlen(webcli_wifidaemon_sock.sun_path) + sizeof(webcli_wifidaemon_sock.sun_family);
	if (bind(*webcli_sockfd, (struct sockaddr *)&webcli_wifidaemon_sock, sock_addr_len) == -1)
	{
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_BIND_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_BIND_ERROR, SOCK_BIND_ERROR_LEN);
		return FAIL;
	}

	wifidaemon_webcli_sock.sun_family = AF_UNIX;
	memset(wifidaemon_webcli_sock.sun_path, 0, sizeof(wifidaemon_webcli_sock.sun_path));
	strncat(wifidaemon_webcli_sock.sun_path, WIFIDAEMON_WEBCLIENT_SOCK,strlen(WIFIDAEMON_WEBCLIENT_SOCK));

	/*d rwx rwx rwx = dec
	  0 110 110 110 = 666
	  */

	//system("chmod 777 /etc/qcmap_cgi_webclient_file");
	sock_addr_len = strlen(wifidaemon_webcli_sock.sun_path) + sizeof(wifidaemon_webcli_sock.sun_family);
	if ((bytes_sent_to_cli = sendto(*webcli_sockfd, wifidaemon_webcli_buff, wifidaemon_webcli_buff_size, 0,
					(struct sockaddr *)&wifidaemon_webcli_sock, sock_addr_len)) == -1)
	{
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_SEND_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_SEND_ERROR, SOCK_SEND_ERROR_LEN);
		return FAIL;
	}

	if (bytes_sent_to_cli == wifidaemon_webcli_buff_size)
	{
		return SUCCESS;
	}
	else
	{
		close((int)*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_SEND_COMPLETE_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_SEND_COMPLETE_ERROR, SOCK_SEND_COMPLETE_ERROR_LEN);
		return FAIL;
	}
}


static int wifi_daemon_recv_from_webcli(char *webcli_wifidaemon_buff,
		int *webcli_wifidaemon_buff_size,
		int *webcli_sockfd)
{
	//FD set used to hold sockets for select.
	fd_set wifidaemon_sockfd;

	//Time out for atfwd response.
	struct timeval webcli_sock_timeout;

	//To evaluate webclient socket responce(timed out, error, ....)
	int webcli_sock_resp_status = 0;

	//Variables to track data received and sent.
	int bytes_recv_from_cli = 0;

	int retry = 0;
	do{
		FD_ZERO(&wifidaemon_sockfd);
		if( *webcli_sockfd < 0 )
		{
			*webcli_wifidaemon_buff_size = SOCK_FD_ERROR_LEN;
			strcat(webcli_wifidaemon_buff, SOCK_FD_ERROR);
			return FAIL;
		}
		FD_SET(*webcli_sockfd, &wifidaemon_sockfd);
		webcli_sock_timeout.tv_sec = 30;//SOCK_TIMEOUT;
		webcli_sock_timeout.tv_usec = 0;

		webcli_sock_resp_status = select(((int)(*webcli_sockfd))+1,
				&wifidaemon_sockfd, NULL, NULL,
				&webcli_sock_timeout);
		retry++;

		if(retry > 20)
			break;
		if(webcli_sock_resp_status == -1)
			WIFI_LOG("atfwd_select_from_webcli==============%s \r\n",strerror(errno));
	}while((webcli_sock_resp_status == -1) && (errno == EINTR));

	if (webcli_sock_resp_status == 0)
	{
		FD_CLR(*webcli_sockfd, &wifidaemon_sockfd);
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_TIMEOUT_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_TIMEOUT_ERROR, SOCK_TIMEOUT_ERROR_LEN);
		return FAIL;  //Time out
	}
	else if  (webcli_sock_resp_status == -1)
	{
		FD_CLR(*webcli_sockfd, &wifidaemon_sockfd);
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_RESPONSE_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_RESPONSE_ERROR, SOCK_RESPONSE_ERROR_LEN);
		return FAIL;  //Error
	}

	memset(webcli_wifidaemon_buff, 0, MAX_BUFFER_SIZE);
	bytes_recv_from_cli = recv(*webcli_sockfd, webcli_wifidaemon_buff, MAX_BUFFER_SIZE, 0);
	if (bytes_recv_from_cli == -1)
	{
		FD_CLR(*webcli_sockfd, &wifidaemon_sockfd);
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = SOCK_RECEIVE_ERROR_LEN;
		strncat(webcli_wifidaemon_buff, SOCK_RECEIVE_ERROR, SOCK_RECEIVE_ERROR_LEN);
		return FAIL;
	}
	else
	{
		FD_CLR(*webcli_sockfd, &wifidaemon_sockfd);
		close(*webcli_sockfd);
		*webcli_wifidaemon_buff_size = bytes_recv_from_cli;
		webcli_wifidaemon_buff[bytes_recv_from_cli] = 0;
		return SUCCESS;
	}
}


static int wifi_daemon_webcli_communication(  char *wifidaemon_webcli_buff,
		int  wifidaemon_webcli_buff_size,
		char *webcli_wifidaemon_buff,
		int  *webcli_wifidaemon_buff_size)
{
	//Socket FD for Webclient socket. Used to communicate with Webclient socket.
	int webcli_sockfd = 0;
	int ret = FAIL;

	//Send data to WEB_CLI socket
	if (wifi_daemon_send_to_webcli( wifidaemon_webcli_buff, wifidaemon_webcli_buff_size,
				webcli_wifidaemon_buff, webcli_wifidaemon_buff_size,
				&webcli_sockfd) == 0)
	{
		//Receive data from Webclient
		ret = wifi_daemon_recv_from_webcli(webcli_wifidaemon_buff, webcli_wifidaemon_buff_size,&webcli_sockfd);
		WIFI_LOG("======wifi_daemon_webcli_communication return[%d]\r\n", ret);
	}
	WIFI_LOG("wifi_daemon_webcli_communication: ========ret[%d]\r\n", ret);
	return ret;
}

static int wifi_daemon_get_key_value(char* str_buf, const char *key, char *value, int *value_len)
{
	char *offset;
	char *offset1;
	if(str_buf == NULL)
	{
		return FALSE;
	}

	offset = strstr(str_buf, key);
	if(offset == NULL)
	{
		return FALSE;
	}

	offset = strstr(offset, "\":\"");
	if(offset == NULL)
	{
		return FALSE;
	}

	offset += 3;

	offset1 = strstr(offset, "\",");//fix bug: when ssid include "("test", not test), can't parse the ssid
	if(offset1 == NULL)
	{
		return FALSE;
	}

	if(offset1 == offset)
	{
		return TRUE;
	}

	*value_len = offset1 - offset;	
	memcpy(value, offset, *value_len);

	return TRUE;

}

static void wifi_daemon_set_key_value(char * send_str, const char *key, const char *value)
{
	if(send_str == NULL)
	{
		return;
	}
	if(strlen(send_str) != 0)
	{
		strncat(send_str, "&",  1);
	}

	strncat(send_str, key, strlen(key));
	strncat(send_str, "=",  1);

	if(value != NULL)
	{
		strncat(send_str, value, strlen(value));
	}
}

//////////////////////////////////////////////////////////////////////////////
static int is_Hex_String(char *str, int len)
{
	int  i;
	char c;
	int  ret = TRUE;
	for(i = 0; i < len; i++)
	{
		c = *(str + i);
		if(   !( c >= 0x30 && c <= 0x39) 
				&&!( c >= 0x41 && c <= 0x46)
				&&!( c >= 0x61 && c <= 0x66) ) 
		{
			ret = FALSE;
			break;
		}
	}
	return ret;
}

static int is_ascii_key_string(char *str, int len)
{
	int i;
	char c;
	int ret = TRUE;
	for(i = 0; i < len; i++)
	{
		c = *(str + i);
		if( c < 0x20 || c > 0x7F)
		{
			ret = FALSE;
		}
	}

	return ret;
}

static unsigned char HexNumberToAscii(unsigned char value)
{
	unsigned char ret;
	if(value < 10){
		ret = value + 0x30; 
	}else if(value < 0x10){
		ret = value + 0x57;
	}else{
		ret = 0;
	}
	return ret;
}

static int check_wep_key_string(char *str, int len, int cur_wep_index, int loop)
{
	if(len != 0 && len != 5 && len != 10 && len != 13 && len != 26)
	{
		return FALSE;
	}

	if(len == 0)
	{
		if(cur_wep_index == loop)
		{
			return FALSE;
		}
	}
	else 
	{
		if(len == 5 || len == 13)  //ASCII
		{
			return is_ascii_key_string(str + 1, len - 2);
		}
		else   //hex digit
		{
			return is_Hex_String(str + 1, len - 2);
		}
	}

	return 	TRUE;	
}

static int check_valid_channel(const char *country_code, int mode, int channel)
{
	int i;

	if(channel == 0)
	{
		return TRUE;
	}

	/*current just check china channel*/
	if((mode == 1) || (mode ==5))         // a/n/ac
	{
		for(i = 0; i < sizeof(CN_5G_channel_list); i++)
		{
			if(	channel == 	CN_5G_channel_list[i])
			{
				return TRUE;
			}
		}
	}

	if(mode > 1 && mode <= 4)           // b  b/g   b/g/n
	{
		if(channel >= 1 && channel <=13)
		{
			return TRUE;
		}
	}

	return FALSE;
}

static boolean check_valid_apindex(int ap_index)
{
	wifi_mode_type mode;
	if(ap_index < AP_INDEX_0 || ap_index > AP_INDEX_STA)
	{
		return FALSE;
	}
	mode = get_ap_cfg();
	if(mode == AP_MODE)
	{
		if(ap_index != AP_INDEX_0)
		{
			return FALSE;
		}
	}
	else if(mode == APAP_MODE)
	{
		if(ap_index == AP_INDEX_STA)
		{
			return FALSE;
		}
	}
	else
	{
		if(ap_index != AP_INDEX_STA)
		{
			return FALSE;
		}       
	}
	return TRUE;
}


/*****************************************************************************
 * Function Name : wifi_init
 * Description   : 获取WIFI状态 FLAG 1：打开  0： 关闭
 * Input			: uint8_t *flag
 * Output        : None
 * Return        : 0:success, -1:failed
 * Auther        : qjh
 * Date          : 2018.04.11
 *****************************************************************************/

boolean get_wifi_status(uint8 *flag)
{
	FILE *fp = NULL; 

	fp = fopen("/data/mifi/cwmap.log", "r");
	if(fp == NULL)
	{
		printf("\nERROR: ====== fopen[/data/mifi/cwmap.log] failed\n");
		return FALSE;
	}
	else
	{
		fread(flag, 1, 1, fp);
		fclose(fp);
	}

	return TRUE;
}

//get net status
boolean get_net_status(char *net_enable_str)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	int value_len = 0;
	//char net_enable_str[sizeof(int) + 1] ={0};
	char mask_flag[sizeof(int) + 1] = {0};      // FIELD_MASK_1 : set   FIELD_MASK_2: get	

	WIFI_LOG("\n get_net_status: 1.]\n");

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	snprintf(mask_flag,sizeof(int),"%d",FIELD_MASK_2);  

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, net_status_string[NET_STATUS_PAGE], NETCNCT);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, net_status_string[NET_MASK], mask_flag);
	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									

	WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);
	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, net_status_string[NET_STATUS], net_enable_str, &value_len))
	{
		WIFI_LOG("\r\n==============\r\net_enable_str = %s\r\n===========\r\n", net_enable_str);
		if(value_len <= 0)
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			return FALSE;
		}
	}

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}

//get the user name and password that used in the authtica
static boolean wifi_daemon_read_usr_name_pwd(char *usr_name, int name_len, char *usr_pwd, int pwd_len)
{
	char *usr_name_pwd_file_name = "/data/mifi/usr_name_pwd.cfg";
	FILE *fp = NULL;

	char *get_ptr;

	boolean ret = TRUE;

	WIFI_LOG("\n wifi_daemon_read_usr_name_pwd[%s], [%d, %d] \n", usr_name_pwd_file_name, name_len, pwd_len);
	if(usr_name == NULL || usr_pwd == NULL)
	{
		printf("/n ERROR: usr_name or usr_pwd is NULL \n");
		return FALSE;
	}

	fp = fopen(usr_name_pwd_file_name, "r");
	if(fp == NULL)
	{
		printf(" /n ERROR: ====== fopen[%s] failed \n", usr_name_pwd_file_name);
		return FALSE;
	}

	do
	{
		get_ptr = fgets(usr_name, name_len, fp);
		if (!ferror(fp) && (get_ptr !=NULL))
		{
			usr_name[strlen(usr_name)-1]=0;
		}
		else
		{
			printf("\n 1. ERROR: ====== fread failed \n");
			ret = FALSE;
			break;
		}

		get_ptr = fgets(usr_pwd, pwd_len, fp);
		if (!ferror(fp) && (get_ptr !=NULL))
		{
			usr_pwd[strlen(usr_pwd)-1]=0;
		}
		else
		{
			printf("\n 2. ERROR: ====== fread failed\n");
			ret = FALSE;
			break;
		}
	}while(0);

	fclose(fp);
	return ret;
}

static boolean wifi_daemon_save_usr_name_pwd(char *usr_name, char *usr_pwd)
{
	char *usr_name_pwd_file_name = "/data/mifi/usr_name_pwd.cfg";
	FILE *fp = NULL;
	size_t read_size;

	char buf[130] = {0};

	boolean ret = TRUE;

	WIFI_LOG("\n wifi_daemon_save_usr_name_pwd[%s] \n", usr_name_pwd_file_name);
	if(usr_name == NULL || usr_pwd == NULL)
	{
		printf("\n usr_name or usr_pwd is NULL \n");
		return FALSE;
	}

	fp = fopen(usr_name_pwd_file_name, "w");
	if(fp == NULL)
	{
		printf("\n ERROR: ====== fopen[%s] failed \n", usr_name_pwd_file_name);
		return FALSE;
	}

	do
	{
		//sprintf(buf, "%s\n", usr_name);
		sprintf(buf, "%s", usr_name);
		read_size = fputs(buf/*usr_name*/, fp);
		WIFI_LOG("\n write user name[%d] \n", read_size);
		if (!ferror(fp) && (read_size > 0))
		{
			WIFI_LOG("\n write user name OK \n");
		}
		else
		{
			printf("\n ERROR: ====== fputs failed \n");
			ret = FALSE;
			break;
		}

		memset(buf, 0, sizeof(buf));
		//sprintf(buf, "%s\n", usr_pwd);
		sprintf(buf, "%s", usr_pwd);
		read_size = fputs(buf/*usr_pwd*/, fp);
		WIFI_LOG("\n write user pwd[%d] \n", read_size);
		if (!ferror(fp) && (read_size > 0))
		{
			WIFI_LOG("\n 2. write user pwd OK \n");
		}
		else
		{
			printf("\n ERROR: ====== fputs failed \n");
			ret = FALSE;
			break;
		}
	}while(0);

	fclose(fp);
	return ret;
}

//get the mac address
boolean get_mac_addr(char *mac_addr, ap_index_type ap_index)
{
	int  num = 0;
	int  interface_num = 0;
	char command[100]={0};
	char buf[100]={0};
	char tmp[30]={0};

	char *endstr; 
	FILE *fp;

	if(ap_index == AP_INDEX_STA)
	{
		interface_num = 1;  //wlan1
	}
	else if(ap_index == AP_INDEX_1)
	{
		interface_num = 1;  //wlan1
	}
	else if(ap_index == AP_INDEX_0)
	{
		interface_num = 0;  //wlan0
	}

	//AP mac
	sprintf(command, "ifconfig wlan%d | grep \"HWaddr\" | awk '{ print $5}'", interface_num);
	WIFI_LOG("command=%s",command);

	fp=popen(command,"r");
	if(fp == NULL)
	{
		printf("\n get_mac_addr: 1. popen error! \n");
		return FALSE;	
	}

	fgets(buf,sizeof(buf),fp); 
	pclose(fp);

	endstr = strchr(buf,'\n');
	if(endstr != NULL)
	{
		*endstr = 0;  
	}

	if(strlen(buf) == 0)
	{
		printf("\n get_mac_addr: 2. buf length is 0! \n");
		return FALSE;
	}

	sprintf(tmp, "%d,%s\r\n", num, buf);
	num ++;
	strcpy(mac_addr, tmp);
	WIFI_LOG("\nget_mac_addr: 1.num[%d]\n", num);

	//client mac
	sprintf(command, "hostapd_cli -i wlan%d all_sta | grep \"dot11RSNAStatsSTAAddress\" | awk -F= '{print $2}'", interface_num);
	WIFI_LOG("command=%s",command);

	fp=popen(command,"r");
	if(fp == NULL)
	{
		printf("\n get_mac_addr: 3. popen error! \n");
		return FALSE;    
	}

	while(!feof(fp))
	{
		memset(buf, 0, 100);
		fgets(buf,sizeof(buf),fp);

		endstr = strchr(buf,'\n');
		if(endstr != NULL)
		{
			*endstr = 0;  
		}

		if(strlen(buf) == 0)
		{
			break;
		}

		sprintf(mac_addr, "%d,%s\r\n", num, buf);
		num ++;
	} 
	pclose(fp);
	WIFI_LOG("\nget_mac_addr: 2.num[%d]\n", num);

	return TRUE;
}



static wifi_mode_type get_ap_cfg()
{
	FILE *fp = NULL;
	size_t read_size;
	char read_buf[2] = {0};
	wifi_mode_type mode = AP_MODE;

	//check wifi mode
	fp = fopen(MAP_CFG_FILE, "r");
	if(fp)
	{
		read_size = fread(read_buf,1,1,fp);
		if (!ferror(fp) && (read_size > 0))
		{
			if(read_buf[0] == 0x30)   // 0: AP 1: AP-AP 2:AP-STA
			{
				mode = AP_MODE;
			}
			else if(read_buf[0] == 0x31)   // 0: AP 1: AP-AP 2:AP-STA
			{
				mode = APAP_MODE;
			}
			else if(read_buf[0] == 0x32)   // 0: AP 1: AP-AP 2:AP-STA
			{
				mode = APSTA_MODE;
			}
		}
		fclose(fp);
	}
	//enable ssid2, configselect
	return mode; 	
}

/*======================
  set_ap_cfg 
NOTE: this api is not avaliable in W58L
=======================*/
static boolean set_ap_cfg(wifi_mode_type mode)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;
	char mask_flag[sizeof(int) + 1] = {0};      // FIELD_MASK_3 : set   FIELD_MASK_2: get	
	char wlan_mode_str[sizeof(int) + 1] = {0};
	char cfg_str[16]= {0};
	FILE *fp = NULL;
	size_t write_size;

	//write to mobileap_cfg.xml
	snprintf(mask_flag, sizeof(int),"%d", FIELD_MASK_3);  
	snprintf(wlan_mode_str, sizeof(int), "%d",(int)mode + 1);  
	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_PAGE], SETLAN_CONFIG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_GW_IP], "");
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_SUBNET_MASK], "");
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_DHCP_MODE], "");
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_DHCP_START_IP], "");
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_DHCP_END_IP], "");
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_LAN_AP_DHCP_LEASE_TIME], "");
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_WLAN_ENABLE_DISABLE], "");
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, lan_config_str[LAN_PAGE_WLAN_MODE], wlan_mode_str);

	WIFI_LOG("\nsend_str:%s len=%d\n", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, &webcli_wifidaemon_buff_size);  
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;               
	}                                           
	WIFI_LOG("\nrecv_str = %s\n", webcli_wifidaemon_buff);
	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);

	//write file MAP_CFG_FILE
	fp = fopen(MAP_CFG_FILE, "w");
	if(fp == NULL)
	{
		printf("\n set_ap_cfg ERROR: open [%s] file error", MAP_CFG_FILE);
		return FALSE;
	}
	sprintf(cfg_str,"%d",mode);

	write_size = fwrite(cfg_str,1,1,fp);
	if (ferror(fp) || (write_size <= 0))
	{
		printf("\n set_ap_cfg ERROR: write to [%s] file error", MAP_CFG_FILE);
		return FALSE;
	}
	fclose(fp);

	ds_system_call("sync",strlen("sync")); 

	return TRUE;
}



//restore the wifi settings
void restore_wifi()
{
	char command[100] = {0};
	char command1[100] = {0};

	sprintf(command, "%s", "cp /etc/default_mobileap_cfg.xml /data/mobileap_cfg.xml"); 
	ds_system_call(command, strlen(command));

	sprintf(command1,"%s", "cp /etc/default_hostapd.conf /data/mifi/hostapd.conf");
	ds_system_call(command1, strlen(command1));

	//for enablessid2: AP-AP mode
	sprintf(command1,"%s", "cp /etc/default_hostapd-wlan1.conf /data/mifi/hostapd-wlan1.conf");
	ds_system_call(command1, strlen(command1));

	//for enablessid2: AP-STA mode
	sprintf(command1,"%s", "cp /etc/default_sta_mode_hostapd.conf /data/mifi/sta_mode_hostapd.conf");
	ds_system_call(command1, strlen(command1));
	sprintf(command1,"%s", "cp /etc/default_wpa_supplicant.conf /data/mifi/wpa_supplicant.conf");
	ds_system_call(command1, strlen(command1));

	sprintf(command, "rm %s", MAP_CFG_FILE);
	ds_system_call(command, strlen(command));

	memset(command, 0, sizeof(command));
	sprintf(command, "rm %s", MAP_FIX_CFG_FILE);
	ds_system_call(command, strlen(command));

	ds_system_call("sync", strlen("sync"));		

	ds_system_call("shutdown -r now", strlen("shutdown -r now"));
}



/*****************************************************************************
 * Function Name : get_wifi_mode
 * Description   : 获取当前WIFI模式设置
 * Input         : None
 * Output        : 0：单AP模式     1：双AP模式     2: AP+STA模式
 * Return        : wifi_mode_type
 * Auther        : qjh
 * Date          : 2018.04.10
 *****************************************************************************/

wifi_mode_type get_wifi_mode()
{
	return get_ap_cfg();
}
/*****************************************************************************
 * Function Name : set_wifi_mode
 * Description   : 设置WIFI模式
 * Input         : 0：单AP模式     1：双AP模式     2: AP+STA模式
 * Output        : TRUE or FALSE
 * Return        : boolean
 * Auther        : qjh
 * Date          : 2018.04.10
 *****************************************************************************/

boolean set_wifi_mode(wifi_mode_type mode)
{
	return set_ap_cfg(mode);
}

int sys_mylog3(char *plog)
{
#if SYS_MYLOG
#define LOG_PATH	"/data/mylog"
	char buff[1024] = "";
	sprintf(buff, "== %s ==\n", plog);
	FILE *flogfd;
	flogfd = fopen(LOG_PATH, "a+");
	if(flogfd == NULL)
	{
		printf("filelog open failed\n");
		return -1;
	}

	fwrite(buff, 1, strlen(buff), flogfd);

	fclose(flogfd);
#endif
	return 0;
}

/*****************************************************************************
 * Function Name : wifi_power
 * Description   : WIFI开关
 * Input         : act 1： 打开      0： 关闭
 * Output        : None
 * Return        : int
 * Auther        : qjh
 * Date          : 2018.04.10
 *****************************************************************************/
boolean wifi_power(int act)
{  
	if(act){
		printf("wifi open\n");
		sys_mylog3("wifi_open\n");
	}
	else{
		printf("wifi close\n");
		sys_mylog3("wifi_close\n");

	}
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	char map_enable_str[sizeof(int) + 1] ={0};
	char mask_flag[sizeof(int) + 1] = {0};      // FIELD_MASK_1 : set   FIELD_MASK_2: get	

	if(act < 0 || act > 1)
	{
		printf("\n map_enable value invalid! \n");
		return FALSE;			
	}

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	snprintf(map_enable_str,sizeof(int),"%d",act);
	snprintf(mask_flag,sizeof(int),"%d",FIELD_MASK_1); 

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, map_status_string[MAP_STATUS_PAGE], SETMOBILEAP);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, map_status_string[MAP_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, map_status_string[MAP_STATUS], map_enable_str);
//	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));
	printf("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									
//	WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);
	printf("recv_str = %s", webcli_wifidaemon_buff);

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;

}


///////////////////////////////////////////////////////////////////////////////
//set the ssid
boolean set_ssid(char *SSID, ap_index_type ap_index)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	boolean is_chines = FALSE;
	int chines_cnt = 0;
	int  ssid_param_len;
	int  old_ssid_param_len;

	char ssid[65] = {0};
	char mask_flag[8] = {0};

	char ap_index_str[8] = {0};

	WIFI_LOG("\n1. set_ssid[%s], ap_index[%d]\n", SSID, ap_index);

	if(!check_valid_apindex(ap_index))
	{
		return FALSE;
	}  
	ssid_param_len = strlen(SSID);
	old_ssid_param_len = ssid_param_len;
	WIFI_LOG("\r\n1. ===========ssid_param_len[%d]\r\n", ssid_param_len);
	{
		int i = 1;
		while(i<old_ssid_param_len-2)
		{
			if(SSID[i]>=0xb0 && SSID[i]<=0xf7 && SSID[i+1]>=0xa1 && SSID[i+1]<=0xfe)
			{
				is_chines = TRUE;
				chines_cnt++;

				ssid_param_len--;
				i += 2;
			}
			else
			{
				i++;
			}
			WIFI_LOG("\r\n==============i=[%d], ssid_param_len[%d]\r\n", i, ssid_param_len);
		}
	}
	WIFI_LOG("\r\n======ssid_param_len[%d], is_chines[%d], chines_cnt[%d]\r\n", ssid_param_len, is_chines, chines_cnt);

	if(chines_cnt > 10)         /* Chines in ssid len 1~10 */
	{
		printf("\r\n1. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
		return FALSE;				
	}

	if(is_chines)
	{
		int max_len = chines_cnt + 2 + (10-chines_cnt)*2;
		WIFI_LOG("\r\n====max_len[%d]\r\n", max_len);
		if(ssid_param_len < 1 || ssid_param_len > max_len)         /* ssid len 1~22(include Chinese and not Chinese) */
		{	
			printf("\r\n2. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
			return FALSE;				
		}
	}
	else                
	{
		if(ssid_param_len < 1 || ssid_param_len > 32)         /* ssid len 1~32 */
		{	
			printf("\r\n3. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
			return FALSE;				
		}
	}

	ssid_param_len = old_ssid_param_len;

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	memset(ap_index_str, 0, sizeof(ap_index_str));
	sprintf(ap_index_str, "%d", ap_index);

	strcpy(ssid, SSID);
	WIFI_LOG("\r\nssid=%s\r\n",ssid);
	//fix bug: set & in ssid, can't write to the hostapd.conf
	{
		int i = 0;
		for(i=0; i<strlen(SSID); i++)
		{
			if(ssid[i] == '&')
			{
				ssid[i] = 8;//convert to BS (backspace, asscii code is 8)
			}
		}
	}

	snprintf(mask_flag,31,"%d",FIELD_MASK_1);   //ssid

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], SETHOSTAPDBASECFG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_SSID], ssid);
	printf("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									
	WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}

//get the ssid
boolean get_ssid(char *str_SSID, ap_index_type ap_index)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	int value_len = 0;
	char ssid[69] = {0};
	char mask_flag[32];

	char ap_index_str[8] = {0};

	WIFI_LOG("\n get_ssid: 1.ap_index[%d]\n", ap_index);
	printf("\n get_ssid: 1.ap_index[%d]\n", ap_index);
	if(!check_valid_apindex(ap_index))
	{
		printf("check apindex fail");
		return FALSE;
	}

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	memset(ap_index_str, 0, sizeof(ap_index_str));
	sprintf(ap_index_str, "%d", ap_index);

	snprintf(mask_flag,31,"%d",FIELD_MASK_1);   //ssid

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], GETHOSTAPDBASECFG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);
	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}	

	WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);
	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_SSID], ssid, &value_len))
	{
		WIFI_LOG("\r\n==============\r\nssid = %s\r\n===========\r\n", ssid);
		if(value_len > 0)
		{
			strcpy(str_SSID, ssid);
		}
		else
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			return FALSE;			
		}
	}

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}

//set the auth type and password when connect to the wifi
boolean set_auth(char *str_pwd, int auth_type, int  encrypt_mode, ap_index_type ap_index)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;
	char mask_flag[32] = {0};

	char ap_index_str[8] = {0};

	char auth_type_str[16] = {0};
	char encrypt_mode_str[16] = {0};

	char wpa_pw[129]  = {0};
	char wep_pw[4][55];// = {0};

	char cur_wep_index_str[8] = {0};
	int  cur_wep_index;
	int	 i;

	WIFI_LOG("\n1. set_auth[%d, %d, %d], ap_index[%d]\n", strlen(str_pwd), 
			auth_type, encrypt_mode,ap_index);

	if(!check_valid_apindex(ap_index))
	{
		return FALSE;
	}

	//auth type
	if(auth_type < AUTH_MIN || auth_type > AUTH_MAX)
	{
		printf("\nauth_type error!\n");
		return FALSE;;			
	}
	sprintf(auth_type_str, "%s", p_auth[auth_type]);
	WIFI_LOG("\n1. auth_type_str[%s]\n", auth_type_str);

	//
	if(auth_type >= AUTH_WPA)  // wpa 
	{
		if(encrypt_mode < E_TKIP || encrypt_mode > E_AES_TKIP)           // 		2: "TKIP", 2: "AES", 4: "AES-TKIP"
		{
			printf("\n1. encrypt_mode error\n");
			return FALSE;					
		}
	}
	else   		// WEP
	{
		if(encrypt_mode < E_NULL || encrypt_mode > E_WEP)   
		{
			printf("\n2. encrypt_mode error\n");
			return FALSE;					
		}

		/* share mode need set wep key */
		if(auth_type == AUTH_SHARE && encrypt_mode == E_NULL)
		{
			printf("\n3. encrypt_mode error\n");
			return FALSE;				
		}
	}
	sprintf(encrypt_mode_str, "%s", p_encrypt[encrypt_mode]);
	WIFI_LOG("\n1. encrypt_mode_str[%s]\n", encrypt_mode_str);

	//----------- password ---------------------------------------------
	if( auth_type >= AUTH_WPA )
	{
		int   len;
		char *pw_tmp;
		int i = 0;

		len = strlen(str_pwd);
		pw_tmp = str_pwd;
		WIFI_LOG("\n====str_pwd length[%d]\n", len);
		if(len < 8 || len > 64)         /* key len 8~64 */
		{
			printf("\npwd length error\n");
			return FALSE;				
		}

		for(i=0; i<len; i++)
		{
			if(str_pwd[i] < 32 || str_pwd[i] > 126)
			{
				printf("\nnot char in asscii code 32-126\n");
				return FALSE;
			}
		}

		if(len == 64)
		{
			if(is_Hex_String(pw_tmp, 64) == FALSE)
			{
				printf("\nnot hex string\n");
				return FALSE;						
			}
		}
		else
		{
			if(is_ascii_key_string(pw_tmp, len) == FALSE)
			{
				printf("\nnot ascii string\n");
				return FALSE;						
			}
		}

		for(i = 0; i < len; i++)
		{
			wpa_pw[i * 2]     = HexNumberToAscii(*(uint8*)(pw_tmp + i) / 0x10);
			wpa_pw[i * 2 + 1] = HexNumberToAscii(*(uint8*)(pw_tmp + i) % 0x10);
		}
	}
	else
	{
		int   len;
		char *pw_tmp;

		if(encrypt_mode == E_WEP)
		{
			const int wep_key_num = 1;

			cur_wep_index = 1;
			sprintf(cur_wep_index_str, "%d", cur_wep_index);

			for(i = 0; i < wep_key_num; i++)
			{
				int j;
				len = strlen(str_pwd);
				pw_tmp = str_pwd;

				WIFI_LOG("\n==== i = %d, cur_wep_index = %d, len = %d\n",i, cur_wep_index,len);
				WIFI_LOG("\npw_tmp = %s\n", pw_tmp);

				if(FALSE == check_wep_key_string(pw_tmp, len, cur_wep_index, (i + 1)))
				{
					printf("\nwep key error\n");
					return FALSE;					
				}

				if( len >=5 )
				{
					for(j = 0; j < len; j++)
					{
						wep_pw[i][j * 2] = HexNumberToAscii(*(uint8*)(pw_tmp + j) / 0x10);
						wep_pw[i][j * 2 + 1] = HexNumberToAscii(*(uint8*)(pw_tmp + j) % 0x10);
					}					
				}					
			}
		}
	}

	//////////////////////////
	/*for(i = 0; i < 4; i++)
	  {
	  memset(wep_pw[i], 0, sizeof(char)*55);
	  }*/

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	memset(ap_index_str, 0, sizeof(ap_index_str));
	sprintf(ap_index_str, "%d", ap_index);

	// ------------  mask  --------------------
	snprintf(mask_flag,31,"%d",FIELD_MASK_3);   // auth set

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], SETHOSTAPDBASECFG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_SSID], NULL);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_BROADCAST], NULL);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AUTH_TYPE], auth_type_str);

	WIFI_LOG("\r\n=====gggggggggggg encrypt_mode = %d====\r\n",encrypt_mode);

	if(encrypt_mode >= E_TKIP && encrypt_mode <= E_AES_TKIP)
	{
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PAIRWISE], p_encrypt[encrypt_mode]);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY_STATUS], NULL);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_DEFAULT_KEY], NULL);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PASSPHRASE], wpa_pw);
	}
	else
	{	
		WIFI_LOG("\r\n======hhhhhhhh 22: cur_wep_index_str[%s], wep_pw[0]=[%s]\r\n", cur_wep_index_str, wep_pw[0]);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PAIRWISE], NULL);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY_STATUS], p_encrypt[encrypt_mode]);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_DEFAULT_KEY], cur_wep_index_str);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PASSPHRASE], NULL);
		if(encrypt_mode == E_WEP)
		{
			wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY0], wep_pw[0]);
			wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY1], wep_pw[1]);
			wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY2], wep_pw[2]);
			wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY3], wep_pw[3]);
		}
	}
	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									
	WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}


boolean set_ssid_and_auth(char *SSID, char *str_pwd, int auth_type, int  encrypt_mode, ap_index_type ap_index)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;
	boolean is_chines = FALSE;
	int chines_cnt = 0;
	int  ssid_param_len;
	int  old_ssid_param_len;

	char ssid[65] = {0};
	//    char mask_flag[8] = {0};
	char ap_index_str[8] = {0};
	char mask_flag[32] = {0};
	//    char ap_index_str[8] = {0};  
	char auth_type_str[16] = {0};
	char encrypt_mode_str[16] = {0};

	char wpa_pw[129]  = {0};
	char wep_pw[4][55];// = {0};

	char cur_wep_index_str[8] = {0};
	int  cur_wep_index;
	int	 i;	


	WIFI_LOG("\n1. set_ssid[%s], ap_index[%d]\n", SSID, ap_index);

	if(!check_valid_apindex(ap_index))
	{
		return FALSE;
	}  
	ssid_param_len = strlen(SSID);
	old_ssid_param_len = ssid_param_len;
	WIFI_LOG("\r\n1. ===========ssid_param_len[%d]\r\n", ssid_param_len);
	{
		int i = 1;
		while(i<old_ssid_param_len-2)
		{
			if(SSID[i]>=0xb0 && SSID[i]<=0xf7 && SSID[i+1]>=0xa1 && SSID[i+1]<=0xfe)
			{
				is_chines = TRUE;
				chines_cnt++;

				ssid_param_len--;
				i += 2;
			}
			else
			{
				i++;
			}
			WIFI_LOG("\r\n==============i=[%d], ssid_param_len[%d]\r\n", i, ssid_param_len);
		}
	}
	WIFI_LOG("\r\n======ssid_param_len[%d], is_chines[%d], chines_cnt[%d]\r\n", ssid_param_len, is_chines, chines_cnt);

	if(chines_cnt > 10)         /* Chines in ssid len 1~10 */
	{
		printf("\r\n1. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
		return FALSE;				
	}

	if(is_chines)
	{
		int max_len = chines_cnt + 2 + (10-chines_cnt)*2;
		WIFI_LOG("\r\n====max_len[%d]\r\n", max_len);
		if(ssid_param_len < 1 || ssid_param_len > max_len)         /* ssid len 1~22(include Chinese and not Chinese) */
		{	
			printf("\r\n2. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
			return FALSE;				
		}
	}
	else                
	{
		if(ssid_param_len < 1 || ssid_param_len > 32)         /* ssid len 1~32 */
		{	
			printf("\r\n3. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
			return FALSE;				
		}
	}

	ssid_param_len = old_ssid_param_len;

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	memset(ap_index_str, 0, sizeof(ap_index_str));
	sprintf(ap_index_str, "%d", ap_index);

	strcpy(ssid, SSID);
	WIFI_LOG("\r\nssid=%s\r\n",ssid);
	//fix bug: set & in ssid, can't write to the hostapd.conf
	{
		int i = 0;
		for(i=0; i<strlen(SSID); i++)
		{
			if(ssid[i] == '&')
			{
				ssid[i] = 8;//convert to BS (backspace, asscii code is 8)
			}
		}
	}

	snprintf(mask_flag,31,"%d",FIELD_MASK_1 | FIELD_MASK_3);   //ssid and auth



	WIFI_LOG("\n1. set_auth[%d, %d, %d], ap_index[%d]\n", strlen(str_pwd), 
			auth_type, encrypt_mode,ap_index);

	//auth type
	if(auth_type < AUTH_MIN || auth_type > AUTH_MAX)
	{
		printf("\nauth_type error!\n");
		return FALSE;;			
	}
	sprintf(auth_type_str, "%s", p_auth[auth_type]);
	WIFI_LOG("\n1. auth_type_str[%s]\n", auth_type_str);

	//
	if(auth_type >= AUTH_WPA)  // wpa 
	{
		if(encrypt_mode < E_TKIP || encrypt_mode > E_AES_TKIP)           // 		2: "TKIP", 2: "AES", 4: "AES-TKIP"
		{
			printf("\n1. encrypt_mode error\n");
			return FALSE;					
		}
	}
	else   		// WEP
	{
		if(encrypt_mode < E_NULL || encrypt_mode > E_WEP)   
		{
			printf("\n2. encrypt_mode error\n");
			return FALSE;					
		}

		/* share mode need set wep key */
		if(auth_type == AUTH_SHARE && encrypt_mode == E_NULL)
		{
			printf("\n3. encrypt_mode error\n");
			return FALSE;				
		}
	}
	sprintf(encrypt_mode_str, "%s", p_encrypt[encrypt_mode]);
	WIFI_LOG("\n1. encrypt_mode_str[%s]\n", encrypt_mode_str);

	//----------- password ---------------------------------------------
	if( auth_type >= AUTH_WPA )
	{
		int   len;
		char *pw_tmp;
		int i = 0;

		len = strlen(str_pwd);
		pw_tmp = str_pwd;
		WIFI_LOG("\n====str_pwd length[%d]\n", len);
		if(len < 8 || len > 64)         /* key len 8~64 */
		{
			printf("\npwd length error\n");
			return FALSE;				
		}

		for(i=0; i<len; i++)
		{
			if(str_pwd[i] < 32 || str_pwd[i] > 126)
			{
				printf("\nnot char in asscii code 32-126\n");
				return FALSE;
			}
		}

		if(len == 64)
		{
			if(is_Hex_String(pw_tmp, 64) == FALSE)
			{
				printf("\nnot hex string\n");
				return FALSE;						
			}
		}
		else
		{
			if(is_ascii_key_string(pw_tmp, len) == FALSE)
			{
				printf("\nnot ascii string\n");
				return FALSE;						
			}
		}

		for(i = 0; i < len; i++)
		{
			wpa_pw[i * 2]     = HexNumberToAscii(*(uint8*)(pw_tmp + i) / 0x10);
			wpa_pw[i * 2 + 1] = HexNumberToAscii(*(uint8*)(pw_tmp + i) % 0x10);
		}
	}
	else
	{
		int   len;
		char *pw_tmp;

		if(encrypt_mode == E_WEP)
		{
			const int wep_key_num = 1;

			cur_wep_index = 1;
			sprintf(cur_wep_index_str, "%d", cur_wep_index);

			for(i = 0; i < wep_key_num; i++)
			{
				int j;
				len = strlen(str_pwd);
				pw_tmp = str_pwd;

				WIFI_LOG("\n==== i = %d, cur_wep_index = %d, len = %d\n",i, cur_wep_index,len);
				WIFI_LOG("\npw_tmp = %s\n", pw_tmp);

				if(FALSE == check_wep_key_string(pw_tmp, len, cur_wep_index, (i + 1)))
				{
					printf("\nwep key error\n");
					return FALSE;					
				}

				if( len >=5 )
				{
					for(j = 0; j < len; j++)
					{
						wep_pw[i][j * 2] = HexNumberToAscii(*(uint8*)(pw_tmp + j) / 0x10);
						wep_pw[i][j * 2 + 1] = HexNumberToAscii(*(uint8*)(pw_tmp + j) % 0x10);
					}					
				}					
			}
		}
	}

	//////////////////////////
	for(i = 0; i < 4; i++)
	{
		memset(wep_pw[i], 0, sizeof(char)*55);
	}


	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], SETHOSTAPDBASECFG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_SSID], ssid);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_BROADCAST], NULL);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AUTH_TYPE], auth_type_str);

	WIFI_LOG("\r\n=====gggggggggggg encrypt_mode = %d====\r\n",encrypt_mode);

	if(encrypt_mode >= E_TKIP && encrypt_mode <= E_AES_TKIP)
	{
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PAIRWISE], p_encrypt[encrypt_mode]);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY_STATUS], NULL);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_DEFAULT_KEY], NULL);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PASSPHRASE], wpa_pw);
	}
	else
	{	
		WIFI_LOG("\r\n======hhhhhhhh 22: cur_wep_index_str[%s], wep_pw[0]=[%s]\r\n", cur_wep_index_str, wep_pw[0]);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PAIRWISE], NULL);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY_STATUS], p_encrypt[encrypt_mode]);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_DEFAULT_KEY], cur_wep_index_str);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PASSPHRASE], NULL);
		if(encrypt_mode == E_WEP)
		{
			wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY0], wep_pw[0]);
			wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY1], wep_pw[1]);
			wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY2], wep_pw[2]);
			wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY3], wep_pw[3]);
		}
	}
	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									
	WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);

	return TRUE;

}


//get the auth type and password when connect to the wifi
boolean get_auth(int *auth_type_ptr, int *encrypt_mode_ptr, char *pwd_str, ap_index_type ap_index)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	int value_len = 0;
	char mask_flag[32] = {0};

	char auth_type_str[16] = {0};
	int auth_type = 0;
	char encrypt_mode_str[16] = {0};
	int encrypt_mode = 0;

	char cur_wep_index_str[8] = {0};
	int cur_wep_index = 0;

	char ap_index_str[8] = {0};

	WIFI_LOG("\n get_auth: 1.ap_index[%d]\n", ap_index);

	if(!check_valid_apindex(ap_index))
	{
		return FALSE;
	}

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	memset(ap_index_str, 0, sizeof(ap_index_str));
	sprintf(ap_index_str, "%d", ap_index);

	snprintf(mask_flag,31,"%d",FIELD_MASK_3);   //auth		

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], GETHOSTAPDBASECFG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);

	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, &webcli_wifidaemon_buff_size);

	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}		
	WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

	// ----------------------  authentication ------------------------------------
	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AUTH_TYPE], auth_type_str, &value_len))
	{
		WIFI_LOG("\r\n======sizeof p_auth = %d, auth_type_str[%s]\r\n",sizeof(p_auth) /sizeof(char *), auth_type_str);
		if(value_len <= 0)
		{
			printf("\n1. ERROR: value_len <= 0\n");
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			return FALSE;					
		}

		for(auth_type = 0; auth_type < (sizeof(p_auth) / sizeof(char *)); auth_type++)
		{
			if(strncmp(auth_type_str, p_auth[auth_type], strlen(p_auth[auth_type])) == 0)
			{
				break;
			}
		}
	}

	// -------------------- encrypt mode -------------------------------------------
	// 0: NULL 1: WEP  2: TKIP  3: AES  4: TKIP-AES  
	if(auth_type >= AUTH_WPA)
	{
		if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PAIRWISE], encrypt_mode_str, &value_len))
		{
			if(value_len <= 0)
			{
				printf("\n2. ERROR: value_len <= 0\n");
				wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
				return FALSE;					
			}
		}
	}
	else
	{
		if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY_STATUS], encrypt_mode_str, &value_len))
		{
			if(value_len <= 0)
			{
				printf("\n3. ERROR: value_len <= 0\n");
				wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
				return FALSE;					
			}
		}			
	}

	for(encrypt_mode = sizeof(p_encrypt) / sizeof(char *) - 1; encrypt_mode >= 0; encrypt_mode--)
	{
		if(strncmp(encrypt_mode_str, p_encrypt[encrypt_mode], strlen(p_encrypt[encrypt_mode])) == 0)
		{
			break;
		}
	}	

	*auth_type_ptr = auth_type;
	*encrypt_mode_ptr = encrypt_mode;
	// -------------------------- password ------------------------------------------------------------
	WIFI_LOG("\r\n=======auth_type[%d], encrypt_mode[%d]======\r\n", auth_type, encrypt_mode);
	if(auth_type >= AUTH_WPA)
	{
		if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WPA_PASSPHRASE], pwd_str, &value_len))
		{
			if(value_len <= 0)
			{
				printf("\n4.ERROR: value_len <= 0\n");
				wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
				return FALSE;					
			}
		}
		//printf("\nwpa_pw[%s]\n", pwd_str);				
	}
	else
	{
		char wep_pw[4][55];
		WIFI_LOG("\n=========encrypt_mode is E_WEP===========\n");
		if(encrypt_mode == E_WEP)       // WEP 
		{
			if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_DEFAULT_KEY], cur_wep_index_str, &value_len))
			{
				if(value_len <= 0)
				{
					printf("\n5. ERROR: value_len <= 0\n");
					wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
					return FALSE;					
				}
				cur_wep_index = atoi(cur_wep_index_str);
			}

#ifdef WEP_KEY0_ONLY
			if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY0], wep_pw[0], &value_len))
			{
				if(value_len <= 0)
				{
					printf("\n6. ERROR: value_len <= 0\n");
					wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
					return FALSE;					
				}
			}
			strcpy(pwd_str, wep_pw[0]);
#else

			for(i = 0; i < 4; i++)
			{
				if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_WEP_KEY0 + i], wep_pw[i], &value_len))
				{
					if(value_len <= 0)
					{
						printf("\n7. ERROR: value_len <= 0\n");
						wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
						return FALSE;					
					}
				}
			}			
			sprintf(pwd_str, ",%d,\"%s\",\"%s\",\"%s\",\"%s\"\n", cur_wep_index,wep_pw[0],wep_pw[1],wep_pw[2],wep_pw[3]);
#endif
		}
		else //no password
		{
			sprintf(pwd_str, "\n");
		}
	}

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}

//broadcast setting: 0-disable; 1-enable
boolean set_bcast(int broadcast, ap_index_type ap_index)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	char mask_flag[8] = {0};

	char ap_index_str[8] = {0};

	WIFI_LOG("\n1. set_bcast[%d], ap_index[%d]\n", broadcast, ap_index);

	if(!check_valid_apindex(ap_index))
	{
		return FALSE;
	}

	if(broadcast < 0 || broadcast > 1)
	{
		printf("\nERROR: broadcast error!\n");
		return FALSE;			
	}

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	memset(ap_index_str, 0, sizeof(ap_index_str));
	sprintf(ap_index_str, "%d", ap_index);

	snprintf(mask_flag,31,"%d",FIELD_MASK_2);   //broadcast

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], SETHOSTAPDBASECFG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_SSID], NULL);
	if( broadcast )
	{
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_BROADCAST], "enabled");
	}
	else
	{
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_BROADCAST], "disabled");
	}
	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									
	WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}

//get broadcast setting: 0-disable; 1-enable
boolean get_bcast(int *broadcast,ap_index_type ap_index)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	int value_len = 0;
	char broadcast_str[64] = {0};
	char mask_flag[32];

	char ap_index_str[8] = {0};

	WIFI_LOG("\n get_bcast: 1.ap_index[%d]\n", ap_index);

	if(!check_valid_apindex(ap_index))
	{
		return FALSE;
	}

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	memset(ap_index_str, 0, sizeof(ap_index_str));
	sprintf(ap_index_str, "%d", ap_index);

	snprintf(mask_flag,31,"%d",FIELD_MASK_2);   //broadcast	

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_PAGE], GETHOSTAPDBASECFG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_AP_INDEX], ap_index_str);
	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									


	WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);

	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_basecfg_str[HOSTAPD_BASECFG_PAGE_BROADCAST], broadcast_str, &value_len))
	{
		printf("\r\n==============\r\nbroadcast_str = %s\r\n===========\r\n", broadcast_str);
		if(value_len <= 0)
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			return FALSE;
		}

		if(strncmp(broadcast_str, "enabled", strlen("enabled")) == 0)
		{
			*broadcast = 1;
		}
		else
		{
			*broadcast = 0;
		}
		WIFI_LOG("\n*broadcast[%d]\n", *broadcast);
	}

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}


/*****************************************************************************
 * Function Name : set_channel_mode
 * Description	: 设置wifi channel 和 hw mode
 * Input 		: channel：0（自动）、1~13等（channel_number数组里面）
 ：hw mode：1：a 2：b 3：g 4：n 5：c  
 * Output		: None
 * Return		: boolean
 * Auther		: qjh
 * Date			: 2018.04.11
 * Note          ：不推荐二次开发用户使用这个接口，一般来说把wifi 默认在channel=0
 *****************************************************************************/	

static boolean set_channel_mode(int channel, int mode,ap_index_type ap_index)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;
	int i = 0;

	char mask_flag[32] = {0};
	char channel_str[8] = {0};	
	char 	hw_mode_str[8] = {0};

	char ap_index_str[8] = {0};

	WIFI_LOG("\n1. set_channel_mode[%d], ap_index[%d]\n", channel, ap_index);
	if(!check_valid_apindex(ap_index))
	{
		return FALSE;
	}

	for(i = 0; i < sizeof(channel_number) / sizeof(int); i++)
	{
		if(channel_number[i] == channel)
		{
			break;		
		}
	}

	if(i >= sizeof(channel_number) / sizeof(int))
	{
		printf("\n ERROR: 1. channel error!\n");
		return FALSE;			
	}

	if(mode < 1 || mode > 5)
	{
		printf("\n ERROR: 2. mode error!\n");
		return FALSE;			
	}

	if((mode == 1) || (mode == 5))
	{
		if(channel != 149 && channel != 153 && channel != 157 && channel != 161 && channel != 165)
		{
			printf("\n ERROR: 3. channel error!\n");
			return FALSE;	
		}
	}
	else
	{
		if(channel < 0 || channel > 11)
		{
			printf("\n ERROR: 4. channel error!\n");
			return FALSE;			
		}
	}

	switch( mode )
	{
		case 1:
			hw_mode_str[0] = 'a';
			break;

		case 2:
			hw_mode_str[0] = 'b';
			break;	

		case 3:
			hw_mode_str[0] = 'g';
			break;	

		case 4:
			hw_mode_str[0] = 'n';
			break;

		case 5:
			hw_mode_str[0] = 'c';
			break;	

		default:
			hw_mode_str[0] = 'n';
			break;
	}

	if(check_valid_channel("CN", mode, channel) == FALSE)
	{
		printf("\n ERROR: 5. channel error!\n");
		return FALSE;			
	}

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	memset(ap_index_str, 0, sizeof(ap_index_str));
	sprintf(ap_index_str, "%d", ap_index);

	snprintf(mask_flag,31,"%d",FIELD_MASK_3 | FIELD_MASK_2);   //hw_mode and channel

	sprintf(channel_str, "%d", channel);

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_PAGE], SETHOSTAPDEXTCFG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_AP_INDEX], ap_index_str);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_COUNTRY], NULL);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_CHANNEL], channel_str);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_HW_MODE], hw_mode_str);

	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));
	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									
	WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}


/*****************************************************************************
 * Function Name : get_channel_mode
 * Description	: 获取wifi channel 和 hw mode
 * Input 		: ap_index
 * Output		: channel：0（自动）、1~13等（channel_number数组里面）
 ：hw mode：1：a 2：b 3：g 4：n 5：c 
 * Return		: boolean
 * Auther		: qjh
 * Date			: 2018.04.11
 * Note          ：不推荐二次开发用户使用这个接口，一般来说把wifi 默认在channel=0
 ：hw_mode=n  头文件隐藏这个接口
 *****************************************************************************/	
static boolean get_channel_mode(char *channel_str, int *mode,ap_index_type ap_index)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	int value_len = 0;
	char mask_flag[32];
	char 	hw_mode_str[8] = {0};

	char ap_index_str[8] = {0};

	WIFI_LOG("\n get_channel_mode: 1.ap_index[%d]\n", ap_index);
	if(!check_valid_apindex(ap_index))
	{
		return FALSE;
	}

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	memset(ap_index_str, 0, sizeof(ap_index_str));
	sprintf(ap_index_str, "%d", ap_index);

	snprintf(mask_flag,31,"%d",FIELD_MASK_3 | FIELD_MASK_2);   //hw_mode and channel

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_PAGE], GETHOSTAPDEXTCFG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_AP_INDEX], ap_index_str);
	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									


	WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);
	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_HW_MODE], hw_mode_str, &value_len))
	{
		if(value_len <= 0)
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			return FALSE;
		}

		if(value_len > 0)
		{
			if(strncmp(hw_mode_str, "n", 1) == 0)
			{
				*mode = 4;
			}
			else if(strncmp(hw_mode_str, "g", 1) == 0)
			{
				*mode = 3;
			}
			else if(strncmp(hw_mode_str, "b", 1) == 0)
			{
				*mode = 2;
			}
			else if(strncmp(hw_mode_str, "a", 1) == 0)
			{
				*mode = 1;
			}				
		}
	}

	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, hostapd_extcfg_str[HOSTAPD_EXTCFG_PAGE_CHANNEL], channel_str, &value_len))
	{
		if(value_len <= 0)
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			return FALSE;
		}
	}

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}

//get the current DHCP configuration
boolean get_dhcp(char *host_ip_str, char *start_ip_str, char *end_ip_str, char *time_str)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	int value_len = 0;

	WIFI_LOG("\n get_dhcp: 1.\n");

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, dhcp_config_str[DHCP_PAGE_PAGE], GETDHCP_CONFIG);
	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									


	WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);

	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, dhcp_config_str[DHCP_PAGE_GW_IP], host_ip_str, &value_len))
	{
		if(value_len <= 0)
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			printf("\n 1. host_ip_str error!\n");
			return FALSE;
		}
	}

	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, dhcp_config_str[DHCP_PAGE_DHCP_START_IP], start_ip_str, &value_len))
	{
		if(value_len <= 0)
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			printf("\n 2. start_ip_str error!\n");
			return FALSE;
		}
	}

	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, dhcp_config_str[DHCP_PAGE_DHCP_END_IP], end_ip_str, &value_len))
	{
		if(value_len <= 0)
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			printf("\n 3. end_ip_str error!\n");
			return FALSE;
		}
	}

	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, dhcp_config_str[DHCP_PAGE_DHCP_LEASE_TIME], time_str, &value_len))
	{
		if(value_len <= 0)
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			printf("\n 4. time_str error!\n");
			return FALSE;
		}
	}

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}

//get the client number that connected to the wifi
int get_client_count(ap_index_type ap_index)
{
	int	client_num;
	int	interface_num = 0;
	char command[100]={0};
	char buf[16];
	char *endstr; 
	FILE *fp;

	if(!check_valid_apindex(ap_index))
	{
		printf("invalid index\n");
		return FALSE;
	}

	if(ap_index == AP_INDEX_STA)
	{
		interface_num = 1;  //wlan1
	}
	else if(ap_index == AP_INDEX_1)
	{
		interface_num = 1;  //wlan1
	}
	else if(ap_index == AP_INDEX_0)
	{
		interface_num = 0;  //wlan0
	}

	//sprintf(command, "echo `hostapd_cli -i wlan%d all_sta | awk -v RS='AUTHORIZED' 'END {print --NR}'`", interface_num);
	sprintf(command, "hostapd_cli -i wlan%d all_sta | awk -v RS='AUTHORIZED' 'END {print --NR}'", interface_num);
	//WIFI_LOG("command=%s",command);

	//printf("cmd: %s\n", command);
	fp=popen(command,"r");
	if(fp == NULL)
	{
		printf("\n popen error!! \n");
		return 0;	
	}

	fgets(buf,sizeof(buf),fp);
	//printf("buf %s\n",buf);
	pclose(fp); 

	endstr = strchr(buf,'\n');
	if(endstr != NULL)
	{
		*endstr = 0;  
	}

	client_num = atoi(buf);
	if(client_num < 0)    //at+cwmap=0
		client_num = 0;

	return client_num;
}

boolean get_sta_ip(char *ip_str, int len)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	char   sta_ip_str[30] = {0};
	int value_len = 0;
	char tmp[30]={0};

	char mask_flag[32];

	/*if(get_ap_cfg() != APSTA_MODE)
	  {
	  return FALSE;
	  }*/

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	snprintf(mask_flag,31,"%d",FIELD_MASK_1);   

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, sta_ip_string[STA_IP_PAGE], GETSTAIP);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, sta_ip_string[STA_IP_MASK], mask_flag);
	WIFI_LOG("====\nsend_str:%s len=%d\n", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(   wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}

	WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);
	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, sta_ip_string[STA_IP_ADDR], sta_ip_str, &value_len))
	{
		if(value_len < 0)
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			return FALSE;			
		}
	}

	WIFI_LOG("\n====sta_ip_str[%s]====\n", sta_ip_str);

	strncpy(ip_str, sta_ip_str, len);
	return TRUE;
}

/*****************************************************************************
 * Function Name : set_sta_cfg
 * Description   : 设置STA连接外部AP的 SSID 和密码
 * Input         : char *SSID, char *psk_value（密码）
 * Output        : None
 * Return        : boolean
 * Auther        : qjh
 * Date          : 2018.04.11
 *****************************************************************************/
boolean set_sta_cfg(char *ssid_str, char *psk_value)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	boolean is_chines = FALSE;
	int chines_cnt = 0;
	int  ssid_param_len;
	int  old_ssid_param_len;

	char ssid[67] = {0};
	char mask_flag[8] = {0};

	char ap_index_str[8] = {0};

	//printf("\n1. set_sta_cfg[%s],\n", ssid_str);
	/*if(get_ap_cfg() != APSTA_MODE)
	  {
	  return FALSE;
	  }*/

	ssid_param_len = strlen(ssid_str);
	if(ssid_param_len < 1)
	{
		return FALSE;
	}
	old_ssid_param_len = ssid_param_len;

	WIFI_LOG("\r\n1. ===========ssid_param_len[%d]\r\n", ssid_param_len);
	{
		int i = 0;
		while(i<old_ssid_param_len-2)
		{
			WIFI_LOG("\r\n========[%0x, %0x]\r\n", ssid_str[i], ssid_str[i+1]);
			if(ssid_str[i]>=0xb0 && ssid_str[i]<=0xf7 && ssid_str[i+1]>=0xa1 && ssid_str[i+1]<=0xfe)
			{
				is_chines = TRUE;
				chines_cnt++;

				ssid_param_len--;
				i += 2;
			}
			else
			{
				i++;
			}
			WIFI_LOG("\r\n==============i=[%d], ssid_param_len[%d]\r\n", i, ssid_param_len);
		}
	}

	WIFI_LOG("\r\n======ssid_param_len[%d], is_chines[%d]\r\n", ssid_param_len, is_chines);
	if(chines_cnt > 10)         /* Chines in ssid len 1~10 */
	{
		printf("\r\n1. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
		return FALSE;				
	}

	if(is_chines)
	{
		int max_len = chines_cnt + 2 + (10-chines_cnt)*2;
		WIFI_LOG("\r\n====max_len[%d]\r\n", max_len);
		if(ssid_param_len < 1 || ssid_param_len > max_len)         /* ssid len 1~22(include Chinese and not Chinese) */
		{	
			printf("\r\n2. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
			return FALSE;				
		}
	}
	else
	{
		if(ssid_param_len < 1 || ssid_param_len > 32)         /* ssid len 1~32 */
		{	
			printf("\r\n3. Error: ssid_param_len[%d] error\r\n", ssid_param_len);
			return FALSE;				
		}
	}

	ssid_param_len = old_ssid_param_len;

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	memset(ap_index_str, 0, sizeof(ap_index_str));
	sprintf(ap_index_str, "%d", 2);

	strcpy(ssid, ssid_str);
	WIFI_LOG("\r\nssid=%s\r\n",ssid);
	//fix bug: set & in ssid, can't write to the hostapd.conf
	{
		int i = 1;
		for(i; i<strlen(ssid_str); i++)
		{
			if(ssid[i] == '&')
			{
				ssid[i] = 8;//convert to BS (backspace, asscii code is 8)
			}
		}
	}

	snprintf(mask_flag,31,"%d",FIELD_MASK_1);   //ssid

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_PAGE], SETWPASUPPLICANTCFG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_AP_INDEX], ap_index_str);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_SSID], ssid);

	if(psk_value != NULL && strlen(psk_value) >= 5 &&  strlen(psk_value) <= 64)
	{ 
		char sim_psk_value[67] = {0};
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_WEP_KEY_STATUS], NULL);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_AUTH_TYPE], NULL);

		printf("psk_value = %s\n", psk_value);
#if 0
		sim_psk_value[0] = '\"';
		strcpy(sim_psk_value+1, psk_value);
		sim_psk_value[strlen(psk_value) + 1] = '\"';
#else
		strcpy(sim_psk_value, psk_value);
#endif

		printf("sim_psk_value = %s\n", sim_psk_value);
		wifi_daemon_set_key_value(wifidaemon_webcli_buff, "basecfg_wpa_passphrase"/*wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_WPA_PASSPHRASE]*/, sim_psk_value);
	}

	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									
	WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}

/*****************************************************************************
 * Function Name : get_sta_cfg
 * Description   : 获取STA设置的SSID 和 密码
 * Input         : char *SSID, char *psk_value（密码）
 * Output        : None
 * Return        : boolean
 * Auther        : qjh
 * Date          : 2018.04.11
 *****************************************************************************/
boolean get_sta_cfg(char *ssid_str, char *psk_value)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	int value_len = 0;
	char vptr[69] = {0};
	char mask_flag[32];

	char ap_index_str[8] = {0};

	/*if(get_ap_cfg() != APSTA_MODE)
	  {
	  return FALSE;
	  }*/

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	memset(ap_index_str, 0, sizeof(ap_index_str));
	sprintf(ap_index_str, "%d", AP_INDEX_STA);

	snprintf(mask_flag,31,"%d",FIELD_MASK_1);   //ssid

	wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_PAGE], GETWPASUPPLICANTCFG);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_AP_INDEX], ap_index_str);
	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									


	WIFI_LOG("\r\n==============\r\nrecv_str = %s\r\n===========\r\n", webcli_wifidaemon_buff);
	//ssid
	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_SSID], vptr, &value_len))
	{
		WIFI_LOG("\r\n==============\r\nssid = %s\r\n===========\r\n", vptr);
		if(value_len > 0)
		{
			strcpy(ssid_str, vptr);
		}
		else
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			return FALSE;				
		}
	}

	//psk
	memset(vptr, 0, sizeof(vptr));
	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, "basecfg_wpa_passphrase"/*wpa_supplicant_cfg_str[WPA_SUPPLICANT_CFG_PAGE_WPA_PASSPHRASE]*/, vptr, &value_len))
	{
		WIFI_LOG("\r\n==============\r\npsk = %s\r\n===========\r\n", vptr);
		if(value_len > 0)
		{
			strcpy(psk_value, vptr);
		}
		else
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			return FALSE;				
		}
	}

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}


boolean sta_scan(char *list_str)
{
	char command[100]={0};
	char buf[100]={0};
	//char tmp[100]={0};
	FILE *fp;
	char *endstr;
	int i,count = 0;

	/*if(get_ap_cfg() != APSTA_MODE)
	  {
	  return FALSE;
	  }*/

	//command
	system("wpa_cli scan");
	sprintf(command, "wpa_cli scan_result > %s", SCAN_TEMP_FILE);
	system(command);

	//count
	memset(command, 0, sizeof(command));
	sprintf(command, "wc -l %s | awk '{ print $1}'", SCAN_TEMP_FILE);

	fp=popen(command,"r");

	if(fp == NULL)
	{
		printf("\n 1. sta_scan ERROR: popen faile \n");
		return FALSE;	
	}

	fgets(buf,sizeof(buf),fp); 
	pclose(fp);

	count = atoi(buf);
	if(count < 3)
	{
		printf("\n 2. sta_scan ERROR: count[%d] is less than 3\n", count);
		return FALSE;
	}

	strcpy(list_str, "\r\n");

	for(i=3; i<=count; i++)
	{
		//bssid
		memset(command, 0, sizeof(command));
		sprintf(command, "sed -n \"%d,%dp\" %s | awk '{ print $1}'", i, i, SCAN_TEMP_FILE);

		fp=popen(command,"r");

		if(fp == NULL)
		{
			printf("\n 3. sta_scan ERROR: popen faile \n");
			return FALSE;	
		}

		memset(buf, 0, sizeof(buf));
		fgets(buf,sizeof(buf),fp); 
		pclose(fp);
		endstr = strchr(buf,'\n');
		if(endstr != NULL)
		{
			*endstr = 0;  
		}			

		strcat(list_str, buf);
		strcat(list_str, ",");

		//ssid
		memset(command, 0, sizeof(command));
		sprintf(command, "sed -n \"%d,%dp\" %s | awk '{ print $5}'", i, i, SCAN_TEMP_FILE);

		fp=popen(command,"r");

		if(fp == NULL)
		{
			printf("\n 4. sta_scan ERROR: popen faile \n");
			return FALSE;	
		}
		memset(buf, 0, sizeof(buf));
		fgets(buf,sizeof(buf),fp); 
		pclose(fp);
		endstr = strchr(buf,'\n');
		if(endstr != NULL)
		{
			*endstr = 0;  
		}			

		strcat(list_str, buf);
		strcat(list_str, "\r\n");
	}
}


//set the usr name and password used when wifi data call in CDMA/EVDO net mode
boolean set_user_name_pwd(char *sz_usrname, char *sz_usrpwd)
{
	if( strlen(sz_usrname) < 1 || strlen(sz_usrname) > 127
			|| strlen(sz_usrpwd) < 1 || strlen(sz_usrpwd) > 127)
	{
		printf("\n usr name or pwd length error!\n");
		return FALSE;
	}

	if( !wifi_daemon_save_usr_name_pwd(sz_usrname, sz_usrpwd) )
	{
		printf("\n wifi_daemon_save_usr_name_pwd error \n");
		return FALSE;
	}

	return TRUE;
}

//get the usr name and password used when wifi data call in CDMA/EVDO net mode
boolean get_user_name_pwd(char *sz_usrname, int len_name, char *sz_usrpwd, int len_pwd)
{
	if( !wifi_daemon_read_usr_name_pwd(sz_usrname, len_name, sz_usrpwd, len_pwd) )
	{
		strcpy(sz_usrname, "ctnet@mycdma.cn");
		strcpy(sz_usrpwd, "vnet.mobi");
	}

	return TRUE;
}

static boolean wifi_is_W58L()
{
	const char* ifconfig_result_str = "/data/ifconfig_result";
	char command[100] = {0};
	FILE *fp;   
	char buf[100]={0};
	char *buf_read = NULL;

	boolean is_W58L = FALSE;

	sprintf(command, "rm -f %s", ifconfig_result_str);
	system(command);

	memset(command, 0, sizeof(command));
	sprintf(command, "ifconfig > %s", ifconfig_result_str);
	system(command);
	//printf("\n1. wifi_is_sta_mode\n");
	fp=fopen(ifconfig_result_str,"r");
	if(fp == NULL)
	{
		printf("\n 1. wifi_is_W58L ERROR: popen faile \n");
		return FALSE;	
	}

	buf_read = fgets(buf,sizeof(buf),fp); 
	//printf("\n[%s]\n", buf);
	if(strlen(buf) > strlen("br0") && strstr(buf_read, "br0") != NULL)
	{
		is_W58L = TRUE;
	}

	fclose(fp);

	printf("\nis_W58L[%d]\n", is_W58L);
	return is_W58L;
}

boolean wifi_is_sta_mode()
{
	boolean ret = FALSE;
	const char* ifconfig_result_str = "/data/ifconfig_result";
	char command[100] = {0};
	FILE *fp;   
	char buf[100]={0};
	char *buf_read = NULL;

	boolean is_W58L = FALSE;

	sprintf(command, "ifconfig > %s", ifconfig_result_str);
	system(command);
	//printf("\n1. wifi_is_sta_mode\n");
	fp=fopen(ifconfig_result_str,"r");
	if(fp == NULL)
	{
		printf("\n 1. wifi_is_sta_mode ERROR: popen faile \n");
		return FALSE;	
	}

	buf_read = fgets(buf,sizeof(buf),fp); 
	//printf("\n[%s]\n", buf);
	if(strlen(buf) > strlen("br0") && strstr(buf_read, "br0") != NULL)
	{
		is_W58L = TRUE;
	}

	printf("\nis_W58L[%d]\n", is_W58L);
	if(is_W58L)
	{
		while(!feof(fp))
		{
			buf_read = fgets(buf,sizeof(buf),fp); 
			//printf("\n[%s]\n", buf);
			if(strlen(buf) > strlen("wlan0-vxd") && strstr(buf_read, "wlan0-vxd") != NULL)
			{
				ret = TRUE;
				break;
			}
		}
	}
	else
	{
		if(get_ap_cfg() == APSTA_MODE)
		{
			ret = TRUE;
		}
	}

	//printf("\n2. wifi_is_sta_mode: close file\n");
	fclose(fp);

	printf("\nIs STA mode[%d]\n", ret);
	return ret;
}


//#ifdef WIFI_RTL_DAEMON
/*======================
sta_init: open/close station mode

NOTE: this api is only avaliable in W58L
=======================*/
boolean sta_init(int sta_enable)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	char sta_enable_str[sizeof(int) + 1] ={0};
	char mask_flag[sizeof(int) + 1] = {0};      // FIELD_MASK_1 : set   FIELD_MASK_2: get	

	if(!wifi_is_W58L())
	{
		printf("\nERROR: not W58L!\n");
		return FALSE;
	}

	if(sta_enable < 0 || sta_enable > 1)
	{
		printf("\n map_enable value invalid! \n");
		return FALSE;			
	}

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	snprintf(sta_enable_str,sizeof(int),"%d",sta_enable);
	snprintf(mask_flag,sizeof(int),"%d",FIELD_MASK_1); 

	wifi_daemon_set_key_value(wifidaemon_webcli_buff,  rtl_stainit_status_string[STA_INIT_STATUS_PAGE], RTLSTAINIT);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, rtl_stainit_status_string[STA_INIT_MASK], mask_flag);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, rtl_stainit_status_string[STA_INIT_STATUS], sta_enable_str);
	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									
	WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}

/*======================
sta_init: get sta status

NOTE: this api is only avaliable in W58L
=======================*/
boolean get_sta_status(uint8 *flag)
{
	char * wifidaemon_webcli_buff = NULL;
	char * webcli_wifidaemon_buff = NULL;
	int webcli_wifidaemon_buff_size;
	int ret;

	char sta_init_str[sizeof(int) + 1] ={0};
	int  value_len;
	char mask_flag[sizeof(int) + 1] = {0};      // FIELD_MASK_1 : set   FIELD_MASK_2: get

	if(!wifi_is_W58L())
	{
		printf("\nERROR: not W58L!\n");
		return FALSE;
	}

	if(wifi_daemon_init_send_string(&wifidaemon_webcli_buff, &webcli_wifidaemon_buff) == FAIL)
	{
		return FALSE;
	}

	snprintf(mask_flag,sizeof(int),"%d",FIELD_MASK_2); 

	wifi_daemon_set_key_value(wifidaemon_webcli_buff,  rtl_stainit_status_string[STA_INIT_STATUS_PAGE], RTLSTAINIT);
	wifi_daemon_set_key_value(wifidaemon_webcli_buff, rtl_stainit_status_string[STA_INIT_MASK], mask_flag);
	WIFI_LOG("send_str:%s len=%d", wifidaemon_webcli_buff, strlen(wifidaemon_webcli_buff));

	ret = wifi_daemon_webcli_communication(	wifidaemon_webcli_buff, 
			strlen(wifidaemon_webcli_buff), 
			webcli_wifidaemon_buff, 
			&webcli_wifidaemon_buff_size);
	if(ret == FAIL)
	{
		wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
		return FALSE;				
	}									
	WIFI_LOG("recv_str = %s", webcli_wifidaemon_buff);

	if(wifi_daemon_get_key_value(webcli_wifidaemon_buff, rtl_stainit_status_string[STA_INIT_STATUS], sta_init_str, &value_len))
	{
		WIFI_LOG("\r\n==============\r\nsta_init_str = %s\r\n===========\r\n", sta_init_str);
		if(value_len <= 0)
		{
			wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
			return FALSE;
		}

		*flag = atoi(sta_init_str);
		WIFI_LOG("\n*flag[%d]\n", *flag);
	}

	wifi_daemon_free_send_string(wifidaemon_webcli_buff, webcli_wifidaemon_buff);
	return TRUE;
}




