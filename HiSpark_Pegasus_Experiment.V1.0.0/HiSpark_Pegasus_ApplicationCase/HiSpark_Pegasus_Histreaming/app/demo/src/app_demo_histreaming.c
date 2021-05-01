/*----------------------------------------------------------------------------
* Copyright (c) <2013-2015>, <Huawei Technologies Co., Ltd>
* All rights reserved.
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
* 1. Redistributions of source code must retain the above copyright notice, this list of
* conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice, this list
* of conditions and the following disclaimer in the documentation and/or other materials
* provided with the distribution.
* 3. Neither the name of the copyright holder nor the names of its contributors may be used
* to endorse or promote products derived from this software without specific prior written
* permission.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*---------------------------------------------------------------------------*/
#ifdef CONFIG_HISTREAMING_SUPPORT
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <hi_pwm.h>
#include <hi_time.h>
/* Link Header Files */
#include <link_service.h>
#include <link_platform.h>
#include <histreaming.h>
#include <hi_io.h>
#include <hi_early_debug.h>
#include <hi_gpio.h>
#include <hi_task.h>
#include <hi_types_base.h>
#include <app_demo_multi_sample.h>

#define HISTREAMING_TASK
#ifdef HISTREAMING_TASK
#define HISTREAMING_DEMO_TASK_STAK_SIZE (1024*8)
#define HISTREAMING_DEMO_TASK_PRIORITY  25
hi_u32 g_histreaming_demo_task_id = 0;
#endif

extern hi_u8 g_menu_mode;
extern hi_u8 g_current_type;
extern hi_u8 g_current_mode;
extern hi_u8 g_menu_select;
extern hi_u8 g_key_down_flag ;
extern hi_void oled_main_menu_display(hi_void);

static hi_void histreaming_colorful_light_control(const char* property, char* value);
static hi_void histreaming_traffic_light_control(const char* property, char* value);
static hi_void histreaming_environment_control(const char* property, char* value);
static hi_void histreaming_nfc_control(const char* property, char* value);
static hi_u32 histreaming_colorful_light_return_main_menu(const char* property, char* value);
static hi_u32 histreaming_traffic_light_return_main_menu(const char* property, char* value);
extern hi_void gpio_control(hi_io_name gpio, hi_gpio_idx id, hi_gpio_dir  dir,  hi_gpio_value gpio_val, hi_u8 val);

/*histreaming colorful light function control*/
static hi_void histreaming_colorful_light_control(const char* property, char* value)
{
    /*manual control module*/
    if (strcmp(property, "cl_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = COLORFUL_LIGHT_MENU;
        g_current_mode = CONTROL_MODE;
        if (strcmp(value, "red_on") == 0) {
            // g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = RED_ON;
            printf("red on\r\n");
        }  
        if (strcmp(value, "green_on") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = YELLOW_ON;
            printf("green on\r\n");               
        } 
        if (strcmp(value, "blue_on") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = GREEN_ON;
            printf("blue on\r\n");  
        }
    }
    /*period led control*/
    if (strcmp(property, "pl_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = COLORFUL_LIGHT_MENU;
        g_current_mode = COLORFUL_LIGHT_MODE;
        if (strcmp(value, "p1s") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = CYCLE_FOR_ONE_SECOND;
            printf("period_1s\r\n");
        } 
        if (strcmp(value, "p05s") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = CYCLE_FOR_HALF_SECOND;
            printf("period_05s\r\n");               
        }
         if (strcmp(value, "p025s") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = CYCLE_FOR_QUARATER_SECOND;
            printf("period_025s\r\n");  
        }
    }
    /*pwm led control*/
    if (strcmp(property, "pwm_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = COLORFUL_LIGHT_MENU;
        g_current_mode = PWM_CONTROL_MODE;
        if (strcmp(value, "red_on") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = RED_ON;
            printf("red on\r\n");
        } 
        if (strcmp(value, "green_on") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = YELLOW_ON;
            printf("green on\r\n");               
        }
         if (strcmp(value, "blue_on") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = GREEN_ON;
            printf("blue on\r\n");  
        }
         if (strcmp(value, "purple_on") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = PWM_CONTROL_PURPLE;
            printf("purple on\r\n");  
        }
         if (strcmp(value, "white_on") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = PWM_CONTROL_ALL;
            printf("white on\r\n");  
        }
    }
    /*brightness led control*/
    if (strcmp(property, "br_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = COLORFUL_LIGHT_MENU;
        g_current_mode = BIRGHTNESS_MODE;
        if (strcmp(value, "br_l") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = BRIGHTNESS_LOW;
            printf("birghtness low\r\n");
        }
         if (strcmp(value, "br_m") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = BRIGHTNESS_MIDDLE;
            printf("brightness middle\r\n");               
        }
         if (strcmp(value, "br_h") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = BRIGHTNESS_HIGH;
            printf("brightness high\r\n");  
        } 
    }
    /*human detect*/
    if (strcmp(property, "ht_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = COLORFUL_LIGHT_MENU;
        if (strcmp(value, "ht") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = HUMAN_DETECT_MODE;
            printf("human_detect_start\r\n");
        }
    }
    /*light detect*/
    if (strcmp(property, "lt_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = COLORFUL_LIGHT_MENU;
        if (strcmp(value, "lt") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = LIGHT_DETECT_MODE;
            printf("light_detect_start\r\n");
        }
    }
    /*union detect*/
    if (strcmp(property, "ut_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = COLORFUL_LIGHT_MENU;
        if (strcmp(value, "ut") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = UNION_DETECT_MODE;
            printf("union_detect_start\r\n");
        }
    }
}
/*histreaming traffic light function control*/
static hi_void histreaming_traffic_light_control(const char* property, char* value)
{
    /*manual control module*/
    if (strcmp(property, "tl_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = TRAFFIC_LIGHT_MENU;
        g_current_mode = TRAFFIC_CONTROL_MODE;
        if (strcmp(value, "red_on") == 0) {
            // g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = RED_ON;
            printf("traffic light red on\r\n");
        }
        if (strcmp(value, "yellow_on") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = YELLOW_ON;
            printf("traffic light yellow on\r\n");               
        }
        if (strcmp(value, "green_on") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_type = GREEN_ON;
            printf("traffic light green on\r\n");  
        }
    }
    /*auto module*/
    if (strcmp(property, "tla_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = TRAFFIC_LIGHT_MENU;
        if (strcmp(value, "tla") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = TRAFFIC_AUTO_MODE;
            printf("traffic light auto module start\r\n");
        } 
    }
    /*human module normal*/
    if (strcmp(property, "tlh_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = TRAFFIC_LIGHT_MENU;
        if (strcmp(value, "tlh") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = TRAFFIC_HUMAN_MODE;
            g_current_type =TRAFFIC_NORMAL_TYPE;
            printf("traffic light human module start\r\n");
        } 
    }
    /*human module human control*/
    if (strcmp(property, "tlm_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = TRAFFIC_LIGHT_MENU;
        if (strcmp(value, "tlm") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = TRAFFIC_HUMAN_MODE;
            g_current_type = TRAFFIC_HUMAN_TYPE;
            printf("traffic light human control start\r\n");
        } 
    }
}
/*histreaming environment function control*/
static hi_void histreaming_environment_control(const char* property, char* value)
{

    /*get temperature/humidity/G_Gas value*/
    if (strcmp(property, "ena_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = ENVIRONMENT_MENU;
        if (strcmp(value, "ena") == 0) {
            // g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = ENV_ALL_MODE;
            printf("environment all control start\r\n");
        } 
    }
    /*get temperature value*/
    if (strcmp(property, "ent_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = ENVIRONMENT_MENU;
        if (strcmp(value, "ent") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = ENV_TEMPERRATURE_MODE;
            printf("environment temperature control start\r\n");
        } 
    }
    /*get humidity value*/
    if (strcmp(property, "enh_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = ENVIRONMENT_MENU;
        if (strcmp(value, "enh") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = ENV_HUMIDITY_MODE;
            printf("environment humiduty control start\r\n");
        } 
    }
    /*get G_Gas value*/
    if (strcmp(property, "eng_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = ENVIRONMENT_MENU;
        if (strcmp(value, "eng") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = COMBUSTIBLE_GAS_MODE;
            printf("environment g_gas control start\r\n");
        } 
    }
}
/*histreaming nfc function control*/
static hi_void histreaming_nfc_control(const char* property, char* value)
{
    /*wechat*/
    if (strcmp(property, "nfw_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = NFC_TEST_MENU;
        if (strcmp(value, "nfw") == 0) {
            g_current_mode = NFC_TAG_WECHAT_MODE;
            printf("nfc wechat control start\r\n");
        }     
    }
    /*today headline*/
    if (strcmp(property, "nfth_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = NFC_TEST_MENU;
        if (strcmp(value, "nfth") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = NFC_TAG_TODAY_HEADLINE_MODE;
            printf("nfc today headline control start\r\n");
        } 
    }
    /*taobao*/
    if (strcmp(property, "nft_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = NFC_TEST_MENU;
        if (strcmp(value, "nft") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = NFC_TAG_TAOBAO_MODE;
            printf("nfc taobao control start\r\n");
        } 
    } 
    /*huawei smart lift*/
    if (strcmp(property, "nfhs_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = NFC_TEST_MENU;
        if (strcmp(value, "nfhs") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = NFC_TAG_HUAWEI_LIFE_MODE;
            printf("nfc huawei smart lift control start\r\n");
        } 
    }
    /*histreaming*/
    if (strcmp(property, "nfh_s") == 0) {
        g_key_down_flag = KEY_GPIO_7;
        g_menu_mode = MAIN_FUNCTION_SELECT_MODE;
        g_menu_select = NFC_TEST_MENU;
        if (strcmp(value, "nfh") == 0) {
            g_menu_mode = SUB_MODE_SELECT_MODE;
            g_current_mode = NFC_TAG_HISTREAMING_MODE;
            printf("nfc histreaming control start\r\n");
        } 
    }
}
static hi_u32 histreaming_colorful_light_return_main_menu(const char* property, char* value)
{
    if (strcmp(property, "clr_s") == 0) {
        if (strcmp(value, "clr") == 0) {
         printf("colorful_light_return_start\r\n");    
        g_current_mode = RETURN_MODE;
        g_current_type = KEY_DOWN;
        gpio_control(HI_IO_NAME_GPIO_10, HI_GPIO_IDX_10, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0,HI_IO_FUNC_GPIO_10_GPIO);
        gpio_control(HI_IO_NAME_GPIO_11, HI_GPIO_IDX_11, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0,HI_IO_FUNC_GPIO_11_GPIO); 
        gpio_control(HI_IO_NAME_GPIO_12, HI_GPIO_IDX_12, HI_GPIO_DIR_OUT, HI_GPIO_VALUE0,HI_IO_FUNC_GPIO_12_GPIO);       
        oled_main_menu_display();                    
        return HI_ERR_SUCCESS;
        }
    }
}
static hi_u32 histreaming_traffic_light_return_main_menu(const char* property, char* value)
{
    if (strcmp(property, "tlr_s") == 0) {
        if (strcmp(value, "tlr") == 0) {
            printf("traffic_light_return_start\r\n");    
            g_current_mode = TRAFFIC_RETURN_MODE;
            g_current_type = KEY_DOWN;
            oled_main_menu_display();                    
            return HI_ERR_SUCCESS;
        }
    }
}
static hi_u32 histreaming_environment_return_main_menu(const char* property, char* value)
{
    if (strcmp(property, "enr_s") == 0) {
        if (strcmp(value, "enr") == 0) {
            printf("environment return start\r\n");  
            g_current_mode = ENV_RETURN_MODE;
            g_current_type = KEY_DOWN;
            oled_main_menu_display();                    
            return HI_ERR_SUCCESS;
        }
    }
}
static hi_u32 histreaming_nfc_return_main_menu(const char* property, char* value)
{
    if (strcmp(property, "nfr_s") == 0) {
        if (strcmp(value, "nfr") == 0) {
            printf("nfc return start\r\n");  
            g_current_mode = NFC_RETURN_MODE;
            g_current_type = KEY_DOWN;
            oled_main_menu_display();                    
            return HI_ERR_SUCCESS;
        }
    }
}
static int GetStatusValue(struct LinkService* ar, const char* property, char* value, int len)
{
    (void)(ar);

    printf("Receive property: %s(value=%s[%d])\n", property, value, len);

    if (strcmp(property, "Status") == 0) {
        strcpy(value, "Opend");
    }

    /*
     * if Ok return 0,
     * Otherwise, any error, return StatusFailure
     */
    return 0;
}
/* recv from app cmd */
static int ModifyStatus(struct LinkService* ar, const char* property, char* value, int len)
{
    (void)(ar);

    if (property == NULL || value == NULL) {
        return -1;
    }
    /* modify status property*/
    /*colorful light module*/
    histreaming_colorful_light_control(property, value);
    /*traffic light module*/
    histreaming_traffic_light_control(property, value);
    /*environment*/
    histreaming_environment_control(property, value);
    /*nfc*/
    histreaming_nfc_control(property, value);
    /*colorful light return main menu*/
    histreaming_colorful_light_return_main_menu(property, value);
    /*traffic light return main menu*/
    histreaming_traffic_light_return_main_menu(property, value);
    /*environment return main menu*/
    histreaming_environment_return_main_menu(property, value);
    /*nfc return main menu*/
    histreaming_nfc_return_main_menu(property, value);
    printf("Receive property: %s(value=%s[%d])\n", property, value,len);
    /*
     * if Ok return 0,
     * Otherwise, any error, return StatusFailure
     */
    return 0;
}

/*
 * It is a Wifi IoT device
 */
static const char* g_wifista_type = "Light";
static const char* GetDeviceType(struct LinkService* ar)
{
    (void)(ar);

    return g_wifista_type;
}

static void *g_link_platform = NULL;

void* histreaming_open(void)
{
    hi_u32 ret = hi_gpio_init();
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ===== gpio -> hi_gpio_init ret:%d\r\n", ret);
        return NULL;
    }
    else{
        /* code */
        printf("----- gpio init success-----\r\n");
    }
    
    LinkService* wifiIot = 0;
    LinkPlatform* link = 0;

    wifiIot = (LinkService*)malloc(sizeof(LinkService));
    if (!wifiIot){
        printf("malloc wifiIot failure\n");
        return NULL;
    }

    wifiIot->get    = GetStatusValue;
    wifiIot->modify = ModifyStatus;
    wifiIot->type = GetDeviceType;

    link = LinkPlatformGet();
    if (!link) {
        printf("get link failure\n");
        return NULL;
    }

    if (link->addLinkService(link, wifiIot, 1) != 0) {
        histreaming_close(link);
        return NULL;
    }

    if (link->open(link) != 0) {
        histreaming_close(link);
        return NULL;
    }

    /* cache link ptr*/
    g_link_platform = (void*)(link);
#ifdef HISTREAMING_TASK    
    hi_task_delete(g_histreaming_demo_task_id);
#endif
    return (void*)link;
}

void histreaming_close(void *link)
{
    LinkPlatform *linkPlatform = (LinkPlatform*)(link);
    if (!linkPlatform) {
        return;
    }

    linkPlatform->close(linkPlatform);

    if (linkPlatform != NULL) {
        LinkPlatformFree(linkPlatform);
    }
}
#ifdef HISTREAMING_TASK 
hi_void histreaming_demo(hi_void)
{
    hi_u32 ret;
    hi_task_attr histreaming ={0};
    histreaming.stack_size = HISTREAMING_DEMO_TASK_STAK_SIZE;
    histreaming.task_prio = HISTREAMING_DEMO_TASK_PRIORITY;
    histreaming.task_name = "histreaming_demo";
    ret = hi_task_create(&g_histreaming_demo_task_id, &histreaming, histreaming_open, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("Falied to create histreaming demo task!\n");
    }
}
#endif
#endif