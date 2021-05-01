#ifndef _GYROSCOPE_LOG_H_
#define _GYROSCOPE_LOG_H_
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>


typedef enum {
    GYRO_LEVEL_TRACE = 0,
    GYRO_LEVEL_DEBUG,
    GYRO_LEVEL_INFO,
    GYRO_LEVEL_WARN,
    GYRO_LEVEL_ERROR,
    GYRO_LEVEL_FATAL,
    GYRO_LEVEL_MAX
} gyroscope_log_type;

const char *gyroscope_level_num (gyroscope_log_type gyro_level);

#define GYROSCOPE_PRINT(fmt, ...) \
    do \
    { \
        printf(fmt, ##__VA_ARGS__); \
    } while(0)

 #define GYROSCOPE_LOG(level, fmt, ...) \
    do \
    { \
        GYROSCOPE_PRINT("<%s>, <%s>, <%d> "fmt" \r\n", \
        gyroscope_level_num((level)), __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define GYROSCOPE_LOG_TRACE(fmt, ...) \
    do \
    { \
        GYROSCOPE_LOG (GYRO_LEVEL_TRACE, fmt, ##__VA_ARGS__); \
    } while(0)
   
#define GYROSCOPE_LOG_DEBUG(fmt, ...)   \
    do \
    { \
        GYROSCOPE_LOG (GYRO_LEVEL_DEBUG, fmt, ##__VA_ARGS__); \
    } while(0)
    
#define GYROSCOPE_LOG_INFO(fmt, ...) \
    do \
    { \
       GYROSCOPE_LOG (GYRO_LEVEL_INFO, fmt, ##__VA_ARGS__); \
    } while(0) 

#define GYROSCOPE_LOG_WARN(fmt, ...) \
    do \
    { \
        GYROSCOPE_LOG (GYRO_LEVEL_WARN, fmt, ##__VA_ARGS__); \
    } while(0)
    
#define GYROSCOPE_LOG_ERROR(fmt, ...) \
    do \
    { \
        GYROSCOPE_LOG (GYRO_LEVEL_ERROR, fmt, ##__VA_ARGS__); \
    } while(0)
    
#define GYROSCOPE_LOG_FATAL(fmt, ...) \
    do \
    { \
        GYROSCOPE_LOG (GYRO_LEVEL_FATAL, fmt, ##__VA_ARGS__); \
    } while(0)
       
#endif