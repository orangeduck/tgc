#ifndef TGC_H
#define TGC_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>

enum {
  GC_ROOT   = 0x01,
  GC_MARKED = 0x02
};

typedef struct {
  void *ptr;
  int flags;
  size_t size;
  size_t hash;
} gc_item_t;

typedef struct {
  void *bottom;
  gc_item_t *items;
  size_t nitems, nslots, mitems;
  uintptr_t minptr, maxptr;
  size_t freenum;
  void **freelist;
  double loadfactor;
  double sweepfactor;
} gc_t;

void gcstart(gc_t *gc);
void gcstop(gc_t *gc);
void gcrun(gc_t *gc);

void *gcalloc(gc_t *gc, size_t size);
void *gccalloc(gc_t *gc, size_t num, size_t size);
void *gcrootalloc(gc_t *gc, size_t size);
void *gcrootcalloc(gc_t *gc, size_t num, size_t size);

void *gcrealloc(gc_t *gc, void *ptr, size_t size);
void gcfree(gc_t *gc, void *ptr);


#endif