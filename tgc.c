#include "tgc.h"

static void gcsetptr(gc_t* gc, void *ptr, size_t size, int flags);

enum {
  GC_PRIMES_COUNT = 24
};

static const size_t gcprimes[GC_PRIMES_COUNT] = {
  0,       1,       5,       11,
  23,      53,      101,     197,
  389,     683,     1259,    2417,
  4733,    9371,    18617,   37097,
  74093,   148073,  296099,  592019,
  1100009, 2200013, 4400021, 8800019
};

static size_t gcprobe(gc_t* gc, size_t i, size_t h) {
  long v = i - (h-1);
  if (v < 0) {
    v = gc->nslots + v;
  }
  return v;
}

static size_t gcidealsize(gc_t* gc, size_t size) {
  size_t i, last;
  size = (size_t)((double)(size+1) / gc->loadfactor);
  for (i = 0; i < GC_PRIMES_COUNT; i++) {
    if (gcprimes[i] >= size) { return gcprimes[i]; }
  }
  last = gcprimes[GC_PRIMES_COUNT-1];
  for (i = 0;; i++) {
    if (last * i >= size) { return last * i; }
  }
  return 0;
}

static int gcrehash(gc_t* gc, size_t new_size) {

  size_t i;
  gc_item_t *old_items = gc->items;
  size_t old_size = gc->nslots;
  
  gc->nslots = new_size;
  gc->items = calloc(gc->nslots, sizeof(gc_item_t));
  
  if (gc->items == NULL) {
    gc->nslots = old_size;
    gc->items = old_items;
    return 0;
  }
  
  for (i = 0; i < old_size; i++) {
    if (old_items[i].hash != 0) {
      gcsetptr(gc, old_items[i].ptr, old_items[i].size, old_items[i].flags);
    }
  }
  
  free(old_items);
  
  return 1;
}

static int gcresizemore(gc_t *gc) {
  size_t new_size = gcidealsize(gc, gc->nitems);  
  size_t old_size = gc->nslots;
  return (new_size > old_size) ? gcrehash(gc, new_size) : 1;
}

static int gcresizeless(gc_t *gc) {
  size_t new_size = gcidealsize(gc, gc->nitems);  
  size_t old_size = gc->nslots;
  return (new_size < old_size) ? gcrehash(gc, new_size) : 1;
}

static size_t gchash(void *ptr) {
  return ((uintptr_t)ptr) >> 3;
}

static void gcsetptr(gc_t *gc, void *ptr, size_t size, int flags) {
  
  gc_item_t item;
  size_t h, p, i, j;

  i = gchash(ptr) % gc->nslots; j = 0;
  
  item.ptr = ptr;
  item.flags = flags;
  item.size = size;
  item.hash = i+1;
  
  while (1) {
    
    h = gc->items[i].hash;
    if (h == 0) { gc->items[i] = item; return; }
    if (gc->items[i].ptr == item.ptr) { return; }
    p = gcprobe(gc, i, h);
    if (j >= p) {
      gc_item_t tmp = gc->items[i];
      gc->items[i] = item;
      item = tmp;
      j = p;
    }
    
    i = (i+1) % gc->nslots;
    j++;
  }
  
}

static void gcremptr(gc_t *gc, void *ptr) {
  
  size_t i, j, h, nj, nh;
  
  if (gc->nslots == 0) { return; }
  
  for (i = 0; i < gc->freenum; i++) {
    if (gc->freelist[i] == ptr) { gc->freelist[i] = NULL; }
  }
  
  i = gchash(ptr) % gc->nslots; j = 0;
  
  while (1) {
    h = gc->items[i].hash;
    if (h == 0 || j > gcprobe(gc, i, h)) { return; }
    if (gc->items[i].ptr == ptr) {
      memset(&gc->items[i], 0, sizeof(gc_item_t));
      j = i;
      while (1) { 
        nj = (j+1) % gc->nslots;
        nh = gc->items[nj].hash;
        if (nh != 0 && gcprobe(gc, nj, nh) > 0) {
          memcpy(&gc->items[ j], &gc->items[nj], sizeof(gc_item_t));
          memset(&gc->items[nj],              0, sizeof(gc_item_t));
          j = nj;
        } else {
          break;
        }  
      }
      
      gc->nitems--;
      return;
    }
    
    i = (i+1) % gc->nslots; j++;
  }
  
}

static void gcresizeitem(gc_t *gc, void *ptr, size_t size) {
  size_t i, j, h;
  i = gchash(ptr) % gc->nslots; j = 0;
  while (1) {
    h = gc->items[i].hash;
    if (h == 0 || j > gcprobe(gc, i, h)) { return; }
    if (gc->items[i].ptr == ptr) { gc->items[i].size = size; return; }
    i = (i+1) % gc->nslots; j++;
  }
}

static int gcflagsitem(gc_t *gc, void *ptr) {
  size_t i, j, h;
  i = gchash(ptr) % gc->nslots; j = 0;
  while (1) {
    h = gc->items[i].hash;
    if (h == 0 || j > gcprobe(gc, i, h)) { return 0; }
    if (gc->items[i].ptr == ptr) { return gc->items[i].flags; }
    i = (i+1) % gc->nslots; j++;
  }
  return 0;
}

static void gcmarkitem(gc_t *gc, void *ptr) {
  
  size_t i, j, h, k;
  uintptr_t pval = (uintptr_t)ptr;
  if (pval < gc->minptr || pval > gc->maxptr) { return; }
  
  i = gchash(ptr) % gc->nslots; j = 0;
  
  while (1) {
    h = gc->items[i].hash;
    if (h == 0 || j > gcprobe(gc, i, h)) { return; }
    if (pval >= ((uintptr_t)gc->items[i].ptr)
    &&  pval <  ((uintptr_t)gc->items[i].ptr) + gc->items[i].size
    && !(gc->items[i].flags & GC_MARKED)) {
      gc->items[i].flags |= GC_MARKED;
      for (k = 0;
        k + sizeof(void*) <= gc->items[k].size;
        k += sizeof(void*)) {
        gcmarkitem(gc, *((void**)(((char*)ptr) + k)));
      }
      return;
    }
    i = (i+1) % gc->nslots; j++;
  }
  
}

static void gcmarkstack(gc_t *gc) {
  
  void *stk, *bot, *top, *p;
  bot = gc->bottom; top = &stk;
  
  if (bot == top) { return; }
  
  if (bot < top) {
    for (p = top; p >= bot; p = ((char*)p) - sizeof(void*)) {
      gcmarkitem(gc, *((void**)p));
    }
  }
  
  if (bot > top) {
    for (p = top; p <= bot; p = ((char*)p) + sizeof(void*)) {
      gcmarkitem(gc, *((void**)p));
    }
  }
  
}

static void gcmarkstackfake(gc_t *gc) { }

static void gcmark(gc_t *gc) {
  
  size_t i, k;
  volatile int noinline;
  
  if (gc->nitems == 0) { return; }
  
  for (i = 0; i < gc->nslots; i++) {
    if (gc->items[i].hash == 0) { continue; }
    if (gc->items[i].flags & GC_MARKED) { continue; }
    if (gc->items[i].flags & GC_ROOT) {
      gc->items[i].flags |= GC_MARKED;
      for (k = 0;
        k + sizeof(void*) <= gc->items[k].size;
        k += sizeof(void*)) {
        gcmarkitem(gc, *((void**)(((char*)gc->items[i].ptr) + k)));
      }
      return;
    }
  }
  
  noinline = 1;
  
  if (noinline) {
    jmp_buf env;
    memset(&env, 0, sizeof(jmp_buf));
    setjmp(env);
  }  

  (noinline ? gcmarkstack : gcmarkstackfake)(gc);
  
}

void gcsweep(gc_t *gc) {
  
  size_t i, j, nj, nh;
  
  gc->freelist = realloc(gc->freelist, sizeof(void*) * gc->nitems);
  gc->freenum = 0;
  
  i = 0;
  while (i < gc->nslots) {
    
    if (gc->items[i].hash == 0) { i++; continue; }
    if (gc->items[i].flags & GC_MARKED) { i++; continue; }
    
    if (!(gc->items[i].flags & GC_ROOT)
    &&  !(gc->items[i].flags & GC_MARKED)) {
      
      gc->freelist[gc->freenum] = gc->items[i].ptr;
      gc->freenum++;
      memset(&gc->items[i], 0, sizeof(gc_item_t));
      
      j = i;
      while (1) { 
        nj = (j+1) % gc->nslots;
        nh = gc->items[nj].hash;
        if (nh != 0 && gcprobe(gc, nj, nh) > 0) {
          memcpy(&gc->items[ j], &gc->items[nj], sizeof(gc_item_t));
          memset(&gc->items[nj],              0, sizeof(gc_item_t));
          j = nj;
        } else {
          break;
        }  
      }
      
      gc->nitems--;
      continue;
    }
    
    i++;
  }
  
  for (i = 0; i < gc->nslots; i++) {
    if (gc->items[i].hash == 0) { continue; }
    if (gc->items[i].flags & GC_MARKED) {
      gc->items[i].flags &= (~GC_MARKED);
      continue;
    }
  }
  
  gcresizeless(gc);
  gc->mitems = gc->nitems + (size_t)(gc->nitems * gc->sweepfactor) + 1;
  
  for (i = 0; i < gc->freenum; i++) {
    if (gc->freelist[i]) {
      free(gc->freelist[i]);
    }
  }
  
  free(gc->freelist);
  gc->freelist = NULL;
  gc->freenum = 0;
  
}

void gcstart(gc_t *gc) {
  gc->bottom = gc;
  gc->items = NULL;
  gc->nitems = 0;
  gc->nslots = 0;
  gc->mitems = 0;
  gc->maxptr = 0;
  gc->minptr = UINTPTR_MAX;
  gc->freelist = NULL;
  gc->freenum = 0;
  gc->loadfactor = 0.9;
  gc->sweepfactor = 0.5;
}

void gcstop(gc_t *gc) {
  free(gc->items);
  free(gc->freelist);
}

void gcrun(gc_t *gc) {
  gcmark(gc);
  gcsweep(gc);
}

static void *gcset(gc_t *gc, void *ptr, size_t size, int flags) {
  gc->nitems++;
  gc->maxptr = (uintptr_t)ptr > gc->maxptr ? (uintptr_t)ptr : gc->maxptr + size; 
  gc->minptr = (uintptr_t)ptr < gc->minptr ? (uintptr_t)ptr : gc->minptr;
  if (gcresizemore(gc)) {
    gcsetptr(gc, ptr, size, flags);
    if (gc->nitems > gc->mitems) { gcrun(gc); }
    return ptr;
  } else {
    gc->nitems--;
    free(ptr);
    return NULL;
  }
}

void *gcalloc(gc_t *gc, size_t size) {
  void *ptr = malloc(size);
  if (ptr != NULL) {
    ptr = gcset(gc, ptr, size, 0);
  }
  return ptr;
}

void *gcrootalloc(gc_t *gc, size_t size) {
  void *ptr = malloc(size);
  if (ptr != NULL) {
    ptr = gcset(gc, ptr, size, GC_ROOT);
  }
  return ptr;
}

void *gccalloc(gc_t *gc, size_t num, size_t size) {
  void *ptr = calloc(num, size);
  if (ptr != NULL) {
    ptr = gcset(gc, ptr, num * size, 0);
  }
  return ptr;
}

void *gcrootcalloc(gc_t *gc, size_t num, size_t size) {
  void *ptr = calloc(num, size);
  if (ptr != NULL) {
    ptr = gcset(gc, ptr, num * size, GC_ROOT);
  }
  return ptr;
}

void *gcrealloc(gc_t *gc, void *ptr, size_t size) {
  
  void *newptr = realloc(ptr, size);
  
  if (newptr == NULL) {
    gcremptr(gc, ptr);
  } else if (newptr == ptr) {
    gcresizeitem(gc, ptr, size);
  } else {
    int flags = gcflagsitem(gc, ptr);
    gcremptr(gc, ptr);
    gcsetptr(gc, newptr, size, flags);
  }
  
  return newptr;
}

void gcfree(gc_t *gc, void *ptr) {
  if (ptr == NULL) { return; }
  gcremptr(gc, ptr);
  free(ptr);
  gcresizeless(gc);
  gc->mitems = gc->nitems + gc->nitems / 2 + 1;
}
