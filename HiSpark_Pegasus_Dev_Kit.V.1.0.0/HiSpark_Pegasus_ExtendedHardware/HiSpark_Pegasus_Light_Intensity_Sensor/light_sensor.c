#include <stdio.h>
#include <stdlib.h>
#include <hi_io.h>
#include <hi_gpio.h>
#include <hi_adc.h>
#include "light_sensor.h"

hi_void lightsensor(hi_void)
{
    hi_u16 data = 0;
    hi_u32 ret = 0;

    hi_io_set_func(HI_IO_NAME_GPIO_11, HI_IO_FUNC_GPIO_11_GPIO);  //采集传感器数据的引脚
    hi_gpio_set_dir(HI_GPIO_IDX_11,HI_GPIO_DIR_IN);
    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_GPIO);  //主板上的灯
    hi_gpio_set_dir(HI_GPIO_IDX_9, HI_GPIO_DIR_OUT);
    
    while(1){
        ret = hi_adc_read(HI_ADC_CHANNEL_5, &data, HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, 0xF0);
        if (ret != HI_ERR_SUCCESS) {
                printf("ADC Read Fail\n");
                return  HI_NULL;
        }
        printf("data :%d \r\n",data); 

        //光强度阈值 300
        if (data > 300) {
            hi_gpio_set_ouput_val(HI_GPIO_IDX_9, HI_GPIO_VALUE1);   
        } else {
            hi_gpio_set_ouput_val(HI_GPIO_IDX_9, HI_GPIO_VALUE0);
        }

        hi_sleep(500);
    }
}
