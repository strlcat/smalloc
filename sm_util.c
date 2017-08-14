/*
 * This file is a part of SMalloc.
 * SMalloc is MIT licensed.
 * Copyright (c) 2017 Andrey Rys.
 */

#include "smalloc_i.h"

static int smalloc_check_bounds(struct smalloc_pool *spool, struct smalloc_hdr *shdr)
{
	if (!spool) return 0;
	if (CHAR_PTR(shdr) >= CHAR_PTR(spool->pool)
	&& CHAR_PTR(shdr) <= (CHAR_PTR(spool->pool)+spool->pool_size))
		return 1;
	return 0;
}

static int smalloc_valid_tag(struct smalloc_hdr *shdr)
{
	uintptr_t r = smalloc_mktag(shdr);
	if (shdr->tag == r) return 1;
	return 0;
}

static void smalloc_do_crash(struct smalloc_pool *spool, void *p)
{
	char *c = NULL;
	*c = 'X';
}

smalloc_bad_block_handler smalloc_bad_block = smalloc_do_crash;

void sm_set_bad_block_handler(smalloc_bad_block_handler handler)
{
	if (!handler) smalloc_bad_block = smalloc_do_crash;
	else smalloc_bad_block = handler;
}

int smalloc_is_alloc(struct smalloc_pool *spool, struct smalloc_hdr *shdr)
{
	if (!smalloc_check_bounds(spool, shdr)) return 0;
	if (!smalloc_valid_tag(shdr)) return 0;
	if (shdr->rsz == 0) return 0;
	if (shdr->rsz > SIZE_MAX) return 0;
	if (shdr->usz > SIZE_MAX) return 0;
	if (shdr->usz > shdr->rsz) return 0;
	if (shdr->rsz % HEADER_SZ) return 0;
	return 1;
}
