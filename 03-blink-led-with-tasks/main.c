/* 
    Solution to 03 - Blink LED
    One task toggles LED at a rate specified by value set in another task.

    Introduction to RTOS Part 3 - Task Scheduling | Digi-Key Electronics
    https://youtu.be/95yUbClyf3E

*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define buffer_size 7
#define base10 10
#define BLINK_GPIO GPIO_NUM_5
#define LED_STATE_ON    1
#define LED_STATE_OFF   (1-LED_STATE_ON)

#define MIN_DELAY 200
#define MAX_DELAY 10000

volatile int led_state_interval = 1000;

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

void TaskBlink(void *parameter)
{
	static uint8_t led_state = LED_STATE_OFF;

	while (true)
	{
		set_led(led_state);
		led_state = (led_state == LED_STATE_ON) ? LED_STATE_OFF : LED_STATE_ON;
		vTaskDelay(led_state_interval / portTICK_PERIOD_MS);
	}
}

void TaskConsole(void *parameter)
{
	char buffer[buffer_size];

	while (true)
	{
		// reset index and buffer
		int buffer_ix = 0;
		memset(buffer, 0, buffer_size);

		// write information to console
		printf("\n\n");
		printf("Refresh rate changed to %d\n", led_state_interval);
		printf("Enter value for delay (%d...%d): ", MIN_DELAY, MAX_DELAY);

		// capture console input in loop
		while (true)
		{
			char keycode = getchar();		
			// take valid inputs only
			if (keycode != 0xFF)
			{
				// do not exceed buffer limit
				if (buffer_ix < buffer_size - 1)			
				{ 
					buffer[buffer_ix++] = keycode; 
				}
				// parse the input value when "return" was received
				if (keycode == '\n')
				{
					// store value in temporary variable and clip agaist min / max
					long new_led_state_interval = strtol(buffer, NULL, base10);
					if (new_led_state_interval < MIN_DELAY) new_led_state_interval = MIN_DELAY;
					else if (new_led_state_interval > MAX_DELAY)	new_led_state_interval = MAX_DELAY;
					led_state_interval = new_led_state_interval;
					// leave capture loop to display information
					break;
				}
				else
				{
					putchar(keycode);
				}
			}
			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
	}
}

void app_main(void)
{

	configure_led();

	xTaskCreatePinnedToCore
	(
		TaskBlink,
		"TaskBlink",
		1024,
		NULL,
		1,
		NULL,
		1
	);

	xTaskCreatePinnedToCore
	(
		TaskConsole,
		"TaskConsole",
		2048,
		NULL,
		1,
		NULL,
		1
	);

	vTaskDelete(NULL);
}
