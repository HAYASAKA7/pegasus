/*
 * Copyright (c) 2020 HiHope Community.
 * Description: IoT Platform 
 * Author: HiSpark Product Team.
 * Create: 2020-6-20
 */

#ifndef IOT_CONFIG_H_
#define IOT_CONFIG_H_

///<CONFIG THE LOG
/*if you need the iot log for the development , please enable it, else please comment it*/
#define CONFIG_LINKLOG_ENABLE   1

///< CONFIG THE WIFI
/* Please modify the ssid and pwd for the own */
#define CONFIG_AP_SSID  "TP-LINK_IOT_LINK"
#define CONFIG_AP_PWD   "iotlink_2019"

///< Configure the iot platform
/*Please modify the device id and pwd for your own */
#define CONFIG_DEVICE_ID  "5f9a646aadf72402d0d30c43_HiSpark_SmartTrashCan"
#define CONFIG_DEVICE_PWD "123456789"
///< if you use the tls mode and x509mode, please modify the DEVICE CA AND PRIVATE KEY IN iot_main.c
// #define CONFIG_MQTT_SSL                  ///< which means use the tls
// #define CONFIG_MQTT_SSL_X509             ///< which means use the x509 mode, and must enable the SSL; if both disabled, it means use the tcp mode

#endif