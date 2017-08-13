/*
 * This file is a part of SMalloc.
 * SMalloc is MIT licensed.
 * Copyright (c) 2017 Andrey Rys.
 */

#include "smalloc_i.h"

void *sm_realloc_pool(struct smalloc_pool *spool, void *p, size_t n)
{
	struct smalloc_hdr *basehdr, *shdr, *dhdr;
	void *r;
	int found;
	size_t rsz, usz, x;

	if (!smalloc_verify_pool(spool)) {
		errno = EINVAL;
		return NULL;
	}

	if (!p) return sm_malloc_pool(spool, n);
	if (!n && p) {
		sm_free_pool(spool, p);
		return NULL;
	}

	/* determine user size */
	shdr = USER_TO_HEADER(p);
	if (!smalloc_is_alloc(spool, shdr)) smalloc_bad_block(spool, p);
	usz = shdr->usz;
	rsz = shdr->rsz;

	/* newsize is lesser than allocated - truncate */
	if (n <= usz) {
		shdr->usz = n;
		shdr->tag = smalloc_mktag(shdr);
		if (spool->do_zero) memset(p + shdr->usz, 0, shdr->rsz - shdr->usz);
		return p;
	}

	/* newsize is bigger than allocated, but there is free room - modify */
	if (n > usz && n <= rsz) {
		shdr->usz = n;
		shdr->tag = smalloc_mktag(shdr);
		return p;
	}

	/* newsize is bigger, larger than rsz but there are free blocks beyond - extend */
	basehdr = spool->pool; dhdr = shdr+(rsz/HEADER_SZ); found = 0;
	while (CHAR_PTR(dhdr)-CHAR_PTR(basehdr) < spool->pool_size) {
		x = CHAR_PTR(dhdr)-CHAR_PTR(shdr);
		if (smalloc_is_alloc(spool, dhdr))
			goto allocblock;
		if (n <= x) {
			found = 1;
			goto outfound;
		}
		dhdr++;
	}

outfound:
	/* write new numbers of same allocation */
	if (found) {
		shdr->rsz = x;
		shdr->usz = n;
		shdr->tag = smalloc_mktag(shdr);
		return p;
	}

allocblock:
	/* newsize is bigger than allocated and no free space - move */
	r = sm_malloc_pool(spool, n);
	if (!r) return NULL;
	memcpy(r, p, usz);
	sm_free_pool(spool, p);

	return r;
}

void *sm_realloc(void *p, size_t n)
{
	return sm_realloc_pool(&smalloc_curr_pool, p, n);
}
