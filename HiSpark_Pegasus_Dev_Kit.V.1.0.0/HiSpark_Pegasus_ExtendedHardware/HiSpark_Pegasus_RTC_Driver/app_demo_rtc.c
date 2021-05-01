#include <hi_early_debug.h>
#include <hi_i2c.h>
#include <app_demo_rtc.h>
#include <hi_task.h>
#include <hi_time.h>
#include <hi_stdlib.h>
#include <hi_errno.h>
#include <ssd1306_oled.h>
#include <hi_gpio.h>
#include <hi_io.h>

hi_u32 g_rtc_demo_task_id =0;

hi_u16 gpio_init() {
    hi_io_set_func(HI_IO_NAME_GPIO_13, HI_IO_FUNC_GPIO_13_I2C0_SDA);
    hi_io_set_func(HI_IO_NAME_GPIO_14, HI_IO_FUNC_GPIO_14_I2C0_SCL);
}

/*写寄存器*/
hi_u32 ds1307_i2c_write( hi_u8 reg_addr, hi_u8 high_8, hi_u8 low_8, hi_u8 reg_len)
{
    hi_u32 status =0;
    hi_i2c_idx id = HI_I2C_IDX_0;
    hi_i2c_data ds1307_i2c_write_cmd_addr ={0};
    hi_u8 temp1 =0; 
    hi_u8 temp2 =0;
    temp1 = (high_8/10*16)+(high_8%10); //16进制转BCD  
    temp2 = (low_8/10*16)+(low_8%10); //16进制转BCD  
    // printf("temp1= %x, temp2 = %x \r\n", temp1, temp2);
    hi_u8 _send_user_cmd[SEND_BUF_LEN] = {reg_addr, temp1, temp2};

    ds1307_i2c_write_cmd_addr.send_buf = _send_user_cmd;
    ds1307_i2c_write_cmd_addr.send_len = reg_len;
    
    status = hi_i2c_write(id, (DS1307_ADDRESS<<1)|DS1307_WRITE_ADDRESS, &ds1307_i2c_write_cmd_addr);
    if (status != HI_ERR_SUCCESS) {
        printf("===== Error: ds1307 sensor I2C write cmd address status = 0x%x! =====\r\n", status);
        return status;
    }
    return HI_ERR_SUCCESS;
}
hi_u8 *ds1307_read(hi_u8 rtc_reg, hi_u32 recv_len, hi_u8 *rct_buf)
{
    // ds1307_rtc_type read_rtc;
    hi_u32 status = 0;
    hi_i2c_idx id =HI_I2C_IDX_0;
    hi_u8 recv_data[DS1307_REG_ARRAY_LEN] = { 0 };
    hi_i2c_data ds1307_i2c_data = { 0 };

    /* Request memory space */
    memset(rct_buf, 0x00, sizeof(rct_buf));
    memset(recv_data, 0x0, sizeof(recv_data));
    memset(&ds1307_i2c_data, 0x0, sizeof(hi_i2c_data));
    ds1307_i2c_data.receive_buf = recv_data;
    ds1307_i2c_data.receive_len = recv_len;
    
    status = hi_i2c_read(id, (DS1307_ADDRESS<<1)|DS1307_READ_ADDRESS, &ds1307_i2c_data);
    if (status != HI_ERR_SUCCESS) {
        printf("===== Error: ds1307 sencor I2C read status = 0x%x! =====\r\n", status);
        return status;
    }
    switch (rtc_reg) {
    case RCT_SECOND:
         rct_buf[0] = recv_data[0];
        break;
    case RCT_MINUTE:
         rct_buf[0] = recv_data[0];
         break;
    case RCT_HOUR:
         rct_buf[0]  =  recv_data[0];
         break;
    case RCT_DAY :
         rct_buf[0] =  recv_data[0]; 
         break;
    case RCT_DATE:
         rct_buf[0]  =  recv_data[0];
         break; 
    case RCT_MONTH:
         rct_buf[0]  =  recv_data[0]; 
         break; 
    case RCT_YEAR:
         rct_buf[0]  =  recv_data[0]; 
         break;          
    default:
        break;
    }
    return rct_buf;
}
/*rtc timer setting*/
hi_void rct_set_init(hi_void)
{
    hi_u32 ret;
    ds1307_rtc_type rct_time_set ={0};
    rct_time_set.rtc_second[0] = 30;
    rct_time_set.rtc_minue[0] = 40;
    rct_time_set.rtc_hour[0] = 10;
    rct_time_set.rtc_day[0] = 4;
    rct_time_set.rtc_date[0] = 4;
    rct_time_set.rtc_month[0] = 3;
    rct_time_set.rtc_year[0] = 21;
    //set second
    ret = ds1307_i2c_write(RCT_SECOND, rct_time_set.rtc_second[0], NULL, SEND_SET_REG_LEN);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to second cmd\r\n");
    }
    hi_udelay(DELAY_TIME); 
    //set minute
    ret = ds1307_i2c_write(RCT_MINUTE, rct_time_set.rtc_minue[0], NULL, SEND_SET_REG_LEN);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to minute cmd\r\n");
    }
    hi_udelay(DELAY_TIME); 
    //set hour
    ret = ds1307_i2c_write(RCT_HOUR, rct_time_set.rtc_hour[0], NULL, SEND_SET_REG_LEN);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to hour cmd\r\n");
    }
    hi_udelay(DELAY_TIME);
    //set day
    ret = ds1307_i2c_write(RCT_DAY, rct_time_set.rtc_day[0], NULL, SEND_SET_REG_LEN);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to day cmd\r\n");
    }
    hi_udelay(DELAY_TIME);
    //set date
    ret = ds1307_i2c_write(RCT_DATE, rct_time_set.rtc_date[0], NULL, SEND_SET_REG_LEN);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to date cmd\r\n");
    }
    hi_udelay(DELAY_TIME);  
    //set month
    ret = ds1307_i2c_write(RCT_MONTH, rct_time_set.rtc_month[0], NULL, SEND_SET_REG_LEN);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to month cmd\r\n");
    }
    hi_udelay(DELAY_TIME);   
    //set year
    ret = ds1307_i2c_write(RCT_YEAR, rct_time_set.rtc_year[0], NULL, SEND_SET_REG_LEN);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to year cmd\r\n");
    }
    hi_udelay(DELAY_TIME); 
}
/*  change type*/
hi_u8 *int_to_char(hi_s32 dec, hi_u8 *str)
{
    hi_u8 str1[40] = {0};
	hi_s32 j = 0;
    hi_s32 k = 0;
    hi_s32 i = 0;

	i = (hi_s32)dec;//浮点数的整数部分
	while (i > 0) {
		str1[j++] = i % 10 + '0';
		i = i / 10;
	}

	for (k = 0;k < j;k++) {
		str[k] = str1[j-1-k];//被提取的整数部分正序存放到另一个数组
	}
    return str;
}

/*read rtc time*/
hi_void *rtc_timer(hi_void *param)
{
    hi_u8 rct_read_data[2] ={0};
    ds1307_rtc_type rtc_data ={0};
    hi_s32 temp_second =0;
    hi_s32 temp_minute =0;
    hi_s32 temp_hour =0;
    hi_s32 temp_day =0;
    hi_s32 temp_date =0;
    hi_s32 temp_month =0;
    hi_s32 temp_year =0;
    hi_u8 ch_year[3] ={0};
    hi_u8 ch_month[3] ={0};
    hi_u8 ch_day[3] ={0};
    hi_u8 ch_date[3] ={0};
    hi_u8 ch_hour[3] ={0};
    hi_u8 ch_minute[3] ={0};
    hi_u8 ch_second[3] ={0};

    hi_unref_param(param);
    /*初始化时屏幕 i2c baudrate setting*/
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400kbps */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);
    /*ssd1306 config init*/
    oled_init();
    rct_set_init(); //设置RTC时间，已经设置好了，如有需要再打开
    oled_fill_screen(OLED_CLEAN_SCREEN);//clean screen
    //oled_show_str(0, 6, "2020-06-20 19:12:00", 1);

    while (1) {

        /*----------------------second--------------*/
        ds1307_i2c_write(0x00, NULL, NULL, SEND_READ_DATA_LEN);  
        ds1307_read(RCT_SECOND, SEND_READ_DATA_LEN, rct_read_data);
        if (rtc_data.rtc_second[0] != rct_read_data[0]) {
            rtc_data.rtc_second[0] = rct_read_data[0];
            temp_second = rct_read_data[0]/16*10 + rct_read_data[0]%16;
            int_to_char(temp_second, ch_second);
            if (temp_second>=10) {               
                oled_show_str(48, 6, ch_second, 1);
            } else {
                oled_show_str(48, 6, "0", 1);
                oled_show_str(56, 6, ch_second, 1);
            }
            oled_show_str(64, 6, " ", 1);
        } 
        /*----------------------minute--------------*/
        ds1307_i2c_write(0x01, NULL, NULL, SEND_READ_DATA_LEN);   
        ds1307_read(RCT_MINUTE, SEND_READ_DATA_LEN, rct_read_data);
        if (rtc_data.rtc_minue[0] != rct_read_data[0]) {
            rtc_data.rtc_minue[0] = rct_read_data[0];
            temp_minute = rct_read_data[0]/16*10 + rct_read_data[0]%16;
            int_to_char(temp_minute, ch_minute);
            if (temp_minute >= 10) {
                oled_show_str(24, 6, ch_minute, 1);   
                oled_show_str(40, 6, ":", 1);  
            } else {
                oled_show_str(24, 6, "0", 1); 
                oled_show_str(32, 6, ch_minute, 1);     
                oled_show_str(40, 6, ":", 1);  
            }
            oled_show_str(56, 6, "0", 1);                    
        }

        /*----------------------hour--------------*/
        ds1307_i2c_write(0x02, NULL, NULL, SEND_READ_DATA_LEN);  
        ds1307_read(RCT_HOUR, SEND_READ_DATA_LEN, rct_read_data);
        if (rtc_data.rtc_hour[0] != rct_read_data[0]) {
            rtc_data.rtc_hour[0] = rct_read_data[0];
            temp_hour = rct_read_data[0]/16*10 + rct_read_data[0]%16;
            int_to_char(temp_hour, ch_hour);
            if (temp_hour >= 10) {
                oled_show_str(0, 6, ch_hour, 1);
                oled_show_str(16, 6, ":", 1);
            } else {
                oled_show_str(0, 6, "0", 1);
                oled_show_str(8, 6, ch_hour, 1);
                oled_show_str(16, 6, ":", 1);
            }
            oled_show_str(64, 6, " ", 1);
        }

        /*----------------------day-------------*/
        ds1307_i2c_write(0x03, NULL, NULL, SEND_READ_DATA_LEN);            
        ds1307_read(RCT_DAY, SEND_READ_DATA_LEN, rct_read_data);
        if (rtc_data.rtc_day[0] != rct_read_data[0]) {
            rtc_data.rtc_day[0] = rct_read_data[0];
            temp_day = rct_read_data[0]/16*10 + rct_read_data[0]%16;
            int_to_char(temp_day, ch_day);
            oled_show_str(72, 6, "week:", 1);
            oled_show_str(112, 6, ch_day, 1);
        }
        /*----------------------date--------------*/
        ds1307_i2c_write(0x04, NULL, NULL, SEND_READ_DATA_LEN);      
        ds1307_read(RCT_DATE, SEND_READ_DATA_LEN, rct_read_data);
        if (rtc_data.rtc_date[0] != rct_read_data[0]) {
            rtc_data.rtc_date[0] = rct_read_data[0];
            temp_date = rct_read_data[0]/16*10 + rct_read_data[0]%16;
            int_to_char(temp_date, ch_date);
            if (temp_date >= 10) {               
                oled_show_str(64, 5, ch_date, 1);
            } else {
                oled_show_str(64, 5, "0", 1);
                oled_show_str(72, 5, ch_date, 1);
            }
             oled_show_str(80, 5, " ", 1);
        }

        /*----------------------month--------------*/
        ds1307_i2c_write(0x05, NULL, NULL, SEND_READ_DATA_LEN); 
        ds1307_read(RCT_MONTH, SEND_READ_DATA_LEN, rct_read_data);
        if (rtc_data.rtc_month[0] != rct_read_data[0]) {
            rtc_data.rtc_month[0] = rct_read_data[0];
            temp_month = rct_read_data[0]/16*10 + rct_read_data[0]%16;
            int_to_char(temp_month, ch_month);
            if (temp_month >= 10) {
                oled_show_str(40, 5, ch_month, 1);
                oled_show_str(56, 5, "-", 1); 
            } else {
                oled_show_str(40, 5, "0", 1);
                oled_show_str(48, 5, ch_month, 1);
                oled_show_str(56, 5, "-", 1);
            }
            // oled_show_str(60, 5, " ", 1);
        }

        /*----------------------year--------------*/
        ds1307_i2c_write(0x06, NULL, NULL, SEND_READ_DATA_LEN);            
        ds1307_read(RCT_YEAR, SEND_READ_DATA_LEN, rct_read_data);
        if (rtc_data.rtc_year[0] != rct_read_data[0]) {
            rtc_data.rtc_year[0] = rct_read_data[0];
            temp_year = rct_read_data[0]/16*10 + rct_read_data[0]%16;
            int_to_char(temp_year, ch_year);
            oled_show_str(0, 5, "20", 1);
            oled_show_str(16, 5, ch_year, 1);
            oled_show_str(32, 5, "-", 1);
            oled_show_str(80, 5, " ", 1);
        }
        hi_sleep(1);
    }

}
/* ds1307 task*/
hi_void app_demo_rtc_task(hi_void)
{
    hi_u32 ret =0;
    hi_task_attr attr ={0};
    gpio_init();
    attr.stack_size = DS1307_TASK_STAK_SIZE;
    attr.task_prio = DS1307_TASK_PRIORITY;
    attr.task_name = (hi_char*)"rtc_task";
    ret = hi_task_create(&g_rtc_demo_task_id, &attr, rtc_timer, HI_NULL);
    if(ret != HI_ERR_SUCCESS){
        printf("Failed to create app_demo_rtc_task\r\n");
    }
}