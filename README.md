Tiny Garbage Collector
======================

About
-----

`tgc` is a tiny garbage collector for C written in ~500 lines of code and based 
on the [Cello Garbage Collector](http://libcello.org/learn/garbage-collection).

```c
#include "tgc.h"

static tgc_t gc;

static void example_function() {
  char *message = tgc_alloc(&gc, 64);
  strcpy(message, "No More Memory Leaks!");
}

int main(int argc, char **argv) {
  tgc_start(&gc, &argc);
  
  example_function();

  tgc_stop(&gc);
}
```

Usage
-----

`tgc` is a conservative, thread local, mark and sweep garbage collector,
which supports destructors, and automatically frees memory allocated by 
`tgc_alloc` and friends after it becomes _unreachable_.

A memory allocation is considered _reachable_ by `tgc` if...

* a pointer points to it, located on the stack at least one function call 
deeper than the call to `tgc_start`, or,
* a pointer points to it, inside memory allocated by `tgc_alloc` 
and friends.

Otherwise a memory allocation is considered _unreachable_.

Therefore some things that _don't_ qualify an allocation as _reachable_ are, 
if...

* a pointer points to an address inside of it, but not at the start of it, or,
* a pointer points to it from inside the `static` data segment, or, 
* a pointer points to it from memory allocated by `malloc`, 
`calloc`, `realloc` or any other non-`tgc` allocation methods, or, 
* a pointer points to it from a different thread, or, 
* a pointer points to it from any other unreachable location.

Given these conditions, `tgc` will free memory allocations some time after 
they become _unreachable_. To do this it performs an iteration of _mark and 
sweep_ when `tgc_alloc` is called and the number of memory allocations exceeds 
some threshold. It can also be run manually with `tgc_run`.

Memory allocated by `tgc_alloc` can be manually freed with `tgc_free`, and 
destructors (functions to be run just before memory is freed), can be 
registered with `tgc_set_dtor`.


Reference
---------

```c
void tgc_start(tgc_t *gc, void *stk);
```

Start the garbage collector on the current thread, beginning at the stack 
location given by the `stk` variable. Usually this can be found using the 
address of any local variable, and then the garbage collector will cover all 
memory at least one function call deeper.

* * *

```c
void tgc_stop(tgc_t *gc);
```

Stop the garbage collector and free its internal memory.

* * *

```c
void tgc_run(tgc_t *gc);
```

Run an iteration of the garbage collector, freeing any unreachable memory.

* * *

```c
void tgc_pause(tgc_t *gc);
void tgc_resume(tgc_t *gc);
```

Pause or resume the garbage collector. While paused the garbage collector will
not run during any allocations made.

* * * 

```c
void *tgc_alloc(gc_t *gc, size_t size);
```

Allocate memory via the garbage collector to be automatically freed once it
becomes unreachable.

* * *

```c
void *tgc_calloc(gc_t *gc, size_t num, size_t size);
```

Allocate memory via the garbage collector and initalise it to zero.

* * *

```c
void *tgc_realloc(gc_t *gc, void *ptr, size_t size);
```

Reallocate memory allocated by the garbage collector.

* * *

```c
void tgc_free(gc_t *gc, void *ptr);
```

Manually free an allocation made by the garbage collector. Runs any destructor
if registered.

* * *

```c
void *tgc_alloc_opt(tgc_t *gc, size_t size, int flags, void(*dtor)(void*));
```

Allocate memory via the garbage collector with the given flags and destructor.

For the `flags` argument, the flag `TGC_ROOT` may be specified to indicate that 
the allocation is a garbage collection _root_ and so should not be 
automatically freed and instead will be manually freed by the user with 
`tgc_free`. Because roots are not automatically freed, they can exist in 
normally unreachable locations such as in the `static` data segment or in 
memory allocated by `malloc`. 

The flag `TGC_LEAF` may be specified to indicate that the allocation is a 
garbage collection _leaf_ and so contains no pointers to other allocations
inside. This can benefit performance in many cases. For example, when 
allocating a large string there is no point the garbage collector scanning 
this allocation - it can take a long time and doesn't contain any pointers.

Otherwise the `flags` argument can be set to zero.

The `dtor` argument lets the user specify a _destructor_ function to be run 
just before the memory is freed. Destructors have many uses, for example they 
are often used to automatically release system resources (such as file handles)
when a data structure is finished with them. For no destructor the value `NULL` 
can be used.

* * *

```c
void *tgc_calloc_opt(tgc_t *gc, size_t num, size_t size, int flags, void(*dtor)(void*));
```

Allocate memory via the garbage collector with the given flags and destructor 
and initalise to zero.

* * *

```c
void tgc_set_dtor(tgc_t *gc, void *ptr, void(*dtor)(void*));
```

Register a destructor function to be called after the memory allocation `ptr`
becomes unreachable, and just before it is freed by the garbage collector.

* * *

```c
void tgc_set_flags(tgc_t *gc, void *ptr, int flags);
```

Set the flags associated with a memory allocation, for example the value 
`TGC_ROOT` can be used to specify that an allocation is a garbage collection 
root.

* * *

```c
int tgc_get_flags(tgc_t *gc, void *ptr);
```

Get the flags associated with a memory allocation.

* * *

```c
void(*tgc_get_dtor(tgc_t *gc, void *ptr))(void*);
```

Get the destructor associated with a memory allocation.

* * *

```c
size_t tgc_get_size(tgc_t *gc, void *ptr);
```

Get the size of a memory allocation.

F.A.Q
-----

### Is this real/safe/portable?

Definitely! While there is no way to create a _completely_ safe/portable 
garbage collector in C this collector doesn't use any platform specific tricks 
and only makes the most basic assumptions about the platform, such as that the 
architecture using a continuous call stack to implement function frames.

It _should_ be safe to use for more or less all reasonable architectures found 
in the wild and has been tested on Linux, Windows, and OSX, where it was easily 
integrated into several large real world programs (see `examples`) such as 
`bzip2` and `oggenc` without issue.

Saying all of that, there are the normal warnings - this library performs 
_undefined behaviour_ as specified by the C standard and so you use it at your 
own risk - there is no guarantee that something like a compiler or OS update 
wont mysteriously break it.


### What happens when some data just happens to look like a pointer?

In this unlikely case `tgc` will treat the data as a pointer and assume that 
the memory allocation it points to is still reachable. If this is causing your
application trouble by not allowing a large memory allocation to be freed 
consider freeing it manually with `tgc_free`.


### `tgc` isn't working when I increment pointers!

Due to the way `tgc` works, it always needs a pointer to the start of each 
memory allocation to be reachable. This can break algorithms such as the 
following, which work by incrementing a pointer.

```c
void bad_function(char *y) {
  char *x = tgc_alloc(&gc, strlen(y) + 1);
  strcpy(x, y);
  while (*x) {
    do_some_processsing(x);
    x++;
  }
}
```

Here, when `x` is incremented, it no longer points to the start of the memory 
allocation made by `tgc_alloc`. Then during `do_some_processing`, if a sweep 
is performed, `x` will be declared as unreachable and the memory freed.

If the pointer `x` is also stored elsewhere such as inside a heap structure 
there is no issue with incrementing a copy of it - so most of the time you 
don't need to worry, but occasionally you may need to adjust algorithms which
do significant pointer arithmetic. For example, in this case the pointer can be 
left as-is and an integer used to index it instead:

```c
void good_function(char *y) {
  int i;
  char *x = tgc_alloc(&gc, strlen(y) + 1);
  strcpy(x, y);
  for (i = 0; i < strlen(x); i++) {
    do_some_processsing(&x[i]);
  }
}
```

For now this is the behaviour of `tgc` until I think of a way to 
deal with offset pointers nicely.


### `tgc` isn't working when optimisations are enabled!

Variables are only considered reachable if they are one function call shallower 
than the call to `tgc_start`. If optimisations are enabled sometimes the 
compiler will inline functions which removes this one level of indirection.

The most portable way to get compilers not to inline functions is to call them 
through `volatile` function pointers.

```c
static tgc_t gc;

void please_dont_inline(void) {
  ...
}

int main(int argc, char **argv) {
  
  tgc_start(&gc, &argc);

  void (*volatile func)(void) = please_dont_inline;
  func();
  
  tgc_stop(&gc);

  return 1;
}
```

### `tgc` isn't working with `setjmp` and `longjmp`!

Unfortunately `tgc` doesn't work properly with `setjmp` and `longjmp` since 
these functions can cause complex stack behaviour. One simple option is to 
disable the garbage collector while using these functions and to re-enable
it afterwards.

### Why do I get _uninitialised values_ warnings with Valgrind?

The garbage collector scans the stack memory and this naturally contains 
uninitialised values. It scans memory safely, but if you are running through 
Valgrind these accesses will be reported as warnings/errors. Other than this 
`tgc` shouldn't have any memory errors in Valgrind, so the easiest way to 
disable these to examine any real problems is to run Valgrind with the option 
`--undef-value-errors=no`.

### Is `tgc` fast?

At the moment `tgc` has decent performance - it is competative with many 
existing memory management systems - but definitely can't claim to be the 
fastest garbage collector on the market. Saying that, there is a fair amount of 
low hanging fruit for anyone interested in optimising it - so some potential to
be faster exists.


How it Works
------------

For a basic _mark and sweep_ garbage collector two things are required. The 
first thing is a list of all of the allocations made by the program. The second 
is a list of all the allocations _in use_ by the program at any given time. 
With these two things the algorithm is simple - compare the two lists and free 
any allocations which are in the first list, but not in the second - exactly 
those allocations which are no longer in use.

To get a list of all the allocations made by the progam is relatively 
simple. We make the programmer use a special function we've prepared (in this
case `tgc_alloc`) which allocates memory, and then adds a pointer to that 
memory to an internal list. If at any point this allocation is freed (such as 
by `tgc_free`), it is removed from the list.

The second list is the difficult one - the list of allocations _in use_ by the 
program. At first, with C's semantics, pointer arithematic, and all the crazy 
flexibility that comes with it, it might seem like finding all the allocations 
in use by the program at any point in time is impossible, and to some extent 
you'd be right. It can actually be shown that this problem reduces to the 
halting problem in the most general case - even for languages saner than C - 
but by slightly adjusting our problem statement, and assuming we are only 
dealing with a set of _well behaved_ C programs of some form, we can come up 
with something that works.

First we have to relax our goal a little. Instead of trying to find all of 
the memory allocations _in use_ by a program, we can instead try to find all 
the _reachable_ memory allocations - those allocations which have a pointer 
pointing to them somewhere in the program's memory. The distinction here is 
subtle but important. For example, I _could_ write a C program which makes an 
allocation, encodes the returned pointer as a string, and performs `rot13` on 
that string, later on decoding the string, casting it back to a pointer, 
and using the memory as if nothing had happened. This is a perfectly valid, C 
program, and the crazy memory allocation is _is use_ throughout. It is just 
that during the pointer's `rot13` encoding there is no practical way to know 
that this memory allocation is still going to be used later on.

So instead we want to make a list of all memory allocations which are pointed 
to by pointers in the program's memory. For most _well behaved_ C programs this
is enough to tell if an allocation is in use.

In general, memory in C exists in three different segments. We have the stack,
the heap, and the data segment. This means - if a pointer to a certain 
allocation exists in the program's memory it must be in one of these locations.
Now the challenge is to find these locations, and scan them for pointers.

The data segment is the most difficult - there is no portable way to get the 
bounds of this segment. But because the data segment is somewhat limited in use 
we can choose to ignore it - we tell users that allocations only pointed to 
from the data segment are not considered reachable.

As an aside, for programmers coming from other languages, this might seem like 
a poor solution - to simply ask the programmer not to store pointers to 
allocations in this segment - and in many ways it is. It is never a good 
interface to _request_ the programmer do something in the documentation - 
instead it is better to handle every edge case to make it impossible for them 
to create an error. But this is C - in C programmers are constantly asked _not_
to do things which are perfectly possible. In fact - one of the very things 
this library is trying to deal with is the fact that programmers are only 
_asked_ to make sure they free dynamically allocated memory - there is no 
system in place to enforce this. So _for C_ this is a perfectly reasonable 
interface. And there is an added advantage - it makes the implementation far 
more simple - far more adaptable. In other words - [Worse Is Better](https://en.wikipedia.org/wiki/Worse_is_better).

With the data segment covered we have the heap and the stack. If we consider
only the heap allocations which have been made via `tgc_alloc` and friends then
our job is again made easy - in our list of all allocations we also store the
size of each allocation. Then, if we need to scan one of the memory regions 
we've allocated, the task is made easy.

With the heap and the data segment covered, this leaves us with the stack - 
this is the most tricky segment. The stack is something we don't have any 
control over, but we do know that for most reasonable implementations of C, the
stack is a continuous area of memory that is expanded downwards (or for some
implementations upwards, but it doesn't matter) for each function call. It 
contains the most important memory in regards to reachability - all of the 
local variables used in functions.

If we can get the memory addresses of the top and the bottom of the stack we 
can scan the memory inbetween as if it were heap memory, and add to our list of 
reachable pointers all those found inbetween.

Assuming the stack grows from top to bottom we can get a conservative 
approximation of the bottom of the stack by just taking the address of some 
local variable.

```c
void *stack_bottom(void) {
  int x;
  return &x;
}
```

This address should cover the memory of all the local variables for whichever
function calls it. For this reason we need to ensure two things before we 
actually do call it. First we want to make sure we flush all of the values in 
the registers onto the stack so that we don't miss a pointer hiding in a 
register, and secondly we want to make sure the call to `stack_bottom` isn't 
inlined by the compiler.

We can spill the registers into stack memory in a somewhat portable way with 
`setjmp` - which puts the registers into a `jmp_buf` variable. And we can 
ensure that the function is not inlined by only calling it via a volatile 
function pointer. The `volatile` keyword forces the compiler to always manually
read the pointer value from memory before calling the function, ensuring it
cannot be inlined.

```c
void *get_stack_bottom(void) {
  jmt_buf env;
  setjmp(env);
  void *(*volatile f)(void) = stack_bottom;
  return f();
}
```

To get the top of the stack we can again get the address of a local variable.
This time it is easier if we simply ask the programmer to supply us with one.
If the programmer wishes for the garbage collector to scan the whole stack he 
can give the address of a local variable in `main`. This address should cover 
all function calls one deeper than `main`. This we can store in some global 
(or local) variable.


```c
static void *stack_top = NULL;

int main(int argc, char **argv) {
  stack_top = &argc;
  run_program(argc, argv);
  return 1;
}
```

Now, at any point we can get a safe approximate upper and lower bound of the 
stack memory, allowing us to scan it for pointers. We interprit each bound as a
`void **` - a pointer to an array of pointers, and iterate, interpriting the
memory inbetween as pointers.

```c
void mark(void) {
  void **p;
  void **t = stack_top;
  void **b = get_stack_bottom();
  
  for (p = t; p < b; p++) {
    scan(*p);
  }
}
```




