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
#include "app_rainmaker.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "app_driver.h"

#define GPIO_WS2812 8
#define GPIO_LIGHT  4
#define GPIO_VOICE  5 
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_LIGHT) | (1ULL<<GPIO_VOICE))
#define BLINK_LED_RMT_CHANNEL 0
#define ESP_INTR_FLAG_DEFAULT 0

static xQueueHandle gpio_evt_queue = NULL;
static uint8_t s_led_state = 0;
static led_strip_t *pStrip_a;
static TimerHandle_t light_timer;

static const char *TAG = "app_driver";
static bool light_en = true;
static bool rainmaker_light_on = false;
static bool light_on = false;
static int s_voice = 0;
static int s_light = 0; 
static int r,g,b = 0;

void driver_set_light_color(int h)
{
	// R,G,B from 0-255, H from 0-360, S,V from 0-100
	int i;
    int v=100;
    int s=100;
	float RGB_min, RGB_max;
	RGB_max = v*2.55f;
	RGB_min = RGB_max*(100 - s) / 100.0f;

	i = h / 60;
    // factorial part of h
	int difs = h % 60; 

    // RGB adjustment amount by hue 
	float RGB_Adj = (RGB_max - RGB_min)*difs / 60.0f;

	switch (i) {
	case 0:
		r = RGB_max;
		g = RGB_min + RGB_Adj;
		b = RGB_min;
		break;
	case 1:
		r = RGB_max - RGB_Adj;
        g = RGB_max;
		b = RGB_min;
		break;
	case 2:
		r = RGB_min;
		g = RGB_max;
		b = RGB_min + RGB_Adj;
		break;
	case 3:
		r = RGB_min;
		g = RGB_max - RGB_Adj;
		b = RGB_max;
		break;
	case 4:
		r = RGB_min + RGB_Adj;
		g = RGB_min;
		b = RGB_max;
		break;
	default:
		r = RGB_max;
		g = RGB_min;
		b = RGB_max - RGB_Adj;
		break;
	}
    ESP_LOGI(TAG,"i=%d,r=%d,g=%d,b=%d",i,r,g,b);
}

void driver_set_light_status(bool flag)
{
    rainmaker_light_on = flag;
}

void driver_set_light_en(bool flag)
{
    light_en = flag;
}

int get_light_sensor_status()
{
    return gpio_get_level(GPIO_LIGHT);
}

int get_voice_sensor_status()
{
    return !gpio_get_level(GPIO_VOICE);
}

static void led_init(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    pStrip_a = led_strip_init(BLINK_LED_RMT_CHANNEL, GPIO_WS2812, 1);
    /* Set all LED off to clear all pixels */
    pStrip_a->clear(pStrip_a, 50);

    pStrip_a->set_pixel(pStrip_a, 0, 90, 16, 16);
    /* Refresh the strip to send data */
    pStrip_a->refresh(pStrip_a, 100);
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void vTimerCallback( TimerHandle_t xTimer )
{
    light_on = false;
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    uint32_t i = 0;

    while(true) {
        // control if open auto mode
        if(!light_en){
            light_on = false;
        }
        // control the light
        if(light_on || rainmaker_light_on ){
            pStrip_a->set_pixel(pStrip_a, 0, r, g, b);
            /* Refresh the strip to send data */
            pStrip_a->refresh(pStrip_a, 100);
        }else{
            pStrip_a->clear(pStrip_a, 50);
        }
        // read the sensors
        if(xQueueReceive(gpio_evt_queue, &io_num, 10)) {
            if (io_num == GPIO_LIGHT){
                s_light = gpio_get_level(io_num);
            }else if(io_num == GPIO_VOICE){
                s_voice = gpio_get_level(io_num);
            }
            if( light_on == false && s_light == 1 && s_voice == 0){
                light_on = true;
                xTimerStart(light_timer,0);
            }
        }
    }
}

void app_driver_init()
{
    led_init();

    gpio_config_t io_conf = {\
        .intr_type = GPIO_INTR_POSEDGE,\
        .pin_bit_mask = GPIO_INPUT_PIN_SEL,\
        .mode = GPIO_MODE_INPUT,\
        .pull_up_en = 1\
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_LIGHT, gpio_isr_handler, (void*) GPIO_LIGHT);
    gpio_isr_handler_add(GPIO_VOICE, gpio_isr_handler, (void*) GPIO_VOICE);
    light_timer = xTimerCreate("light_timer", 3000 / portTICK_PERIOD_MS, pdFALSE,(void*)0,vTimerCallback);

    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);
}