/*
  SIMCOM SIM7600 QMI PROFILE DEMO
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#include "DMSControl.h"
#define LOG printf
#define DMS_QMI_TIMEOUT_VALUE  10000

static qmi_idl_service_object_type  dms_service_object;
static qmi_client_type              dms_svc_client;
static qmi_client_os_params         dms_os_params;
static qmi_client_type              dms_notifier;

static qmi_client_type  cur_dms_svc_client;

static int dms_qmi_init()
{
    unsigned int num_services = 0, num_entries = 0;
    qmi_client_error_type qmi_error = QMI_NO_ERR;
    int num_retries = 0;
    dms_svc_client = NULL;

    dms_service_object = dms_get_service_object_v01();
    if(dms_service_object == NULL)
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
        qmi_error = qmi_client_init_instance(dms_service_object,
                                               QMI_CLIENT_INSTANCE_ANY,
                                               NULL,
                                               NULL,
                                               &dms_os_params,
                                               (int) 4,
                                               &dms_svc_client);
        num_retries++;
    } while ( (qmi_error != QMI_NO_ERR) && (num_retries < 2) );


    if ( qmi_error != QMI_NO_ERR )
    {
        dms_service_object = NULL;
        return -1;
    }
    return 0;
}


void dms_deinit()
{
    qmi_client_release(cur_dms_svc_client);
    cur_dms_svc_client = NULL;
}

int dms_init()
{
    int ret;

    if(cur_dms_svc_client != NULL)
    {
        return 0;
    }

    ret = dms_qmi_init();

    if(ret != 0)
    {
        printf("dms_qmi_init fail!\n");
        return -1;
    }

    cur_dms_svc_client = dms_svc_client;
    return 0;
}



int get_imei(char *pImei)
{
    qmi_client_error_type qmi_error = QMI_NO_ERR;
    dms_get_device_serial_numbers_req_msg_v01   req_msg;
    dms_get_device_serial_numbers_resp_msg_v01  resp_msg;
    int  index, i;

    memset(&req_msg, 0, sizeof(dms_get_device_serial_numbers_req_msg_v01));
    memset(&resp_msg, 0, sizeof(dms_get_device_serial_numbers_resp_msg_v01));

    if(pImei == NULL || cur_dms_svc_client == NULL)
    {
        return -1;
    }

    qmi_error = qmi_client_send_msg_sync(cur_dms_svc_client,
                                       QMI_DMS_GET_DEVICE_SERIAL_NUMBERS_REQ_V01,
                                       (void*)&req_msg,
                                       sizeof(dms_get_device_serial_numbers_req_msg_v01),
                                       (void*)&resp_msg,
                                       sizeof(dms_get_device_serial_numbers_resp_msg_v01),
                                       DMS_QMI_TIMEOUT_VALUE); 

    if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
       ( qmi_error != QMI_NO_ERR ) ||
       ( resp_msg.resp.result != QMI_NO_ERR ) )
    {
        printf("get imei  fail %d : %d\n",qmi_error, resp_msg.resp.error);
        return -1;
    }

    if(!resp_msg.imei_valid)
    {
        printf("get imei fail, imei_valid == 0\n");
        return -1;
    }


    strcpy(pImei, resp_msg.imei);

    return 0;
}

int get_meid(char *pMeid)
{
    qmi_client_error_type qmi_error = QMI_NO_ERR;
    dms_get_device_serial_numbers_req_msg_v01   req_msg;
    dms_get_device_serial_numbers_resp_msg_v01  resp_msg;
    int  index, i;

    memset(&req_msg, 0, sizeof(dms_get_device_serial_numbers_req_msg_v01));
    memset(&resp_msg, 0, sizeof(dms_get_device_serial_numbers_resp_msg_v01));

    if(pMeid == NULL || cur_dms_svc_client == NULL)
    {
        return -1;
    }

    qmi_error = qmi_client_send_msg_sync(cur_dms_svc_client,
                                       QMI_DMS_GET_DEVICE_SERIAL_NUMBERS_REQ_V01,
                                       (void*)&req_msg,
                                       sizeof(dms_get_device_serial_numbers_req_msg_v01),
                                       (void*)&resp_msg,
                                       sizeof(dms_get_device_serial_numbers_resp_msg_v01),
                                       DMS_QMI_TIMEOUT_VALUE); 

    if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
       ( qmi_error != QMI_NO_ERR ) ||
       ( resp_msg.resp.result != QMI_NO_ERR ) )
    {
        printf("get meid  fail %d : %d\n",qmi_error, resp_msg.resp.error);
        return -1;
    }

    if(!resp_msg.meid_valid)
    {
        printf("get meid fail, meid_valid == 0\n");
        return -1;
    }
    strcpy(pMeid, resp_msg.meid);

    return 0;
}


int get_mac_address_from_nv(dms_device_mac_enum_v01 device_type, uint8_t *mac_addr)
{
    qmi_client_error_type qmi_error = QMI_NO_ERR;
    dms_get_mac_address_req_msg_v01 req_msg;
    dms_get_mac_address_resp_msg_v01 resp_msg;
    memset(&req_msg, 0, sizeof(dms_get_mac_address_req_msg_v01)); 
    memset(&resp_msg, 0, sizeof(dms_get_mac_address_resp_msg_v01));
    req_msg.device = device_type;
    int i;

    if(mac_addr == NULL)
    {
        return -1;
    }
    qmi_error = qmi_client_send_msg_sync(cur_dms_svc_client,
                                        QMI_DMS_GET_MAC_ADDRESS_REQ_V01,
                                        (void*)&req_msg,
                                        sizeof(dms_get_mac_address_req_msg_v01),
                                        (void*)&resp_msg,
                                        sizeof(dms_get_mac_address_resp_msg_v01),
                                        DMS_QMI_TIMEOUT_VALUE);    

    if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
       ( qmi_error != QMI_NO_ERR ) ||
       ( resp_msg.resp.result != QMI_NO_ERR ) )
    {
        printf("get mac address  fail %d : %d\n",qmi_error, resp_msg.resp.error);
        return -1;
    }

    if(resp_msg.mac_address_len != 6)
    {
        printf("mac addr len != 6, error\n");
        return -1;
    }

    for(i = 0; i < 6; i++)
    {
        mac_addr[i] = resp_msg.mac_address[i];
    }

    printf("type: %d, address: %02X:%02X:%02X:%02X:%02X:%02X", 
                              mac_addr[0],
                              mac_addr[1],
                              mac_addr[2],
                              mac_addr[3],
                              mac_addr[4],
                              mac_addr[5]);
    return 0;                         

}

/*
get firmware revision identification
*/
int get_rev_id(char *pRev_id)
{
    qmi_client_error_type qmi_error = QMI_NO_ERR;
    dms_get_device_rev_id_req_msg_v01   req_msg;
    dms_get_device_rev_id_resp_msg_v01  resp_msg;
    int  index, i;

    memset(&req_msg, 0, sizeof(dms_get_device_rev_id_req_msg_v01));
    memset(&resp_msg, 0, sizeof(dms_get_device_rev_id_resp_msg_v01));

    if(pRev_id == NULL || cur_dms_svc_client == NULL)
    {
        return -1;
    }

    qmi_error = qmi_client_send_msg_sync(cur_dms_svc_client,
                                       QMI_DMS_GET_DEVICE_REV_ID_REQ_V01,
                                       (void*)&req_msg,
                                       sizeof(dms_get_device_rev_id_req_msg_v01),
                                       (void*)&resp_msg,
                                       sizeof(dms_get_device_rev_id_resp_msg_v01),
                                       DMS_QMI_TIMEOUT_VALUE); 

    if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
       ( qmi_error != QMI_NO_ERR ) ||
       ( resp_msg.resp.result != QMI_NO_ERR ) )
    {
        printf("get Rev_id  fail %d : %d\n",qmi_error, resp_msg.resp.error);
        return -1;
    }


    strcpy(pRev_id, resp_msg.device_rev_id);

    return 0;
}


/*
get firmware revision identification
*/
int dms_set_operating_mode(unsigned char mode)
{
    qmi_client_error_type qmi_error = QMI_NO_ERR;
    dms_set_operating_mode_req_msg_v01   req_msg;
    dms_set_operating_mode_resp_msg_v01  resp_msg;
    int  index, i;

    memset(&req_msg, 0, sizeof(dms_set_operating_mode_req_msg_v01));
    memset(&resp_msg, 0, sizeof(dms_set_operating_mode_resp_msg_v01));

    if(cur_dms_svc_client == NULL)
    {
        return -1;
    }
    req_msg.operating_mode = mode;
    qmi_error = qmi_client_send_msg_sync(cur_dms_svc_client,
                                       QMI_DMS_SET_OPERATING_MODE_REQ_V01,
                                       (void*)&req_msg,
                                       sizeof(dms_set_operating_mode_req_msg_v01),
                                       (void*)&resp_msg,
                                       sizeof(dms_set_operating_mode_resp_msg_v01),
                                       DMS_QMI_TIMEOUT_VALUE); 

    if ( ( qmi_error == QMI_TIMEOUT_ERR ) ||
       ( qmi_error != QMI_NO_ERR ) ||
       ( resp_msg.resp.result != QMI_NO_ERR ) )
    {
        printf("set operating_mode  fail %d : %d\n",qmi_error, resp_msg.resp.error);
        return -1;
    }

    return 0;
}





