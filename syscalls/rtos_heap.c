
#include <stdlib.h>

void *pvPortMalloc(size_t s) {
  return malloc(s); 
}


void vPortFree(void *p) {
  return free(p);
}

