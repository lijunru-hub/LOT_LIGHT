#pragma once
#ifdef __cplusplus
extern "C"
{
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

void app_driver_init();
void driver_set_light_color(int color);
void driver_set_light_status(bool flag);
void driver_set_light_en(bool flag);
int get_light_sensor_status();
int get_voice_sensor_status();

#ifdef __cplusplus
}
#endif