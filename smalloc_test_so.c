/*
 * This file is a part of SMalloc.
 * SMalloc is MIT licensed.
 * Copyright (c) 2017 Andrey Rys.
 */

#include "smalloc.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* our big data pool */
static char xpool[262144];

/* atexit call: wipe all the data out of pool */
static void exit_smalloc(void)
{
	sm_release_default_pool();
	memset(xpool, 0, sizeof(xpool));
}

/* single time init call: setup default pool descriptor */
static void init_smalloc(void)
{
	static int done;

	if (!done) {
		if (!sm_set_default_pool(xpool, sizeof(xpool), 0, NULL)) _exit(24);
		memset(xpool, 'X', sizeof(xpool));
		atexit(exit_smalloc);
		done = 1;
	}
}

/* our replacement memory management functions */

void *malloc(size_t n)
{
	init_smalloc();
	/* return sm_zalloc(n); */
	return sm_malloc(n);
}

void free(void *p)
{
	init_smalloc();
	sm_free(p);
}

void *realloc(void *p, size_t n)
{
	init_smalloc();
	return sm_realloc(p, n);
}

void *calloc(size_t y, size_t x)
{
	init_smalloc();
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
