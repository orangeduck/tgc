#include "../tgc.h"

typedef struct object {
  char *b;  
  int a;
  struct object *first;
  struct object *second;
  struct object **others;
} object;

static tgc_t gc;

static void waste(void) {
  object *x = tgc_alloc(&gc, sizeof(object));
}

static object *object_new(void) {
  object *x = tgc_alloc(&gc, sizeof(object));
  waste();
  x->a = 1;
  x->b = tgc_calloc(&gc, 1, 100);
  x->b[0] = 'a';
  x->first = tgc_alloc(&gc, sizeof(object));
  waste();
  x->second = tgc_alloc(&gc, sizeof(object));
  waste();
  x->others = NULL;
  waste();
  return x;
}

static void object_resize(object *x, int num) {
  x->others = tgc_realloc(&gc, x->others, sizeof(object*) * num);
  waste();
}

static void example_function(int depth) {
  object *x = object_new();
  object *y = object_new();

  object_resize(x, 100);
  object_resize(y, 50);

  object_resize(x, 75);
  object_resize(y, 75);

  if (depth < 10) {example_function(depth+1); }

  x->others[10] = object_new();
  y->others[10] = object_new();

  x->others[25] = object_new();
  object_resize(x->others[25], 30);

}

int main(int argc, char **argv) {
  
  tgc_start(&gc, &argc);

  example_function(0);
  
  tgc_stop(&gc);
  
  return 0;
}
