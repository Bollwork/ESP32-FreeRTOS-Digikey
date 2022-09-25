/* 
    Solution to 02 - Blink LED
    Toggles LED at different rates using separate tasks.

    Introduction to RTOS Part 2 - Getting Started with FreeRTOS | Digi-Key Electronics
    https://youtu.be/JIr7Xm_riRs

*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"


/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO GPIO_NUM_5 
#define BLINK_PERIOD_TASK1 500
#define BLINK_PERIOD_TASK2 333
#define LED_OFF 0
#define LED_ON  1

static void set_led(uint8_t state)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, state);
}

static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void Task1(void* parameter)
{
    static uint8_t led_state = LED_OFF;

    while(true)
    {
        set_led(led_state);
        /* Toggle the LED state */
        led_state = led_state == LED_ON ? LED_OFF : LED_ON;
        vTaskDelay(BLINK_PERIOD_TASK1 / portTICK_PERIOD_MS);        
    }
}

void Task2(void* parameter)
{
    static uint8_t led_state = LED_OFF;

    while(true)
    {
        set_led(led_state);
        /* Toggle the LED state */
        led_state = led_state == LED_ON ? LED_OFF : LED_ON;
        vTaskDelay(BLINK_PERIOD_TASK2 / portTICK_PERIOD_MS);        
    }
}

void app_main(void)
{
    /* Configure the peripheral according to the LED type */
    configure_led();

    /* Create and start two blink tasks in without using any synchronization */
    xTaskCreatePinnedToCore ( Task1, "Task1", 1024, NULL, 1, NULL, 1 );

    xTaskCreatePinnedToCore ( Task2, "Task2", 1024, NULL, 1, NULL, 1 );

    vTaskDelete (NULL);
}
