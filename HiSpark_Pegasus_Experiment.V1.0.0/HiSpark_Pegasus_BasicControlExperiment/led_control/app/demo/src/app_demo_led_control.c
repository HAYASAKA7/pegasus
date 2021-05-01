#include <hi_gpio.h>
#include <hi_early_debug.h>
#include <hi_io.h>
#include <hi_time.h>
#include <hi_watchdog.h>
#include <hi_task.h>

#define LED_LIGHT_DELAY_500MS       (500*1000)
#define hi_unuse_param(p)           ((p) = (p))
#define LED_CONTROL_TASK_SLEEP_20MS (20)
#define LED_CONTROL_TASK_SIZE       (1024)
#define LED_CONTROL_TASK_PRIO       (28)
hi_u32 g_led_control_id;

hi_void led_control(hi_void)
{
    hi_gpio_set_ouput_val(HI_GPIO_IDX_10, HI_GPIO_VALUE1);
    hi_udelay(LED_LIGHT_DELAY_500MS);
    hi_gpio_set_ouput_val(HI_GPIO_IDX_10, HI_GPIO_VALUE0);
    hi_udelay(LED_LIGHT_DELAY_500MS);
}
/*方法一*/
hi_void app_demo_led_control(hi_void)
{
    hi_watchdog_disable();
    for(;;) {
        led_control();
    }
}
/*方法二*/
hi_void *led_control_demo(hi_void *param)
{
    hi_u32 ret;
    hi_unuse_param(param);
    for(;;) {
        led_control();
        /**/
        hi_sleep(LED_CONTROL_TASK_SLEEP_20MS);
    }
    ret = hi_task_delete(g_led_control_id);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to delete led control demo task\r\n");
    }
}
hi_u32 app_demo_led_control_task(hi_void)
{
    hi_u32 ret;
    hi_task_attr led_control_attr = {0};
    led_control_attr.stack_size = LED_CONTROL_TASK_SIZE;
    led_control_attr.task_prio = LED_CONTROL_TASK_PRIO;
    led_control_attr.task_name = (hi_char*)"app demo led control";
    ret = hi_task_create(&g_led_control_id, &led_control_attr, led_control_demo, HI_NULL);
    if (ret != HI_ERR_SUCCESS) {
        printf("Failed to create led control demo\r\n");
        return HI_ERR_FAILURE;
    }
    return HI_ERR_SUCCESS;
}

