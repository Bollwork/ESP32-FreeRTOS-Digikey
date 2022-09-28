/* 
    Solution to 05 - Queues
    One task toggles LED at a rate specified by value set in another task reading console input. 
		Both task use queues for communication.
    Introduction to RTOS Part 5 - Queue | Digi-Key Electronics
    https://youtu.be/pHJ3lxOoWeI
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define TASK_MAIN_TAG "Main"
#define TASK_A_TAG "TaskA"
#define TASK_B_TAG "TaskB"

#define BLINK_GPIO GPIO_NUM_5
#define LED_STATE_ON    1
#define LED_STATE_OFF   (1-LED_STATE_ON)

#define buffer_size 81
#define MIN_DELAY 1
#define MAX_DELAY 10000
#define COUNTER_INTERVAL 100

static const uint8_t  CONSOLE_TASK_QUEUE_SIZE = 3;
static const uint8_t  WORK_TASK_QUEUE_SIZE = 3;

static QueueHandle_t queue1;
static QueueHandle_t queue2;

char msg[buffer_size];


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

// TaskB takes care on blinking LED
// It will receive interval time from other task and sends a notification every 100 blinks  
void TaskB(void* parameter)
{
	uint32_t interval = 1000;
	uint32_t new_interval = interval;
	uint32_t counter = 0;

	ESP_LOGI(TASK_B_TAG,"Task running");

	while (true)
	{
		// Check, if there is a message in the queue (wait time 0 means "no waiting")
		if (xQueueReceive(queue1, (void*) &new_interval, 0) == pdTRUE)
		{
			// clip value of new interval time
			interval = new_interval;
			if (interval < MIN_DELAY) interval = MIN_DELAY;
			else if (interval > MAX_DELAY) interval = MAX_DELAY;
		}

		// Blink the LED ...
		set_led(LED_STATE_ON);
		vTaskDelay(interval);
		set_led(LED_STATE_OFF);
		vTaskDelay(interval);

		// increment blink counter
		counter++; 
		// every 100 counts we will notify the console by passing a message to queue2
		if (counter % COUNTER_INTERVAL == 0)
		{
			memset(msg, 0, buffer_size);
			sprintf(msg, "Blink counter : %u", counter );
			if (xQueueSend(queue2, (void*)msg, 10) != pdTRUE)
			{
				ESP_LOGE (TASK_B_TAG, "TaskB 'LED' failed to write to queue\n");
			}
		}
	}
}

// TaskA takes care on console 
// 
void TaskA(void* parameter)
{
	char buffer[buffer_size];
	char buffer2[buffer_size];
	char cmd_tag[buffer_size];
	uint32_t cmd_arg = 0;
	int buffer_ix = 0;
	bool MessageReceived = false;
	memset(buffer, 0, buffer_size);

	ESP_LOGI(TASK_A_TAG,"Task running");

	while (true)
	{
		if (xQueueReceive(queue2, (void*) buffer2, 0) == pdTRUE)
		{
			printf("%s\n", buffer2);
		}

		char c = getchar();
		// read input from console (non blocking)
		if (c != 0xFF)
		{
			if (c == '\n' || c == '\r')
			{
				MessageReceived = true;
			}
			else
			{
				if (buffer_ix < buffer_size - 1)
				{
					buffer[buffer_ix] = c;
					buffer_ix++;
				}
			}
			putchar(c);

			// Check for NewLine or end of buffer
			if (MessageReceived)
			{
				buffer[buffer_ix] = '\0';
				memset(cmd_tag, 0, buffer_size);

			 	int scan_res = sscanf(buffer, "%s %u", cmd_tag, &cmd_arg);
		   	if (scan_res == 2 && strcmp(cmd_tag, "delay") == 0)
			 	{
					ESP_LOGI(TASK_A_TAG, "Task 'Console' command detected : %s, %u", cmd_tag, cmd_arg); 
				  if (xQueueSend(queue1, (void*)(&cmd_arg), 0) != pdTRUE)
				  { ESP_LOGE(TASK_A_TAG, "Task 'Console' failed to write to Queue\n");}
				}
				else
				{
					ESP_LOGE(TASK_A_TAG, "%s is no valid command", buffer); 
				}
				// Cleanup
				buffer_ix = 0;
				MessageReceived = false;
			}
		}
		vTaskDelay(1 / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	ESP_LOGI(TASK_MAIN_TAG, "App running");
	// Configure the LED
	configure_led();
	set_led(LED_STATE_OFF);

	// Create the 2 queues
	queue1 = xQueueCreate(CONSOLE_TASK_QUEUE_SIZE, sizeof(uint32_t));
	if (queue1 == NULL) 
	{
		ESP_LOGE(TASK_MAIN_TAG, "Queue1 created");
		return; 
	}
	queue2 = xQueueCreate(WORK_TASK_QUEUE_SIZE, sizeof(msg));
	if (queue2 == NULL) 
	{
		ESP_LOGE(TASK_MAIN_TAG, "Queue2 created");
		return; 
	}

	// Create Task A, that takes care on the console
  xTaskCreate
  (
    TaskA,
    "TaskA",
    2048,
    NULL,
    1,
    NULL
  );

	// Create Task B, that takes care on LED blinking
  xTaskCreate
  (
    TaskB,
    "TaskB",
    2048,
    NULL,
    1,
    NULL
  );
}