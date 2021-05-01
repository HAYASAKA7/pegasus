#include <hi_early_debug.h>
#include <hi_task.h>
#include <stdio.h>
#include <hi_adc.h>
#include <hi_gpio.h>
#include <hi_io.h>
#include <hi_stdlib.h>
#include <hi_pwm.h>

#define ROTATION_SENSOR_TASK_SIZE    ( 1024*2 )
#define ROTATION_SENSOR_TASK_PRIO    ( 27 )
#define ADC_LENGTH    ( 20 )
#define VLT_MIN    ( 100 )
#define ADC_READ_DATA    ( 200 )

hi_u16 g_rotation_sensor_adc_buf[ ADC_LENGTH ];
hi_u16 data  = 0;
extern hi_void all_light_out( hi_void );

hi_void rotation_sensor_gpio_init( hi_void )
{
    hi_io_set_func( HI_IO_NAME_GPIO_7, HI_IO_FUNC_GPIO_7_GPIO );
    hi_gpio_set_dir( HI_GPIO_IDX_7, HI_GPIO_DIR_IN );
}

hi_void light_led_init( hi_void )
{
    hi_io_set_pull( HI_IO_NAME_GPIO_10, HI_IO_PULL_DOWN );
    hi_io_set_pull( HI_IO_NAME_GPIO_11, HI_IO_PULL_DOWN );
    hi_io_set_pull( HI_IO_NAME_GPIO_12, HI_IO_PULL_DOWN );
}
hi_void sensor_all_light_dark_to_bright( hi_void )
{
    hi_pwm_start( HI_PWM_PORT_PWM1, data, 1920 );
    hi_pwm_start( HI_PWM_PORT_PWM2, data, 1920 );
    hi_pwm_start( HI_PWM_PORT_PWM3, data, 1920 );
}

hi_void colorful_light_stepless_dimming( hi_void )
{
    hi_u32 ret;
    hi_float voltage;
    hi_float vlt_max = 0;
    hi_float vlt_min = VLT_MIN;

    memset_s( g_rotation_sensor_adc_buf, sizeof( g_rotation_sensor_adc_buf ), 0x0, sizeof( g_rotation_sensor_adc_buf ));
    
    for (int i = 0; i < ADC_LENGTH; i++) {
        vlt_max =0;
        ret = hi_adc_read( HI_ADC_CHANNEL_3, &data, HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, 0xF0 ); //CHANNAL 3 GPIO 7
        if ( ret != HI_ERR_SUCCESS ) {
            printf( "ADC Read Fail\n" );
            return HI_NULL;
        }
        voltage = ( float )data * 1.8 * 4 / 4096.0;  /* vlt * 1.8 * 4 / 4096.0为将码字转换为电压 */
        vlt_max = (voltage > vlt_max) ? voltage : vlt_max;
        vlt_min = (voltage < vlt_min) ? voltage : vlt_min;
        if ( data > ADC_READ_DATA ) {
            sensor_all_light_dark_to_bright();
        }
          
    }
}

hi_void *rotation_sensor( hi_void *param )
{
    hi_u32 ret;
    float voltage;
    hi_float vlt_max = 0;
    hi_float vlt_min = VLT_MIN;
    hi_u16 vlt;
    while (1) {
        memset_s(g_rotation_sensor_adc_buf, sizeof(g_rotation_sensor_adc_buf), 0x0, sizeof(g_rotation_sensor_adc_buf));
        for (int i = 0; i < ADC_LENGTH; i++) {
            vlt_max =0;
            ret = hi_adc_read(HI_ADC_CHANNEL_2, &data, HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, 0); //CHANNAL 2 GPIO 5
            if (ret != HI_ERR_SUCCESS) {
                printf("ADC Read Fail\n");
                return HI_NULL;
            }
            g_rotation_sensor_adc_buf[ i ] = data;
            printf( "vlt_min:%.3f, vlt_max:%.3f \n", vlt_min, vlt_max );
        }
        for ( int i = 0; i < ADC_LENGTH; i++) {
            vlt = g_rotation_sensor_adc_buf[ i ];
            float voltage = (float)data * 1.8 * 4 / 4096.0;  /* vlt * 1.8 * 4 / 4096.0为将码字转换为电压 */
            vlt_max = (voltage > vlt_max) ? voltage : vlt_max;
            vlt_min = (voltage < vlt_min) ? voltage : vlt_min;
        }    
        printf( "vlt_min:%.3f, vlt_max:%.3f \n", vlt_min, vlt_max );              
        hi_sleep(100);
    }
}

hi_u32 rotation_sensor_task( hi_void )
{
    hi_u32 ret;
    hi_task_attr attr = { 0 };
    hi_u32 ration_sensor_id;
    /*adc gpio init*/
    rotation_sensor_gpio_init();
    /*led gpio init*/
    light_led_init();

    attr.stack_size = ROTATION_SENSOR_TASK_SIZE;
    attr.task_prio = ROTATION_SENSOR_TASK_PRIO;
    attr.task_name = (hi_char*)"rotation_sensor_task";
    ret = hi_task_create(&ration_sensor_id, &attr, rotation_sensor, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf( "Failed to create rotation_sensor_task\r\n" );
    }
}