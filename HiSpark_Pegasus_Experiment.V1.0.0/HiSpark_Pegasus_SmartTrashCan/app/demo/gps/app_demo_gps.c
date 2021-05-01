/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: app uart demo
 * Author: Hisilicon
 * Create: 2019-12-09
 */

#include <app_demo_uart.h>
#include "string.h"
#include "stdlib.h"
#include <hi_stdlib.h>
#include <hi_wifi_api.h>
#include <global_config.h>
#include <hi_mux.h>
#include <app_demo_gps_log.h>

#define GPS_UART_NUM HI_UART_IDX_1

hi_u32 g_gps_sem_id;
char gps_data[30] = {0};
hi_u32 g_uart_demo_task_id = 0;

double data_calc(double in)
{
    in /=100;
    int a = in;
    return  (in -a)/0.6+a;
}
static int get_gps(char *gps,char * str)
{
    double Longitude = 0,Latitude = 0;
    char EW = 0,NS = 0;
    hi_u8 satellites_num = 0;
    if(gps == NULL && str == NULL) {
        return HI_ERR_FAILURE;
    }
    if(strstr(str,"GNGGA")) {
        char *pData = str;       
        for(int i = 0;i<2;i++) {
            pData = strstr(pData,",");
            pData++;
        }
        Latitude = data_calc(atof(pData));        
        pData = strstr(pData,",");
        pData++;
        NS = *pData;  
        pData = strstr(pData,",");
        pData++;
        Longitude= data_calc(atof(pData));
        pData = strstr(pData,",");
        pData++;
        EW = *pData;
        if(Longitude == 0 ||Latitude == 0) {
            GPS_LOG_ERROR("Failed to obtain GPS information.\n");
        } else {
            for(int i = 0;i<2;i++) {
                pData = strstr(pData,",");
                pData++;
            }
            satellites_num = atoi(pData);
            GPS_LOG_INFO("Longitude = %f%c,Latitude = %f%c,satellites_num = %d\n",Longitude ,EW,Latitude ,NS,satellites_num );
        }
        sprintf_s(gps_data,sizeof(gps_data),"Longitude = %f%c,Latitude = %f%c",Longitude,EW,Latitude ,NS);
    }
    return HI_ERR_SUCCESS;
}
static hi_void *gps_demo_task(hi_void *param)
{
    hi_u8 uart_buff[UART_BUFF_SIZE] = {0};
    hi_u8 temp[20] = {0};
    hi_u8 *uart_buff_ptr = uart_buff;
    hi_unref_param(param);
    GPS_LOG_INFO("Initialize gps demo successfully\n");
    int offset = 0;
    
    for (;;) {
        hi_s32 len = hi_uart_read(GPS_UART_NUM, temp, UART_BUFF_SIZE);
        if (len > 0) {
#ifdef WRITE_BY_INT
            /*test log*/
            GPS_LOG_INFO("%s",temp);
            memcpy_s(uart_buff_ptr + offset, UART_BUFF_SIZE - offset, (hi_void *) (uintptr_t)(temp), len);
            offset += len;
            if(len < 16) {
                offset = 0;
                get_gps(gps_data,(char *)uart_buff_ptr);
                memset(uart_buff_ptr, 0, UART_BUFF_SIZE);
            }            
#else
            hi_uart_write_immediately(GPS_UART_NUM, uart_buff_ptr, len);
#endif
        
        } else {
            GPS_LOG_INFO("Read nothing!\n");
            hi_sleep(1000); /* sleep 1000ms */
        }
        hi_sleep(1000);
    }

    hi_task_delete(g_uart_demo_task_id);
    g_uart_demo_task_id = 0;

    return HI_NULL;
}
hi_void gps_demo(hi_void)
{
    hi_u32 ret;
    
    hi_uart_attribute uart_attr = {
        .baud_rate = 9600, /* baud_rate: 115200 */
        .data_bits = 8,      /* data_bits: 8bits */
        .stop_bits = 1,
        .parity = 0,
    };

    /* Initialize uart driver */
    ret = hi_uart_init(GPS_UART_NUM, &uart_attr, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        GPS_LOG_ERROR("Failed to init uart! Err code = %d\n", ret);
        return;
    }

    /* Create a task to handle uart communication */
    hi_task_attr attr = {0};
    attr.stack_size = UART_DEMO_TASK_STAK_SIZE;
    attr.task_prio = UART_DEMO_TASK_PRIORITY;
    attr.task_name = (hi_char*)"gps_demo";

    ret = hi_task_create(&g_uart_demo_task_id, &attr, gps_demo_task, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        GPS_LOG_ERROR("Falied to create gps demo task!\n");
    }

}

