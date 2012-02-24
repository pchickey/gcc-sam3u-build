
#include <stdint.h>

#include <utility/trace.h>

uint16_t fib(uint16_t);

void main (void) {
  
  TRACE_CONFIGURE(TRACE_USART_0, 115200, BOARD_MCK);
  TRACE_INFO("hello world\n\n\n");

  while (1) {
    for (uint16_t i = 0; i < 20; i++) {
      TRACE_INFO("fib %d = %d\n",i, fib(i));
    }
  }
}


uint16_t fib (uint16_t n) {
  if (n == 0) return 0;
  if (n == 1) return 1;
  return fib(n-1) + fib(n-2);
}

