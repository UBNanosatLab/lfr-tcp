REV = $(shell git describe --dirty --always)

CC ?= gcc
CFLAGS = -Wall -Werror -O2

SOURCES = lfr-tcp.c cmd_parser.c cmd_handler.c

all: lfr-tcp

lfr-tcp: $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) 

clean:
	rm -f lfr-tcp

.PHONY: all clean
