CC ?= gcc
AR ?= ar
CFLAGS = -ansi -O3 -fpic -pedantic -g -Wall -Wno-unused
LFLAGS = -fpic
ECFLAGS = -std=c99 -O3 -g

INCDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib

OBJECT = tgc.o
STATIC = libtgc.a
DYNAMIC = libtgc.so

ifeq ($(findstring MINGW,$(shell uname)),MINGW)
  CHECKER = 
else 
  CHECKER = valgrind --undef-value-errors=no  --leak-check=full 
endif

all: $(STATIC) $(DYNAMIC)

$(OBJECT): tgc.c tgc.h
	$(CC) -c $(CFLAGS) tgc.c

$(DYNAMIC): $(OBJECT)
	$(CC) -shared -o $@ $^

$(STATIC): $(OBJECT)
	$(AR) rcs $@ $^

install:
	cp -f $(STATIC) $(LIBDIR)
	cp -f tgc.h $(INCDIR)

check:
	$(CC) $(ECFLAGS) examples/basic.c tgc.c -o ./examples/basic && \
$(CHECKER) ./examples/basic
	$(CC) $(ECFLAGS) examples/advanced.c tgc.c -o ./examples/advanced && \
$(CHECKER) ./examples/advanced  
	$(CC) $(ECFLAGS) examples/bzip2.c tgc.c -o ./examples/bzip2 && \
$(CHECKER) ./examples/bzip2 -c ./examples/Lenna.png -9 > ./examples/Lenna.png.bz
	$(CC) $(ECFLAGS) examples/mpc.c tgc.c -o ./examples/mpc && \
$(CHECKER) ./examples/mpc ./examples/prelude.lspy
	$(CC) $(ECFLAGS) examples/oggenc.c tgc.c -lm -o ./examples/oggenc && \
$(CHECKER) ./examples/oggenc ./examples/jfk.wav  

clean:
	rm -rf $(STATIC) $(DYNAMIC) $(OBJECT)
