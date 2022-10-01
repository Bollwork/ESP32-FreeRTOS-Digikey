/* 
    Solution to 06 - Mutex
    A Mutex is used to start a task from main_app and passed a local variable,
		ensuring that variable is not going out of scope before the task read it.
    Introduction to RTOS Part 6 - Mutex | Digi-Key Electronics
    https://youtu.be/I55auRpbiTs
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"

static SemaphoreHandle_t mutex = NULL;

#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif
#define BLINK_GPIO GPIO_NUM_5
#define BLINK_INTERVAL    500/portTICK_PERIOD_MS
#define MAIN_INTERVAL   10000/portTICK_PERIOD_MS
#define LED_STATE_ON    1
#define LED_STATE_OFF   (1-LED_STATE_ON)


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

// A simple task with start parameter
void Task(void* parameter)
{
  ESP_LOGI("Task","Started!");
	
	xSemaphoreTake(mutex, portMAX_DELAY);
	int val2 = * ((int*) parameter);	
	xSemaphoreGive(mutex);
		
	ESP_LOGI("Task","Received %d", val2);

	configure_led();

	// The unit of work of this task
	while (true)
	{
		// Blink the LED ...
		set_led(LED_STATE_ON);
		vTaskDelay(BLINK_INTERVAL);
		set_led(LED_STATE_OFF);
		vTaskDelay(BLINK_INTERVAL);
	}
}

void Init()
{
	// Create a local varial on stack
	int val1 = 42;

	// Create a the mutex
	mutex = xSemaphoreCreateMutex();

	if (mutex == NULL ) 
	{
		ESP_LOGI("INIT","Failed to create mutex!\n");
		return;
	}

	// Take the mutex
	if (xSemaphoreTake(mutex, 100/portTICK_PERIOD_MS)!= pdTRUE) 
	{
	 	ESP_LOGI("INIT","Failed to take mutex!\n");
	 	return;
	}

	// Create and start a new task that will receive local variable value as parameter
	// The task will block since it is trying to take the mutex at the very beginning.
  xTaskCreatePinnedToCore
  (
    Task,
    "Task1",
    2048,
    (void*) (&val1),
    1,
    NULL,
		app_cpu
  );

	// Release the mutex so the task can continue
  xSemaphoreGive(mutex);
	
	// Optional:
	// Give the task time to get started and take the mutex
  vTaskDelay(10/portTICK_PERIOD_MS);
	// Finally wait for the task to release the mutex again
	xSemaphoreTake(mutex, portMAX_DELAY);

	// Value is passed safely
}

void app_main(void)
{
	// Write some info to console
	printf("App Core is %d\n", app_cpu);
	// Call function Init() which creates a local variable and a task that will use it
	Init();
	// Here, the local variable in function Init() will be out of scope!
	printf("Init complete!\n");

	while (true)
	{
		vTaskDelay(MAIN_INTERVAL);
	}
}