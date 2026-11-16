/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Host (native simulator) side: built against the host libc. */

#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

void *npcx_kbd_test_map_regs(uintptr_t addr, size_t len)
{
	void *p = mmap((void *)addr, len, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);

	return p == MAP_FAILED ? NULL : p;
}
