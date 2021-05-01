#include <hi_types_base.h>
#include <hi_early_debug.h>
#include <gyroscope_log.h>

const char *gyroscope_level[] = 
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

const char *gyroscope_level_num (gyroscope_log_type gyro_level)
{
    if (gyro_level >= GYRO_LEVEL_MAX) {
        return "NULL";
    } else {
        return gyroscope_level[gyro_level];
    }
}
