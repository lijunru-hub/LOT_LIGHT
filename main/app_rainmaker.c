/* Temperature Sensor Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>
#include <esp_rmaker_console.h>
#include <esp_rmaker_common_events.h>

#include <app_wifi.h>
#include <app_insights.h>

#include "app_rainmaker.h"
#include "app_driver.h"

static const char *TAG = "app_main";

esp_rmaker_device_t *gpio_device;
esp_rmaker_param_t *noise_param;
esp_rmaker_param_t *light_param;
esp_rmaker_param_t *auto_light;

#define COLOR_DEFAULT 100
#define DEFAULT_POWER false

static TimerHandle_t xTimer;

static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    const char *device_name = esp_rmaker_device_get_name(device);
    const char *param_name = esp_rmaker_param_get_name(param);
    if (strcmp(param_name, "声音传感器") == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                val.val.b? "true" : "false", device_name, param_name);
        // if(val.val.b){
        //     set_s_angle(s_value);
        // }
    } else if (strcmp(param_name, "颜色") == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                val.val.i, device_name, param_name);
        driver_set_light_color(val.val.i);
    } else if (strcmp(param_name, "LIGHT") == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                val.val.b? "true" : "false", device_name, param_name);
        driver_set_light_status(val.val.b);
    } else if (strcmp(param_name, "自动亮灯") == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                val.val.b? "true" : "false", device_name, param_name);
        driver_set_light_en(val.val.b);
    } else {
        /* Silently ignoring invalid params */
        return ESP_OK;
    }
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}



void app_sensor_update(TimerHandle_t handle)
{
    bool noise = false;
    bool light = false;
    noise = get_voice_sensor_status();
    light = get_light_sensor_status();
    esp_rmaker_param_update_and_report(noise_param,esp_rmaker_bool(noise));
    esp_rmaker_param_update_and_report(light_param,esp_rmaker_bool(light));
}

void app_rainmaker_init()
{
    /* Initialize Application specific hardware drivers and
     * set initial state.
     */
    // app_driver_init();

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init()
     */
    app_wifi_init();
    
    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Temperature Sensor");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    /* Create a device and add the relevant parameters to it */
    //temp_sensor_device = esp_rmaker_temp_sensor_device_create("光敏传感器", NULL, app_get_current_temperature());
    // esp_rmaker_device_t *gpio_device = esp_rmaker_lightbulb_device_create("智慧路灯", NULL, DEFAULT_POWER);
    esp_rmaker_device_t *gpio_device = esp_rmaker_device_create("智慧路灯", ESP_RMAKER_DEVICE_LIGHTBULB, NULL);
    esp_rmaker_device_add_cb(gpio_device, write_cb, NULL);

    noise_param = esp_rmaker_param_create("声音传感器", NULL, esp_rmaker_bool(false), PROP_FLAG_READ );
    esp_rmaker_param_add_ui_type(noise_param, ESP_RMAKER_UI_TEXT);
    esp_rmaker_device_add_param(gpio_device, noise_param);

    light_param = esp_rmaker_param_create("光线传感器", NULL, esp_rmaker_bool(false), PROP_FLAG_READ );
    esp_rmaker_param_add_ui_type(light_param, ESP_RMAKER_UI_TEXT);
    esp_rmaker_device_add_param(gpio_device, light_param);

    esp_rmaker_param_t *auto_light = esp_rmaker_power_param_create("自动亮灯",!DEFAULT_POWER);
    esp_rmaker_device_add_param(gpio_device, auto_light);

    esp_rmaker_param_t *primary = esp_rmaker_power_param_create("LIGHT", DEFAULT_POWER);
    esp_rmaker_device_add_param(gpio_device, primary);
    esp_rmaker_device_assign_primary_param(gpio_device, primary);
    
    // red_param = esp_rmaker_param_create("LIGHT", NULL, esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    // esp_rmaker_param_add_ui_type(red_param, ESP_RMAKER_UI_TOGGLE);
    // esp_rmaker_device_add_param(gpio_device, red_param);

    esp_rmaker_param_t *color_param = esp_rmaker_param_create("颜色", ESP_RMAKER_PARAM_HUE,esp_rmaker_int(COLOR_DEFAULT), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (color_param) {
        esp_rmaker_param_add_ui_type(color_param, ESP_RMAKER_UI_HUE_CIRCLE);
        esp_rmaker_param_add_bounds(color_param, esp_rmaker_int(0), esp_rmaker_int(360), esp_rmaker_int(1));
    }
    esp_rmaker_device_add_param(gpio_device, color_param);


    esp_rmaker_node_add_device(node, gpio_device);



    /* Enable OTA */
    esp_rmaker_ota_enable_default();

    /* Enable timezone service which will be require for setting appropriate timezone
     * from the phone apps for scheduling to work correctly.
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_timezone_service_enable();

    /* Enable scheduling. */
    esp_rmaker_schedule_enable();

    /* Enable Scenes */
    esp_rmaker_scenes_enable();

    /* Enable Insights. Requires CONFIG_ESP_INSIGHTS_ENABLED=y */
    app_insights_enable();

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
    xTimer =  xTimerCreate("app_sensor_update_tm",  1000 / portTICK_PERIOD_MS,pdTRUE, NULL, app_sensor_update);
    xTimerStart(xTimer,0);
    driver_set_light_color(COLOR_DEFAULT);
}
