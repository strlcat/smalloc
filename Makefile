# This file is a part of SMalloc.
# SMalloc is MIT licensed.
# Copyright (c) 2017 Andrey Rys.

SRCS = $(wildcard *.c)
LIB_OBJS = $(filter-out smalloc_test_so.o, $(SRCS:.c=.o))
TEST_OBJS = smalloc_test_so.o
override CFLAGS += -Wall -fPIC

ifneq (,$(DEBUG))
override CFLAGS+=-O0 -g
else
override CFLAGS+=-O2
endif

all: $(LIB_OBJS) libsmalloc.a

%.o: %.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -I. -c -o $@ $<

libsmalloc.a: $(LIB_OBJS)
	$(CROSS_COMPILE)$(AR) cru $@ *.o

smalloc_test_so.so: $(TEST_OBJS)
	@$(MAKE) libsmalloc.a
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $< -shared -o $@ libsmalloc.a
	@echo Now you can test it with LD_PRELOAD=./$@ and see it works for conformant apps.

docs: smalloc.3
	mandoc -Tascii smalloc.3 >smalloc.3.txt

clean:
	rm -f *.a *.so *.o
