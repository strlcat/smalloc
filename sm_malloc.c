/*
 * This file is a part of SMalloc.
 * SMalloc is MIT licensed.
 * Copyright (c) 2017 Andrey Rys.
 */

#include "smalloc_i.h"

void *sm_malloc_pool(struct smalloc_pool *spool, size_t n)
{
	struct smalloc_hdr *basehdr, *shdr, *dhdr;
	struct smalloc_stats *sstats;
	char *s;
	int found;
	size_t x;

again:	if (!smalloc_verify_pool(spool)) {
		errno = EINVAL;
		return NULL;
	}
	sstats = &spool->sp_stats;

	if (n == 0) n++; /* return a block successfully */
	if (n > SIZE_MAX
	|| n > (spool->sp_pool_size - HEADER_SZ)) goto oom;

	shdr = basehdr = (struct smalloc_hdr *)spool->sp_pool;
	while (CHAR_PTR(shdr)-CHAR_PTR(basehdr) < spool->sp_pool_size) {
		/*
		 * Already allocated block.
		 * Skip it by jumping over it.
		 */
		if (smalloc_is_alloc(spool, shdr)) {
			s = CHAR_PTR(HEADER_TO_USER(shdr));
			s += REAL_SIZE(shdr->shdr_size) + HEADER_SZ;
			shdr = HEADER_PTR(s);
			continue;
		}
		/*
		 * Free blocks ahead!
		 * Do a second search over them to find out if they're
		 * really large enough to fit the new allocation.
		 */
		else {
			dhdr = shdr; found = 0;
			while (CHAR_PTR(dhdr)-CHAR_PTR(basehdr) < spool->sp_pool_size) {
				/* pre calculate free block size */
				x = CHAR_PTR(dhdr)-CHAR_PTR(shdr);
				/*
				 * ugh, found next allocated block.
				 * skip this candidate then.
				 */
				if (smalloc_is_alloc(spool, dhdr))
					goto allocblock;
				/*
				 * did not see allocated block yet,
				 * but this free block is of enough size
				 * - finally, use it.
				 */
				if (n+HEADER_SZ <= x) {
					x -= HEADER_SZ;
					found = 1;
					goto outfound;
				}
				dhdr++;
			}

outfound:		if (found) {
				uintptr_t tag;
				/* allocate and return this block */
				shdr->shdr_tag2 = 0;
				shdr->shdr_size = n;
				shdr->shdr_tag = tag = smalloc_mktag(shdr);
				if (spool->sp_do_zero) memset(HEADER_TO_USER(shdr), 0, x);
				s = CHAR_PTR(HEADER_TO_USER(shdr));
				memset(s+n, 0xff, x-n);
				s += x;
				dhdr = HEADER_PTR(s);
				dhdr->shdr_tag2 = smalloc_uinthash(tag);
				dhdr->shdr_tag = smalloc_uinthash(dhdr->shdr_tag2);

				if (sstats->ss_oobsz > 0) {
					size_t total = TOTAL_SIZE(n);
					sstats->ss_total += total;
					sstats->ss_rfree -= total;
					sstats->ss_efree = sstats->ss_rfree-sstats->ss_oobsz;
					sstats->ss_ruser += x;
					sstats->ss_euser += n;
					sstats->ss_blkcnt++;
				}

				return HEADER_TO_USER(shdr);
			}

			/* continue first search for next free block */
allocblock:		shdr = dhdr;
			continue;
		}

		shdr++;
	}

oom:	if (spool->sp_oomfn) {
		x = spool->sp_oomfn(spool, n);
		if (x > spool->sp_pool_size) {
			spool->sp_pool_size = x;
			if (sm_align_pool(spool)) goto again;
		}
	}

	errno = ENOMEM;
	return NULL;
}

void *sm_malloc(size_t n)
{
	return sm_malloc_pool(&smalloc_curr_pool, n);
}
