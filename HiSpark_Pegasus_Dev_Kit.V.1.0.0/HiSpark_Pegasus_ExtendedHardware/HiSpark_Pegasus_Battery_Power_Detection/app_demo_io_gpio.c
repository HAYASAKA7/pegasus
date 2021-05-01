/*
 * Copyright (c) 2020, HiHope Community.. All rights reserved.
 * Description: app io\gpio demo
 * Author: Hisilicon
 * Create: 2019-12-10
 */
#include <app_demo_io_gpio.h>


#include <hi_early_debug.h>
#include <ssd1306_oled.h>
#include <hi_watchdog.h>
#include <led_dispaly.h>
#include <code_tab.h>
#include <hi_time.h>
#include <los_hwi.h>
#include <hi_task.h>
#include <string.h>
#include <hi_mux.h>
#include "hi_i2c.h"



#define I2C_REG_ARRAY_LEN           (64)
#define BH_SEND_BUFF                (1)
#define OLED_SEND_BUFF_LEN          (28)
#define OLED_SEND_BUFF_LEN2         (25)
#define OLED_SEND_BUFF_LEN3         (27)
#define OLED_SEND_BUFF_LEN4         (29)
#define Max_Column                  (128)
#define OLED_DEMO_TASK_STAK_SIZE    (1024)
#define OLED_DEMO_TASK_PRIORITY     (25)
#define CAR_STOP_STATUS             (0)
#define CAR_RUNNING_STATUS          (1)
#define CAR_TRACE_STATUS            (2)


static hi_u8 hi3861_board_led_test =0;
extern hi_u8  g_car_status;

hi_u8 g_oled_demo_task_id =0;

hi_u16 gpio_init() {
    hi_io_set_func(HI_IO_NAME_GPIO_13, HI_IO_FUNC_GPIO_13_I2C0_SDA);
    hi_io_set_func(HI_IO_NAME_GPIO_14, HI_IO_FUNC_GPIO_14_I2C0_SCL);
}

hi_u8 g_oled_send_buff[OLED_SEND_BUFF_LEN]={
    0xAE,0x20,0x02,0xB3,0xc8,0x00,0x1f,
    0x40,0x81,0xff,0xa1,0xa6,0xa8,0x3F,0xa4,
    0xd3,0x00,0xd5,0xf0,0xd9,0x22,0xda,0x12,
    0xdb,0x20,0x8d,0x14,0xaf 
};
hi_u8 g_oled_send_buff2[OLED_SEND_BUFF_LEN2]={
    0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,
    0x8D,0x14,0x20,0x02,0xA1,0xC0,0xDA,0x12,
    0x81,0xEF,0xD9,0xf1,0xDB,0x30,0xA4,0xA6,
    0xAF, 
};
hi_u8 g_oled_send_buff3[OLED_SEND_BUFF_LEN3] ={
    0xAE,0x00,0x10,0x40,0xB0,0x81,0xFF,0xA1,
    0xA6,0xA8,0x3F,0xC8,0xD3,0x00,0xD5,0x80,
    0xD8,0x05,0xD9,0xF1,0xDA,0x12,0xDB,0x30,
    0x8D,0x14,0xAF
};
hi_u8 g_oled_send_buff4[OLED_SEND_BUFF_LEN4] = {
    0xae,0xb0,0x10,0x40,0x81,0xff,0xa1,0xc8,
    0xa6,0xa8,0x3f,0xd3,0x00,0xd5,0x80,0xd9,
    0xf1,0xda,0x12,0xdb,0x40,0x20,0x02,0x8d,
    0x14,0xa4,0xa6,0xaf
};

/*factory test HiSpark board*/
hi_void hispark_board_test(hi_gpio_value value)
{
    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_9, HI_GPIO_DIR_OUT);
    hi_gpio_set_ouput_val(HI_GPIO_IDX_9, value);
}

#define FACTORY_HISPARK_BOARD_TEST(fmt, ...) \
do \
{ \
    printf(fmt, ##__VA_ARGS__); \
     for (hi_s32 i=0; i<3; i++) \
     {  \
        hispark_board_test(HI_GPIO_VALUE0); \
        hi_udelay(DELAY_5_MS); \
        hispark_board_test(HI_GPIO_VALUE1); \
        hi_udelay(DELAY_5_MS); \
    }  \
} while(0)

/* gpio callback func */
hi_void gpio_isr_func(hi_void *arg)
{
    hi_unref_param(arg);
    printf("----- gpio isr success -----\r\n");
}

hi_void io_gpio_demo(hi_void)
{
    /* Take gpio 0 as an example */
    hi_u32 ret;
    hi_gpio_value gpio_val = HI_GPIO_VALUE1;

    ret = hi_gpio_init();
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ===== gpio -> hi_gpio_init ret:%d\r\n", ret);
        return;
    }
    printf("----- gpio init success-----\r\n");

    ret = hi_io_set_func(HI_IO_NAME_GPIO_0, HI_IO_FUNC_GPIO_0_GPIO);
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ===== gpio -> hi_io_set_func ret:%d\r\n", ret);
        return;
    }
    printf("----- io set func success-----\r\n");

    ret = hi_gpio_set_dir(HI_GPIO_IDX_0, HI_GPIO_DIR_IN);
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ===== gpio -> hi_gpio_set_dir1 ret:%d\r\n", ret);
        return;
    }
    printf("----- gpio set dir success! -----\r\n");

    ret = hi_gpio_get_input_val(HI_GPIO_IDX_0, &gpio_val);
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ===== gpio -> hi_gpio_get_input_val ret:%d\r\n", ret);
        return;
    }
    printf("----- gpio input val is:%d. -----\r\n", gpio_val);

    ret = hi_gpio_deinit();
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ===== gpio -> hi_gpio_deinit ret:%d\r\n", ret);
        return;
    }
    printf("----- gpio deinit success-----\r\n");
}

/* gpio callback demo */
hi_void gpio_isr_demo(hi_void)
{
    hi_u32 ret;

    printf("----- gpio isr demo -----\r\n");

    (hi_void)hi_gpio_init();

    ret = hi_gpio_set_dir(HI_GPIO_IDX_0, HI_GPIO_DIR_IN);
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ======gpio -> hi_gpio_set_dir1 ret:%d\r\n", ret);
        return;
    }

    ret = hi_gpio_register_isr_function(HI_GPIO_IDX_0, HI_INT_TYPE_LEVEL,
                                        HI_GPIO_EDGE_RISE_LEVEL_HIGH, gpio_isr_func, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("===== ERROR ======gpio -> hi_gpio_register_isr_function ret:%d\r\n", ret);
    }
}

hi_void app_demo_custom_io_gpio(hi_void)
{
    io_gpio_demo();
    hi_sleep(1000);  /* sleep 1000ms */
    gpio_isr_demo();
}














/*I2C write Byte*/
static hi_u32 i2c_write_byte(hi_u8 reg_addr, hi_u8 cmd)
{
    hi_u32 status = 0;
    hi_i2c_idx id = 0;//I2C 0
    hi_u8 user_data = cmd;
    hi_i2c_data oled_i2c_cmd = { 0 };
    hi_i2c_data oled_i2c_write_cmd = { 0 };

    hi_u8 send_user_cmd [2] = {0x00,user_data};
    hi_u8 send_user_data [2] = {0x40,user_data};

    /*如果是写命令，发写命令地址0x00*/
    if (reg_addr == 0x00) {
        oled_i2c_write_cmd.send_buf = send_user_cmd;
        oled_i2c_write_cmd.send_len = 2;
        status = hi_i2c_write(id, OLED_ADDRESS, &oled_i2c_write_cmd);
        if (status != HI_ERR_SUCCESS) {
            // printf("===== Error: SSD1306 OLED Screen I2C write status = 0x%x! =====\r\n", status);
            return status;
        }
    }
    /*如果是写数据，发写数据地址0x40*/
    else if (reg_addr == 0x40){
        oled_i2c_cmd.send_buf = send_user_data;
        oled_i2c_cmd.send_len = 2;
        status = hi_i2c_write(id, OLED_ADDRESS, &oled_i2c_cmd);
        if (status != HI_ERR_SUCCESS) {
            // printf("===== Error: SSD1306 OLED Screen I2C write status = 0x%x! =====\r\n", status);
            return status;
        }
    }

    return HI_ERR_SUCCESS;
}

static hi_u32 I2C_WriteMutilByte(hi_u8* cmd, hi_u8 send_len)
{
    hi_u32 status;
    hi_i2c_idx id = 0;//I2C 0
    hi_u8 send_i2C_write_cmd_addr[1] = {0x00};//写
    hi_i2c_data oled_i2c_write_cmd_addr= {0};
    hi_i2c_data oled_i2c_cmd = { 0 };
    /*先发0x00 */
    oled_i2c_write_cmd_addr.send_buf = send_i2C_write_cmd_addr;
    oled_i2c_write_cmd_addr.send_len = 1;
    status = hi_i2c_write(id, OLED_ADDRESS, &oled_i2c_write_cmd_addr);
    // if (status != HI_ERR_SUCCESS) {
    //     printf("===== Error: SSD1306 OLED Screen I2C write cmd address status = 0x%x! =====\r\n", status);
    //     return status;
    // }
    /*再发初始化具体操作*/
    oled_i2c_cmd.send_buf = cmd;
    oled_i2c_cmd.send_len = send_len;
    status = hi_i2c_write(id, OLED_ADDRESS, &oled_i2c_cmd);
    // if (status != HI_ERR_SUCCESS) {
    //     printf("===== Error: SSD1306 OLED Screen I2C write status = 0x%x! =====\r\n", status);
    //     return status;
    // }
    return HI_ERR_SUCCESS;
}

/*OLED write cmd*/
static hi_u32 write_cmd(hi_u8 cmd)//写命令
{
    hi_u8 status = 0;
    /*写设备地址*/
	status = i2c_write_byte(OLED_ADDRESS_WRITE_CMD, cmd);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
         //printf("===== Error!!! Write Cmd : write device address cmd failed = 0x%x, %d =====\r\n", status, __LINE__);
    }
}

/*OlED write data*/
static hi_void write_data(hi_u8 I2C_Data)//写数据
{
    hi_u8 status = 0;
    /*写设备地址*/
	status = i2c_write_byte(OLED_ADDRESS_WRITE_DATA, I2C_Data);
    //     if (status != HI_ERR_SUCCESS) {
    //     printf("===== Error!!! Write Data :  write device address failed = 0x%x, %d =====\r\n", status, __LINE__);
    // }
}

/* ssd1306 oled 初始化*/
hi_u32 oled_init(hi_void)
{
    hi_u32 status;
    hi_udelay(DELAY_100_MS);//100ms  这里的延时很重要

    status = write_cmd(DISPLAY_OFF);//--display off
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_LOW_COLUMN_ADDRESS);//---set low column address
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_HIGH_COLUMN_ADDRESS);//---set high column address
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_START_LINE_ADDRESS);//--set start line address
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_PAGE_ADDRESS);//--set page address
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(CONTRACT_CONTROL);// contract control
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(FULL_SCREEN);//--128
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }  
    status= write_cmd(SET_SEGMENT_REMAP);//set segment remap
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    } 
    status = write_cmd(NORMAL);//--normal / reverse
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status =write_cmd(SET_MULTIPLEX);//--set multiplex ratio(1 to 64)
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DUTY);//--1/32 duty
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SCAN_DIRECTION);//Com scan direction
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DISPLAY_OFFSET);//-set display offset
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DISPLAY_TYPE);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(OSC_DIVISION);//set osc division
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(DIVISION);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(COLOR_MODE_OFF);//set area color mode off
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status= write_cmd(COLOR);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PRE_CHARGE_PERIOD);//Set Pre-Charge Period
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PERIOD);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PIN_CONFIGUARTION);//set com pin configuartion
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(CONFIGUARTION);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_VCOMH);//set Vcomh
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(VCOMH);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(SET_CHARGE_PUMP_ENABLE);//set charge pump enable
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(PUMP_ENABLE);
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    status = write_cmd(TURN_ON_OLED_PANEL);//--turn on oled panel
    if (status != HI_ERR_SUCCESS) {
        return HI_ERR_FAILURE;
    }
    return HI_ERR_SUCCESS;
}

/*
    @bref set start position 设置起始点坐标
    @param hi_u8 x:write start from x axis
           hi_u8 y:write start from y axis
*/
hi_void oled_set_position(hi_u8 x, hi_u8 y) 
{ 
    write_cmd(0xb0+y);
    write_cmd(((x&0xf0)>>4)|0x10);
    write_cmd(x&0x0f);
}

/*全屏填充*/
hi_void oled_fill_screen(hi_u8 fii_data)
{
    hi_u8 m =0;
    hi_u8 n =0;

    for (m=0; m<8; m++) {
        write_cmd(0xb0+m);
        write_cmd(0x00);
        write_cmd(0x10);
        for (n=0; n<128; n++) {
            write_data(fii_data);
        }
    }
}

/*
    @bref Clear from a location
    @param hi_u8 fill_data: write data to screen register 
           hi_u8 line:write positon start from Y axis 
           hi_u8 pos :write positon start from x axis 
           hi_u8 len:write data len
*/
hi_void oled_position_clean_screen(hi_u8 fill_data, hi_u8 line, hi_u8 pos, hi_u8 len)
{
    hi_u8 m =line;
    hi_u8 n =0;

    write_cmd(0xb0+m);
    write_cmd(0x00);
    write_cmd(0x10);

    for (n=pos;n<len;n++) {
        write_data(fill_data);
    }   
}


/*
    @bref 8*16 typeface
    @param hi_u8 x:write positon start from x axis 
           hi_u8 y:write positon start from y axis
           hi_u8 chr:write data
           hi_u8 char_size:select typeface
 */
hi_void oled_show_char(hi_u8 x, hi_u8 y, hi_u8 chr,hi_u8 char_size)
{      	
	hi_u8 c=0;
    hi_u8 i=0;

    c = chr-' '; //得到偏移后的值	

    if (x>Max_Column-1) {
        x=0;
        y=y+2;
    }

    if (char_size ==16) {
        oled_set_position(x,y);	
        for(i=0;i<8;i++){
            write_data(F8X16[c*16+i]);
        }
        
        oled_set_position(x,y+1);
        for (i=0;i<8;i++) {
            write_data(F8X16[c*16+i+8]);
        }
        
    } else {	
        oled_set_position(x,y);
        for (i=0;i<6;i++) {
            write_data(F6x8[c][i]);
        }            
    }
}

/*
    @bref display string
    @param hi_u8 x:write positon start from x axis 
           hi_u8 y:write positon start from y axis
           hi_u8 *chr:write data
           hi_u8 char_size:select typeface
*/  
hi_void oled_show_str(hi_u8 x, hi_u8 y, hi_u8 *chr, hi_u8 char_size)
{
	hi_u8 j=0;

    if (chr == NULL) {
        return;
    }

	while (chr[j] != '\0') {		
        oled_show_char(x, y, chr[j], char_size);
		x += 8;
		if (x>120) {
            x = 0;
            y += 2;
        }
		j++;
	}
}

/*小数转字符串
 *输入：double 小数
 *输出：转换后的字符串
*/
hi_u8  *flaot_to_string(hi_double d, hi_u8 *str)
{
	hi_u8 str1[40] = {0};
	hi_s32 j = 0;
    hi_s32 k = 0;
    hi_s32 i = 0;

    if (str == NULL) {
        return;
    }

	i = (hi_s32)d;//浮点数的整数部分
	while (i > 0) {
		str1[j++] = i % 10 + '0';
		i = i / 10;
	}

	for (k = 0;k < j;k++) {
		str[k] = str1[j-1-k];//被提取的整数部分正序存放到另一个数组
	}
	str[j++] = '.';
 
	d = d - (hi_s32)d;//小数部分提取
	for (i = 0;i < 3;i++) {
		d = d*10;
		str[j++] = (hi_s32)d + '0';
		d = d - (hi_s32)d;
	}
	while(str[--j] == '0');
	str[++j] = '\0';
	return str;
}

hi_u32 power_write(hi_i2c_idx id, hi_u16 device_addr, hi_u32 send_len, hi_u8 *g_send_data)
{
    hi_u32 status;
    hi_i2c_data ina219_i2c_data = { 0 };

    ina219_i2c_data.send_buf = g_send_data;
    ina219_i2c_data.send_len = send_len;
    status = hi_i2c_write(id, device_addr, &ina219_i2c_data);
    if (status != 0) {
        printf("===== Error: I2C write status = 0x%x! =====\r\n", status);
        return status;
    }

    return 0;
}

hi_u32 power_writeread(hi_i2c_idx id, hi_u16 device_addr, hi_u32 recv_len)
{
    hi_u8 i = 0;
    hi_u8 m = 0;
    hi_u8 n = 0;
    hi_u32 status;
    hi_u8 recv_data[64] = { 0 };
    hi_i2c_data ina219_i2c_data = { 0 };
    //hi_double d
    hi_u8 vstr[64] = {0};
    hi_double vvalue;
    hi_double cost = 6.900;
    hi_u8 g_send_data[1] = {0x02};

    //printf("----- reg addr[0] = 0x%x! -----\r\n", 0x00);

    // Request memory space 
    memset_s(recv_data, 64, 0x0, sizeof(recv_data));
    memset_s(&ina219_i2c_data, sizeof(hi_i2c_data), 0x0, sizeof(hi_i2c_data));

    ina219_i2c_data.send_buf = g_send_data;
    ina219_i2c_data.send_len = 1;
    ina219_i2c_data.receive_buf = recv_data;
    ina219_i2c_data.receive_len = recv_len;
    status = hi_i2c_writeread(id, device_addr, &ina219_i2c_data);
    if (status != 0) {
        printf("===== Error: I2C read status = 0x%x! =====\r\n", status);
        return status;
    }

    //for (i = 0; i < recv_len; i++) {
        //printf("----- revData[%d] = 0x%x! -----\r\n", i, recv_data[i]);
    //}

    vvalue = (hi_float)((((recv_data[0] << 8) + recv_data[1]) >> 3)*0.004);

    
    //printf("\r\n\r\n----- 0x%x      %d -----\r\n", (((recv_data[0] << 8) + recv_data[1]) >> 3) , (int)((((recv_data[0] << 8) + recv_data[1]) >> 3)*4));
    //printf("电压为 %.3f V\n", vvalue);
    
    flaot_to_string(vvalue, vstr);
    //printf("_______%.3f_____\n", vvalue);
    //printf("_______%s_____\n", vstr);
    if ((vvalue - cost) < 0.000) {
        oled_show_str(0, 6, "   Low power  ", 1);
    } else {
        write_cmd(0xb0+6);
        write_cmd(0x00);
        write_cmd(0x10);
        for (n=0; n<128; n++) {
            write_data(0x00);
        }
    }
    oled_show_str(0, 3, "voltage:      V", 1);
    oled_show_str(70, 3, "7.032", 1);
    //cost = (vstr - 7.900);

    return 0;
}

hi_void oled_display_init(hi_void)
{
    oled_fill_screen(0x00);//全屏灭
    hi_udelay(10000);//10ms
    oled_show_str(15,3, "Hello Hi3861",1);
    hi_udelay(2000000);//10ms
    oled_show_str(15,3, "            ",1);
    //oled_show_str(0,4,"   Robot Car   ",1);
}


hi_void *app_i2c_oled_demo(hi_void* param)
{
    //电池
    //hi_u32 ret = 0;
    //hi_i2c_init(HI_I2C_IDX_0, 100000); 
    hi_u8 g_send_data0[3] = {0x00, 0x3c, 0x1f};
    hi_u8 g_send_data1[3] = {4096};
    //if (ret == 0) ret = 1; else ret = 2;

    /*初始化时屏幕先占用I2C总线*/
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400kbps */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);
    // oled_init();
   

    while( HI_ERR_SUCCESS != oled_init()) {
         printf("Connecting oled board falied!Please access oled board\r\n");
         if (hi3861_board_led_test == 0) {
            hi3861_board_led_test =1;
             /*test HiSpark board*/
            FACTORY_HISPARK_BOARD_TEST("-----------HiSpark board test----------");  
         }
         hi_sleep(SLEEP_100_MS);

    }
    oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen
    oled_display_init();

    while (1)
    {
        /*if (g_car_status == CAR_RUNNING_STATUS) {
            oled_show_str(0,3," Robot Car Run  ",1);
         } else if (g_car_status == CAR_STOP_STATUS) {
            oled_show_str(0,3," Robot Car Stop ",1);
        }*/
        // if (g_car_status == CAR_RUNNING_STATUS) {
        //     oled_show_str(0,3,"   ultrasonic      ",1);
        //  } else if (g_car_status == CAR_STOP_STATUS) {
        //     oled_show_str(0,3," Robot Car Stop    ",1);
        // }  else if (g_car_status == CAR_TRACE_STATUS) {
        //     oled_show_str(0,3,"     trace         ",1);
        // }
        
        power_write(HI_I2C_IDX_0, 0x80, 3, g_send_data0);
        power_write(HI_I2C_IDX_0, 0x80, 1, g_send_data1);
        power_writeread(HI_I2C_IDX_0, 0x81, 2);

        hi_sleep(20);
        hi_watchdog_feed();
    }

    //hi_i2c_deinit(0);
}

hi_u32 app_oled_i2c_demo_task(void)
{
    hi_u32 ret;
    hi_task_attr attr = {0};
    /*创建互斥锁*/
    gpio_init();
    hi_task_lock();
    attr.stack_size = OLED_DEMO_TASK_STAK_SIZE*10;
    attr.task_prio = OLED_DEMO_TASK_PRIORITY;
    attr.task_name = (hi_char*)"app_oled_i2c_demo_task";
    ret = hi_task_create(&g_oled_demo_task_id, &attr, app_i2c_oled_demo, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("Falied to create i2c sht3x demo task!\n");
    }
    hi_task_unlock();
}
