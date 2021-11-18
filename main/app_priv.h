/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#define DEFAULT_TEMPERATURE 25.0
#define REPORTING_PERIOD    1 /* Seconds */

extern esp_rmaker_device_t *gpio_device;
extern esp_rmaker_param_t *noise_param;
extern esp_rmaker_param_t *light_param;
extern esp_rmaker_param_t *red_param;

void app_driver_init(void);
float app_get_current_temperature();
esp_err_t app_driver_set_gpio(const char *name, bool state);
