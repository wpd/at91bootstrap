// SPDX-License-Identifier: GPL-2.0+
/*
 * Simple malloc implementation
 *
 * Copyright (c) 2014 Google, Inc
 */

#include <ubi-common.h>
#include <malloc.h>
#include <mapmem.h>
#include <asm/io.h>

//DECLARE_GLOBAL_DATA_PTR;
static unsigned long malloc_base;
static unsigned long malloc_limit = 0x21000000;
static unsigned long malloc_ptr = 0x20000000;
void *malloc_simple(size_t bytes)
{
	ulong new_ptr;
	void *ptr;

	new_ptr = malloc_ptr + bytes;
	debug("%s: size=%zx, ptr=%lx, limit=%lx: ", __func__, bytes, new_ptr,
	      malloc_limit);
	if (new_ptr > malloc_limit) {
		debug("space exhausted\n");
		return NULL;
	}
	ptr = map_sysmem(malloc_base + malloc_ptr, bytes);
	malloc_ptr = ALIGN(new_ptr, sizeof(new_ptr));
	debug("%lx\n", (ulong)ptr);

	return ptr;
}

void *memalign_simple(size_t align, size_t bytes)
{
	ulong addr, new_ptr;
	void *ptr;

	addr = ALIGN(malloc_base + malloc_ptr, align);
	new_ptr = addr + bytes - malloc_base;
	if (new_ptr > malloc_limit) {
		debug("space exhausted\n");
		return NULL;
	}

	ptr = map_sysmem(addr, bytes);
	malloc_ptr = ALIGN(new_ptr, sizeof(new_ptr));
	debug("%lx\n", (ulong)ptr);

	return ptr;
}

#if CONFIG_IS_ENABLED(SYS_MALLOC_SIMPLE)
void *calloc(size_t nmemb, size_t elem_size)
{
	size_t size = nmemb * elem_size;
	void *ptr;

	ptr = malloc(size);
	if (ptr)
		memset(ptr, '\0', size);

	return ptr;
}
#endif
