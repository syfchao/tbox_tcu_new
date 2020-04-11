#include <unistd.h>  
#include <sys/file.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <stdio.h>  
#include <stdlib.h> 
#include <pthread.h>   

#define ALSA_CLIENT_FIFO 		"/data/alsa_client_%d_fifo"
#define ALSA_DAEMON_FIFO 		"/data/alsa_daemon_fifo"

#define TIMEOUT_SEC             3
#define TIMEOUT_USEC            0

#define PARAM_MAX_LEN           1024
#define PARAM_MAX_NUM           4

typedef struct alsa_msg
{
    char  command[16];
	pid_t client_pid;
	int   result; 
    unsigned char data[PARAM_MAX_NUM][PARAM_MAX_LEN + 1];
}alsa_msg;


/*===========================================================================

                           Definations and typedefs

===========================================================================*/
#ifndef UPCASE
#define  UPCASE( c ) ( ((c) >= 'a' && (c) <= 'z') ? ((c) - 0x20) : (c) )
#endif
/*===========================================================================
  FUNCTION   dsatutil_atoi
===========================================================================*/
/*!
@brief
  
string to int
@return
  0 ok


@note

*/
/*=========================================================================*/
static int dsatutil_atoi
(
  unsigned int *val_arg_ptr,      /*  value returned  */
  unsigned char *s,                        /*  points to string to eval  */
  unsigned int r                        /*  radix */
)
{
  int err_ret = -1;
  unsigned char c;
  unsigned int val, val_lim, dig_lim;

  val = 0;
  val_lim = (unsigned int) ((unsigned int)0xffffffff / r);
  dig_lim = (unsigned int) ((unsigned int)0xffffffff % r);

  while ( (c = *s++) != '\0')
  {
    if (c != ' ')
    {
      c = (unsigned char) UPCASE (c);
      if (c >= '0' && c <= '9')
      {
        c -= '0';
      }
      else if (c >= 'A')
      {
        c -= 'A' - 10;
      }
      else
      {
        err_ret = -2;  /*  char code too small */
        break;
      }

      if (c >= r || val > val_lim
          || (val == val_lim && c > dig_lim))
      {
        err_ret = -2;  /*  char code too large */
        break;
      }
      else
      {
        err_ret = 0;            /*  arg found: OK so far*/
        val = (unsigned int) (val * r + c);
      }
    }
    *val_arg_ptr =  val;
  }
  
  return err_ret;

}

static int lock_set(int fd,int type)  
{  
    struct flock old_lock,lock;  
    lock.l_whence = SEEK_SET;  
    lock.l_start = 0;  
    lock.l_len = 0;  
    lock.l_type = type;  
    //lock.l_pid = -1;  
          
    fcntl(fd,F_GETLK,&lock);  
  
    if(lock.l_type != F_UNLCK)  
    {  
          
        if (lock.l_type == F_RDLCK)    
        {  
            printf("Read lock already set by %d\n",lock.l_pid);  
        }  
        else if (lock.l_type == F_WRLCK)   
        {  
            printf("Write lock already set by %d\n",lock.l_pid);  
        }                         
    }  
      
    lock.l_type = type;  
      
    if ((fcntl(fd,F_SETLK,&lock)) < 0)  
    {  
        printf("Lock failed : type = %d\n",lock.l_type);  
        return 1;  
    }  
      
    switch (lock.l_type)  
    {  
        case F_RDLCK:  
        {  
            printf("Read lock set by %d\n",getpid());  
        }  
        break;  
        case F_WRLCK:  
        {  
            printf("write lock set by %d\n",getpid());  
        }  
        break;  
        case F_UNLCK:  
        {  
            printf("Release lock by %d\n",getpid());  
            return 1;  
        }  
        break;  
          
        default:  
        break;  
  
    }  
    return 0;  
} 

static int lock_get(int fd)
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
          printf("F_UNLCK.\n");
	  return 0;
        }
	else
	{
		printf("not F_UNLCK.\n");
	  return 1;
	}
    }
	printf("lock get fail.\n");
	return 1;
}



static int send_msg_to_alsa_daemon(alsa_msg send_msg, alsa_msg *rev_msg)
{

    int daemon_fifo_fd;
    int client_fifo_fd;
	char client_fifo_name[64] = {0};
    int res = 0;
	int retry = 0;
	int status;
	fd_set fds;
	int lock_fd;
	
	lock_fd = open("/data/fck_hello",O_RDWR | O_CREAT, 0644);
	struct timeval tv;

	send_msg.client_pid = getpid();

	sprintf(client_fifo_name, ALSA_CLIENT_FIFO, send_msg.client_pid);
	
	
	daemon_fifo_fd = open(ALSA_DAEMON_FIFO, O_WRONLY | O_NONBLOCK);

	if(daemon_fifo_fd == -1){
		printf("open deamon fifo fail\n");
		close(lock_fd);
		return -1;
	}

	if (access(client_fifo_name, F_OK) == -1)  
    {  
		if (mkfifo(client_fifo_name,0666) < 0){
			printf("mkfifo client fifo fail\n");
			close(daemon_fifo_fd);			
			close(lock_fd);
			return -1;
		}
    }

	res = write(daemon_fifo_fd, &send_msg, sizeof(alsa_msg));

	if(res < 0){
		printf("send msg write fail\n");
		close(daemon_fifo_fd);
		close(lock_fd);
		return -1;
	}

	client_fifo_fd = open(client_fifo_name, O_RDONLY | O_NONBLOCK);
	
	if(client_fifo_fd == -1){
		printf("open client fifo fail\n");
		close(daemon_fifo_fd);		
		close(lock_fd);
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

		printf("timeout\n");
		res = -1;
		
	}else{
		
		if (FD_ISSET(client_fifo_fd, &fds)){
			res = read(client_fifo_fd, rev_msg, sizeof(alsa_msg));
	    	if (res > 0)
	    	{
		        printf("response cmd = %s\n", rev_msg->command);
				printf("response data[0] = %s\n", rev_msg->data[0]);
	    	}else{
				res = -1;
			}
			
		}
	}

	while(lock_get(lock_fd) == F_WRLCK)
	{
		usleep(20*1000);
		retry ++;
		if(retry > 100)
		{
			break;
		}
	}
	lock_set(lock_fd, F_WRLCK);
	close(client_fifo_fd);
	unlink(client_fifo_name);
	lock_set(lock_fd, F_UNLCK);
	close(lock_fd);

	close(daemon_fifo_fd);
	
	return res;
}


#define POC_ALSA_CLIENT_FIFO 		"/data/poc_alsa_client_%d_fifo"
#define POC_ALSA_DAEMON_FIFO 		"/data/poc_alsa_daemon_fifo"


static int send_msg_to_ext_dmr(alsa_msg send_msg, alsa_msg *rev_msg)
{

    int daemon_fifo_fd;
    int client_fifo_fd;
	char client_fifo_name[64] = {0};
    int res = 0;
	
	int status;
	fd_set fds;

	struct timeval tv;

	send_msg.client_pid = getpid();

	sprintf(client_fifo_name, POC_ALSA_CLIENT_FIFO, send_msg.client_pid);
	
	
	daemon_fifo_fd = open(POC_ALSA_DAEMON_FIFO, O_WRONLY | O_NONBLOCK);

	if(daemon_fifo_fd == -1){
		printf("open deamon fifo fail\n");
		return -1;
	}

	if (access(client_fifo_name, F_OK) == -1)  
    {  
		if (mkfifo(client_fifo_name,0666) < 0){
			printf("mkfifo client fifo fail\n");
			close(daemon_fifo_fd);
			return -1;
		}
    }

	res = write(daemon_fifo_fd, &send_msg, sizeof(alsa_msg));

	if(res < 0){
		printf("send msg write fail\n");
		close(daemon_fifo_fd);
		return -1;
	}

	client_fifo_fd = open(client_fifo_name, O_RDONLY | O_NONBLOCK);
	
	if(client_fifo_fd == -1){
		printf("open client fifo fail\n");
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

		printf("timeout\n");
		res = -1;
		
	}else{
		
		if (FD_ISSET(client_fifo_fd, &fds)){
			res = read(client_fifo_fd, rev_msg, sizeof(alsa_msg));
	    	if (res > 0)
	    	{
		        printf("response cmd = %s\n", rev_msg->command);
				printf("response data[0] = %s\n", rev_msg->data[0]);
	    	}else{
				res = -1;
			}
			
		}
	}

    close(client_fifo_fd);
    close(daemon_fifo_fd);
    unlink(client_fifo_name);

	return res;
}


int set_clvl_value(int clvl_value)
{
	alsa_msg send_msg;
	alsa_msg rev_msg;
	int res;

	if(clvl_value < 0 || clvl_value > 5)
	{ 
		return -1; 		
	}
	memset(&send_msg, 0, sizeof(alsa_msg));
	memset(&rev_msg, 0, sizeof(alsa_msg));
	
	sprintf(send_msg.command, "%s", "CLVL");
	sprintf(send_msg.data[0], "%d", clvl_value);
	
	res = send_msg_to_alsa_daemon(send_msg, &rev_msg);
	if((rev_msg.result<0) || (res<0))
	{
		return -1;
	}		

	return 0;

}


int get_clvl_value(void)
{
	alsa_msg send_msg;
	alsa_msg rev_msg;
	int res;	
	int clvl_value;

	memset(&send_msg, 0, sizeof(alsa_msg));
	memset(&rev_msg, 0, sizeof(alsa_msg));

	sprintf(send_msg.command, "%s", "CLVL");
	sprintf(send_msg.data[0], "%s", "?");

	res = send_msg_to_alsa_daemon(send_msg, &rev_msg);
	if((rev_msg.result<0) || (res<0))
	{
		return -1;
	}		

	if(0 != dsatutil_atoi(&clvl_value, (unsigned char *)rev_msg.data[0], 10))
	{
		return -1;
	}
	
	return clvl_value;
}


int set_micgain_value(int micgain_value)
{
	alsa_msg send_msg;
	alsa_msg rev_msg;
	int res;	

	memset(&send_msg, 0, sizeof(alsa_msg));
	memset(&rev_msg, 0, sizeof(alsa_msg));
	
	if(micgain_value < 0 || micgain_value > 8)
	{
		return -1; 		
	}
	
	sprintf(send_msg.command, "%s", "CMICGAIN");
	sprintf(send_msg.data[0], "%d", micgain_value);
	res = send_msg_to_alsa_daemon(send_msg, &rev_msg);
	if((rev_msg.result<0) || (res<0))
	{
		return -1;
	}		
	send_msg_to_ext_dmr(send_msg, &rev_msg);
	return 0;
}


int get_micgain_value(void)
{
	alsa_msg send_msg;
	alsa_msg rev_msg;
	int res;	
	int micgain_value;

	memset(&send_msg, 0, sizeof(alsa_msg));
	memset(&rev_msg, 0, sizeof(alsa_msg));

	sprintf(send_msg.command, "%s", "CMICGAIN");
	sprintf(send_msg.data[0], "%s", "?");
	res = send_msg_to_alsa_daemon(send_msg, &rev_msg);
	if((rev_msg.result<0) || (res<0))
	{
		return -1;
	}		

	if(0 != dsatutil_atoi(&micgain_value, (unsigned char *)rev_msg.data[0], 10))
	{
		return -1;
	}
	
	return micgain_value;

}

int set_csdvc_value(int csdvc_value)
{
	alsa_msg send_msg;
	alsa_msg rev_msg;
	int res;	

	memset(&send_msg, 0, sizeof(alsa_msg));
	memset(&rev_msg, 0, sizeof(alsa_msg));


	if((csdvc_value != 1) && (csdvc_value != 3))
	{
		return -1; 		
	}
	
	sprintf(send_msg.command, "%s", "CSDVC");
	sprintf(send_msg.data[0], "%d", csdvc_value);
	res = send_msg_to_alsa_daemon(send_msg, &rev_msg);
	if((rev_msg.result<0) || (res<0))
	{
		return -1;
	}		

	send_msg_to_ext_dmr(send_msg, &rev_msg);

	return 0;
}

int get_csdvc_value(void)
{
	alsa_msg send_msg;
	alsa_msg rev_msg;
	int res;	
	int csdvc_value;	
	memset(&send_msg, 0, sizeof(alsa_msg));
	memset(&rev_msg, 0, sizeof(alsa_msg));
	
	sprintf(send_msg.command, "%s", "CSDVC");
	sprintf(send_msg.data[0], "%s", "?");
	res = send_msg_to_alsa_daemon(send_msg, &rev_msg);
	if((rev_msg.result<0) || (res<0))
	{
		return -1;
	}		

	if(0 != dsatutil_atoi(&csdvc_value, (unsigned char *)rev_msg.data[0], 10))
	{
		return -1;
	}
	
	return csdvc_value;

}


