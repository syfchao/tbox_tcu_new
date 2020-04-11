/*
  SIMCOM SIM7600 QMI PROFILE DEMO
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#include "UIMControl.h"
#define LOG printf
#define UIM_QMI_TIMEOUT_VALUE  10000

static qmi_idl_service_object_type uim_service_object;
static qmi_client_type 			uim_svc_client;
static qmi_client_os_params		uim_os_params;
static qmi_client_type 			uim_notifier;

static qmi_client_type  cur_uim_svc_client;

static char qcmap_client_bin_to_hexchar(uint8_t ch)
{
    if (ch < 0x0a)
    {
        return (ch + '0');
    }
    return (ch + 'a' - 10);
}

static void qcmap_client_flip_data ( uint8_t* des, const uint8_t* src, uint32_t data_len)
{ 
    unsigned int i  = 0;
    unsigned short   temp_path = 0;
    for(i = 0; i < (data_len + 1) && i <= 2 ; i += 2)
    { 
        temp_path = (*(src + i) << 8) | (*(src + i + 1));
        memcpy(des, (unsigned char*)&temp_path, sizeof(unsigned short));
        des += sizeof(unsigned short);
    } 
} /* qcril_uim_qmi_flip_data */

static char * qcmap_client_parse_gw_imsi ( const unsigned char * raw_imsi_ptr, unsigned short raw_imsi_len, int* parsed_imsi_len_ptr ) 
{
    int src = 0;
    int dst = 0;
    char* parsed_imsi_ptr = NULL;
    
    /* Sanity check on input parameters */
    if ((raw_imsi_ptr == NULL) || (parsed_imsi_len_ptr == NULL))
    { 
        return NULL; 
    }
    /* Check for the length of IMSI bytes in the first byte */
    *parsed_imsi_len_ptr = *raw_imsi_ptr;
    if (*parsed_imsi_len_ptr >= raw_imsi_len)
    {
        *parsed_imsi_len_ptr = 0;
        return NULL; 
    }
    /* Allocate required amount of memory for IMSI in ASCII string format, note that it is freed by the caller */ 
    parsed_imsi_ptr = (char *)malloc((2 * (*parsed_imsi_len_ptr)));
    if (parsed_imsi_ptr == NULL)
    {
        *parsed_imsi_len_ptr = 0;
        return NULL; 
    }
    /* Compose IMSI */
    memset(parsed_imsi_ptr, 0, (2 * (*parsed_imsi_len_ptr)));
    for (src = 1, dst = 0; (src <= (*parsed_imsi_len_ptr)) && (dst < ((*parsed_imsi_len_ptr) * 2)); src++) 
    { 
        /* Only process lower part of byte for second and subsequent bytes */
        if (src > 1)
        { 
            parsed_imsi_ptr[dst] = qcmap_client_bin_to_hexchar(raw_imsi_ptr[src] & 0x0F); 
            dst++; 
        }
        /* Process upper part of byte for all bytes */
        parsed_imsi_ptr[dst] = qcmap_client_bin_to_hexchar(raw_imsi_ptr[src] >> 4); 
        dst++; 
    }
    /* Update IMSI length in bytes - parsed IMSI in ASCII is raw times 2 */
    *parsed_imsi_len_ptr *= 2;
    return parsed_imsi_ptr; 
}

static int uim_qmi_init()
{
    unsigned int num_services = 0, num_entries = 0;
    qmi_client_error_type qmi_error = QMI_NO_ERR;
    int num_retries = 0;
	uim_svc_client = NULL;
    
    uim_service_object = uim_get_service_object_v01();
    if(uim_service_object == NULL)
    {
        return -1;
    }
    
    do
    {
        if ( num_retries != 0) 
        {
            sleep(1);
            LOG("qmi_client_init_instance status retry : %d", num_retries);
        }

        LOG("qmi_client_init_instance....\n ");
        qmi_error = qmi_client_init_instance(uim_service_object,
	                                           QMI_CLIENT_INSTANCE_ANY,
	                                           NULL,
	                                           NULL,
	                                           &uim_os_params,
	                                           (int) 4,
	                                           &uim_svc_client);
        num_retries++;
    } while ( (qmi_error != QMI_NO_ERR) && (num_retries < 2) );


    if ( qmi_error != QMI_NO_ERR )
    {
        uim_service_object = NULL;
        return -1;
    }	
    return 0;
}

  

int uim_init()
{
	int ret;

	if(cur_uim_svc_client != NULL)
	{
		return 0;
	}
	
	ret = uim_qmi_init();
	
	if(ret != 0)
	{
		printf("uim_qmi_init fail!\n");
		return -1;
	}
	
	cur_uim_svc_client = uim_svc_client;
	return 0;
}

void uim_deinit()
{
	qmi_client_release(cur_uim_svc_client);
    cur_uim_svc_client = NULL;
}


int getSimCardStatus(SimCard_Status_type *simStatus)
{
	qmi_client_error_type qmi_error = QMI_NO_ERR;
	uim_get_card_status_req_msg_v01   req_msg;
	uim_get_card_status_resp_msg_v01  resp_msg;
	int  index, i;
	
	memset(&req_msg, 0, sizeof(uim_get_card_status_req_msg_v01));
	memset(&resp_msg, 0, sizeof(uim_get_card_status_resp_msg_v01));
	
	if(cur_uim_svc_client == NULL)
	{
		return -1;
	}
	
	qmi_error = qmi_client_send_msg_sync(cur_uim_svc_client,
	                                   QMI_UIM_GET_CARD_STATUS_REQ_V01,
	                                   (void*)&req_msg,
	                                   sizeof(uim_get_card_status_req_msg_v01),
	                                   (void*)&resp_msg,
	                                   sizeof(uim_get_card_status_resp_msg_v01),
	                                   UIM_QMI_TIMEOUT_VALUE); 

	if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
	   ( qmi_error != QMI_NO_ERR ) ||
	   ( resp_msg.resp.result != QMI_NO_ERR ) )
	{
		LOG("qiu uim Can not get card status  %d : %d\n",qmi_error, resp_msg.resp.error, 0);
	    return -1;
	}

	if(!resp_msg.card_status_valid)
	{
		return -1;
	}
		
	if(resp_msg.card_status.card_info_len == 1)
	{
		index = 0;
	}

	if(resp_msg.card_status.card_info[0].app_info_len > 1)
	{
		for(i = 0; i < resp_msg.card_status.card_info_len; i++)
		{
			if(resp_msg.card_status.card_info[0].app_info[i].app_type == 2)   //USIM
			{
				index = i;
			}
		}
	}

	simStatus->card_status = resp_msg.card_status.card_info[0].card_state;
	simStatus->app_type = resp_msg.card_status.card_info[0].app_info[index].app_type;
	simStatus->app_state = resp_msg.card_status.card_info[0].app_info[index].app_state;
	simStatus->pin = resp_msg.card_status.card_info[0].app_info[index].pin1;
  	return 0;	
}

int get_iccid(char *pIccid)
{
	qmi_client_error_type qmi_error = QMI_NO_ERR;
	uim_get_slots_status_req_msg_v01   req_msg;
	uim_get_slots_status_resp_msg_v01  resp_msg;
	int  index, i;
	
	memset(&req_msg, 0, sizeof(uim_get_slots_status_req_msg_v01));
	memset(&resp_msg, 0, sizeof(uim_get_slots_status_resp_msg_v01));
	
	if(pIccid == NULL || cur_uim_svc_client == NULL)
	{
		return -1;
	}
	
	qmi_error = qmi_client_send_msg_sync(cur_uim_svc_client,
	                                   QMI_UIM_GET_SLOTS_STATUS_REQ_V01,
	                                   (void*)&req_msg,
	                                   sizeof(uim_get_slots_status_req_msg_v01),
	                                   (void*)&resp_msg,
	                                   sizeof(uim_get_slots_status_resp_msg_v01),
	                                   UIM_QMI_TIMEOUT_VALUE); 

	if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
	   ( qmi_error != QMI_NO_ERR ) ||
	   ( resp_msg.resp.result != QMI_NO_ERR ) )
	{
		LOG("qiu uim Can not get slot status  %d : %d\n",qmi_error, resp_msg.resp.error, 0);
	    return -1;
	}

	if(!resp_msg.physical_slot_status_valid || resp_msg.physical_slot_status_len < 1)
	{
		printf("get slot stauts invalid\n");
		return -1;
	}

	if(resp_msg.physical_slot_status[0].iccid_len < 1)
	{
		printf("get slot iccid len invalid\n");
		return -1;		
	}

	for(i = 0; i < resp_msg.physical_slot_status[0].iccid_len; i++)
	{
		pIccid[i * 2]     = (resp_msg.physical_slot_status[0].iccid[i] & 0x0F) + 0x30;
		pIccid[i * 2 + 1] = ((resp_msg.physical_slot_status[0].iccid[i] >> 4) & 0x0F) + 0x30;
	}

  	return 0;	
}



int get_imsi(char *imsi)
{
    qmi_client_error_type qmi_error, qmi_err_code = QMI_NO_ERR;
    boolean ret = TRUE;

    uim_read_transparent_req_msg_v01  req_msg; 
    uim_read_transparent_resp_msg_v01 resp_msg;
    qmi_client_error_type rc;
    int imsi_len = 0;
    uint8_t gw_path[] = {0x3F, 0x00, 0x7F, 0xFF, 0x00}; //{0x3F, 0x00, 0x7F, 0x20, 0x00}; 
    uint8_t cdma_path[] = {0x3F, 0x00, 0x7F, 0x25, 0x00};

    char *ptr_imsi = NULL;
    
    memset(&req_msg, 0, sizeof(uim_read_transparent_req_msg_v01)); 
    memset(&resp_msg, 0, sizeof(uim_read_transparent_resp_msg_v01)); 
    
    req_msg.session_information.session_type = UIM_SESSION_TYPE_PRIMARY_GW_V01; 
    req_msg.session_information.aid_len = 0;
    req_msg.file_id.file_id = 0x6F07;
    req_msg.file_id.path_len = 4;
    qcmap_client_flip_data(req_msg.file_id.path, (uint8_t*)gw_path,  req_msg.file_id.path_len);

	qmi_error = qmi_client_send_msg_sync(cur_uim_svc_client,
	                                   QMI_UIM_READ_TRANSPARENT_REQ_V01,
	                                   (void*)&req_msg,
	                                   sizeof(uim_read_transparent_req_msg_v01),
	                                   (void*)&resp_msg,
	                                   sizeof(uim_read_transparent_resp_msg_v01),
	                                   UIM_QMI_TIMEOUT_VALUE);    
    if (QMI_NO_ERR != qmi_error)
    { 
        printf("qmi read transparent err=%d\n", rc); 
        return -1;
    }
    else if (QMI_NO_ERR != resp_msg.resp.result)
    { 
        printf("qmi read transparent: failed response err=%d\n", resp_msg.resp.error);
        return -1;
    }
    else
    { 
        //LOG("ok here\n"); }
        ptr_imsi = qcmap_client_parse_gw_imsi( resp_msg.read_result.content,
                     resp_msg.read_result.content_len,
                     &imsi_len); 
    }

    if(ptr_imsi != NULL)
    {
        memcpy(imsi, ptr_imsi,strlen(ptr_imsi));
        free(ptr_imsi);
    }
    else
    {
        printf("get imsi fail\n");
        return -1;
    }

    return 0;

}





