/*
 * Copyright (c) 2020 HiHope Community.
 * Description: mq2 demo
 * Author: HiSpark Product Team.
 * Create: 2020-5-20
 */
#include <math.h>
#include <hi_early_debug.h>
#include <hi_task.h>
#include <hi_time.h>
#include <hi_adc.h>
#include <hi_stdlib.h>
#include <hi_watchdog.h>
#include <hi_i2c.h>
#include <hi_pwm.h>
#include <hi_io.h>
#include <hi_gpio.h>



#include "app_demo_bmp280.h"
#define I2C_EXAMPLE_MASTER_NUM 0


#define HI_I2C_BAUDRATE 400000
/**
*   code auto select ic
*   0->BMP280
*   1->BME280
*   2->BMP180
*/


static unsigned long int  temp_raw, pres_raw;
static signed long int t_fine;

static hi_u16 dig_T1;
static hi_s16 dig_T2;
static hi_s16 dig_T3;
static hi_u16 dig_P1;
static hi_s16 dig_P2;
static hi_s16 dig_P3;
static hi_s16 dig_P4;
static hi_s16 dig_P5;
static hi_s16 dig_P6;
static hi_s16 dig_P7;
static hi_s16 dig_P8;
static hi_s16 dig_P9;

hi_u16 gpio_init() {
    hi_io_set_func(HI_IO_NAME_GPIO_13, HI_IO_FUNC_GPIO_13_I2C0_SDA);
    hi_io_set_func(HI_IO_NAME_GPIO_14, HI_IO_FUNC_GPIO_14_I2C0_SCL);
}

hi_u32 i2c_m_read(hi_i2c_idx id,hi_u8 reg,hi_u8* buf,hi_u8 len)
{
    hi_i2c_data i2c_data = {0};
    i2c_data.receive_len = len;
    i2c_data.receive_buf = buf;
    hi_u8 ret = 0;
    i2c_data.send_len = 1;
    i2c_data.send_buf = &reg;
    ret = hi_i2c_write(id, 0x77<<1, &i2c_data);

    ret = hi_i2c_read(id, 0x77<<1|1, &i2c_data);
	return ret;
}
hi_u32 i2c_m_write(hi_i2c_idx id,hi_u8 addr, hi_u8* data,hi_u8 len)
{
    hi_i2c_data i2c_data = {0};
    hi_u8 buf[20] = {0};
    buf[0] = addr;
    memcpy_s(buf+1,19,data,len);
    i2c_data.send_len = 1 + len;
    i2c_data.send_buf = buf;
    hi_u32 ret = 0;
    ret = hi_i2c_write(id, 0x77<<1, &i2c_data);
    return ret;
}
/*  软复位芯片 */
static hi_void chip_reset(hi_i2c_idx i2c_num)
{
    hi_u8 rbuf = BMX280_RESET_VALUE;
    i2c_m_write(i2c_num,BMX280_RESET_REG, &rbuf, 1);
}
/*
*
*/
static hi_void readTrim(hi_i2c_idx i2c_num)
{
    hi_u8 data[32]= {0};

    
        i2c_m_read(i2c_num, BMP280_DIG_T1_LSB_REG, &data[0], 24);

        
        dig_T1 = (data[1] << 8) | data[0];
        dig_T2 = (data[3] << 8) | data[2];
        dig_T3 = (data[5] << 8) | data[4];
        dig_P1 = (data[7] << 8) | data[6];
        dig_P2 = (data[9] << 8) | data[8];
        dig_P3 = (data[11] << 8) | data[10];
        dig_P4 = (data[13] << 8) | data[12];
        dig_P5 = (data[15] << 8) | data[14];
        dig_P6 = (data[17] << 8) | data[16];
        dig_P7 = (data[19] << 8) | data[18];
        dig_P8 = (data[21] << 8) | data[20];
        dig_P9 = (data[23] << 8) | data[22];

}

/** @brief init bme(p)280 chip 此处主要是启动芯片和做兼容判断
*   BME280多了湿度参数，处理数据时需要加上
*/
static hi_u8 bme280_bmp280_init(hi_i2c_idx i2c_num)
{
    hi_u8 rbuf;
    hi_u8 osrs_t = 1;             //Temperature oversampling x 1
    hi_u8 osrs_p = 1;             //Pressure oversampling x 1
    hi_u8 mode = 0;               //Sleep mode
    hi_u8 t_sb = 7;               //Tstandby 4000ms
    hi_u8 filter = 0;             //Filter off
    hi_u8 spi3w_en = 0;           //3-wire SPI Disable
    hi_u8 ctrl_meas_reg = (osrs_t << 5) | (osrs_p << 2) | mode;
    hi_u8 config_reg    = (t_sb << 5) | (filter << 2) | spi3w_en;
    //i2c_addr = BMX280_I2C_ADDR;
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_BAUDRATE); /* baudrate: 400kbps */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_BAUDRATE); //初始化I2C 速率为400k
    /* 读芯片ID，做BME280和BMP280的兼容*/
    i2c_m_read(i2c_num, BMX280_CHIPID_REG, &rbuf, 1); //id
    hi_udelay(100000);
    if (rbuf == BMP280_CHIP_ID) {
        printf("Get BMP280 sensor...\n");
    }else{
        printf( "No chip id, Get fail sensor.....\n");
    }
    readTrim(i2c_num);
    chip_reset(i2c_num);

    /*手册提到只要2ms就可以启动，可根据时间情况加减 */
    hi_sleep(2);
    i2c_m_write(i2c_num,BMX280_CTRLMEAS_REG, &ctrl_meas_reg, 1);
    i2c_m_write(i2c_num,BMX280_CONFIG_REG, &config_reg, 1);

    /* 解决开机第一次数据不稳的问题 */
    hi_sleep(2);
    
    return 1;
}


static signed long int calibration_T(signed long int adc_T)
{
    signed long int var1, var2, T;
    var1 = ((((adc_T >> 3) - ((signed long int)dig_T1 << 1))) * ((signed long int)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((signed long int)dig_T1)) * ((adc_T >> 4) - ((signed long int)dig_T1))) >> 12) * ((signed long int)dig_T3)) >> 14;

    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

static unsigned long int calibration_P(signed long int adc_P)
{
    signed long int var1, var2;
    unsigned long int P;
    var1 = (((signed long int)t_fine) >> 1) - (signed long int)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((signed long int)dig_P6);
    var2 = var2 + ((var1 * ((signed long int)dig_P5)) << 1);
    var2 = (var2 >> 2) + (((signed long int)dig_P4) << 16);
    var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((signed long int)dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((signed long int)dig_P1)) >> 15);
    if (var1 == 0)
    {
        return 0;
    }
    P = (((unsigned long int)(((signed long int)1048576) - adc_P) - (var2 >> 12))) * 3125;
    if (P < 0x80000000)
    {
        P = (P << 1) / ((unsigned long int) var1);
    }
    else
    {
        P = (P / (unsigned long int)var1) * 2;
    }
    var1 = (((signed long int)dig_P9) * ((signed long int)(((P >> 3) * (P >> 3)) >> 13))) >> 12;
    var2 = (((signed long int)(P >> 2)) * ((signed long int)dig_P8)) >> 13;
    P = (unsigned long int)((signed long int)P + ((var1 + var2 + dig_P7) >> 4));
    return P;
}


static void readData(hi_i2c_idx i2c_num)
{
    hi_u8 data[8];
    i2c_m_read(I2C_EXAMPLE_MASTER_NUM, BMX280_PRESSURE_MSB_REG, &data[0], 6);
    pres_raw = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    temp_raw = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);

}

float calcAltitude(hi_s32 pressure)
{
    return (float)(101325 - pressure)*9 /100; 
}

hi_void i2c_task_bmp280(hi_void)
{
    hi_u8 rbuf;
    double temp_act = 0.0, press_act = 0.0;
    signed long int temp_cal;
    unsigned long int press_cal = 0;
    float altitude = 0;  
    hi_s32 for_print_temp,for_print_temp1,for_print_temp2;
    gpio_init(); 
   //bmp280初始化
    bme280_bmp280_init(I2C_EXAMPLE_MASTER_NUM);
    while(1)
    {
        // Set forced mode again and again.
        i2c_m_read(I2C_EXAMPLE_MASTER_NUM, BMX280_CTRLMEAS_REG, &rbuf, 1); //status
        rbuf &= (~0x03);
        rbuf &= 0xff;
        rbuf |= 0x1;
        // write to Forced mode
        i2c_m_write(I2C_EXAMPLE_MASTER_NUM, BMX280_CTRLMEAS_REG, &rbuf, 1);
        //status 不为0时，可能数据没有准备好，或数据不稳定
        i2c_m_read(I2C_EXAMPLE_MASTER_NUM,BMX280_STATUS_REG, &rbuf, 1);
        //读取温度、气压
        readData(I2C_EXAMPLE_MASTER_NUM); 
        //温度、气压数据处理
        temp_cal = calibration_T(temp_raw);
        press_cal = calibration_P(pres_raw);
        temp_act = (double)temp_cal / 100.0;
        press_act = (double)press_cal / 100.0;
        
        for_print_temp = (int)temp_act;
        for_print_temp1 = (int)press_act;
        printf( "TEMP : %d.%d C PRESS : %d.%dhPa ",
                    for_print_temp,(int)((temp_act-for_print_temp)*100),
                    for_print_temp1, (int)((press_act-for_print_temp1)*100));
        altitude =  calcAltitude(press_cal);
        for_print_temp2 = (int)altitude;
        printf("altitude:  %d.%dm\n",for_print_temp2, (int)((altitude - for_print_temp2)*100));            
        hi_sleep(1000);            

    }
}



