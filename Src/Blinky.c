/*----------------------------------------------------------------------------
 * Name:    Blinky.c
 * Purpose: LED Flasher for STM32F429I-Discovery
 * Author: A. BAUD (AUSY)
 * Example: This exemple illustrates task priority and task synchronization
 *----------------------------------------------------------------------------
 */

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Application includes. */
#include "partest.h"
#include "flash.h"
#include "flop.h"
#include "integer.h"
#include "PollQ.h"
#include "semtest.h"
#include "dynamic.h"
#include "BlockQ.h"
#include "blocktim.h"
#include "countsem.h"
#include "GenQTest.h"
#include "recmutex.h"
#include "death.h"

/* Hardware and starter kit includes. */
#include "stm32f4xx_hal.h"
#include "Board_LED.h"
#include "Board_Buttons.h"
#include "RTE_Components.h"

/* Definition of tasks priority. */
#define tsk1_PRIORITY			( ( UBaseType_t ) 1U )
#define tsk2_PRIORITY			( ( UBaseType_t ) 3U )
#define tsk3_PRIORITY			( ( UBaseType_t ) 2U )

/* Declaration of task functions. */
void vTask1( void *pvParameters );
void vTask2( void *pvParameters );
void vTask3( void *pvParameters );

/* The task handlers. */
TaskHandle_t task1Handle = NULL;
TaskHandle_t task2Handle = NULL;
TaskHandle_t task3Handle = NULL;

/* MUTEX declaration */
SemaphoreHandle_t mutex;

const TickType_t xDelaytst = 500 / portTICK_PERIOD_MS; /* Block for 500ms. */

/* Task1 : Green LED OFF and Red LED OFF */
void vTask1( void *pvParameters )
{
		for( ;; ) {
			
			vTaskResume (task2Handle);
			vTaskResume (task3Handle);
			
		  // Wait 1s
		  vTaskDelay(2*xDelaytst);
			
			vTaskSuspend (task2Handle);
			vTaskSuspend (task3Handle);
			
			LED_Off((int32_t)0);	 				/* Turn LED 1 off */
			LED_Off((int32_t)1);	 				/* Turn LED 2 off */
			
		  // Wait 1s
		  vTaskDelay(2*xDelaytst);
		}
}

/* Task2 : Green LED ON and Red LED OFF */
void vTask2( void *pvParameters )
{
		for( ;; ) {
		
			 if( mutex != NULL ) { 	
				 
					 // See if we can obtain the semaphore. (NO TIMEOUT)
					 if( xSemaphoreTake( mutex, portMAX_DELAY ) == pdTRUE ) { 
						 
							 // We were able to obtain the semaphore and can now access the shared resource.
							 LED_On((int32_t)0);	 			/* Turn LED 1 on */
							 LED_Off((int32_t)1);	 			/* Turn LED 2 off */
						 
							 // Wait 500ms
							 vTaskDelay(xDelaytst);
						 
							 // We have finished accessing the shared resource. Release the semaphore. 
							 xSemaphoreGive( mutex ); 
						 
							 // Wait 500ms
							 vTaskDelay(xDelaytst);
					 }
				}
		}
}

/* Task3 : Red LED ON and Green LED OFF */
void vTask3( void *pvParameters )
{
		for( ;; ) {
		
			 if( mutex != NULL ) { 	
				 
					 // See if we can obtain the semaphore. (NO TIMEOUT)
					 if( xSemaphoreTake( mutex, portMAX_DELAY ) == pdTRUE ) { 
						 
							 // We were able to obtain the semaphore and can now access the shared resource.
							 LED_On((int32_t)1);	 			/* Turn LED 2 on */
							 LED_Off((int32_t)0);	 			/* Turn LED 1 off */
							 
							 // Wait 500ms
							 vTaskDelay(xDelaytst);
						 
							 // We have finished accessing the shared resource. Release the semaphore. 
							 xSemaphoreGive( mutex ); 
					 }
				}
		}
}

/*----------------------------------------------------------------------------
  Main function
 *----------------------------------------------------------------------------*/
int main (void) {

	/* System Initialization. */
	SystemInit();
	SystemCoreClockUpdate();
	HAL_Init();

  LED_Initialize();                         /* LED Initialization             */
  Buttons_Initialize();                     /* Buttons Initialization         */
	
	/* MUTEX creation. */
	mutex = xSemaphoreCreateMutex();
	
	/* Tasks creation. */
	xTaskCreate( vTask1, "Task1", configMINIMAL_STACK_SIZE, NULL, tsk1_PRIORITY, &task1Handle );
	xTaskCreate( vTask2, "Task2", configMINIMAL_STACK_SIZE, NULL, tsk2_PRIORITY, &task2Handle );
	xTaskCreate( vTask3, "Task3", configMINIMAL_STACK_SIZE, NULL, tsk3_PRIORITY, &task3Handle );

	if( mutex != NULL )
  {
    /* Start the scheduler. */
		vTaskStartScheduler();
  }
	
	/* If all is well we will never reach here as the scheduler will now be
	running.  If we do reach here then it is likely that there was insufficient
	heap available for the idle task to be created. */
  if( task1Handle != NULL ) { vTaskDelete( task1Handle );}
	if( task2Handle != NULL ) { vTaskDelete( task2Handle );}
	if( task3Handle != NULL ) { vTaskDelete( task3Handle );}
	for( ;; );
}

void vApplicationMallocFailedHook( void )
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If the application makes use of the
	vTaskDelete() API function (as this demo application does) then it is also
	important that vApplicationIdleHook() is permitted to return to its calling
	function, because it is the responsibility of the idle task to clean up
	memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	/* This example does not use the tick hook to perform any processing.   The
	tick hook will only be called if configUSE_TICK_HOOK is set to 1 in
	FreeRTOSConfig.h. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/
