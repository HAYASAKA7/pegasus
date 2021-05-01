#ifndef __APP_DEMO_GPS_LOG_H__
#define __APP_DEMO_GPS_LOG_H__

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
    GPS_LOG_LEVEL_TRACE = 0,
    GPS_LOG_LEVEL_DEBUG,
    GPS_LOG_LEVEL_INFO,
    GPS_LOG_LEVEL_WARN,
    GPS_LOG_LEVEL_ERROR,
    GPS_LOG_LEVEL_FATAL,
    GPS_LOG_LEVEL_MAX
} gps_log_type;

const char *gps_level_num (gps_log_type GPS_LOG_level);

#define GPS_LOG_PRINT(fmt, ...) \
    do \
    { \
        printf(fmt, ##__VA_ARGS__); \
    } while(0)

 #define GPS_LOG(level, fmt, ...) \
    do \
    { \
        GPS_LOG_PRINT("<%s>, <%s>, <%d> "fmt" \r\n", \
        gps_level_num((level)), __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define GPS_LOG_TRACE(fmt, ...) \
    do \
    { \
        GPS_LOG (GPS_LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__); \
    } while(0)
   
#define GPS_LOG_DEBUG(fmt, ...)   \
    do \
    { \
        GPS_LOG (GPS_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__); \
    } while(0)
    
#define GPS_LOG_INFO(fmt, ...) \
    do \
    { \
       GPS_LOG (GPS_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__); \
    } while(0) 

#define GPS_LOG_WARN(fmt, ...) \
    do \
    { \
        GPS_LOG (GPS_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__); \
    } while(0)
    
#define GPS_LOG_ERROR(fmt, ...) \
    do \
    { \
        GPS_LOG (GPS_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__); \
    } while(0)
    
#define GPS_LOG_FATAL(fmt, ...) \
    do \
    { \
        GPS_LOG (GPS_LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__); \
    } while(0)

#endif