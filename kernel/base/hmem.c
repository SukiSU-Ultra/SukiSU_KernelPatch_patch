/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#include "hook.h"
#include "hmem.h"

#include <stdint.h>

static uint64_t mem_region_start = 0;
static uint64_t mem_region_end = 0;

/*
 * Hook set, appended when a slot is taken and removed when it is released.
 * Written only here and read by kp_hook_reg_snapshot(): this is the safe way
 * to answer "what is hooked", because the hook memory region itself cannot be
 * read back at runtime.
 */
static kp_hook_reg_entry_t hook_reg[KP_HOOK_REG_MAX];
static int hook_reg_count;

static void hook_reg_add(uint64_t origin, uint32_t type)
{
    int i;

    for (i = 0; i < hook_reg_count; i++) {
        if (hook_reg[i].origin == origin) {
            hook_reg[i].type = type;
            return;
        }
    }
    if (hook_reg_count >= KP_HOOK_REG_MAX) return;
    hook_reg[hook_reg_count].origin = origin;
    hook_reg[hook_reg_count].type = type;
    hook_reg[hook_reg_count].reserved = 0;
    hook_reg_count++;
}

static void hook_reg_remove(uint64_t origin)
{
    int i;

    for (i = 0; i < hook_reg_count; i++) {
        if (hook_reg[i].origin == origin) {
            /* Order does not matter here, so move the tail into the hole
             * instead of shifting everything down. */
            hook_reg[i] = hook_reg[hook_reg_count - 1];
            hook_reg_count--;
            return;
        }
    }
}

int kp_hook_reg_snapshot(kp_hook_reg_entry_t *out, int max)
{
    int i, n;

    if (!out || max <= 0) return 0;
    n = hook_reg_count < max ? hook_reg_count : max;
    for (i = 0; i < n; i++) out[i] = hook_reg[i];
    return hook_reg_count;
}

typedef struct
{
    int using;
    enum hook_type type;
    uintptr_t addr;
    // must align 8
    union
    {
        hook_t inl;
        hook_chain_t inl_chain;
        fp_hook_chain_t fp_chain;
    } chain __attribute__((aligned(8)));
} hook_mem_warp_t __attribute__((aligned(16)));

int hook_mem_add(uint64_t start, int32_t size)
{
    for (uint64_t i = start; i < start + size; i += 8) {
        *(uint64_t *)i = 0;
    }
    mem_region_start = start;
    mem_region_end = start + size;
    return 0;
}

void *hook_mem_zalloc(uintptr_t origin_addr, enum hook_type type)
{
    uint64_t start = mem_region_start;
    for (uint64_t addr = start; addr < mem_region_end; addr += sizeof(hook_mem_warp_t)) {
        hook_mem_warp_t *wrap = (hook_mem_warp_t *)addr;
        if (wrap->using) continue;

        wrap->using = 1;
        wrap->addr = origin_addr;
        wrap->type = type;

        for (uintptr_t i = (uintptr_t)&wrap->chain; i < (uintptr_t)&wrap->chain + sizeof(wrap->chain); i += 8) {
            *(uint64_t *)i = 0;
        }

        // todo: assert
        if (((uintptr_t)&wrap->chain) & 0b111) {
            return 0;
        }
        hook_reg_add(origin_addr, (uint32_t)type);
        return &wrap->chain;
    }
    return 0;
}

void hook_mem_free(void *hook_mem)
{
    hook_mem_warp_t *warp = local_container_of(hook_mem, hook_mem_warp_t, chain);
    hook_reg_remove(warp->addr);
    warp->using = 0;
}

void *hook_get_mem_from_origin(uint64_t origin_addr)
{
    uint64_t start = mem_region_start;

    for (uint64_t addr = start; addr < mem_region_end; addr += sizeof(hook_mem_warp_t)) {
        hook_mem_warp_t *wrap = (hook_mem_warp_t *)addr;
        if (wrap->using && wrap->addr == origin_addr) {
            return &wrap->chain;
        }
    }
    return 0;
}
