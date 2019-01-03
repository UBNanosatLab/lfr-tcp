REV=$(shell git describe --dirty --always)

CC=gcc
CFLAGS=-Wall -Werror

SOURCES=lfr-tcp.c

all: lfr-tcp

lfr-tcp: $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) 

clean:
	rm -f lfr-tcp

.PHONY: all clean
