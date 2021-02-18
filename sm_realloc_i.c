/*
 * This file is a part of SMalloc.
 * SMalloc is MIT licensed.
 * Copyright (c) 2017 Andrey Rys.
 */

#include "smalloc_i.h"

#define UPDATE_STATS(usub, rsub, uadd, radd)					\
	do {									\
		if (sstats->ss_oobsz > 0) {					\
			size_t stot, atot;					\
			stot = TOTAL_SIZE(usub);				\
			atot = TOTAL_SIZE(uadd);				\
			sstats->ss_total -= stot;				\
			sstats->ss_total += atot;				\
			sstats->ss_rfree += stot;				\
			sstats->ss_rfree -= atot;				\
			sstats->ss_efree = sstats->ss_rfree-sstats->ss_oobsz;	\
			sstats->ss_ruser -= rsub;				\
			sstats->ss_ruser += radd;				\
			sstats->ss_euser -= usub;				\
			sstats->ss_euser += uadd;				\
			/* sstats->ss_blkcnt is unchanged */			\
		}								\
	} while (0)

/*
 * Please do NOT use this function directly or rely on it's presence.
 * It may go away in future SMalloc versions, or it's calling
 * signature may change. It is internal function, hence "_i" suffix.
 */
void *sm_realloc_pool_i(struct smalloc_pool *spool, void *p, size_t n, int nomove)
{
	struct smalloc_hdr *basehdr, *shdr, *dhdr;
	struct smalloc_stats *sstats;
	size_t rsz, orsz, usz, x;
	uintptr_t tag;
	int found;
	char *s, *d;
	void *r;

	if (!smalloc_verify_pool(spool)) {
		errno = EINVAL;
		return NULL;
	}

	if (!p) return sm_malloc_pool(spool, n);
	if (!n && p) {
		sm_free_pool(spool, p);
		return NULL;
	}

	sstats = &spool->sp_stats;
	/* determine user size */
	shdr = USER_TO_HEADER(p);
	if (!smalloc_is_alloc(spool, shdr)) smalloc_UB(spool, p);
	usz = shdr->shdr_size;
	rsz = orsz = REAL_SIZE(shdr->shdr_size);

	/* n == size, why modify? just return */
	if (n == usz) return p;
	/* newsize is lesser than allocated - truncate */
	if (n < usz) {
		s = CHAR_PTR(HEADER_TO_USER(shdr));
		memset(s+rsz, 0, HEADER_SZ);
		if (spool->sp_do_zero) memset(s+n, 0, rsz-n);
		shdr->shdr_tag2 = 0;
		shdr->shdr_size = n;
		rsz = REAL_SIZE(n);
		shdr->shdr_tag = tag = smalloc_mktag(shdr);
		memset(s+n, 0xff, rsz-n);
		s += rsz;
		dhdr = HEADER_PTR(s);
		dhdr->shdr_tag2 = smalloc_uinthash(tag);
		dhdr->shdr_tag = smalloc_uinthash(dhdr->shdr_tag2);

		UPDATE_STATS(usz, orsz, n, rsz);

		return p;
	}

	/* newsize is bigger than allocated, but there is free room - modify headers only */
	if (n > usz && n <= rsz) {
		s = CHAR_PTR(HEADER_TO_USER(shdr));
		if (spool->sp_do_zero) memset(s+usz, 0, rsz-usz);
		shdr->shdr_tag2 = 0;
		shdr->shdr_size = n;
		shdr->shdr_tag = tag = smalloc_mktag(shdr);
		memset(s+n, 0xff, rsz-n);
		s += rsz;
		dhdr = HEADER_PTR(s);
		dhdr->shdr_tag2 = smalloc_uinthash(tag);
		dhdr->shdr_tag = smalloc_uinthash(dhdr->shdr_tag2);

		UPDATE_STATS(usz, orsz, n, rsz);

		return p;
	}

	/* newsize is bigger, larger than rsz but there are free blocks beyond - extend */
	basehdr = (struct smalloc_hdr *)spool->sp_pool; dhdr = shdr+(rsz/HEADER_SZ); found = 0;
	while (CHAR_PTR(dhdr)-CHAR_PTR(basehdr) < spool->sp_pool_size) {
		x = CHAR_PTR(dhdr)-CHAR_PTR(shdr);
		if (smalloc_is_alloc(spool, dhdr)) goto allocblock;
		if (n+HEADER_SZ <= x) {
			x -= HEADER_SZ;
			found = 1;
			goto outfound;
		}
		dhdr++;
	}

outfound:
	/* write new numbers of same allocation */
	if (found) {
		if (spool->sp_do_zero) {
			d = s = CHAR_PTR(HEADER_TO_USER(shdr));
			s = d+usz;
			memset(s, 0, n-usz);
			s = d+rsz;
			memset(s, 0, HEADER_SZ);
		}
		shdr->shdr_tag2 = 0;
		shdr->shdr_size = n;
		shdr->shdr_tag = tag = smalloc_mktag(shdr);
		s = CHAR_PTR(HEADER_TO_USER(shdr));
		memset(s+n, 0xff, x-n);
		s += x;
		dhdr = HEADER_PTR(s);
		dhdr->shdr_tag2 = smalloc_uinthash(tag);
		dhdr->shdr_tag = smalloc_uinthash(dhdr->shdr_tag2);

		UPDATE_STATS(usz, orsz, n, x);

		return p;
	}

allocblock:
	/* newsize is bigger than allocated and no free space - move */
	if (nomove) {
		/* fail if user asked */
		errno = ERANGE;
		return NULL;
	}
	r = sm_malloc_pool(spool, n);
	if (!r) return NULL;
	memcpy(r, p, usz);
	sm_free_pool(spool, p);

	return r;
}
