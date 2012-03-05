
#include <utility/trace.h>

int fib(int);
int result[20];

void main (void) {
  
  TRACE_CONFIGURE(TRACE_DBGU, 115200, BOARD_MCK);
  TRACE_INFO("hello world\n\n\n");

  while (1) {
    for (int i = 0; i < 20; i++) {
      int r = fib(i);
      result[i] = r;
      TRACE_INFO("v7 fib %d = %d\n",i,r);
    }
  }
}


int fib (int n) {
  if (n == 0) return 0;
  if (n == 1) return 1;
  return fib(n-1) + fib(n-2); // Just to abuse the stack a bit.
}

