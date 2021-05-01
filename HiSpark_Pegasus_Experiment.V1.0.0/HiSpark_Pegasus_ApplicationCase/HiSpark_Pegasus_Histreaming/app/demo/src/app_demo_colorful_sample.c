/*
 * Copyright (c) 2020 HiHope Community.
 * Description: colorful light demo
 * Author: HiSpark Product Team.
 * Create: 2020-5-20
 */
#include <app_demo_multi_sample.h>
#include <app_demo_colorful_sample.h>
#include <hi_i2c.h>
static hi_u8 hi3861_board_led_test =0;
/*显示colorful light 界面的具体功能*/
hi_void colorful_light_func(hi_void)
{
    pwm_init(HI_IO_NAME_GPIO_10,HI_IO_FUNC_GPIO_10_PWM1_OUT,HI_PWM_PORT_PWM1);
    pwm_init(HI_IO_NAME_GPIO_11,HI_IO_FUNC_GPIO_11_PWM2_OUT,HI_PWM_PORT_PWM2);
    pwm_init(HI_IO_NAME_GPIO_12,HI_IO_FUNC_GPIO_12_PWM3_OUT,HI_PWM_PORT_PWM3);
    hi_io_set_func(HI_IO_NAME_GPIO_7, HI_IO_FUNC_GPIO_7_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_7,HI_GPIO_DIR_IN);
    hi_gpio_value gpio_val = HI_GPIO_VALUE1;
    hi_gpio_get_input_val(HI_GPIO_IDX_7, &gpio_val);


    hi_io_set_func(HI_IO_NAME_GPIO_9, HI_IO_FUNC_GPIO_9_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_9,HI_GPIO_DIR_IN);
    /*初始化时屏幕 i2c baudrate setting*/
    hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400kbps */
    hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);
    /*ssd1306 config init*/
    //oled_init();
	while( HI_ERR_SUCCESS != oled_init()) {
         printf("Connecting oled board falied!Please access oled board\r\n");
          if (hi3861_board_led_test == 0) {
               hi3861_board_led_test =1;
             /*test HiSpark board*/
            FACTORY_HISPARK_BOARD_CL_TL_TEST("-----------HiSpark board test----------");  
         }
         hi_sleep(SLEEP_1S);
    }
    /*按键中断初始化*/
    extern hi_void test_gpio_init(hi_void);
    test_gpio_init();
    //clear screen 
    oled_fill_screen(OLED_CLEAN_SCREEN);
    oled_show_str(0,0, "WiFi-AP  ON  U:1",1);
    oled_show_str(15,1, "Colorful Light",1);
    oled_show_str(0,2, "                ",1);
    oled_show_str(0,3, "Mode:           ",1);
    oled_show_str(0,4, "1.Control Mode  ",1);
    oled_show_str(0,6, "Type:           ",1);

    g_current_type = KEY_UP; 
    
    while(1){
        switch (g_current_mode) {
            case CONTROL_MODE:
                 oled_show_str(0,3, "Mode:           ",1);
                 oled_show_str(0,4, "1.Control Mode",1);
                 oled_show_str(0,6, "Type:           ",1);
                 switch (g_current_type) {
                     case RED_ON:
                          oled_show_str(0,7, "1.Red On        ",1);                      
                          break;
                     case YELLOW_ON:
                          oled_show_str(0,7, "2.Green On      ",1);
                          break;
                     case GREEN_ON:
                          oled_show_str(0,7, "3.Blue On       ",1);
                          break;
                     default:
                          break;
                 }
                 control_mode_sample();
                 break;
            case COLORFUL_LIGHT_MODE:
                 oled_show_str(0,4, "2.Colorful Light",1);
                 switch (g_current_type) {
                     case CYCLE_FOR_ONE_SECOND:
                          oled_show_str(0,7, "1.Period 1s     ",1);
                          break;
                     case CYCLE_FOR_HALF_SECOND:
                          oled_show_str(0,7, "2.Period 0.5s   ",1);                    
                          break;
                     case CYCLE_FOR_QUARATER_SECOND:
                          oled_show_str(0,7, "3.Period 0.25s  ",1);
                          break;
                     default:
                          break;
                }
                colorful_light_sample();
                break;
            case PWM_CONTROL_MODE:
                 oled_show_str(0,4, "3.Pwm Control   ",1);
                 switch (g_current_type) {
                     case PWM_CONTROL_RED:
                          oled_show_str(0,7, "1.Red           ",1);
                         break;
                     case PWM_CONTROL_YELLOW:
                         oled_show_str(0,7, "2.Green         ",1);
                         break;
                     case PWM_CONTROL_GREEN:
                         oled_show_str(0,7, "3.Blue          ",1);
                         break;
                     case PWM_CONTROL_PURPLE:
                          oled_show_str(0,7, "4.Purple        ",1);
                          break;
                     case PWM_CONTROL_ALL:
                          oled_show_str(0,7, "5.All           ",1);
                          break;
                     default:
                          break;
                 }    
                 pwm_control_sample();
                 break;
            case BIRGHTNESS_MODE:
                oled_show_str(0,4, "4.Brightness    ",1);
                switch (g_current_type) {
                    case BRIGHTNESS_LOW:
                         oled_show_str(0,7, "1.Low           ",1);
                         break;
                    case BRIGHTNESS_MIDDLE:
                         oled_show_str(0,7, "2.Middle        ",1);
                         break;
                    case BRIGHTNESS_HIGH:
                         oled_show_str(0,7, "3.High          ",1);
                         break;
                    default:
                         break;
                 }    
                 brightness_control_sample();
                 break;
            case HUMAN_DETECT_MODE:
                 oled_show_str(0,4, "5.Human Detect  ",1);
                 oled_show_str(0,6, "                ",1);
                 oled_show_str(0,7, "                ",1);
                 all_light_out();
                 human_detect_sample();
                 break;
            case LIGHT_DETECT_MODE:
                 oled_show_str(0,4, "6.Light Detect  ",1);
                 oled_show_str(0,6, "                ",1);
                 oled_show_str(0,7, "                ",1);
                 all_light_out();
                 light_detect_sample();
                 break;
            case UNION_DETECT_MODE:
                 oled_show_str(0,4, "7.Union detect  ",1);
                 oled_show_str(0,6, "                ",1);
                 oled_show_str(0,7, "                ",1);
                 all_light_out();
                 union_detect_sample();
                 break;   
            case RETURN_MODE:
                 oled_show_str(0,4, "Return Menu     ",1);
                 oled_show_str(0,6, "                ",1);
#ifdef    CONFIG_COLORFUL_LIGHT
                 oled_show_str(0,7, "Continue        ",1); 
#elif     CONFIG_ALL_DEMO                
                 oled_show_str(0,7, "Continue   Exit ",1); 
                 return_main_enum_sample();
                 if (g_current_type == KEY_DOWN) {
                     g_current_mode = MAIN_FUNCTION_SELECT_MODE;
                     oled_main_menu_display();                    
                     return;
                 }
                 g_current_type = KEY_UP;
#endif                               
                 break;
            default:
                break;
        }
    }
    return;
}