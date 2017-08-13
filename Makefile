# This file is a part of SMalloc.
# SMalloc is MIT licensed.
# Copyright (c) 2017 Andrey Rys.

SRCS = $(wildcard *.c)
LIB_OBJS = $(filter-out smalloc_test_so.o, $(SRCS:.c=.o))
TEST_OBJS = smalloc_test_so.o
override CFLAGS += -Wall -fPIC

all: $(LIB_OBJS) libsmalloc.a

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

libsmalloc.a: $(LIB_OBJS)
	ar cru $@ *.o

smalloc_test_so.so: $(TEST_OBJS)
	$(CC) $(CFLAGS) $< -shared -o $@ libsmalloc.a
	@echo Now you can test it with LD_PRELOAD=./$@ and see it works for conformant apps.

clean:
	rm -f *.a *.so *.o
