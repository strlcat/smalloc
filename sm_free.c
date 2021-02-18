/*
 * This file is a part of SMalloc.
 * SMalloc is MIT licensed.
 * Copyright (c) 2017 Andrey Rys.
 */

#include "smalloc_i.h"

void sm_free_pool(struct smalloc_pool *spool, void *p)
{
	struct smalloc_hdr *shdr;
	struct smalloc_stats *sstats;
	char *s;
	size_t rsz, usz;

	if (!smalloc_verify_pool(spool)) {
		errno = EINVAL;
		return;
	}
	sstats = &spool->sp_stats;

	if (!p) return;

	shdr = USER_TO_HEADER(p);
	if (smalloc_is_alloc(spool, shdr)) {
		usz = shdr->shdr_size;
		rsz = REAL_SIZE(usz);
		if (spool->sp_do_zero) memset(p, 0, rsz);
		s = CHAR_PTR(p);
		s += rsz;
		memset(HEADER_PTR(s), 0, HEADER_SZ);
		memset(shdr, 0, HEADER_SZ);

		if (sstats->ss_oobsz > 0) {
			size_t total = TOTAL_SIZE(usz);
			sstats->ss_total -= total;
			sstats->ss_rfree += total;
			sstats->ss_efree = sstats->ss_rfree-sstats->ss_oobsz;
			sstats->ss_ruser -= rsz;
			sstats->ss_euser -= usz;
			sstats->ss_blkcnt--;
		}

		return;
	}

	smalloc_UB(spool, p);
	return;
}

void sm_free(void *p)
{
	sm_free_pool(&smalloc_curr_pool, p);
}
