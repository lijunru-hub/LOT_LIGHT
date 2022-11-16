#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "app_rainmaker.h"
#include "app_driver.h"

static const char *TAG = "main";

void app_main()
{
    app_driver_init();
    app_rainmaker_init();
}
