/*
 * This file is a part of SMalloc.
 * SMalloc is MIT licensed.
 * Copyright (c) 2017 Andrey Rys.
 */

#include "smalloc_i.h"

static int smalloc_check_bounds(struct smalloc_pool *spool, struct smalloc_hdr *shdr)
{
	if (!spool) return 0;
	if (CHAR_PTR(shdr) >= CHAR_PTR(spool->sp_pool)
	&& CHAR_PTR(shdr) <= (CHAR_PTR(spool->sp_pool)+spool->sp_pool_size))
		return 1;
	return 0;
}

static int smalloc_valid_tag(struct smalloc_hdr *shdr)
{
	char *d, *s;
	struct smalloc_hdr *dhdr;
	uintptr_t z, r = smalloc_mktag(shdr);
	size_t x, rsz, usz;

	if (shdr->shdr_tag == r) {
		rsz = REAL_SIZE(shdr->shdr_size);
		usz = shdr->shdr_size;
		d = s = CHAR_PTR(HEADER_TO_USER(shdr));
		s += usz; x = 0;
		while (x < rsz-usz) {
			if (s[x] != '\xFF') return 0;
			x++;
		}
		s = d+rsz;
		dhdr = HEADER_PTR(s);
		r = smalloc_uinthash(r);
		z = smalloc_uinthash(r);
		if (dhdr->shdr_tag2 != r) return 0;
		if (dhdr->shdr_tag != z) return 0;
		return 1;
	}
	return 0;
}

static void smalloc_do_crash(struct smalloc_pool *spool, const void *p)
{
	char *c = NULL;
	*c = 'X';
}

smalloc_ub_handler smalloc_UB = smalloc_do_crash;

void sm_set_ub_handler(smalloc_ub_handler handler)
{
	if (!handler) smalloc_UB = smalloc_do_crash;
	else smalloc_UB = handler;
}

int smalloc_is_alloc(struct smalloc_pool *spool, struct smalloc_hdr *shdr)
{
	if (!smalloc_check_bounds(spool, shdr)) return 0;
	if (shdr->shdr_size == 0 || shdr->shdr_size > SIZE_MAX) return 0;
	if (!smalloc_valid_tag(shdr)) return 0;
	return 1;
}
