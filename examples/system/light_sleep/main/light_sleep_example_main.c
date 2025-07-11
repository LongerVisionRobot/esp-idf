/*
 * SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "light_sleep_example.h"

static void light_sleep_task(void *args)
{
    while (true) {
        printf("Entering light sleep\n");
        /* To make sure the complete line is printed before entering sleep mode,
         * need to wait until UART TX FIFO is empty:
         */
        uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);

        /* Get timestamp before entering sleep */
        int64_t t_before_us = esp_timer_get_time();

        /* Enter sleep mode */
        esp_light_sleep_start();

        /* Get timestamp after waking up from sleep */
        int64_t t_after_us = esp_timer_get_time();

        /* Determine wake up reason */
        const char* wakeup_reason;
        uint32_t wakup_causes = esp_sleep_get_wakeup_causes();
        if (wakup_causes & BIT(ESP_SLEEP_WAKEUP_TIMER)) {
            wakeup_reason = "timer";
        } else if (wakup_causes & BIT(ESP_SLEEP_WAKEUP_GPIO)) {
            wakeup_reason = "pin";
        } else if (wakup_causes & (BIT(ESP_SLEEP_WAKEUP_UART) | BIT(ESP_SLEEP_WAKEUP_UART1) | BIT(ESP_SLEEP_WAKEUP_UART2))) {
            wakeup_reason = "uart";
            /* Hang-up for a while to switch and execute the uart task
             * Otherwise the chip may fall sleep again before running uart task */
            vTaskDelay(1);
        } else {
            wakeup_reason = "other";
        }
#if CONFIG_NEWLIB_NANO_FORMAT
        /* printf in newlib-nano does not support %ll format, causing example test fail */
        printf("Returned from light sleep, reason: %s, t=%d ms, slept for %d ms\n",
                wakeup_reason, (int) (t_after_us / 1000), (int) ((t_after_us - t_before_us) / 1000));
#else
        printf("Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms\n",
                wakeup_reason, t_after_us / 1000, (t_after_us - t_before_us) / 1000);
#endif
        if (wakup_causes & BIT(ESP_SLEEP_WAKEUP_GPIO)) {
            /* Waiting for the gpio inactive, or the chip will continuously trigger wakeup*/
            example_wait_gpio_inactive();
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    /* Enable wakeup from light sleep by gpio */
    example_register_gpio_wakeup();
    /* Enable wakeup from light sleep by timer */
    example_register_timer_wakeup();
    /* Enable wakeup from light sleep by uart */
    example_register_uart_wakeup();

    xTaskCreate(light_sleep_task, "light_sleep_task", 4096, NULL, 6, NULL);
}
