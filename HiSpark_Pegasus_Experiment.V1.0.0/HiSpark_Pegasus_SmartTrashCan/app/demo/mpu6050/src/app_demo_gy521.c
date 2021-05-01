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
#include <hi_io.h>
#include <hi_gpio.h>
#include <math.h>

#include <hi_time.h>
#include <hi_adc.h>
#include <hi_uart.h>
#include <hi_wifi_api.h>
#include <global_config.h>
#include <gyroscope_log.h>
#include <ssd1306_oled.h>

#define GY521_ADDR      0X68    //陀螺仪addr
#define	SMPLRT_DIV		0x19	//陀螺仪采样率，典型值：0x07(125Hz)
#define	CONFIG			0x1A	//低通滤波频率，典型值：0x06(5Hz)
#define	GYRO_CONFIG		0x1B	//陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s)
#define	ACCEL_CONFIG	0x1C	//加速计自检、测量范围及高通滤波频率，典型值：0x01(不自检，2G，5Hz)
#define	ACCEL_XOUT_H	0x3B
#define	ACCEL_XOUT_L	0x3C
#define	ACCEL_YOUT_H	0x3D
#define	ACCEL_YOUT_L	0x3E
#define	ACCEL_ZOUT_H	0x3F
#define	ACCEL_ZOUT_L	0x40
#define	TEMP_OUT_H		0x41
#define	TEMP_OUT_L		0x42
#define	GYRO_XOUT_H		0x43
#define	GYRO_XOUT_L		0x44	
#define	GYRO_YOUT_H		0x45
#define	GYRO_YOUT_L		0x46
#define	GYRO_ZOUT_H		0x47
#define	GYRO_ZOUT_L		0x48
#define	PWR_MGMT_1		0x6B	//电源管理，典型值：0x00(正常启用)
#define	WHO_AM_I			0x75	//IIC的ADDR寄存器(默认数值0x68，只读)
#define	SlaveAddress	0xD0	//IIC写入时的ADDR字节数据，+1为读取

#define Kp 100.0f                        // 比例增益支配率收敛到加速度计/磁强计
#define Ki 0.002f                // 积分增益支配率的陀螺仪偏见的衔接
#define halfT 0.001f                // 采样周期的一半

#define MPU_ADDR_WRITE 0
#define MPU_ADDR_READ  1
#define MPU_SEND_DATA_BUFF 20
float q0 = 1, q1 = 0, q2 = 0, q3 = 0;          // 四元数的元素，代表估计方向
float exInt = 0, eyInt = 0, ezInt = 0;        // 按比例缩小积分误差
 
float Yaw = 0.0, Pitch = 0.0,Roll = 0.0; 
static hi_u8 gy521_init_flag = 0;

hi_u8 Gy521_Write_Byte(hi_u8 addr, hi_u8 data)
{
    hi_i2c_idx id = 0;
    hi_i2c_data i2c_data = {0};
    hi_u8 buf[3] = {addr,data};
    i2c_data.send_len = 2;
    i2c_data.send_buf = buf;
    hi_u32 ret = 0;
    ret = hi_i2c_write(id, GY521_ADDR<<1|MPU_ADDR_WRITE, &i2c_data);
    return ret;
}
hi_u8 Gy521_Write_Len(hi_u8 addr, hi_u8 reg, hi_u8 len, hi_u8 *buf)
{
    hi_i2c_idx id = 0;
    hi_i2c_data i2c_data = {0};
    hi_u32 ret = 0;
    hi_u8 data[MPU_SEND_DATA_BUFF] = {0};

    /* send reg*/
    data[0] = reg;
    /*send reg and data*/
    memcpy(&data[0]+1, buf, len);
    /*i2c send reg and data  config*/
    i2c_data.send_len = len +1;
    i2c_data.send_buf = data;
   
    ret = hi_i2c_write(id, addr<<1|MPU_ADDR_WRITE, &i2c_data);
    if (ret == HI_ERR_I2C_INVALID_PARAMETER) {
        GYROSCOPE_LOG_ERROR("i2c invalid parameter 0x%x\r\n", ret);
        return HI_ERR_FAILURE;
    } 
    if (ret == HI_ERR_I2C_WAIT_SEM_FAIL) {
        GYROSCOPE_LOG_ERROR("i2c wait sem failed 0x%x\r\n", ret);
        HI_ERR_FAILURE;
    }
    return ret;
}

hi_u8 GY521_Read_Len(hi_u8 addr, hi_u8 reg, hi_u8 len, hi_u8 *data)
{
    hi_i2c_idx id = 0;
    hi_u32 ret = 0;
    hi_i2c_data i2c_data = {0};
    hi_u8 tem[1] = {reg};

    /*send reg config*/
    i2c_data.send_len = 1;
    i2c_data.send_buf = tem;
    /*read data config*/
    i2c_data.receive_len = len;
    i2c_data.receive_buf = data;
    /*send reg*/
    ret = hi_i2c_write(id, addr<<1|MPU_ADDR_WRITE, &i2c_data);
    if (ret == HI_ERR_I2C_INVALID_PARAMETER) {
        GYROSCOPE_LOG_ERROR("GY521_Read_Len : i2c invalid parameter 0x%x\r\n", ret);
        return HI_ERR_FAILURE;
    } 
    /*read data*/
    ret = hi_i2c_read(id, addr<<1|MPU_ADDR_READ, &i2c_data);
    if (ret == HI_ERR_I2C_WAIT_SEM_FAIL) {
        GYROSCOPE_LOG_ERROR("i2c wait sem failed 0x%x\r\n", ret);
        return HI_ERR_I2C_WAIT_SEM_FAIL;
    } else if(ret == HI_ERR_I2C_NOT_INIT) {
        GYROSCOPE_LOG_ERROR("i2c not init 0x%x\r\n", ret);
        return HI_ERR_I2C_NOT_INIT;
    }
	return HI_ERR_SUCCESS;
}
hi_void InitMPU6050(hi_void)
{
    Gy521_Write_Byte(PWR_MGMT_1, 0x00);	//解除休眠状态
	Gy521_Write_Byte(SMPLRT_DIV, 0x07);
	Gy521_Write_Byte(CONFIG, 0x06);
	Gy521_Write_Byte(GYRO_CONFIG, 0x18);
	Gy521_Write_Byte(ACCEL_CONFIG, 0x01);
}
hi_s16 GetData(unsigned char REG_Address)
{
	unsigned char buf[2] = {0};
    GY521_Read_Len(GY521_ADDR,REG_Address,2,buf);
	return (buf[0]<<8)+buf[1];   //合成数据
}
void IMUupdate(float gx, float gy, float gz, float ax, float ay, float az)
{
        float norm;
        float vx, vy, vz;
        float ex, ey, ez;   
        // 测量正常化
        norm = sqrt(ax*ax + ay*ay + az*az);      
        ax = ax / norm;                   //
        ay = ay / norm;
        az = az / norm;
        // 估计方向的重力
        vx = 2*(q1*q3 - q0*q2);
        vy = 2*(q0*q1 + q2*q3);
        vz = q0*q0 - q1*q1 - q2*q2 + q3*q3; 
        // 错误的领域和方向传感器测量参考方向之间的交叉乘积的总和
        ex = (ay*vz - az*vy);
        ey = (az*vx - ax*vz);
        ez = (ax*vy - ay*vx); 
        // 积分误差比例积分增益
        exInt = exInt + ex*Ki;
        eyInt = eyInt + ey*Ki;
        ezInt = ezInt + ez*Ki; 
        // 调整后的陀螺仪测量
        gx = gx + Kp*ex + exInt;
        gy = gy + Kp*ey + eyInt;
        gz = gz + Kp*ez + ezInt; 
        // 整合四元数率和正常化
        q0 = q0 + (-q1*gx - q2*gy - q3*gz)*halfT;
        q1 = q1 + (q0*gx + q2*gz - q3*gy)*halfT;
        q2 = q2 + (q0*gy - q1*gz + q3*gx)*halfT;
        q3 = q3 + (q0*gz + q1*gy - q2*gx)*halfT;   
        // 正常化四元
        norm = sqrt(q0*q0 + q1*q1 + q2*q2 + q3*q3);
        q0 = q0 / norm;
        q1 = q1 / norm;
        q2 = q2 / norm;
        q3 = q3 / norm; 
        Pitch  = asin(-2 * q1 * q3 + 2 * q0* q2)* 57.3; // pitch ,转换为度数
        Roll = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2* q2 + 1)* 57.3; // rollv
        //Yaw = atan2(2*(q1*q2 + q0*q3),q0*q0+q1*q1-q2*q2-q3*q3) * 57.3;   //此处没有价值，注掉
}
float AccX, AccY, AccZ;

float GyroX, GyroY, GyroZ;
extern hi_void iot_publish_tarsh_is_dumped(char * string);
hi_void *gy521_mpu6050(hi_void* param)
{
    hi_unref_param(param);
    InitMPU6050();	//初始化MPU6050
	hi_sleep(150);

	while(1)
	{
        // InitMPU6050();	//初始化MPU6050
        AccX = GetData(ACCEL_XOUT_H)/ 16384.0;	//显示X轴加速度
        AccY = GetData(ACCEL_YOUT_H)/ 16384.0;	//显示Y轴加速度
        AccZ = GetData(ACCEL_ZOUT_H)/ 16384.0;	//显示Z轴加速度
        GyroX = GetData(GYRO_XOUT_H) / 131.0; // For a 250deg/s range we have to divide first the raw value by 131.0, according to the datasheet
        GyroY = GetData(GYRO_YOUT_H) / 131.0;
        GyroZ = GetData(GYRO_ZOUT_H) / 131.0;
        IMUupdate(GyroX, GyroY, GyroZ,AccX, AccY, AccZ);
        GYROSCOPE_LOG_INFO("Pitch : %.02f Roll : %.02f \n",Pitch,Roll);
        if(((((Pitch > -10.00) && (Pitch <= 0)) && ((Roll > -70) && (Roll <= 0))) || (((Pitch > 0) && (Pitch <= 5)) && ((Roll >= -10) && (Roll <= 11))))) {//正面
            if (wifi_status == HI_WIFI_EVT_CONNECTED && gy521_init_flag == 1 && mqtt_connect_success == HI_TRUE) {
                // GYROSCOPE_LOG_INFO("<publish success>:");
                GYROSCOPE_LOG_INFO("publish SmartCan front demped");
                iot_publish_tarsh_is_dumped("true");
                oled_show_str(0,4, "               ",1);
                oled_show_str(0,4, "Can:front dempe",1);
            } else if (gy521_init_flag == HI_FALSE) {
                GYROSCOPE_LOG_INFO("Please  horizontal the box first, and adjust the base value of gyroscope\r\n");
            } else {
                GYROSCOPE_LOG_INFO("Ready to publish SmartCan positive demped, but huawei cloud is not connected");
            }
        } else if ((((Pitch > 15) && (Pitch <= 90)) && ((Roll > -180) && (Roll <= -90)))|| (((Pitch > 15) && (Pitch <= 90)) && ((Roll > -75) && (Roll <= 0)))) { //右侧面
            if (wifi_status == HI_WIFI_EVT_CONNECTED  && gy521_init_flag == 1 && mqtt_connect_success == HI_TRUE) {
                // GYROSCOPE_LOG_INFO("<publish success>:");
                GYROSCOPE_LOG_INFO("publish SmartCan Right side demped");
                iot_publish_tarsh_is_dumped("true");
                oled_show_str(0,4, "               ",1);
                oled_show_str(0,4, "Can:Right dempe",1);
            } else if (gy521_init_flag == HI_FALSE) {
                GYROSCOPE_LOG_INFO("Please  horizontal the box first, and adjust the base value of gyroscope\r\n");
            } else {
                GYROSCOPE_LOG_INFO("Ready to publish SmartCan Right side demped, but huawei cloud is not connected");
            }
        } else if (((Pitch > -10.00) && (Pitch <= 0)) && ((Roll > -179) && (Roll <= -100))) { //后面 
            if (wifi_status == HI_WIFI_EVT_CONNECTED && gy521_init_flag == 1 && mqtt_connect_success == HI_TRUE) {
                // GYROSCOPE_LOG_INFO("<publish success>:");
                GYROSCOPE_LOG_INFO("publish SmartCan behind side demped");
                iot_publish_tarsh_is_dumped("true");
                oled_show_str(0,4, "                ",1);
                oled_show_str(0,4, "Can:behind dempe",1);
            } else if (gy521_init_flag == HI_FALSE) {
                GYROSCOPE_LOG_INFO("Please  horizontal the box first, and adjust the base value of gyroscope\r\n");
            } else {
                GYROSCOPE_LOG_INFO("Ready to publish SmartCan behind side demped, but huawei cloud is not connected");
            }
        } else if (((Pitch > -90) && (Pitch <= -10))) { //左侧面
            if (wifi_status == HI_WIFI_EVT_CONNECTED && gy521_init_flag == 1 && mqtt_connect_success == HI_TRUE) {
                // GYROSCOPE_LOG_INFO("<publish success>:");
                GYROSCOPE_LOG_INFO("publish SmartCan left side demped");
                iot_publish_tarsh_is_dumped("true");
                oled_show_str(0,4, "                ",1);
                oled_show_str(0,4, "Can:Left dempe",1);
            } else if (gy521_init_flag == HI_FALSE) {
                GYROSCOPE_LOG_INFO("Please  horizontal the box first, and adjust the base value of gyroscope\r\n");
            } else {
                GYROSCOPE_LOG_INFO("Ready to publish SmartCan left side demped, but huawei cloud is not connected");
            }
        } else if (((Pitch > -10) && (Pitch <= -1.5)) && ((Roll > -97) && (Roll <= -80))) {//正常
            if (gy521_init_flag == HI_FALSE) {
                gy521_init_flag = HI_TRUE;
                GYROSCOPE_LOG_INFO("Base value adjusted already\r\n");
            }
             
            if (wifi_status == HI_WIFI_EVT_CONNECTED && mqtt_connect_success == HI_TRUE) {
                // GYROSCOPE_LOG_INFO("<publish success>:");
                GYROSCOPE_LOG_INFO("publish SmartTrashCan status normol");
                oled_show_str(0,4, "                ",1);
                oled_show_str(0,4, "Can:normal",1);
            } else {
                oled_show_str(0,4, "                ",1);
                oled_show_str(0,4, "Can:normal",1);
                GYROSCOPE_LOG_INFO("Ready to publish SmartTrashCan status normol, but huawei cloud is not connected");
            }
        }
		hi_sleep(1000);
	}
}
#define GY521_TASK_STACKSIZE  0x1000   
#define GY521_TASK_PRIOR 27
#define GY521_TASK_NAME "GY521DEMO"
int app_demo_gy521(void)
{
    hi_u32 ret;
    hi_task_attr attr = {0};
    hi_u32 taskID;
    attr.stack_size = GY521_TASK_STACKSIZE;
    attr.task_prio = GY521_TASK_PRIOR;
    attr.task_name = GY521_TASK_NAME;
    ret = hi_task_create(&taskID, &attr, gy521_mpu6050, NULL);
    if (ret != HI_ERR_SUCCESS) {
       GYROSCOPE_LOG_ERROR("GY521 TASK CTREATE FAILED");
    }

    return 0;
}