/*
 * This file is a part of SMalloc.
 * SMalloc is MIT licensed.
 * Copyright (c) 2017 Andrey Rys.
 *
 * This is an example program which is actually
 * a DSO overriding std. malloc functions.
 * It shows how to allocate new memory for a
 * task with mmap and use it as a dynamically
 * extendable heap space, which is placed at
 * almost random base address at each startup.
 *
 * On each OOM situation new page(s) are requested.
 * These pages are never released during runtime but
 * only at program exit, and the whole space is used
 * as a "scratch space".
 *
 * This is a bright example of how SMalloc can be
 * used as a generic system memory allocator.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include "smalloc.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

static long sc_page_size;

/* base pointer and size of allocated pool */
static char *xpool;
static size_t xpool_n;

static int smalloc_initialised;

/* atexit call: wipe all the data out of pool */
static void exit_smalloc(void)
{
	if (smalloc_initialised) {
		sm_release_default_pool();
		memset(xpool, 0, xpool_n);
		munmap(xpool, xpool_n);
		smalloc_initialised = 0;
	}
}

static void xerror(int x, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	if (errno) fprintf(stderr, " (%s)\n", strerror(errno));
	else fprintf(stderr, "\n");
	va_end(ap);

	exit_smalloc();
	_exit(x);
}

static void xgetrandom(void *buf, size_t size)
{
	char *ubuf = buf;
	int fd = -1;
	size_t rd;
	int x;

	/* Most common and probably available on every Nix, */
	fd = open("/dev/urandom", O_RDONLY);
	/* OpenBSD arc4 */
	if (fd == -1) fd = open("/dev/arandom", O_RDONLY);
	/* OpenBSD simple urandom */
	if (fd == -1) fd = open("/dev/prandom", O_RDONLY);
	/* OpenBSD srandom, blocking! */
	if (fd == -1) fd = open("/dev/srandom", O_RDONLY);
	/* Most common blocking. */
	if (fd == -1) fd = open("/dev/random", O_RDONLY);
	/* Very bad, is this a crippled chroot? */
	if (fd == -1) xerror(2, "urandom is required");

	x = 0;
_again:	rd = read(fd, ubuf, size);
	/* I want full random block, and there is no EOF can be! */
	if (rd < size) {
		if (x >= 100) xerror(2, "urandom always returns less bytes! (rd = %zu)", rd);
		x++;
		ubuf += rd;
		size -= rd;
		goto _again;
	}

	close(fd);
}

static void *getrndbase(void)
{
	uintptr_t r;
	xgetrandom(&r, sizeof(uintptr_t));
	r &= ~(sc_page_size-1);
#if UINTPTR_MAX == UINT64_MAX
	r &= 0xffffffffff;
#endif
	return (void *)r;
}

static void xpool_ub(struct smalloc_pool *spool, const void *offender)
{
	errno = 0;
	xerror(5, "%p: address is not from %p-%p range!", offender, xpool, xpool+spool->pool_size);
}

/* called each time we ran out of memory, in hope to get more */
static size_t xpool_oom(struct smalloc_pool *spool, size_t n)
{
	void *t;
	size_t newsz;

	newsz = (n / sc_page_size) * sc_page_size;
	if (n % sc_page_size) newsz += sc_page_size;
	if (newsz == 0) newsz += sc_page_size;

	/* get new page(s) */
	t = mmap(xpool+xpool_n, newsz,
		PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	/* failed to get? */
	if (t == MAP_FAILED) return 0;
	/* !MAP_FIXED and there are existing pages on the road already? */
	if (t != xpool+xpool_n) {
		munmap(t, newsz);
		return 0;
	}

	/* success! Return new pool size */
	xpool_n += newsz;
	return xpool_n;
}

/* single time init call: setup default pool descriptor */
static void init_smalloc(void)
{
	if (!smalloc_initialised) {
		void *p;
		sc_page_size = sysconf(_SC_PAGE_SIZE);
		if (sc_page_size == 0) sc_page_size = PAGE_SIZE;
		sm_set_ub_handler(xpool_ub);
_again:		p = getrndbase(); /* get random base pointer */
		/* allocate initial base page */
		xpool = mmap(p, sc_page_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		if (xpool == MAP_FAILED
		|| xpool != p) {
			/* try again several times */
			if (xpool != p && xpool != MAP_FAILED) munmap(p, sc_page_size);
			smalloc_initialised++;
			if (smalloc_initialised > 10) xerror(3, "failed to map page at base = %p", p);
			goto _again;
		}
		/* initial pool size == PAGE_SIZE */
		xpool_n = sc_page_size;

		/* setup SMalloc to use this pool */
		if (!sm_set_default_pool(xpool, xpool_n, 0, xpool_oom))
			xerror(4, "sm_set_default_pool failed!");

		/* register atexit cleanup call */
		atexit(exit_smalloc);

		/* well done! */
		smalloc_initialised = 1;
	}
}

/* our replacement memory management functions */

void *malloc(size_t n)
{
	if (!smalloc_initialised) init_smalloc();
	/* return sm_zalloc(n); */
	return sm_malloc(n);
}

void free(void *p)
{
	if (!smalloc_initialised) init_smalloc();
	sm_free(p);
}

void *realloc(void *p, size_t n)
{
	if (!smalloc_initialised) init_smalloc();
	return sm_realloc(p, n);
}

void *calloc(size_t y, size_t x)
{
	if (!smalloc_initialised) init_smalloc();
	return sm_calloc(y, x);
}

char *strdup(const char *s)
{
	size_t n = strlen(s);
	char *r = sm_zalloc(n+1);
	if (!r) return NULL;
	memcpy(r, s, n);
	return r;
}

char *strndup(const char *s, size_t n)
{
	size_t x = strnlen(s, n);
	char *r = sm_zalloc(x+1);
	if (!r) return NULL;
	memcpy(r, s, x);
	return r;
}
