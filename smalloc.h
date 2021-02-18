/*
 * SMalloc -- a *static* memory allocator.
 *
 * See README for a complete description.
 *
 * SMalloc is MIT licensed.
 * Copyright (c) 2017 Andrey Rys.
 * Written during Aug2017.
 */

#ifndef _SMALLOC_H
#define _SMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define SM_NOSIZE ((size_t)-1)

struct smalloc_pool;

/* undefined behavior handler is called on typical malloc UB situations */
typedef void (*smalloc_ub_handler)(struct smalloc_pool *, const void *);
/* out of memory handler is called on hard out of memory conditions */
typedef size_t (*smalloc_oom_handler)(struct smalloc_pool *, size_t);

/* pool statistics easily accessible from pool struct */
struct smalloc_stats {
	size_t ss_total; /* total used bytes */
	size_t ss_ruser; /* real user only accessible space */
	size_t ss_euser; /* effective user only accessible space */
	size_t ss_blkcnt; /* nr. of allocated blocks */
	size_t ss_rfree; /* total free bytes */
	size_t ss_efree; /* size of next allocation cannot exceed this value */
	size_t ss_oobsz; /* overhead size in bytes per each allocation */
};

/* describes static pool, if you're going to use multiple pools at same time */
struct smalloc_pool {
	void *sp_pool; /* pointer to your pool */
	size_t sp_pool_size; /* it's size. Must be aligned with sm_align_pool. */
	int sp_do_zero; /* zero pool before use and all the new allocations from it. */
	smalloc_oom_handler sp_oomfn; /* this will be called, if non-NULL, on OOM condition in pool */
	struct smalloc_stats sp_stats; /* current pool allocation statistics */
};

/* a default one which is initialised with sm_set_default_pool. */
extern struct smalloc_pool smalloc_curr_pool;

void sm_set_ub_handler(smalloc_ub_handler);

int sm_align_pool(struct smalloc_pool *);
int sm_set_pool(struct smalloc_pool *, void *, size_t, int, int, smalloc_oom_handler);
int sm_set_default_pool(void *, size_t);
int sm_release_pool(struct smalloc_pool *);
int sm_release_default_pool(void);

/* Use these with multiple pools which you control */

void *sm_malloc_pool(struct smalloc_pool *, size_t);
void *sm_zalloc_pool(struct smalloc_pool *, size_t);
void sm_free_pool(struct smalloc_pool *, void *);

void *sm_realloc_pool(struct smalloc_pool *, void *, size_t);
void *sm_realloc_move_pool(struct smalloc_pool *, void *, size_t);
void *sm_calloc_pool(struct smalloc_pool *, size_t, size_t);

int sm_alloc_valid_pool(struct smalloc_pool *spool, const void *p);

size_t sm_szalloc_pool(struct smalloc_pool *, const void *);

/* Use these when you use just default smalloc_curr_pool pool */

void *sm_malloc(size_t);
void *sm_zalloc(size_t); /* guarantee zero memory allocation */
void sm_free(void *);

void *sm_realloc(void *, size_t);
void *sm_realloc_move(void *, size_t);
void *sm_calloc(size_t, size_t); /* calls zalloc internally */

int sm_alloc_valid(const void *p); /* verify pointer without intentional crash, doesn't call UB handler on invalid area */

size_t sm_szalloc(const void *); /* get size of allocation, does call UB handler on invalid area */

#ifdef __cplusplus
}
#endif

#endif
