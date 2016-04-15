CC = gcc
#CFLAGS = -ansi -pedantic -g -Wall -Werror -Wno-unused
CFLAGS = -std=c99 -pedantic -g -Wall -Wno-unused

EXAMPLES = $(wildcard examples/*.c)
EXAMPLESEXE = $(EXAMPLES:.c=)

all: $(EXAMPLESEXE) 

examples/%: examples/%.c tgc.c
	$(CC) $(CFLAGS) $^ -lm -o $@
  
clean:
	rm -rf examples/basic examples/bzip2