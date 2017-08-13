# This file is a part of SMalloc.
# SMalloc is MIT licensed.
# Copyright (c) 2017 Andrey Rys.

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
override CFLAGS += -Wall

all: $(OBJS) libsmalloc.a

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

libsmalloc.a: $(OBJS)
	ar cru $@ *.o

clean:
	rm -f *.a *.o
