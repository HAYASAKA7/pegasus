#ifndef __GLOBAL_CONFIG__
#define __GLOBAL_CONFIG__

#include <hi_types_base.h>

int app_demo_iot(void);
int app_demo_gy521(void);
hi_void app_smart_trash_can_task(hi_void);
hi_void gps_demo(hi_void);
hi_u32 app_oled_i2c_demo_task(hi_void);
hi_void get_gps_position_information(hi_void);
hi_void app_c08i_nfc_i2c_demo_task(hi_void);
extern hi_u8 wifi_status;
extern hi_u8 flag_demped;
extern hi_bool  mqtt_connect_success;
#endif