/*
 * AWS IoT Kit - Core2 for AWS IoT Kit
 * Factory Firmware v2.2.0
 * main.c
 * 
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_freertos_hooks.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_phy_init.h"

#include "core2forAWS.h"

#include "bluetooth.h"
#include "vario.h"
#include "home.h"
#include "wifi.h"
#include "mpu.h"
#include "mic.h"
#include "clock.h"
#include "power.h"
#include "touch.h"
#include "led_bar.h"
#include "crypto.h"
#include "cta.h"
#include "screen.h"
#include "config.h"

void app_main(void)
{
    ESP_LOGI("APP_MAIN", "Bluethroat Vario Meter, Designed by Snailtrail.ORG, All rights Reserved.");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    esp_log_level_set("gpio", ESP_LOG_NONE);
    esp_log_level_set("ILI9341", ESP_LOG_NONE);

    Core2ForAWS_Init();
    
    config_load_all_namespace();

    screen_init();

    vario_start();

    esp_phy_erase_cal_data_in_nvs();

    if (config_get_integer(CONFIG_NAMESPACE_BLUETOOTH, CONFIG_BLUETOOTH_ENABLE)) {
        ui_set_bluetooth(BLUETOOTH_STATE_ADVERTISING);
        bluetooth_init();
    } else {
        ui_set_bluetooth(BLUETOOTH_STATE_OFF);
    }
}