CC ?= gcc
AR ?= ar
CFLAGS = -ansi -O3 -fpic -pedantic -g -Wall -Wno-unused
LFLAGS = -fpic
ECFLAGS = -std=gnu99 -O3 -g -Wno-overlength-strings

INCDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib

OBJECT = tgc.o
STATIC = libtgc.a
DYNAMIC = libtgc.so

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
valgrind --undef-value-errors=no --leak-check=full ./examples/basic
	$(CC) $(ECFLAGS) examples/advanced.c tgc.c -o ./examples/advanced && \
valgrind --undef-value-errors=no --leak-check=full ./examples/advanced  
	$(CC) $(ECFLAGS) examples/bzip2.c tgc.c -o ./examples/bzip2 && \
valgrind --undef-value-errors=no --leak-check=full ./examples/bzip2 -c ./examples/Lenna.png -9 > ./examples/Lenna.png.bz
	$(CC) $(ECFLAGS) examples/mpc.c tgc.c -o ./examples/mpc && \
valgrind --undef-value-errors=no  --leak-check=full ./examples/mpc ./examples/prelude.lspy
	$(CC) $(ECFLAGS) examples/oggenc.c tgc.c -lm -o ./examples/oggenc && \
valgrind --undef-value-errors=no  --leak-check=full ./examples/oggenc ./examples/jfk.wav  

clean:
	rm -rf $(STATIC) $(DYNAMIC) $(OBJECT)
