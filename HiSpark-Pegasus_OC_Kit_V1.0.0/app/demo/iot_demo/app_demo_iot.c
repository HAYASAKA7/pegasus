/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT Platform
 * Author: HiSpark Product Team
 * Create: 2020-5-20
 */

#include "iot_config.h"
#include "iot_log.h"
#include "iot_main.h"
#include "iot_profile.h"
#include <hi_task.h>
#include <string.h>
#include <app_demo_traffic_sample.h>


extern hi_u8 g_current_mode;
extern hi_u8 g_current_type;
extern hi_u8 g_someone_walking_flag;
extern hi_u8 g_with_light_flag;
extern hi_u8 g_menu_select;
extern hi_float g_temperature;
extern hi_float g_humidity;
extern hi_float g_combustible_gas_value ;
extern hi_u8 g_ahu20_temperature_buff[6]; 
extern hi_u8 g_ahu20_humidity_buff[6];
extern hi_u8 g_ahu20_gas_buff[6];

/*report traffic light count*/
extern hi_u32 g_red_led_auto_modu_time_count;
extern hi_u32 g_yellow_led_auto_modu_time_count;
extern hi_u32 g_green_led_auto_modu_time_count;
extern hi_u32 g_red_led_human_modu_time_count;
extern hi_u32 g_yellow_led_human_modu_time_count;
extern hi_u32 g_green_led_human_modu_time_count;
hi_u8 oc_beep_status = BEEP_OFF;
/*attribute initiative to report */
#define TAKE_THE_INITIATIVE_TO_REPORT
/*oc request id*/
#define CN_COMMADN_INDEX                    "commands/request_id="
/*oc report HiSpark attribute*/
#define COLORFUL_LIGHT_SERVICE_ID_PAYLOAD   "ColorfulLight"
#define COLORFUL_LIGHT_CONTROL_MODE_CMD     "ControlModule"
#define COLORFUL_LIGHT_COLORFUL_MODE_CMD    "ColorfulLightModule" 
#define COLORFUL_LIGHT_PWM_CONTROL_CMD      "PWM_Module"
#define COLORFUL_LIGHT_BRIGHTNESS_CMD       "BrightnessModule"
#define COLORFUL_LIGHT_HUMANDETECT_CMD      "HumanDetectModule"
#define COLORFUL_LIGHT_LIGHTDETECT_CMD      "LightDetectModule"     
#define COLORFUL_LIGHT_UNIONDETECT_CMD      "UnionDetectModule"
#define COLORFUL_LIGHT_BLUE_ON_PAYLOAD      "BLUE_LED_ON"
#define COLORFUL_LIGHT_RED_ON_PAYLOAD       "RED_LED_ON"
#define COLORFUL_LIGHT_GREEN_ON_PAYLOAD     "GREEN_LED_ON"
#define COLORFUL_LIGHT_PERIOD_1_S           "PERIOD_1_S"  
#define COLORFUL_LIGHT_PERIOD_05_S          "PERIOD_05_S"
#define COLORFUL_LIGHT_PERIOD_025_S         "PERIOD_025_S"  
#define COLORFUL_LIGHT_PWM_RED_LED_ON       "PWM_RED_LED_ON"
#define COLORFUL_LIGHT_PWM_GREEN_LED_ON     "PWM_GREEN_LED_ON"
#define COLORFUL_LIGHT_PWM_BLUE_LED_ON      "PWM_BLUE_LED_ON"
#define COLORFUL_LIGHT_PWM_PURPLE_LED_ON    "PWM_PURPLE_LED_ON"
#define COLORFUL_LIGHT_PWM_TRICOLOR_LED_ON  "PWM_TRICOLOR_LED_ON"
#define COLORFUL_LIGHT_BRIGHTNESS_LOW       "BRIGHTNESS_LOW_LED_ON"
#define COLORFUL_LIGHT_BRIGHTNESS_MIDDLE    "BRIGHTNESS_MIDDLE_LED_ON"
#define COLORFUL_LIGHT_BRIGHTNESS_HIGH      "BRIGHTNESS_HIGH_LED_ON"

#define TRAFFIC_LIGHT_CMD_PAYLOAD           "led_value"
#define TRAFFIC_LIGHT_CMD_CONTROL_MODE      "ControlModule"
#define TRAFFIC_LIGHT_CMD_AUTO_MODE         "AutoModule"
#define TRAFFIC_LIGHT_CMD_HUMAN_MODE        "HumanModule"
#define TRAFFIC_LIGHT_YELLOW_ON_PAYLOAD     "YELLOW_LED_ON"
#define TRAFFIC_LIGHT_RED_ON_PAYLOAD        "RED_LED_ON"
#define TRAFFIC_LIGHT_GREEN_ON_PAYLOAD      "GREEN_LED_ON"
#define TRAFFIC_LIGHT_SERVICE_ID_PAYLOAD    "TrafficLight"
#define TRAFFIC_LIGHT_BEEP_CONTROL          "BeepControl"
#define TRAFFIC_LIGHT_BEEP_ON               "BEEP_ON" 
#define TRAFFIC_LIGHT_BEEP_OFF              "BEEP_OFF"  
#define TRAFFIC_LIGHT_HUMAN_INTERVENTION_ON     "HUMAN_MODULE_ON"
#define TRAFFIC_LIGHT_HUMAN_INTERVENTION_OFF    "HUMAN_MODULE_OFF"    

#define ENVIRONMENT_SERVICE_ID_PAYLOAD      "Environment"
#define ENVIRONMENT_TEM_HUM_GAS_MODE        "AllEnvironmentValueCMD"
#define ENVIRONMENT_TEMPERATURE_PAYLOAD     "TemperatureModuleCMD"
#define ENVIRONMENT_HUMIDITY_PAYLOAD        "HumidityModuleCMD"
#define ENVIRONMENT_COMBUSTIBLE_GAS_PAYLOAD "CombustibleGasModuleCMD"


hi_void oc_colorful_light_app_option( hi_colorful_light_mode app_option_mode, hi_control_mode_type app_option_type)
{     
    g_current_mode = app_option_mode;
    switch (g_current_mode) {
        case CONTROL_MODE:
            // setup_clean_trfl_status(TRAFFIC_CONTROL_MODE);
            oc_colorful_light_status_report(CONTROL_MODE, setup_clfl_control_module);
            break;
        case COLORFUL_LIGHT_MODE:
            // setup_clean_trfl_status(TRAFFIC_AUTO_MODE);
            oc_colorful_light_status_report(COLORFUL_LIGHT_MODE, setup_clfl_colorful_light_module);
            break;
        case PWM_CONTROL_MODE:
            // setup_clean_trfl_status(TRAFFIC_HUMAN_MODE);
            oc_colorful_light_status_report(PWM_CONTROL_MODE, setup_clfl_pwm_control_module);
            break;
        case BIRGHTNESS_MODE:
            oc_colorful_light_status_report(BIRGHTNESS_MODE, setup_clfl_brightness_module);
            break;
        case HUMAN_DETECT_MODE:
            oc_colorful_light_status_report(HUMAN_DETECT_MODE, setup_clfl_human_detect_module);
            break;
        case LIGHT_DETECT_MODE:
            oc_colorful_light_status_report(LIGHT_DETECT_MODE, setup_clfl_light_detect_module);
            break; 
        case UNION_DETECT_MODE:
            oc_colorful_light_status_report(UNION_DETECT_MODE, setup_clfl_union_detect_module);
            break;               
        default:
            break;
    }  
}

hi_void oc_traffic_light_app_option( hi_traffic_light_mode app_option_mode, hi_control_mode_type app_option_type)
{     
    g_current_mode = app_option_mode;
    switch (g_current_mode) {
        case TRAFFIC_CONTROL_MODE:
            // setup_clean_trfl_status(TRAFFIC_CONTROL_MODE);
            oc_traffic_light_status_report(TRAFFIC_CONTROL_MODE, setup_trfl_control_module);
            break;
        case TRAFFIC_AUTO_MODE:
            // setup_clean_trfl_status(TRAFFIC_AUTO_MODE);
            oc_traffic_light_status_report(TRAFFIC_AUTO_MODE, setup_trfl_auto_module);
            break;
        case TRAFFIC_HUMAN_MODE:
            // setup_clean_trfl_status(TRAFFIC_HUMAN_MODE);
            oc_traffic_light_status_report(TRAFFIC_HUMAN_MODE, setup_trfl_human_module);
            break;
        default:
            break;
    }  
}
hi_void oc_environment_app_option( hi_environment_mode app_option_mode, hi_control_mode_type app_option_type)
{     
    g_current_mode = app_option_mode;
    switch (g_current_mode) {
        case ENV_ALL_MODE:
            // setup_clean_trfl_status(TRAFFIC_CONTROL_MODE);
            oc_environment_status_report(ENV_ALL_MODE, setup_env_all_module);
            break;
        case ENV_TEMPERRATURE_MODE:
            // setup_clean_trfl_status(TRAFFIC_AUTO_MODE);
            oc_environment_status_report(ENV_TEMPERRATURE_MODE, setup_env_temperature_module);
            break;
        case ENV_HUMIDITY_MODE:
            // setup_clean_trfl_status(TRAFFIC_HUMAN_MODE);
            oc_environment_status_report(ENV_HUMIDITY_MODE, setup_env_humidity_module);
            break;
        case COMBUSTIBLE_GAS_MODE:
            oc_environment_status_report(ENV_HUMIDITY_MODE, setup_env_combustible_gas_module);
            break;    
        default:
            break;
    }  
}
///< this is the callback function, set to the mqtt, and if any messages come, it will be called
///< The payload here is the json string
static void DemoMsgRcvCallBack(int qos, const char *topic, const char *payload)
{
    const char *requesID;
    char *tmp;
    IoTCmdResp_t resp;
    IOT_LOG_DEBUG("RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n",qos,topic, payload);
    /*app 下发的操作*/
    if (strstr(payload, TRAFFIC_LIGHT_SERVICE_ID_PAYLOAD) != NULL) {        //traffic light module
        if (strstr(payload, TRAFFIC_LIGHT_CMD_CONTROL_MODE) != NULL) { 
            g_current_mode = TRAFFIC_CONTROL_MODE; 
            if (strstr(payload, TRAFFIC_LIGHT_YELLOW_ON_PAYLOAD) != NULL) { //YELLOW LED
                g_current_type = YELLOW_ON;
            } else if (strstr(payload, TRAFFIC_LIGHT_RED_ON_PAYLOAD) != NULL) {//RED LED
                g_current_type = RED_ON;
            } else if (strstr(payload, TRAFFIC_LIGHT_GREEN_ON_PAYLOAD) != NULL) { //GREEN LED
                g_current_type = GREEN_ON;
            } 
            oc_traffic_light_app_option(g_current_mode, g_current_type);

        } else if (strstr(payload, TRAFFIC_LIGHT_CMD_AUTO_MODE) != NULL) {//Auto module
            g_current_mode = TRAFFIC_AUTO_MODE; 
            g_current_type = NULL;     
            oc_traffic_light_app_option(g_current_mode, g_current_type);
        } else if(strstr(payload, TRAFFIC_LIGHT_CMD_HUMAN_MODE) != NULL) { //Human module
            g_current_mode = TRAFFIC_HUMAN_MODE; 
            if (strstr(payload ,TRAFFIC_LIGHT_HUMAN_INTERVENTION_ON) != NULL) {
                g_current_type = TRAFFIC_HUMAN_TYPE;
            } else if (strstr(payload, TRAFFIC_LIGHT_HUMAN_INTERVENTION_OFF)) {
                g_current_type = TRAFFIC_NORMAL_TYPE;
            }
            oc_traffic_light_app_option(g_current_mode, g_current_type);
        } else if (strstr(payload , TRAFFIC_LIGHT_BEEP_CONTROL) != NULL) { //BEEP option
            if (strstr(payload, TRAFFIC_LIGHT_BEEP_ON) != NULL) { //BEEP ON
                oc_beep_status = BEEP_ON;
            } else if (strstr(payload, TRAFFIC_LIGHT_BEEP_OFF) != NULL) { //BEEP OFF
                oc_beep_status = BEEP_OFF;
            }
        }

    } else if (strstr(payload, COLORFUL_LIGHT_SERVICE_ID_PAYLOAD) != NULL) { //colorful light module
        if (strstr(payload, COLORFUL_LIGHT_CONTROL_MODE_CMD) != NULL) { //control module
            g_current_mode = CONTROL_MODE; 
            if (strstr(payload, COLORFUL_LIGHT_RED_ON_PAYLOAD) != NULL) { //RED LED
                g_current_type = RED_ON;
            } else if (strstr(payload, COLORFUL_LIGHT_GREEN_ON_PAYLOAD) != NULL) { //GREEN LED
                g_current_type = YELLOW_ON;
            } else if (strstr(payload, COLORFUL_LIGHT_BLUE_ON_PAYLOAD) != NULL) { // BLUELED
                g_current_type = GREEN_ON;
            }
            oc_colorful_light_app_option(g_current_mode, g_current_type);
        } else if(strstr(payload, COLORFUL_LIGHT_COLORFUL_MODE_CMD) != NULL){ //colorful light module
            g_current_mode = COLORFUL_LIGHT_MODE;
            if (strstr(payload, COLORFUL_LIGHT_PERIOD_1_S) != NULL) { //Period 1s flash
                g_current_type = CYCLE_FOR_ONE_SECOND;
            } else if (strstr(payload, COLORFUL_LIGHT_PERIOD_05_S) != NULL) { //Period 0.5s flash
                g_current_type = CYCLE_FOR_HALF_SECOND;
            } else if (strstr(payload, COLORFUL_LIGHT_PERIOD_025_S) != NULL) { //Period 0.25s flash
                g_current_type = CYCLE_FOR_QUARATER_SECOND;
            }   
            oc_colorful_light_app_option(g_current_mode, g_current_type);
        } else if (strstr(payload, COLORFUL_LIGHT_PWM_CONTROL_CMD) != NULL) { //PWM control module
             g_current_mode = PWM_CONTROL_MODE; 
            if(strstr(payload, COLORFUL_LIGHT_PWM_RED_LED_ON) != NULL) { //RED LED
                g_current_type = PWM_CONTROL_RED;
            } else if(strstr(payload, COLORFUL_LIGHT_PWM_GREEN_LED_ON) != NULL) { //GREEN LED
                g_current_type = PWM_CONTROL_YELLOW;
            } else if (strstr(payload, COLORFUL_LIGHT_PWM_BLUE_LED_ON) != NULL) { //BLUE LED
                g_current_type = PWM_CONTROL_GREEN;
            } else if (strstr(payload, COLORFUL_LIGHT_PWM_PURPLE_LED_ON) != NULL) { //PURPLE LED
                g_current_type = PWM_CONTROL_PURPLE;
            } else if (strstr(payload, COLORFUL_LIGHT_PWM_TRICOLOR_LED_ON) != NULL) {//TRICOLOR LED
                g_current_type = PWM_CONTROL_ALL;
            }
            oc_colorful_light_app_option(g_current_mode, g_current_type);
        } else if (strstr(payload, COLORFUL_LIGHT_BRIGHTNESS_CMD) != NULL) { //Brightness module
            g_current_mode = BIRGHTNESS_MODE; 
            if (strstr(payload, COLORFUL_LIGHT_BRIGHTNESS_LOW) != NULL) {  //Brightness low
                g_current_type = BRIGHTNESS_LOW;
            } else if (strstr(payload, COLORFUL_LIGHT_BRIGHTNESS_MIDDLE) != NULL) { //Brightness middle
                g_current_type = BRIGHTNESS_MIDDLE;
            } else if (strstr(payload, COLORFUL_LIGHT_BRIGHTNESS_HIGH) != NULL) { //Brightness high
                g_current_type = BRIGHTNESS_HIGH;
            }
            oc_colorful_light_app_option(g_current_mode, g_current_type);
        } else if (strstr(payload, COLORFUL_LIGHT_HUMANDETECT_CMD) != NULL) { //Human detect module 
            g_current_mode = HUMAN_DETECT_MODE;
            g_current_type = NULL;
            oc_colorful_light_app_option(g_current_mode, g_current_type);
        } else if (strstr(payload, COLORFUL_LIGHT_LIGHTDETECT_CMD) != NULL) {//light detect module 
            g_current_mode = LIGHT_DETECT_MODE;
            g_current_type = NULL;
            oc_colorful_light_app_option(g_current_mode, g_current_type);
        } else if (strstr(payload, COLORFUL_LIGHT_UNIONDETECT_CMD) != NULL) {//union detect module 
            g_current_mode = UNION_DETECT_MODE;
            g_current_type = NULL;
            oc_colorful_light_app_option(g_current_mode, g_current_type);
        }
         
    } else if (strstr(payload, ENVIRONMENT_SERVICE_ID_PAYLOAD) != NULL) {//environment test
        if (strstr(payload, ENVIRONMENT_TEM_HUM_GAS_MODE) != NULL) { //report temperture & humidity &combustible gas data
            g_current_mode = ENV_ALL_MODE; 
        } else if (strstr(payload, ENVIRONMENT_TEMPERATURE_PAYLOAD) != NULL) { //report temperture data 
            g_current_mode = ENV_TEMPERRATURE_MODE; 
        } else if(strstr(payload, ENVIRONMENT_HUMIDITY_PAYLOAD) != NULL) { //report humidity data
            g_current_mode = ENV_HUMIDITY_MODE; 
        } else if (strstr(payload, ENVIRONMENT_COMBUSTIBLE_GAS_PAYLOAD) != NULL) { //report combustible data
            g_current_mode = COMBUSTIBLE_GAS_MODE; 
        }
        oc_environment_app_option(g_current_mode, g_current_type);
    }
    
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

/*colorful light:clean status;*/
hi_void setup_clean_clfl_status(hi_colorful_light_mode early_mode)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;
    if (early_mode == CONTROL_MODE) {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "UnionDetectModule";
        property.value = "NA";
        memset(&service, 0,sizeof(service));
        service.serviceID = "ColorfulLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    } else if (early_mode == COLORFUL_LIGHT_MODE) {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "ControlModule";
        property.value = "NA";
        memset(&service, 0,sizeof(service));
        service.serviceID = "ColorfulLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    } else if (early_mode == PWM_CONTROL_MODE) {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "ColorfulLightModule";
        property.value = "NA";
        memset(&service, 0,sizeof(service));
        service.serviceID = "ColorfulLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    } else if (early_mode == BIRGHTNESS_MODE) {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "PWMControlModule";
        property.value = "NA";
        memset(&service, 0,sizeof(service));
        service.serviceID = "ColorfulLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    } else if (early_mode == HUMAN_DETECT_MODE) {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "BrightnessModule";
        property.value = "NA";
        memset(&service, 0,sizeof(service));
        service.serviceID = "ColorfulLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    } else if (early_mode == LIGHT_DETECT_MODE) {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "HumanDetectModule";
        property.value = "NA";
        memset(&service, 0,sizeof(service));
        service.serviceID = "ColorfulLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    } else if (early_mode == UNION_DETECT_MODE) {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "LightDetectModule";
        property.value = "NA";
        memset(&service, 0,sizeof(service));
        service.serviceID = "ColorfulLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    }
}
/*colorful light:1.control module*/
hi_void setup_clfl_control_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("coloful light:control module\r\n");
    if (current_mode != CONTROL_MODE && current_type != RED_ON) {
        printf("select current module is not the CONTROL_MODE\r\n");
        return HI_NULL;
    }
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "ControlModule";
    if (g_current_type == RED_ON) {
            property.value = "RED_LED_ON";
    } else if (g_current_type == YELLOW_ON) {
            property.value = "GREEN_LED_ON";
    } else if (g_current_type == GREEN_ON) {
            property.value = "BLUE_LED_ON";
    }
    memset(&service, 0,sizeof(service));
    service.serviceID = "ColorfulLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}
/*colorful light:2.colorful light module*/
hi_void setup_clfl_colorful_light_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("coloful light:setup_colorful_light_module\r\n");
    if (current_mode != COLORFUL_LIGHT_MODE && current_type != CYCLE_FOR_ONE_SECOND) {
        printf("select current module is not the COLORFUL_LIGHT_MODE, Please select the right module when under the colorful light\r\n");
        return HI_NULL;
    }
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "ColorfulLightModule";
    if (g_current_type == CYCLE_FOR_ONE_SECOND) {
            property.value = "Period_1_S";
    } else if (g_current_type == CYCLE_FOR_HALF_SECOND) {
            property.value = "Period_0.5_S";
    } else if (g_current_type == CYCLE_FOR_QUARATER_SECOND) {
            property.value = "Period_0.25_S";
    }
    memset(&service, 0,sizeof(service));
    service.serviceID = "ColorfulLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}
/*colorful light:3.pwm control module*/
hi_void setup_clfl_pwm_control_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("coloful light:setup_clfl_pwm_control_module\r\n");
    if (current_mode != PWM_CONTROL_MODE && current_type != PWM_CONTROL_RED) {
        printf("select current module is not the PWM_CONTROL_MODE, Please select the right module when under the colorful light\r\n");
        return HI_NULL;
    }
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "PWM_Module";
    if (g_current_type == PWM_CONTROL_RED) {
        property.value = "PWM_RED_ON";
    } else if (g_current_type == PWM_CONTROL_YELLOW) {
        property.value = "PWM_GREEN_ON";
    } else if (g_current_type == PWM_CONTROL_GREEN) {
        property.value = "PWM_BLUE_ON";
    } else if (g_current_type == PWM_CONTROL_PURPLE) {
        property.value = "PWM_PURPLE_ON";
    } else if (g_current_type == PWM_CONTROL_ALL) {
         property.value = "PWM_ALL_LED_ON";
    }
    memset(&service, 0,sizeof(service));
    service.serviceID = "ColorfulLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}
/*colorful light:4.brightness module*/
hi_void setup_clfl_brightness_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("coloful light:setup_clfl_brightness_module\r\n");
    if (current_mode != BIRGHTNESS_MODE && current_type != BRIGHTNESS_LOW) {
        printf("select current module is not the BIRGHTNESS_MODE, Please select the right module when under the colorful light\r\n");
        return HI_NULL;
    }
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "BrightnessModule";
    if (g_current_type == BRIGHTNESS_LOW) {
        property.value = "BRIGHTNESS_LOW";
    } else if (g_current_type == BRIGHTNESS_MIDDLE) {
        property.value = "BRIGHTNESS_MIDDLE";
    } else if (g_current_type == BRIGHTNESS_HIGH) {
        property.value = "BRIGHTNESS_HIGH";
    }
    memset(&service, 0,sizeof(service));
    service.serviceID = "ColorfulLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}
/*colorful light:5.human detect module*/
hi_void setup_clfl_human_detect_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("coloful light:setup_clfl_human_detect_module\r\n");
    if (current_mode != HUMAN_DETECT_MODE) {
        printf("select current module is not the HUMAN_DETECT_MODE, Please select the right module when under the colorful light\r\n");
        return HI_NULL;
    }
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "HumanDetectModule";
    if (g_someone_walking_flag == OLED_FALG_ON) {
        property.value = "Someone_was_detected";
    } else if (g_someone_walking_flag == OLED_FALG_OFF) {
        property.value = "no_one_was_detected";
    }
    memset(&service, 0,sizeof(service));
    service.serviceID = "ColorfulLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}

/*colorful light:6.light detect module*/
hi_void setup_clfl_light_detect_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("coloful light:setup_clfl_light_detect_module\r\n");
    if (current_mode != LIGHT_DETECT_MODE) {
        printf("select current module is not the LIGHT_DETECT_MODE, Please select the right module when under the colorful light\r\n");
        return HI_NULL;
    }
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "LightDetectModule";
    if (g_with_light_flag == OLED_FALG_ON) {
        property.value = "dark";
    } else if (g_someone_walking_flag == OLED_FALG_OFF) {
        property.value = "light";
    }
    memset(&service, 0,sizeof(service));
    service.serviceID = "ColorfulLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}

/*colorful light:7.union detect module*/
hi_void setup_clfl_union_detect_module(hi_colorful_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("coloful light:setup_clfl_light_detect_module\r\n");
    if (current_mode != UNION_DETECT_MODE) {
        printf("select current module is not the UNION_DETECT_MODE, Please select the right module when under the colorful light\r\n");
        return HI_NULL;
    }
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "UnionDetectModule";
    if (((g_someone_walking_flag == OLED_FALG_ON)&&(g_with_light_flag == OLED_FALG_OFF)) ||(g_someone_walking_flag == OLED_FALG_OFF)) {
        property.value = "light_no_one_and_dark";
    } else if  ((g_with_light_flag == OLED_FALG_ON) && (g_someone_walking_flag == OLED_FALG_ON)) {
        property.value = "someone_and_dark";
    }
    memset(&service, 0,sizeof(service));
    service.serviceID = "ColorfulLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}
/*traffic light:clean stautus and report status to the huawei ocean cloud*/
hi_void setup_clean_trfl_status(hi_traffic_light_mode early_mode)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;
    if (early_mode == TRAFFIC_CONTROL_MODE) {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "HumanModule";
        property.value = "OFF";
        memset(&service, 0,sizeof(service));
        service.serviceID = "TrafficLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    } else if (early_mode == TRAFFIC_AUTO_MODE) {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "ControlModule";
        property.value = "OFF";
        memset(&service, 0,sizeof(service));
        service.serviceID = "TrafficLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    } else if (early_mode == TRAFFIC_HUMAN_MODE) {
        memset(&property, 0, sizeof(property));
        property.type = EN_IOT_DATATYPE_STRING;
        property.key = "AutoModule";
        property.value = "OFF";
        memset(&service, 0,sizeof(service));
        service.serviceID = "TrafficLight";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    }
}
/*traffic light:1.control module*/
hi_void setup_trfl_control_module(hi_traffic_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("traffic light:control module\r\n");
    if (current_mode != TRAFFIC_CONTROL_MODE && current_type != RED_ON) {
        printf("select current module is not the TRAFFIC_CONTROL_MODE\r\n");
        return HI_NULL;
    }
    g_current_mode = TRAFFIC_CONTROL_MODE;

    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "ControlModule";
    if (g_current_type == RED_ON) {
            property.value = "RED_LED_ON";
    } else if (g_current_type == YELLOW_ON) {
            property.value = "YELLOW_LED_ON";
    } else if (g_current_type == GREEN_ON) {
            property.value = "GREEN_LED_ON";
    }
    memset(&service, 0,sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    /*report beep status*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "AutoModule";
    if (oc_beep_status == BEEP_ON) {
        property.value = "BEEP_ON";
    } else {
        property.value = "BEEP_OFF";
    }
    memset(&service, 0,sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}
/*report light time count*/
hi_void report_led_light_time_count(hi_void)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;
    /*report red led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "AutoModuleRLedTC";
    property.i_value = g_red_led_auto_modu_time_count ;
    memset(&service, 0,sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    /*report yellow led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "AutoModuleYLedTC";
    property.i_value = g_yellow_led_auto_modu_time_count ;
    memset(&service, 0,sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    /*report green led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "AutoModuleGLedTC";
    property.i_value = g_green_led_auto_modu_time_count;
    memset(&service, 0,sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}
/*traffic light:2.auto module*/
hi_void setup_trfl_auto_module(hi_traffic_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("traffic light:auto module\r\n");
    if (current_mode != TRAFFIC_AUTO_MODE) {
        printf("select current module is not the CONTROL_MODE\r\n");
        return HI_NULL;
    }
    /*report beep status*/
    g_current_mode = TRAFFIC_AUTO_MODE;
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "AutoModule";
    if (oc_beep_status == BEEP_ON) {
        property.value = "BEEP_ON";
    } else {
        property.value = "BEEP_OFF";
    }
    memset(&service, 0,sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    /*report light time count*/
    report_led_light_time_count();
}

/*traffic light:3.human module*/
hi_void setup_trfl_human_module(hi_traffic_light_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("traffic light:human module\r\n");
    if (current_mode != TRAFFIC_HUMAN_MODE) {
        printf("select current module is not the CONTROL_MODE\r\n");
        return HI_NULL;
    }
    g_current_mode = TRAFFIC_HUMAN_MODE;
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "HumanModule";
    if( oc_beep_status == BEEP_ON) {
        property.value = "BEEP_ON";
    } else {
        property.value = "BEEP_OFF";
    }
    memset(&service, 0,sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);

    /*red led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "HumanModuleRledTC";
    property.i_value = g_red_led_human_modu_time_count;
    memset(&service, 0,sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    /*yellow led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "HumanModuleYledTC";
    property.i_value = g_yellow_led_human_modu_time_count;
    memset(&service, 0,sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    /*green led light time count*/
    memset(&property, 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_INT;
    property.key = "HumanModuleGledTC";
    property.i_value = g_green_led_human_modu_time_count;
    memset(&service, 0,sizeof(service));
    service.serviceID = "TrafficLight";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}
/*environment:1. temperature&humidity&gas module*/
hi_void setup_env_all_module(hi_environment_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("environment:env all module\r\n");
    if (current_mode != ENV_ALL_MODE) {
        printf("select current module is not the ENV_ALL_MODE\r\n");
        return HI_NULL;
    }
/*--------------------------------------temperature--------------------*/
    memset(&property, 0, sizeof(property));
    // property.type = EN_IOT_DATATYPE_LAST;
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "Env_AllModule_Temperature";
    // property.oc_evvironment_value = g_temperature;
    property.value = g_ahu20_temperature_buff;
    memset(&service, 0,sizeof(service));
    service.serviceID = "Environment";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
/*-----------------------------------&humidity--------------------------*/
    memset(&property, 0, sizeof(property));
    // property.type = EN_IOT_DATATYPE_LAST;
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "Env_AllModule_Humidity";
    // property.oc_evvironment_value = g_humidity;
    property.value = g_ahu20_humidity_buff;
    memset(&service, 0,sizeof(service));
    service.serviceID = "Environment";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
/*--------------------combustible gas---------------------------------*/
    memset(&property, 0, sizeof(property));
    // property.type = EN_IOT_DATATYPE_LAST;
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "Env_AllModule_Combustible_Gas";
    // property.oc_evvironment_value = (hi_float)g_combustible_gas_value;
    property.value = g_ahu20_gas_buff;
    memset(&service, 0,sizeof(service));
    service.serviceID = "Environment";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}
/*environment:2. temperature module*/
hi_void setup_env_temperature_module(hi_environment_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("environment:temperature module\r\n");
    if (current_mode != ENV_TEMPERRATURE_MODE) {
        printf("select current module is not the ENV_TEMPERRATURE_MODE\r\n");
        return HI_NULL;
    }
    memset(&property, 0, sizeof(property));
    // property.type = EN_IOT_DATATYPE_LAST;
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "TemperatureModule";
    // property.oc_evvironment_value = g_temperature;
    property.value = g_ahu20_temperature_buff;
    memset(&service, 0,sizeof(service));
    service.serviceID = "Environment";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}
/*environment:3. humidity module*/
hi_void setup_env_humidity_module(hi_environment_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("environment:humidity module\r\n");
    if (current_mode != ENV_HUMIDITY_MODE) {
        printf("select current module is not the ENV_HUMIDITY_MODE\r\n");
        return HI_NULL;
    }
    memset(&property, 0, sizeof(property));
    // property.type = EN_IOT_DATATYPE_LAST;
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "HumidityModule";
    // property.oc_evvironment_value = g_humidity; 
    property.value = g_ahu20_humidity_buff;
    memset(&service, 0,sizeof(service));
    service.serviceID = "Environment";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
}
/*environment:4.combustible gas module*/
hi_void setup_env_combustible_gas_module(hi_environment_mode current_mode, hi_control_mode_type current_type)
{
    IoTProfileService_t service;
    IoTProfileKV_t property;

    printf("combustible gas:combustible gas module\r\n");
    if (current_mode != COMBUSTIBLE_GAS_MODE) {
        printf("select current module is not the COMBUSTIBLE_GAS_MODE\r\n");
        return HI_NULL;
    }
    memset(&property, 0, sizeof(property));
    // property.type = EN_IOT_DATATYPE_LAST;
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "CombustibleGasModule";
    if (g_combustible_gas_value > 0.0) {
        property.value = g_ahu20_gas_buff;
        // property.oc_evvironment_value =(hi_float) g_combustible_gas_value; 
        memset(&service, 0,sizeof(service));
        service.serviceID = "Environment";
        service.serviceProperty = &property;
        IoTProfilePropertyReport(CONFIG_DEVICE_ID,&service);
    }
}
/*call back func and report status to huawei ocean cloud*/
hi_void oc_colorful_light_status_report(hi_colorful_light_mode current_mode,oc_clfl_call_back_func msg_report)
{
    printf("colorful light: reporting status...\r\n"); 
    switch (current_mode) {
    case CONTROL_MODE: 
        msg_report(CONTROL_MODE, RED_ON);
        break;
    case COLORFUL_LIGHT_MODE:
        msg_report(COLORFUL_LIGHT_MODE, CYCLE_FOR_ONE_SECOND);
        break;
    case PWM_CONTROL_MODE:
        msg_report(PWM_CONTROL_MODE, PWM_CONTROL_RED);
        break;
    case BIRGHTNESS_MODE:
        msg_report(BIRGHTNESS_MODE, BRIGHTNESS_LOW);  
        break; 
    case HUMAN_DETECT_MODE:
        msg_report(HUMAN_DETECT_MODE, NULL);
        break;
    case LIGHT_DETECT_MODE:
        msg_report(LIGHT_DETECT_MODE, NULL);
        break; 
    case UNION_DETECT_MODE:
        msg_report(UNION_DETECT_MODE, NULL);
        break;                  
    default:
        break;
    }
     return HI_NULL;
}

hi_void oc_traffic_light_status_report(hi_traffic_light_mode current_mode, oc_trfl_call_back_func msg_report)
{
    // printf("tarffic light: reporting status...\r\n"); 
        switch (current_mode) {
    case TRAFFIC_CONTROL_MODE: 
        msg_report(TRAFFIC_CONTROL_MODE, RED_ON);
        break;
    case TRAFFIC_AUTO_MODE:
        msg_report(TRAFFIC_AUTO_MODE, NULL);
        break;
    case TRAFFIC_HUMAN_MODE:
        msg_report(TRAFFIC_HUMAN_MODE, NULL);
        break;             
    default:
        break;
    }
     return HI_NULL;
}

hi_void oc_environment_status_report(hi_environment_mode current_mode, oc_env_call_back_func msg_report)
{
    printf("environment: reporting status...\r\n"); 
    switch (current_mode) {
        case ENV_ALL_MODE:
            msg_report(ENV_ALL_MODE, NULL);
        case ENV_TEMPERRATURE_MODE: 
            msg_report(ENV_TEMPERRATURE_MODE, NULL);
        break;
        case ENV_HUMIDITY_MODE:
            msg_report(ENV_HUMIDITY_MODE, NULL);
        break;
        case COMBUSTIBLE_GAS_MODE:
            msg_report(COMBUSTIBLE_GAS_MODE, NULL);
        break;             
        default:
        break;
    }
     return HI_NULL;
}

///< this is the demo main task entry,here we will set the wifi/cjson/mqtt ready ,and 
///< wait if any work to do in the while
static hi_void *DemoEntry(hi_void *arg)
{
    // IoTProfileService_t service;
    // IoTProfileKV_t property;
    hi_watchdog_disable();
    
    extern hi_void WifiStaReadyWait(hi_void);
    WifiStaReadyWait();

    extern void cJsonInit(void);
    cJsonInit();

    IoTMain();
    IoTSetMsgCallback(DemoMsgRcvCallBack);
/*主动上报*/
#ifdef TAKE_THE_INITIATIVE_TO_REPORT
    while (1){
        ///< here you could add your own works here--we report the data to the IoTplatform
        hi_sleep(1000);
        ///< now we report the data to the iot platform
        if (g_menu_select == COLORFUL_LIGHT_MENU && g_menu_mode == SUB_MODE_SELECT_MODE) {
            switch (g_current_mode) {
            case CONTROL_MODE:
                oc_colorful_light_status_report(CONTROL_MODE, setup_clfl_control_module);
                break;
            case COLORFUL_LIGHT_MODE:
                // setup_clean_clfl_status(COLORFUL_LIGHT_MODE);
                oc_colorful_light_status_report(COLORFUL_LIGHT_MODE, setup_clfl_colorful_light_module);
                break;
            case PWM_CONTROL_MODE:
                // setup_clean_clfl_status(PWM_CONTROL_MODE);
                oc_colorful_light_status_report(PWM_CONTROL_MODE, setup_clfl_pwm_control_module);          
                break; 
            case BIRGHTNESS_MODE:
                // setup_clean_clfl_status(BIRGHTNESS_MODE);
                oc_colorful_light_status_report(BIRGHTNESS_MODE, setup_clfl_brightness_module); 
                break;
            case HUMAN_DETECT_MODE:
                // setup_clean_clfl_status(HUMAN_DETECT_MODE);
                oc_colorful_light_status_report(HUMAN_DETECT_MODE, setup_clfl_human_detect_module); 
                break; 
            case LIGHT_DETECT_MODE:
                // setup_clean_clfl_status(LIGHT_DETECT_MODE);
                oc_colorful_light_status_report(LIGHT_DETECT_MODE, setup_clfl_light_detect_module); 
                break;
            case UNION_DETECT_MODE:
                // setup_clean_clfl_status(UNION_DETECT_MODE);
                oc_colorful_light_status_report(UNION_DETECT_MODE, setup_clfl_union_detect_module); 
                break;                 
            default:
                break;
            }
        } else if (g_menu_select == TRAFFIC_LIGHT_MENU && g_menu_mode == SUB_MODE_SELECT_MODE) {           
            switch (g_current_mode) {
                case TRAFFIC_CONTROL_MODE:
                    // setup_clean_trfl_status(TRAFFIC_CONTROL_MODE);
                    oc_traffic_light_status_report(TRAFFIC_CONTROL_MODE, setup_trfl_control_module);
                    break;
                case TRAFFIC_AUTO_MODE:
                    // setup_clean_trfl_status(TRAFFIC_AUTO_MODE);
                    oc_traffic_light_status_report(TRAFFIC_AUTO_MODE, setup_trfl_auto_module);
                    break;
                case TRAFFIC_HUMAN_MODE:
                    // setup_clean_trfl_status(TRAFFIC_HUMAN_MODE);
                    oc_traffic_light_status_report(TRAFFIC_HUMAN_MODE, setup_trfl_human_module);
                    break;
                default:
                    break;
            }
        } else if (g_menu_select == ENVIRONMENT_MENU && g_menu_mode == SUB_MODE_SELECT_MODE) {
            switch (g_current_mode) {
                case ENV_ALL_MODE:
                    oc_environment_status_report(ENV_ALL_MODE, setup_env_all_module);
                case ENV_TEMPERRATURE_MODE:
                    oc_environment_status_report(ENV_TEMPERRATURE_MODE, setup_env_temperature_module);
                    break;
                case ENV_HUMIDITY_MODE:  
                    oc_environment_status_report(ENV_HUMIDITY_MODE, setup_env_humidity_module);
                    break;
                case COMBUSTIBLE_GAS_MODE:
                    oc_environment_status_report(COMBUSTIBLE_GAS_MODE, setup_env_combustible_gas_module);
                    break;
                default:
                    break;
            }
        }    
    }
#endif
    return NULL;
}

///< This is the demo entry, we create a task here, and all the works has been done in the demo_entry
#define CN_IOT_TASK_STACKSIZE  0x1000   
#define CN_IOT_TASK_PRIOR 28
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
