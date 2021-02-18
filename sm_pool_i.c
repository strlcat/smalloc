/*
 * This file is a part of SMalloc.
 * SMalloc is MIT licensed.
 * Copyright (c) 2017 Andrey Rys.
 */

#include "smalloc_i.h"

struct smalloc_pool smalloc_curr_pool;

int smalloc_verify_pool(struct smalloc_pool *spool)
{
	if (!spool->sp_pool || !spool->sp_pool_size) return 0;
	if (spool->sp_pool_size % HEADER_SZ) return 0;
	return 1;
}

int sm_align_pool(struct smalloc_pool *spool)
{
	size_t x;

	if (smalloc_verify_pool(spool)) return 1;

	x = spool->sp_pool_size % HEADER_SZ;
	if (x) spool->sp_pool_size -= x;
	if (spool->sp_pool_size <= MIN_POOL_SZ) {
		errno = ENOSPC;
		return 0;
	}

	return 1;
}

int sm_set_pool(struct smalloc_pool *spool, void *new_pool, size_t new_pool_size, int do_zero, int do_stats, smalloc_oom_handler oom_handler)
{
	if (!spool) {
		errno = EINVAL;
		return 0;
	}

	if (!new_pool || !new_pool_size) {
		if (smalloc_verify_pool(spool)) {
			if (spool->sp_do_zero) memset(spool->sp_pool, 0, spool->sp_pool_size);
			memset(spool, 0, sizeof(struct smalloc_pool));
			return 1;
		}

		errno = EINVAL;
		return 0;
	}

	spool->sp_pool = new_pool;
	spool->sp_pool_size = new_pool_size;
	spool->sp_oomfn = oom_handler;
	if (!sm_align_pool(spool)) return 0;

	if (do_zero) {
		spool->sp_do_zero = do_zero;
		memset(spool->sp_pool, 0, spool->sp_pool_size);
	}

	memset(&spool->sp_stats, 0, sizeof(struct smalloc_stats));
	if (do_stats) {
		spool->sp_stats.ss_rfree = spool->sp_stats.ss_efree = spool->sp_pool_size;
		spool->sp_stats.ss_oobsz = (HEADER_SZ*2);
	}

	return 1;
}

int sm_release_pool(struct smalloc_pool *spool)
{
	return sm_set_pool(spool, NULL, 0, 0, 0, NULL);
}
