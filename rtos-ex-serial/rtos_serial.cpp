
#include <stdint.h>

extern "C" {
#include <utility/trace.h>
}

uint32_t fib(uint32_t);

int main (void) {
  
  TRACE_CONFIGURE(TRACE_DBGU, 115200, BOARD_MCK);
  TRACE_INFO("hello world\n\n\n");

  while (1) {
    for (uint32_t i = 0; i < 48; i++) {
      TRACE_INFO("fib %ld = %ld\n",i, fib(i));
    }
  }
  return 0;
}


uint32_t fib (uint32_t n) {
  if (n == 0) return 0;
  if (n == 1) return 1;
  return fib(n-1) + fib(n-2); // Just to abuse the stack a bit.
}


/* FreeRTOS Callbacks */

extern "C" void vApplicationTickHook(void) {
  static int i = 0;
  if (i < 1000) i++;
  else { TRACE_INFO("tick\n"); i = 0; }
}

extern "C" void vApplicationStackOverflowHook(void) {
  TRACE_ERROR("Stack Overflow!\n")
  for(;;);
}
