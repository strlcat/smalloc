/*
 * This file is a part of SMalloc.
 * SMalloc is MIT licensed.
 * Copyright (c) 2017 Andrey Rys.
 */

#include "smalloc_i.h"

int sm_set_default_pool(void *new_pool, size_t new_pool_size)
{
	return sm_set_pool(&smalloc_curr_pool, new_pool, new_pool_size, 1, 0, NULL);
}

int sm_release_default_pool(void)
{
	return sm_release_pool(&smalloc_curr_pool);
}
