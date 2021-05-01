#include <hi_early_debug.h>
#include <hi_task.h>
#include <stdio.h>
#include <hi_adc.h>
#include <hi_gpio.h>
#include <hi_io.h>
#include <hi_stdlib.h>
#include <hi_i2c.h>
#include <ssd1306_oled.h>

#define WATER_SENSOR_TASK_SIZE  ( 1024*2 )
#define WATER_SENSOR_TASK_PRIO  ( 28 )
#define ADC_LENGTH         ( 20 )
#define VLT_MIN (100)

hi_u16 g_water_sensor_adc_buf[ADC_LENGTH];

extern hi_u32 oled_init(hi_void);
extern hi_void oled_show_str(hi_u8 x, hi_u8 y, hi_u8 *chr, hi_u8 char_size);

hi_void gpio_config() {
    hi_io_set_func(HI_IO_NAME_GPIO_13, HI_IO_FUNC_GPIO_13_I2C0_SDA);
    hi_io_set_func(HI_IO_NAME_GPIO_14, HI_IO_FUNC_GPIO_14_I2C0_SCL);
}

hi_void water_sensor_display(hi_void)
{
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400kbps */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);
    oled_init();

    oled_fill_screen(OLED_CLEAN_SCREEN);//clear screen  
}

hi_void water_sensor_gpio_init(hi_void)
{
    hi_io_set_func(HI_IO_NAME_GPIO_7, HI_IO_FUNC_GPIO_7_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_7, HI_GPIO_DIR_IN);
}

hi_void *water_sensor(hi_void *param)
{
    hi_u32 ret;
    hi_u16 data;
    float voltage;
    hi_float vlt_max = 0;
    hi_float vlt_min = VLT_MIN;
    hi_u32 i;
    
    while (1) {
        memset_s(g_water_sensor_adc_buf, sizeof(g_water_sensor_adc_buf), 0x0, sizeof(g_water_sensor_adc_buf));
        for (i = 0; i < ADC_LENGTH; i++) {
            vlt_max =0;
            ret = hi_adc_read(HI_ADC_CHANNEL_3, &data, HI_ADC_EQU_MODEL_4, HI_ADC_CUR_BAIS_DEFAULT, 0xF0); //CHANNAL 3 GPIO 7
            if (ret != HI_ERR_SUCCESS) {
                printf("ADC Read Fail\n");
                return HI_NULL;
            }
            printf("adc original data %d \n", data);
            voltage = (float)data * 1.8 * 4 / 4096.0;  /* vlt * 1.8 * 4 / 4096.0为将码字转换为电压 */
            vlt_max = (voltage > vlt_max) ? voltage : vlt_max;
            vlt_min = (voltage < vlt_min) ? voltage : vlt_min;
            printf( "vlt_min:%.3f, vlt_max:%.3f \n", vlt_min, vlt_max );       
            if (vlt_max > 1) {
                oled_show_str(0, 4, "is Rainning", 16);/*将检测结果显示在屏幕上，如果检测到水分，则显示“正在下雨*/
            } else if (vlt_max < 1) {
                oled_show_str(0, 4, "a sunny day", 16);/*将检测结果显示在屏幕上，如果没有检测到水分，则显示“阳光明媚”*/
            }
        }
        hi_sleep(100);
    }
}

hi_u32 water_sensor_task(hi_void)
{
    hi_u32 ret;
    hi_task_attr attr ={0};
    hi_u32 water_sensor_id;
    gpio_config();
    /*adc gpio init*/
    water_sensor_gpio_init();
    /*oled display*/
    water_sensor_display();

    attr.stack_size = WATER_SENSOR_TASK_SIZE;
    attr.task_prio = WATER_SENSOR_TASK_PRIO;
    attr.task_name = (hi_char*)"water_sensor_task";
    ret = hi_task_create(&water_sensor_id, &attr, water_sensor, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to create water_sensor_task\r\n");
    }
    return HI_ERR_SUCCESS;
}