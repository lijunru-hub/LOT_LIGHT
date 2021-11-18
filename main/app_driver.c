/*  Temperature Sensor demo implementation using RGB LED and timer

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <sdkconfig.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h> 
#include <esp_rmaker_standard_params.h> 
#include <string.h>

#include <app_reset.h>
#include <ws2812_led.h>
#include "app_priv.h"

/* This is the button that is used for toggling the power */
#define BUTTON_GPIO          CONFIG_EXAMPLE_BOARD_BUTTON_GPIO
#define BUTTON_ACTIVE_LEVEL  0
/* This is the GPIO on which the power will be set */
#define OUTPUT_GPIO    19

static TimerHandle_t sensor_timer;

#define OUTPUT_GPIO_RED   16ULL
#define INPUT_GPIO_LIGHT 14ULL
#define INTPUT_GPIO_NOISE  15ULL


#define DEFAULT_SATURATION  100
#define DEFAULT_BRIGHTNESS  50

#define WIFI_RESET_BUTTON_TIMEOUT       3
#define FACTORY_RESET_BUTTON_TIMEOUT    10

static uint16_t g_hue;
static uint16_t g_saturation = DEFAULT_SATURATION;
static uint16_t g_value = DEFAULT_BRIGHTNESS;
static float g_temperature;

esp_err_t app_driver_set_gpio(const char *name, bool state)
{
    if (strcmp(name, "LIGHT") == 0) {
        gpio_set_level(OUTPUT_GPIO_RED, state);
    } else {
        return ESP_FAIL;
    }
    return ESP_OK;
}
int count=0;
int light_flag=false;


static void app_sensor_update(TimerHandle_t handle)
{
    if(gpio_get_level(INPUT_GPIO_LIGHT)){
        esp_rmaker_param_update_and_report(light_param,esp_rmaker_bool(true));
        if(!gpio_get_level(INTPUT_GPIO_NOISE)){
            light_flag=true;
            esp_rmaker_param_update_and_report(noise_param,esp_rmaker_bool(true));
        }else{
            esp_rmaker_param_update_and_report(noise_param,esp_rmaker_bool(false));
        }
    }else if(!gpio_get_level(INTPUT_GPIO_NOISE)){
        esp_rmaker_param_update_and_report(light_param,esp_rmaker_bool(false));
        esp_rmaker_param_update_and_report(noise_param,esp_rmaker_bool(true));
    }else{
        esp_rmaker_param_update_and_report(light_param,esp_rmaker_bool(false));
        esp_rmaker_param_update_and_report(noise_param,esp_rmaker_bool(false));
    }
    if(light_flag==true){
        count++;
        gpio_set_level(OUTPUT_GPIO_RED, 1);
        esp_rmaker_param_update_and_report(red_param,esp_rmaker_bool(true));
    }else{
         gpio_set_level(OUTPUT_GPIO_RED, 0);
        esp_rmaker_param_update_and_report(red_param,esp_rmaker_bool(false));
    }
    if(count>3){
        gpio_set_level(OUTPUT_GPIO_RED, 0);
        esp_rmaker_param_update_and_report(red_param,esp_rmaker_bool(false));
        light_flag=false;
        count=0;
    }
    // static float delta = 0.5;
    // g_temperature += delta;
    // if (g_temperature > 99) {
    //     delta = -0.5;
    // } else if (g_temperature < 1) {
    //     delta = 0.5;
    // }
    // g_hue = (100 - g_temperature) * 2;
    // ws2812_led_set_hsv(g_hue, g_saturation, g_value);
    // esp_rmaker_param_update_and_report(
    //         esp_rmaker_device_get_param_by_type(temp_sensor_device, ESP_RMAKER_PARAM_TEMPERATURE),
    //         esp_rmaker_float(g_temperature));
}

float app_get_current_temperature()
{
    return g_temperature;
}

esp_err_t app_sensor_init(void)
{
    esp_err_t err = ws2812_led_init();
    if (err != ESP_OK) {
        return err;
    }

    g_temperature = DEFAULT_TEMPERATURE;
    sensor_timer = xTimerCreate("app_sensor_update_tm", (REPORTING_PERIOD * 1000) / portTICK_PERIOD_MS,
                            pdTRUE, NULL, app_sensor_update);
    if (sensor_timer) {
        xTimerStart(sensor_timer, 0);
        g_hue = (100 - g_temperature) * 2;
        ws2812_led_set_hsv(g_hue, g_saturation, g_value);
        return ESP_OK;
    }
    return ESP_FAIL;
}

void app_driver_init()
{
    app_reset_button_register(app_reset_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL),
                WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);

    /* Configure power */
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
    };
    uint64_t pin_mask = ((uint64_t)1 << OUTPUT_GPIO_RED ) ;
    io_conf.pin_bit_mask = pin_mask;
    /* Configure the GPIO */
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_INPUT;
       
    pin_mask = (((uint64_t)1 << INPUT_GPIO_LIGHT ) | ((uint64_t)1 << INTPUT_GPIO_NOISE ));
    io_conf.pin_bit_mask = pin_mask;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);


    gpio_set_level(OUTPUT_GPIO_RED, false);

    app_sensor_init();
    app_reset_button_register(app_reset_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL),
                WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
}
