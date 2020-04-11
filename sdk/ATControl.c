#include "ATControl.h"

static int at_fd = -1;
static pthread_t timingThreadId;

int atctrl_init()
{
	if(at_fd < 0)
	{
		at_fd = open(SIMCOM_AT_PORT, O_RDWR | O_NONBLOCK | O_NOCTTY);
		if(at_fd < 0)
			return -1;
	}
	printf("\n\n\n  init atctrl success\n\n");
	return 0;
}

void atctrl_deinit()
{
	if(at_fd >= 0)
	{
		close(at_fd);
		at_fd = -1;
	}
}

int switch_stepSPK(int spkflag)
{
	int i,temp;
	int retval = -1;
	static int SpkMuteStatus = 1;
	char buff[64] = {0};

	#if 1
	if(spkflag == 1)/*拨打电话*/
	{
//		if(SpkMuteStatus == 1)
		{
			for(i=0;i<= 0x3C;i++)	/*for(i=0;i<= 0x39;i++)*/
			{
				temp = i | 0x80;
				sprintf(buff,"at+cwiic=0x34,0x6c,0x%2x,1",temp);
				retval = sendATCmd((char*)buff, (char*)"OK", NULL, 0, 5000);
				memset(buff, 0, sizeof(buff));
				usleep(5*1000);
			}
			SpkMuteStatus = 0;
		}
	}
	#endif

	#if 1
	if(spkflag == 0)
	{
		if(SpkMuteStatus == 0)
		{
			for(i=0x3C;i>=0x0;i--)	/*for(i=0x39;i>=0x0;i--)*/
			{
				temp = i | 0x80;
				sprintf(buff,"at+cwiic=0x34,0x6c,0x%2x,1",temp);
				retval = sendATCmd((char*)buff, (char*)"OK", NULL, 0, 5000);
				memset(buff, 0, sizeof(buff));
				usleep(5*1000);
			}
			SpkMuteStatus = 1;
		}
	}
	#endif

	return retval;
}

void AT_SetMuteInitValue()
{
	int retval = 0;
	retval = sendATCmd((char *)"at+cwiic=0x34,0x58,0x03,1", (char*)"OK", NULL, 0, 5000);
	
	retval = sendATCmd((char *)"at+cwiic=0x34,0x5a,0x31,1", (char*)"OK", NULL, 0, 5000);
	
	retval = sendATCmd((char *)"at+cwiic=0x34,0x5e,0x00,1", (char*)"OK", NULL, 0, 5000);
	
	retval = sendATCmd((char *)"at+cwiic=0x34,0x1e,0xf2,1", (char*)"OK", NULL, 0, 5000);
	
	retval = sendATCmd((char *)"at+cwiic=0x34,0x16,0xec,1", (char*)"OK", NULL, 0, 5000);
	
	retval = sendATCmd((char *)"at+cwiic=0x34,0x6c,0x40,1", (char*)"OK", NULL, 0, 5000);
	
	retval = sendATCmd((char *)"at+cwiic=0x34,0x62,0x02,1", (char*)"OK", NULL, 0, 5000);
}






int CallMute(void)
{
	int i;
	int temp = -1;
	char RegValue[100] = {0};
	char *pTemp = NULL;
	#define MuteValue 	"0xbc"

	sendATCmd((char*)"at+criic=0x34,0x6c,2", (char*)"OK", RegValue, 100, 5000);

	pTemp = strstr(RegValue, MuteValue);

	if(pTemp != NULL)
	{
		for(i=0x3c;i>= 0x00;i--)	/*for(i=0;i<= 0x39;i++)*/
		{
			temp = i | 0x80;
			memset(RegValue, 0, sizeof(RegValue));
			sprintf(RegValue,"at+cwiic=0x34,0x6c,0x%2x,1",temp);
			sendATCmd((char*)RegValue, (char*)"OK", NULL, 0, 5000);
		}
	}
	return 0;
}


/***************************************************************************
* Function Name: sendATCmd			
* Function Introduction: send at command to LTE module
* Parameter description:
*     atCmd    : at command
*     finalRsp : at command's response result.
*     buff     : if need function return result, you should pass the buff,
*                then the function will store the result into buff 
*     buffSize : buff size, At least 100bits.
*     timeout_ms: time out
* Function return value:  -1: failed; 0: return error,
*                         other: return data length.
* Auther        : simcom
* Date          : 2018.04.14								
****************************************************************************/
int sendATCmd(char* atCmd, char* finalRsp, char *buff, uint32_t buffSize, long timeout_ms)
{
    int retval;
	int len;
    int readLen;
	uint8_t count = 0;
    unsigned char recvFlag = 0;
    char strAT[100];
    char strFinalRsp[100];
    char strResponse[100];
	
    fd_set fds;
    struct timeval timeout = {0, 0};
    memset(strAT, 0x0, sizeof(strAT));
	
    sprintf(strFinalRsp, "\r\n%s", finalRsp);

    timeout.tv_sec = timeout_ms / 1000;
    //timeout.tv_usec = timeout_ms % 1000;
	
    len = strlen(atCmd);
    if ((atCmd[len - 1] != '\r') && (atCmd[len - 1] != '\n'))
    {
        len = sprintf(strAT, "%s\r\n", atCmd);
        strAT[len] = 0;
		//printf("strAT:%s\n", strAT);
    }

    if((retval = write(at_fd, strAT, len)) > 0)
		//printf("send at success, retval:%d\n", retval);

    if ((buff != NULL) &&(buffSize != 0))
    {
		memset(buff, 0, buffSize);
    }

    while (count++ <= 3)
    {
        FD_ZERO(&fds);
        FD_SET(at_fd, &fds);

        switch (select(at_fd + 1, &fds, NULL, NULL, &timeout))
        {
            case -1:
				printf("select error:%d\n", errno);
				if (errno == EINTR) 
					continue;
				return -1;

            case 0:
                printf("select time out.\n");
                return -1;

            default:
                if (FD_ISSET(at_fd, &fds))
                {
                    do
                    {
                        memset(strResponse, 0x0, sizeof(strResponse));
                        readLen = read(at_fd, strResponse, sizeof(strResponse));
						if (readLen > 0)
						{
							if(strstr(strResponse, strFinalRsp))
							{
								retval = readLen;
								recvFlag = 1;
							}
							else if(strstr(strResponse, "+CME ERROR:")
								 || strstr(strResponse, "+CMS ERROR:") 
								 || strstr(strResponse, "ERROR"))
							{
								retval = 0;
								recvFlag = 1;
							}
							else
							{
								retval = -1;
							}
					   }

						if((buff != NULL) && (buffSize != 0) && (buffSize >=  sizeof(strResponse)))
						{
							memcpy(buff, strResponse, readLen);
						}
                    }
                    while ((readLen > 0) && (sizeof(strResponse) == readLen));
                }
                else
                {
                    printf("at_fd is missed\n");
                }
                break;
        }

        if (recvFlag == 1)
        {
            break;
        }
	}
	
    return retval;
}

/*****************************************************************************
* Function Name : getModuleRevision
* Description   : 获取模块版本
* Input			: char *pBuff;
*                 int size;
* Output        : None
* Return        : -1: failed; 0: return error,
*                 other: return data length.
* Auther        : simcom
* Date          : 2018.04.14
*****************************************************************************/
int getModuleRevision(char *pBuff, int size)
{
	int retval;
	uint8_t i;
	char strResult[128];
	char *pos = NULL;

	for (i = 0; i < 3; i++)
	{
		memset(strResult, 0, sizeof(strResult));
		retval = sendATCmd((char*)"ATI", (char*)"OK", strResult, sizeof(strResult), 2000);
		if (retval > 0)
		{
			pos = strstr(strResult, "Revision: ");
			if (pos != NULL)
			{
				pos += strlen("Revision: ");
				for (i = 0; i < size; i++)
				{
					if (*pos == '\r')
						break;
					*pBuff++ =  *pos++;
				}
			}
			
			break;
		}
	}

	return retval;
}

/*****************************************************************************
* Function Name : getCPIN
* Description   : 查询是否插入SIM�?
* Input			: None
* Output        : None
* Return        : -1: failed; 0: return error,
*                 other: return data length.
* Auther        : simcom
* Date          : 2018.04.14
*****************************************************************************/
int getCPIN()
{
	int i;
	int retval = -1;
	char strResult[100];

	memset(strResult, 0, sizeof(strResult));

	for (i = 0; i < 5; i++)
	{
		memset(strResult, 0, sizeof(strResult));
		retval = sendATCmd((char*)"AT+CPIN?", (char*)"+CPIN: READY", strResult, sizeof(strResult), 5000);
		printf("111 %d\n", retval);
		if (retval > 0)
		{
			printf("strResult:%s,  retval:%d \n",strResult, retval);
			break;
		}
	}
	return retval;
}

/*****************************************************************************
* Function Name : getCSQ
* Description   : 查询信号函数
* Input			: uint8_t *rssi
* Output        : None
* Return        : -1: failed,
*                 other: return data length.
* Auther        : simcom
* Date          : 2018.04.14
*****************************************************************************/
int getCSQ(uint8_t *rssi)
{
	int retval = -1;
	char *pos = NULL;
	uint8_t i, j;
	char rspStr[100];
	char result[4];

	memset(rspStr, 0, sizeof(rspStr));
	retval = sendATCmd((char*)"AT+CSQ", (char*)"OK", rspStr, sizeof(rspStr), 2000);
	if(retval > 0)
	{
		pos = strstr(rspStr, "+CSQ: ");
		if (pos != NULL)
		{
			pos += strlen("+CSQ: ");

			memset(result, 0, sizeof(result));
			for (i = 0; i < sizeof(result) - 1; i++)
			{
				if ((*pos != ',') && (*pos != '\0') && (*pos != '\r'))
				{
					result[i] =  *pos;
				}
				else
				{
					pos++;
					break;
				}
				pos++;
			}
			*rssi = atoi(result);

		}
	}

	return retval;
}

/*****************************************************************************
* Function Name : getCREG
* Description   : 查询网络注册函数
* Input			: None
* Output        : None
* Return        : regState
* Auther        : simcom
* Date          : 2018.04.14
*****************************************************************************/
int getCREG()
{
	int retval;
	char strResult[100];
	char *pos = NULL;
	uint8_t i,j;
	
	char state[4];
	uint32_t regState = 4;

	memset(strResult, 0, sizeof(strResult));
	retval = sendATCmd((char*)"AT+CREG?", (char*)"OK", strResult, sizeof(strResult), 2000);
	if (retval > 0)
	{
		pos = strstr(strResult, "+CREG: ");
		if (pos != NULL)
		{
			pos += strlen("+CREG: ");

			for (i = 0; i < 2; i++)
			{
				memset(state, 0, sizeof(state));
				for (j = 0; j < sizeof(state) - 1; j++)
				{
					if ((*pos != ',') && (*pos != '\0') && (*pos != '\r'))
					{
						state[j] =  *pos;
					}
					else
					{
						pos++;
						break;
					}
					pos++;
				}
				if (i == 1)
					regState = atoi(state);
			}
		}
	}

	return regState;
}

/*****************************************************************************
* Function Name : getICCID
* Description   : 查询ICCID函数
* Input			: char *pBuff
*                 int size
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : simcom
* Date          : 2018.04.14
*****************************************************************************/
int getICCID(char *pBuff, int size)
{
	uint8_t i;
	int retval;
	char strResult[100];
	char *pos = NULL;

	if(size < 20)
	{
		printf("passed the memory space is too small!\n ");
		return -1;
	}
	
	for (i = 0; i < 3; i++)
	{
		memset(strResult, 0, sizeof(strResult));
		retval = sendATCmd((char*)"AT+CICCID", (char*)"OK", strResult, sizeof(strResult), 2000); //AT+QCCID
		if (retval > 0)
		{
			pos = strstr(strResult, "+ICCID: ");
			
			if (pos != NULL)
			{
				pos += strlen("+ICCID: ");
				for (i = 0; i < size; i++)
				{
					if (*pos == '\r')
						break;
					*pBuff++ =  *pos++;
				}
			}
		
			break;
		}
	}

	return 0;
}

/*****************************************************************************
* Function Name : getIMEI
* Description   : 查询IMEI
* Input			: char *pBuff
*                 int size
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : simcom
* Date          : 2018.04.14
*****************************************************************************/
int getIMEI(char *pBuff, int size)
{
	uint8_t i;
	int retval;
	char strResult[100];
	char *pos = NULL;

	if(size < 100)
	{
		printf("passed the memory space is too small!\n ");
		return -1;
	}

	for (i = 0; i < 3; i++)
	{
		memset(strResult, 0, sizeof(strResult));
		retval = sendATCmd((char*)"AT+CGSN", (char*)"OK", strResult, sizeof(strResult), 3000);  //AT+GSN
		if (retval > 0)
		{
			pos = strstr(strResult, "\r\n");
			
			if (pos != NULL)
			{
				pos += strlen("\r\n");
				for (i = 0; i < size; i++)
				{
					if (*pos == '\r')
						break;
					*pBuff++ =  *pos++;
				}
			}
		
			break;
		}
	}

	return 0;
}

/*****************************************************************************
* Function Name : getCIMI
* Description   : 查询CIMI
* Input			: char *pBuff
*                 int size
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : simcom
* Date          : 2014.04.14
*****************************************************************************/
int getCIMI(char *pBuff, int size)
{
	uint8_t i;
	int retval;
	char strResult[100];
	char *pos = NULL;

	if(pBuff == NULL)
	{
		printf("pBuff error!\n ");
		return -1;
	}

	for (i = 0; i < 3; i++)
	{
		memset(strResult, 0, sizeof(strResult));
		retval = sendATCmd((char*)"AT+CIMI", (char*)"OK", strResult, sizeof(strResult), 3000);  //AT+GSN
		if (retval > 0)
		{
			pos = strstr(strResult, "\r\n");
			
			if (pos != NULL)
			{
				pos += strlen("\r\n");
				for (i = 0; i < size; i++)
				{
					if (*pos == '\r')
						break;
					*pBuff++ =  *pos++;
				}
			}
		
			break;
		}
	}

	return 0;
}


/*****************************************************************************
* Function Name : audioPlayTTS
* Description   : 语音播报
* Input			: char *tts
* Output        : None
* Return        : 0:success, -1:faild
* Auther        : simcom
* Date          : 2018.04.16
*****************************************************************************/
int audioPlayTTS(char *tts)
{
    int retval;

    if (strlen(tts) > 2000)
        return  -1;

    char *ttsCmd = (char*)malloc(2 *1024);
    if (ttsCmd == NULL)
    {
        printf("malloc ttsCmd failed.\n");
        return  -1;
    }
    memset(ttsCmd, 0, 2*1024);

    snprintf(ttsCmd, sizeof(char) *1024, "AT+CTTS=2,\"%s\"", tts);  //AT+QTTS=2
	
    printf("ttsCmd:%s",ttsCmd);
    retval = sendATCmd(ttsCmd, (char*)"OK", NULL, 0, 5000);

    if (ttsCmd != NULL)
        free(ttsCmd);

    return retval;
}
