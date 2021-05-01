#include <hi_gpio.h>
#include <hi_early_debug.h>
#include <hi_io.h>
#include <hi_time.h>
#include <hi_watchdog.h>
#include <hi_task.h>
#include <hi_pwm.h>

#define PWM_LOW_DUTY                (1)
#define PWM_DUTY                    (20000)
#define PWM_FULL_DUTY               (65530)
#define BEEP_ON_OFF_DELAY_500MS     (500*1000)
#define hi_unuse_param(p)           ((p) = (p))
#define BEEp_CONTROL_TASK_SLEEP_20MS (20)
#define BEEP_CONTROL_TASK_SIZE       (1024)
#define BEEP_CONTROL_TASK_PRIO       (28)
hi_u32 g_beep_control_id;

hi_void beep_control(hi_void)
{
    hi_pwm_init(HI_PWM_PORT_PWM0);
    hi_pwm_set_clock(PWM_CLK_160M);
    hi_pwm_start(HI_PWM_PORT_PWM0, PWM_DUTY, PWM_FULL_DUTY);
    hi_udelay(BEEP_ON_OFF_DELAY_500MS);
    hi_pwm_start(HI_PWM_PORT_PWM0, PWM_LOW_DUTY, PWM_FULL_DUTY);
    hi_udelay(BEEP_ON_OFF_DELAY_500MS);
}

/*方法一*/
hi_void app_demo_beep_control(hi_void)
{
    hi_watchdog_disable();
    for(;;) {
        beep_control();
    }
}

/*方法二*/
hi_void *beep_control_demo(hi_void *param)
{
    hi_u32 ret;
    hi_unuse_param(param);
    for(;;) {
        beep_control();
        hi_sleep(BEEp_CONTROL_TASK_SLEEP_20MS);
    }
    ret = hi_task_delete(g_beep_control_id);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to delete beep control demo task\r\n");
        return HI_ERR_FAILURE;
    }
    return HI_ERR_SUCCESS;
}
hi_u32 app_demo_beep_control_task(hi_void)
{
    hi_u32 ret;
    hi_task_attr beep_control_attr = {0};
    beep_control_attr.stack_size = BEEP_CONTROL_TASK_SIZE;
    beep_control_attr.task_prio = BEEP_CONTROL_TASK_PRIO;
    beep_control_attr.task_name = (hi_char*)"beep control demo";
    ret = hi_task_create(&g_beep_control_id, &beep_control_attr, beep_control_demo, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to create beep control demo task\r\n");
        return HI_ERR_FAILURE;
    }
    return HI_ERR_FAILURE;
}