/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT Platform
 * Author: HiSpark Product Team.
 * Create: 2020-5-20
 */

#include "iot_config.h"
#include "iot_log.h"
#include "iot_main.h"
#include "iot_profile.h"
#include <hi_task.h>
#include <string.h>
#include <global_config.h>
#include <hi_wifi_api.h>
#include <hi_mux.h>

extern char gps_data[30];
/*attribute initiative to report */
#define TAKE_THE_INITIATIVE_TO_REPORT
/*oc request id*/
#define CN_COMMADN_INDEX                    "commands/request_id="

///< this is the callback function, set to the mqtt, and if any messages come, it will be called
///< The payload here is the json string
static void DemoMsgRcvCallBack(int qos, const char *topic, const char *payload)
{
    const char *requesID;
    char *tmp;
    IoTCmdResp_t resp;
    IOT_LOG_DEBUG("RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n",qos,topic, payload);
    /*app 下发的操作*/
    // if (strstr(payload, TRAFFIC_LIGHT_SERVICE_ID_PAYLOAD) != NULL) {        //traffic light module
            // oc_traffic_light_app_option(g_current_mode, g_current_type);
    // }
    tmp = strstr(topic,CN_COMMADN_INDEX);
    if(tmp != NULL){
        ///< now you could deal your own works here --THE COMMAND FROM THE PLATFORM

            ///< now er roport the command execute result to the platform
            requesID = tmp + strlen(CN_COMMADN_INDEX);
            resp.requestID = requesID;
            resp.respName = NULL;
            resp.retCode = 0;   ////< which means 0 success and others failed
            resp.paras = NULL;
            // IOT_LOG_DEBUG("respose paras------------------------------ = %s\r\n", resp.paras);
            (void)IoTProfileCmdResp(CONFIG_DEVICE_PWD,&resp);
    }
    return;
}

/*Smart Can*/
hi_void iot_publish_persion_time(hi_u32 time)
{
      IoTProfileService_t service;
      IoTProfileKV_t property;
  
      memset(&property, 0, sizeof(property));
      property.type = EN_IOT_DATATYPE_INT;
      property.key = "Person_times";
     
      property.i_value = time;
    
      memset(&service, 0,sizeof(service));
      service.serviceID = "SmartTrashCan";
      service.serviceProperty = &property;
  
      IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}

hi_void iot_publish_tarsh_is_full(char * string)
{
      IoTProfileService_t service;
      IoTProfileKV_t property;
  
      memset(&property, 0, sizeof(property));
      property.type = EN_IOT_DATATYPE_STRING;
      property.key = "full";
    
      property.value = string;
    
      memset(&service, 0,sizeof(service));
      service.serviceID = "SmartTrashCan";
      service.serviceProperty = &property;
  
      IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}

hi_void iot_publish_tarsh_is_dumped(char * string)
{
      IoTProfileService_t service;
      IoTProfileKV_t property;
  
      memset(&property, 0, sizeof(property));
      property.type = EN_IOT_DATATYPE_STRING;
      property.key = "dumped";
      property.value = string;

      memset(&service, 0,sizeof(service));
      service.serviceID = "SmartTrashCan";
      service.serviceProperty = &property;
  
      IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}

hi_void iot_publish_tarsh_gps(char * string)
{
      IoTProfileService_t service;
      IoTProfileKV_t property;
  
      memset(&property, 0, sizeof(property));
      property.type = EN_IOT_DATATYPE_STRING;
      property.key = "gps";     
      property.value = string;    
      memset(&service, 0,sizeof(service));
      service.serviceID = "SmartTrashCan";
      service.serviceProperty = &property;  
      IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}

///< this is the demo main task entry,here we will set the wifi/cjson/mqtt ready ,and 
///< wait if any work to do in the while
static hi_void *DemoEntry(hi_void *arg)
{
    // IoTProfileService_t service;
    // IoTProfileKV_t property;
    extern hi_void WifiStaReadyWait(hi_void);
    WifiStaReadyWait();

    extern void cJsonInit(void);
    cJsonInit();
    
    IoTMain();
    
    /*云端下发*/
    // IoTSetMsgCallback(DemoMsgRcvCallBack);
/*主动上报*/
#ifdef TAKE_THE_INITIATIVE_TO_REPORT
    while (1) {
        // hi_mux_pend(g_mux_id, HI_SYS_WAIT_FOREVER);
        ///< here you could add your own works here--we report the data to the IoTplatform
        ///< now we report the data to the iot platform
       // ///< here you could add your own works here--we report the data to the IoTplatform
       if(strlen(gps_data)) {
            /*等wifi 连上热点之后在接收gps数据*/
           if (wifi_status == HI_WIFI_EVT_CONNECTED && mqtt_connect_success == HI_TRUE ) {
                printf("gps data uploading\r\n");
                iot_publish_tarsh_gps(gps_data);
                printf("<publish success>:Update of GPS position information\r\n");
           }
       }
       hi_sleep(1000);
    }
#endif
    return NULL;
}

///< This is the demo entry, we create a task here, and all the works has been done in the demo_entry
#define CN_IOT_TASK_STACKSIZE  0x1000   
#define CN_IOT_TASK_PRIOR 25
#define CN_IOT_TASK_NAME "IOTDEMO"
int app_demo_iot(void)
{
    hi_u32 ret;
    hi_task_attr attr = {0};
    hi_u32 taskID;

    attr.stack_size = CN_IOT_TASK_STACKSIZE;
    attr.task_prio = CN_IOT_TASK_PRIOR;
    attr.task_name = CN_IOT_TASK_NAME;
    ret = hi_task_create(&taskID, &attr, DemoEntry, NULL);
    if (ret != HI_ERR_SUCCESS) {
       IOT_LOG_ERROR("IOT TASK CTREATE FAILED");
    }
    return 0;
}
