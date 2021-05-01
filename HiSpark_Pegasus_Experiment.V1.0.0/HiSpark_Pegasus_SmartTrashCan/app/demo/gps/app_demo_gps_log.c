#include <hi_types_base.h>
#include <hi_early_debug.h>
#include <app_demo_gps_log.h>

const char *gps_log_level_names[] = 
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

const char *gps_level_num (gps_log_type gps_log_level)
{
    if (gps_log_level >= GPS_LOG_LEVEL_MAX) {
        return "NULL";
    } else {
        return gps_log_level_names[gps_log_level];
    }
}
