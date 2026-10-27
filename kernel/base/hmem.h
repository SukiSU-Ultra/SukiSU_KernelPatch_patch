/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#ifndef _KP_HMEM_H_
#define _KP_HMEM_H_

#include <stdint.h>

#include "hook.h"

int hook_mem_add(uint64_t start, int32_t size);
void *hook_mem_zalloc(uintptr_t origin_addr, enum hook_type type);
void hook_mem_free(void *hook_mem);
void *hook_get_mem_from_origin(uint64_t origin_addr);

/*
 * Installed hooks, kept as a separate static table.
 *
 * The hook memory region holds the trampolines themselves and is not safe to
 * read back at runtime, so the set is recorded here instead: appended when a
 * slot is allocated, removed when it is freed. This table touches nothing but
 * its own storage.
 *
 * Only what the allocation site knows is recorded: the origin address and the
 * hook type. Which module installed a hook is not tracked anywhere in the API.
 */
#define KP_HOOK_REG_MAX 32

typedef struct
{
    uint64_t origin;
    uint32_t type; /* enum hook_type */
    uint32_t reserved;
} kp_hook_reg_entry_t;

/* Fills at most max entries and returns the total number installed. */
int kp_hook_reg_snapshot(kp_hook_reg_entry_t *out, int max);

#endif
