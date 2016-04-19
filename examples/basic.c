#include "../tgc.h"

static tgc_t gc;

static void example_function() {
  void *memory = tgc_alloc(&gc, 1024);
}

int main(int argc, char **argv) {
  
  tgc_start(&gc, &argc);

  example_function();
  
  tgc_stop(&gc);
  
  return 0;
}
