Tiny Garbage Collector
======================

About
-----

`tgc` is a tiny garbage collector written in ~500 lines of C and based on the 
[Cello Garbage Collector](http://libcello.org/learn/garbage-collection).

```c
#include "tgc.h"

static void example_function(gc_t *gc) {
  /* Memory allocated */
  void *memory = gcalloc(gc, 1024);
}

int main(int argc, char **argv) {
  gc_t gc;
  gcstart(&gc);

  example_function(&gc);
  
  /* Memory automatically freed */
  gcstop(&gc);  
}
```

Usage
-----

`tgc` is a thread local, mark and sweep, conservative garbage collector for C 
that automatically frees memory allocated by the `gcalloc` function after it 
becomes _unreachable_.

Memory is _reachable_ if and only if...

* A pointer on the stack, somewhere after the call to `gcstart`, points to it.
* A pointer on the heap, inside memory allocated by `gcalloc`, points to it.

Memory is _not reachable_ if...

* A pointer in the `static` data segment points to it.
* A pointer on the heap, inside memory allocated by `malloc`, `calloc`, 
`realloc` or any other allocation method points to it.
* A pointer in a different thread points to it.
* A pointer in any other unreachable location points to it.

`tgc` runs the mark and sweep collection on a call to `gcalloc` when the number 
of objects exceeds some threshold, or can be manually run using `gcrun`.

If you wish to manually free memory allocated by `gcalloc` then the `gcfree` 
function should be used.




Reference
---------

```c
void gcstart(gc_t *gc);
```

Start the garbage collector on the current thread, beginning at the stack 
location of the local variable `gc`.

* * *

```c
void gcstop(gc_t *gc);
```

Stop the garbage collector and free any data structures used by it.

* * *

```c
void gcrun(gc_t *gc);
```

Run an iteration of mark and sweep.

* * * 

```c
void *gcalloc(gc_t *gc, size_t size);
```

Allocate memory via the garbage collector to be automatically freed once it
becomes unreachable.

* * *

```c
void *gccalloc(gc_t *gc, size_t num, size_t size);
```

Allocate memory via the garbage collector and initalise to zero.

* * *

```c
void *gcrootalloc(gc_t *gc, size_t size);
```

Allocate a garbage collection root. This memory will not be freed 
automatically, and must be manually freed with `gcfree` once it is done with.

* * *

```c
void *gcrootcalloc(gc_t *gc, size_t num, size_t size);
```

Allocate a garbage collection root and initalise to zero.

* * *

```c
void *gcrealloc(gc_t *gc, void *ptr, size_t size);
```

Resize a memory allocated made by the garbage collector.

* * *

```c
void gcfree(gc_t *gc, void *ptr);
```

Manually free an allocation made by the garbage collector.


F.A.Q
-----

### It is possible to avoid passing around the `gc` variable every time?

If your application is single-threaded you can store this `gc_t*` pointer 
(not the actual `gc_t` data structure which must always be allocated on the 
stack) as a static variable and use that instead.

If your application is multithreaded you can store this pointer in thread local
storage for whatever threading model you are using.

### Why do I get _uninitialised values_ warnings with Valgrind?

The garbage collector scans the stack memory and this naturally contains 
uninitialised values. It does this safely, but if you are running through 
Valgrind these accesses will be reported as errors. Other than this `tgc` 
shouldn't have any memory errors in Valgrind, so the easiest way to disable 
these to examine any real problems is to run Valgrind with the option 
`--undef-value-errors=no`.

### Is this real/safe/portable?

More or less. There there is no real way to create a completely portable 
garbage collector in C, so for this reason I have to give the normal warnings 
that you use this library at your own risk. Saying that - unlike the Boehm 
Garbage Collector, this collector doesn't use any platform specific tricks and 
only makes the most basic assumptions, like the fact that the architecture uses 
a continuous call stack to implement function frames. This means it _should_ be 
safe to use for more or less all architectures found in the wild.
