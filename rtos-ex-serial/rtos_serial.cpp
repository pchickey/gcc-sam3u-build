
#include <stdint.h>

extern "C" {
#include <utility/trace.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
}

#define PRINTING_TIMEOUT ( (portTickType) 50 )

xSemaphoreHandle printing_semphr;

void * fib_task_handle;
void fib_task_func(void *);

void * rad_task_handle;
void rad_task_func(void *);

extern "C" {
  int main (void) {
    
    TRACE_CONFIGURE(TRACE_DBGU, 115200, BOARD_MCK);
    TRACE_INFO("entered main\n\n\n");

    vSemaphoreCreateBinary(printing_semphr);

    xTaskCreate(fib_task_func, (signed portCHAR *)"fibt", 400,
                NULL, 1, &fib_task_handle);

    xTaskCreate(rad_task_func, (signed portCHAR *)"radt", 400,
                NULL, 1, &rad_task_handle);

    vTaskStartScheduler();
  }
}

uint16_t fib (uint16_t n) {
  if (n == 0) return 0;
  if (n == 1) return 1;
  return fib(n-1) + fib(n-2); // Just to abuse the stack a bit.
}

uint16_t rad (uint16_t n) {
  if (n == 0) return 1;
  if (n == 1) return 1;
  return n * rad(n-1); // Just to abuse the stack a bit.
}

void fib_task_func (void * args) {

  if ( xSemaphoreTake( printing_semphr, PRINTING_TIMEOUT ) == pdTRUE ) {
    TRACE_INFO("fib task started\n\n");
    xSemaphoreGive( printing_semphr );
  } else {
    for(;;);
  }

  while (1) {
    for (uint16_t i = 0; i < 20; i++) {
      uint16_t res = fib(i);
      if ( xSemaphoreTake( printing_semphr, PRINTING_TIMEOUT ) == pdTRUE ) {
        TRACE_INFO("fib %d = %d\n",i,res);
        xSemaphoreGive( printing_semphr );
      } else {
        TRACE_ERROR("fib task failed to take printing semphr\n");
        for(;;);
      }
      taskYIELD();
    }
  }
}

void rad_task_func (void * args) {

  if ( xSemaphoreTake( printing_semphr, PRINTING_TIMEOUT ) == pdTRUE ) {
    TRACE_INFO("rad task started\n\n");
    xSemaphoreGive( printing_semphr );
  } else {
    for(;;);
  }

  while (1) {
    for (uint16_t i = 0; i < 10; i++) {
      uint16_t res = rad(i);
      if ( xSemaphoreTake( printing_semphr, PRINTING_TIMEOUT ) == pdTRUE ) {
        TRACE_INFO("rad %d = %d\n",i,res);
        xSemaphoreGive( printing_semphr );
      } else {
        TRACE_ERROR("rad task failed to take printing semphr\n");
        for(;;);
      }
      taskYIELD();
    }
  }
}


extern "C" {
  /* FreeRTOS Callbacks */
  void vApplicationTickHook(void) {
    static int i = 0;
    if (i >= 1000) {
      i = 0; 
      TRACE_INFO("tick\n"); /* This is unsafe! */
    } else i++;
  }

  void vApplicationStackOverflowHook(void) {
    TRACE_ERROR("Stack Overflow!\n")
    for(;;);
  }
} // extern "C"
