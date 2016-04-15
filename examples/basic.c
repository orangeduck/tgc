#include "../tgc.h"

static void example_function(gc_t *gc) {
  void *memory = gcalloc(gc, 1024);
}

int main(int argc, char **argv) {
  
  gc_t gc;
  gcstart(&gc);

  example_function(&gc);
  
  gcstop(&gc);
  
  return 0;
}