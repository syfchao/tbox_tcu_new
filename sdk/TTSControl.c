
#include <unistd.h>  
#include <sys/file.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <stdio.h>  
#include <stdlib.h> 
#include <pthread.h>   

#include "TTSControl.h"

#define TIMEOUT_SEC             3
#define TIMEOUT_USEC            0
#define EXT_POC_TTS_PLAY_UTF16LE  (8)
#define EXT_POC_TTS_PLAY_GBK      (9)
#define ALSA_CLIENT_FIFO 		"/data/alsa_client_%d_fifo"
#define ALSA_DAEMON_FIFO 		"/data/alsa_daemon_fifo"
#define ALSATTS_EXT_MSG_FIFO     "/data/ext_app_alsatts_msg_fifo"
#define ALSATTS_AT_START          "ALSATTS AT START"
#define ALSATTS_AT_END            "ALSATTS AT END"
#define ALSATTS_AT_END_ERROR      "ALSATTS AT END ERROR"
#define ALSATTS_POC_START         "ALSATTS POC START"
#define ALSATTS_POC_END           "ALSATTS POC END"
#define ALSATTS_POC_END_ERROR     "ALSATTS POC END ERROR"
#define PARAM_MAX_LEN 			1024
#define PARAM_MAX_NUM                   4

#define TTS_LOG 

typedef struct alsa_msg
{
    char  command[16];
	pid_t client_pid;
	int   result; 
    unsigned char data[PARAM_MAX_NUM][PARAM_MAX_LEN + 1];
}alsa_msg;

typedef enum thread_working{
	NO_WORKING,
	TTS_WORKING,
}thread_working;

thread_working g_thread_working;
pthread_mutex_t g_device_mutex = PTHREAD_MUTEX_INITIALIZER;

int get_thread_work_status(void)
{
	thread_working status;
	pthread_mutex_lock(&g_device_mutex);
	status = g_thread_working;
    pthread_mutex_unlock(&g_device_mutex);	
    return status;
}

void set_thread_work_status(thread_working status)
{
  pthread_mutex_lock(&g_device_mutex);
  g_thread_working = status;
  pthread_mutex_unlock(&g_device_mutex);	
}

int lock_set(int fd,int type)  
{  
    struct flock old_lock,lock;  
    lock.l_whence = SEEK_SET;  
    lock.l_start = 0;  
    lock.l_len = 0;  
    lock.l_type = type;  
          
    fcntl(fd,F_GETLK,&lock);  
  
    if(lock.l_type != F_UNLCK)  
    {  
          
        if (lock.l_type == F_RDLCK)    
        {  
            TTS_LOG("Read lock already set by %d\n",lock.l_pid);  
        }  
        else if (lock.l_type == F_WRLCK)   
        {  
            TTS_LOG("Write lock already set by %d\n",lock.l_pid);  
        }                         
    }
    TTS_LOG("type: %d\n", lock.l_type); 
      
    lock.l_type = type;  
      
    if ((fcntl(fd,F_SETLKW,&lock)) < 0)  
    {  
        TTS_LOG("Lock failed : type = %d\n",lock.l_type);  
        return 1;  
    }  
      
    switch (lock.l_type)  
    {  
        case F_RDLCK:  
        {  
            TTS_LOG("Read lock set by %d\n",getpid());  
        }  
        break;  
        case F_WRLCK:  
        {  
            TTS_LOG("write lock set by %d\n",getpid());  
        }  
        break;  
        case F_UNLCK:  
        {  
            TTS_LOG("Release lock by %d\n",getpid());  
            return 1;  
        }  
        break;  
          
        default:  
        break;  
  
    }  
    return 0;  
} 

int lock_get(int fd)
{

  int res;
  struct flock region;

  region.l_type=F_WRLCK;
  region.l_whence=SEEK_SET;
  region.l_start=0;
  region.l_len=50;
  region.l_pid=1;

  if((res=fcntl(fd,F_GETLK,&region))==0)
    {
      if(region.l_type==F_UNLCK)
        {
          TTS_LOG("F_UNLCK.\n");
	  return 0;
        }
	else
	{
		TTS_LOG("not F_UNLCK.\n");
	  return 1;
	}
    }
	return 1;
}

static int send_msg_to_alsa_daemon(alsa_msg send_msg, alsa_msg *rev_msg)
{

    int daemon_fifo_fd;
    int client_fifo_fd;
	char client_fifo_name[64] = {0};
    int res = 0;
	int retry = 0;
	int ret = 0;
	
	int status;
	fd_set fds;

	int lock_fd;

#if 1
	lock_fd = open("/data/fck_hello",O_RDWR | O_CREAT, 0644);
#endif

	struct timeval tv;

	send_msg.client_pid = getpid();

	sprintf(client_fifo_name, ALSA_CLIENT_FIFO, send_msg.client_pid);
	
	
	daemon_fifo_fd = open(ALSA_DAEMON_FIFO, O_WRONLY | O_NONBLOCK);

	if(daemon_fifo_fd == -1){
		TTS_LOG("[] open deamon fifo fail\n");
		return -1;
	}

	if (access(client_fifo_name, F_OK) == -1)  
    {  
		if (mkfifo(client_fifo_name,0666) < 0){
			TTS_LOG("[] mkfifo client fifo fail\n");
			close(daemon_fifo_fd);
			return -1;
		}
    }

	res = write(daemon_fifo_fd, &send_msg, sizeof(alsa_msg));

	if(res < 0){
		TTS_LOG("[] send msg write fail\n");
		close(daemon_fifo_fd);
		return -1;
	}

	client_fifo_fd = open(client_fifo_name, O_RDONLY | O_NONBLOCK);
	
	if(client_fifo_fd == -1){
		TTS_LOG("open client fifo fail\n");
		close(daemon_fifo_fd);
		return -1;
	}	

  	FD_ZERO(&fds);
  	FD_SET(client_fifo_fd, &fds);
	tv.tv_sec = TIMEOUT_SEC;
	tv.tv_usec = TIMEOUT_USEC;

	status = select(client_fifo_fd + 1, &fds, NULL, NULL, &tv);
	if(status == -1){

		res = -1;

	}else if(status == 0){

		TTS_LOG("[] timeout\n");
		res = -1;
		
	}else{
		
		if (FD_ISSET(client_fifo_fd, &fds)){
			res = read(client_fifo_fd, rev_msg, sizeof(alsa_msg));
	    	if (res > 0)
	    	{
		        TTS_LOG("[] response cmd = %s\n", rev_msg->command);
				TTS_LOG("[] response data[0] = %s\n", rev_msg->data[0]);
	    	}else{
				res = -1;
			}
			
		}
	}
#if 1
	TTS_LOG("===lock get.\n");
	while(lock_get(lock_fd) == F_WRLCK)
	{
		TTS_LOG("===lock get retry: %d.\n", retry);
		usleep(20*1000);
		retry ++;
		if(retry > 100)
		{
			break;
		}
	}
	TTS_LOG("===lock set.\n");
	lock_set(lock_fd, F_WRLCK);
#endif
//get_lock == 0    1 retry 10s
//set_lock 1
    close(client_fifo_fd);
    unlink(client_fifo_name);
//un_lock 0
#if 1
	lock_set(lock_fd, F_UNLCK);
	TTS_LOG("===unlock set.\n");
	close(lock_fd);
#endif

    close(daemon_fifo_fd);
	
	return res;
}

TTS_STATE simcom_get_tts_state()
{
	alsa_msg send_msg;
	alsa_msg rev_msg;

	memset(&send_msg, 0, sizeof(alsa_msg));
	memset(&rev_msg, 0, sizeof(alsa_msg));

	sprintf(send_msg.command, "%s", "CTTS");
	sprintf(send_msg.data[0], "%s", "?");

	send_msg_to_alsa_daemon(send_msg, &rev_msg);
	if(rev_msg.result < 0){
		return TTS_NONE;

	}else{
		return atoi(rev_msg.data[0])?TTS_PLAY:TTS_STOP;
	}
}
static int ejtts_strlen(const unsigned char *s)
{
    const unsigned char *p;
    
    for (p = s; *p != '\0'; p++);

    return p - s;    
}
/*
	return:
		0  function call success
		-1 system error
*/
int simcom_play_tts(TTS_CMD tts_action, uint8 *tts_text)
{
	if(strlen(tts_text) > 512)
	{	
		return -1;				
	}

	alsa_msg send_msg;
	alsa_msg rev_msg;
	int mode  = 2;
	int err   = 0;

	if(tts_action == TTS_PLAY_UTF16LE){
		mode = EXT_POC_TTS_PLAY_UTF16LE;
	}else if(tts_action == TTS_PLAY_GBK){
		mode = EXT_POC_TTS_PLAY_GBK;
	}

	if(mode > 0)
	{
		if((mode == EXT_POC_TTS_PLAY_UTF16LE && ejtts_strlen(tts_text) < 6) 
			|| (mode == EXT_POC_TTS_PLAY_GBK && ejtts_strlen(tts_text) < 3))
		{
			TTS_LOG("[] len error: %d text: %s.\n", ejtts_strlen(tts_text), tts_text);
			return -1;				
		}	
		
		if(ejtts_strlen(tts_text) > 512)
		{	
			TTS_LOG("[] max len error: %d text: %s.\n", ejtts_strlen(tts_text), tts_text);
			return -1;					
		}
			
	}

	memset(&send_msg, 0, sizeof(alsa_msg));
	sprintf(send_msg.command, "%s", "CTTS");
	sprintf(send_msg.data[0], "%d", mode);

	memcpy(send_msg.data[1], tts_text, strlen(tts_text));

	if((err=get_thread_work_status()) > NO_WORKING)
	{
		TTS_LOG("[] get thread work status err[%d].\n", err);
		return -1;
	}
	set_thread_work_status(TTS_WORKING);
	
	TTS_LOG("[] text: %s\n", send_msg.data[1]);
	err = send_msg_to_alsa_daemon(send_msg, &rev_msg);

	if(err == -1)
	{
		set_thread_work_status(NO_WORKING);
	}

	if(rev_msg.result < 0){
		return -1;
	}else{
		return 0;//atoi(rev_msg.data[0]);
	}
}

void simcom_stop_tts()
{
	alsa_msg send_msg;
	alsa_msg rev_msg;
	int mode  = 0;
	int retry = 1000;

	sprintf(send_msg.command, "%s", "CTTS");
	sprintf(send_msg.data[0], "%d", mode);

	send_msg_to_alsa_daemon(send_msg, &rev_msg);

	/*while(get_thread_work_status() == TTS_WORKING)
	{
		TTS_LOG("[] play func running.\n");
		usleep(10*1000);
		retry --;
		if(retry <= 0)
		{
				set_thread_work_status(NO_WORKING);
		 		break;
		}
	}*/
        set_thread_work_status(NO_WORKING);
	/*if(rev_msg.result < 0){
		return -1;
	}else{
		return atoi(rev_msg.data[0]);
	}*/
}

void handle_msg_from_alsatts()
{
    	int pipe_fd = -1;   
    	int res = 0;      
    	char buffer[50];
	fd_set inputs;
	int result;

    pthread_detach(pthread_self());
	
	TTS_LOG("[%s] recv msg from alsatts\n", __func__);
    	if(access(ALSATTS_EXT_MSG_FIFO, F_OK) == -1)  
    	{   
        	res = mkfifo(ALSATTS_EXT_MSG_FIFO, 0777);  
       		if(res != 0)  
        	{  
			TTS_LOG("[%s]Could not create fifo.\n", __func__);   
            		return; 
        	}  
	}

while(1)
{
	pipe_fd = open(ALSATTS_EXT_MSG_FIFO, O_RDONLY);//O_RDWR); 
	if(pipe_fd < 0)  
	{
		TTS_LOG("[%s]Open fifo error.\n", __func__);
		return;
	} 

	FD_ZERO(&inputs); 
	FD_SET(pipe_fd,&inputs);
	FD_SET(STDIN_FILENO,&inputs);
	TTS_LOG("[%s]wait msg.\n", __func__);
	result = select(pipe_fd+1, &inputs, NULL, NULL, NULL);
	if(result <= 0)
	{
		TTS_LOG("[%s]Select error.\n", __func__);
		close(pipe_fd);
		return;
	}
	memset(buffer, '\0', sizeof(buffer));
	read(pipe_fd, buffer, 50);
	close(pipe_fd);

	TTS_LOG("[%s]msg: %s\n", __func__, buffer);
	if(strcmp(buffer, ALSATTS_AT_START)==0){
		set_thread_work_status(TTS_WORKING);
	}
	else if(strcmp(buffer, ALSATTS_POC_START)==0){
		set_thread_work_status(TTS_WORKING);
	}
	else if((strcmp(buffer, ALSATTS_AT_END)==0)
		||(strcmp(buffer, ALSATTS_AT_END_ERROR)==0)){
		set_thread_work_status(NO_WORKING);
	}
	else if((strcmp(buffer, ALSATTS_POC_END)==0)
		||(strcmp(buffer, ALSATTS_POC_END_ERROR)==0)){
		set_thread_work_status(NO_WORKING);
	}
}
}

int simcom_tts_init()
{
	pthread_t recv_ttsmsg_thread;
    
	if(0 != pthread_create(&recv_ttsmsg_thread, NULL, handle_msg_from_alsatts, NULL))
	{
		TTS_LOG("[%s] create recv_ttsmsg_thread fail.\n", __func__);
		return -1;
	}
	
	return 0;
}
