#ifndef __SMART_CAN_DRIVER_LOG_H__
#define __SMART_CAN_DRIVER_LOG_H__

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
    SMART_CAN_LEVEL_TRACE = 0,
    SMART_CAN_LEVEL_DEBUG,
    SMART_CAN_LEVEL_INFO,
    SMART_CAN_LEVEL_WARN,
    SMART_CAN_LEVEL_ERROR,
    SMART_CAN_LEVEL_FATAL,
    SMART_CAN_LEVEL_MAX
} smart_can_log_type;

const char *smart_can_level_num (smart_can_log_type smart_can_level);

#define SMART_CAN_PRINT(fmt, ...) \
    do \
    { \
        printf(fmt, ##__VA_ARGS__); \
    } while(0)

 #define SMART_CAN_LOG(level, fmt, ...) \
    do \
    { \
        SMART_CAN_PRINT("<%s>, <%s>, <%d> "fmt" \r\n", \
        smart_can_level_num((level)), __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define SMART_CAN_LOG_TRACE(fmt, ...) \
    do \
    { \
        SMART_CAN_LOG (SMART_CAN_LEVEL_TRACE, fmt, ##__VA_ARGS__); \
    } while(0)
   
#define SMART_CAN_LOG_DEBUG(fmt, ...)   \
    do \
    { \
        SMART_CANE_LOG (SMART_CAN_LEVEL_DEBUG, fmt, ##__VA_ARGS__); \
    } while(0)
    
#define SMART_CAN_LOG_INFO(fmt, ...) \
    do \
    { \
       SMART_CAN_LOG (SMART_CAN_LEVEL_INFO, fmt, ##__VA_ARGS__); \
    } while(0) 

#define SMART_CAN_LOG_WARN(fmt, ...) \
    do \
    { \
        SMART_CAN_LOG (SMART_CAN_LEVEL_WARN, fmt, ##__VA_ARGS__); \
    } while(0)
    
#define SMART_CAN_LOG_ERROR(fmt, ...) \
    do \
    { \
        SMART_CAN_LOG (SMART_CAN_LEVEL_ERROR, fmt, ##__VA_ARGS__); \
    } while(0)
    
#define SMART_CAN_LOG_FATAL(fmt, ...) \
    do \
    { \
        SMART_CAN_LOG (SMART_CAN_LEVEL_FATAL, fmt, ##__VA_ARGS__); \
    } while(0)

#endif