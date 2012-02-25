
#include <stdint.h>

#include <utility/trace.h>

uint32_t fib(uint32_t);

void main (void) {
  
  TRACE_CONFIGURE(TRACE_DBGU, 115200, BOARD_MCK);
  TRACE_INFO("hello world\n\n\n");

  while (1) {
    for (uint32_t i = 0; i < 48; i++) {
      TRACE_INFO("fib %ld = %ld\n",i, fib(i));
    }
  }
}


uint32_t fib (uint32_t n) {
  if (n == 0) return 0;
  if (n == 1) return 1;
  return fib(n-1) + fib(n-2); // Just to abuse the stack a bit.
}

