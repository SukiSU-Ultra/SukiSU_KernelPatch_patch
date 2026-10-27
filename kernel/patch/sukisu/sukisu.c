/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "supercall.h"
#include <syscall.h>
#include <module.h>
#include <linux/errno.h>
#include <predata.h>
#include <common.h>
#include <kallsyms.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/rculist.h>
#include <linux/rcupdate.h>

#include "../../base/hmem.h"

static long list_snapshot(char *buf, int capacity)
{
    extern struct module modules;
    struct module *mod;
    long used = 0;
    buf[0] = '\0';
    rcu_read_lock();
    list_for_each_entry(mod, &modules.list, list) {
        long len = strlen(mod->info.name);
        long separator = used ? 1 : 0;
        if (len + separator >= capacity - used) {
            rcu_read_unlock();
            return -ENOBUFS;
        }
        if (separator) buf[used++] = '\n';
        memcpy(buf + used, mod->info.name, len);
        used += len;
        buf[used] = '\0';
    }
    rcu_read_unlock();
    return used;
}

static const char *hook_kind_name(uint32_t type)
{
    switch (type) {
    case INLINE:
        return "inline";
    case INLINE_CHAIN:
        return "inline_chain";
    case FUNCTION_POINTER_CHAIN:
        return "fp_chain";
    default:
        return "none";
    }
}

/*
 * Text, not a struct: the two sides of this bridge are built from separate
 * headers and released independently, so a packed layout would couple them.
 *
 *   HOOK_LIST_V1 total=<installed> shown=<lines below>
 *   type=<inline|inline_chain|fp_chain> origin=0x<addr>
 *
 * Read from the registry the allocator maintains, so this touches no hook
 * memory and costs a fixed copy plus a few dozen `snprintf` calls. The caller
 * turns addresses into names.
 */
static long hook_list_text(char *out, long capacity)
{
    kp_hook_reg_entry_t entries[KP_HOOK_REG_MAX];
    int total, shown;
    long used;
    int len;

    total = kp_hook_reg_snapshot(entries, KP_HOOK_REG_MAX);
    shown = total > KP_HOOK_REG_MAX ? KP_HOOK_REG_MAX : total;

    len = snprintf(out, capacity, "HOOK_LIST_V1 total=%d shown=%d", total, shown);
    if (len < 0) return -EINVAL;
    if (len >= capacity) return -ENOBUFS;
    used = len;

    for (int i = 0; i < shown; i++) {
        len = snprintf(out + used, capacity - used, "\ntype=%s origin=0x%llx",
                       hook_kind_name(entries[i].type),
                       (unsigned long long)entries[i].origin);
        if (len < 0) return -EINVAL;
        if (len >= capacity - used) return -ENOBUFS;
        used += len;
    }
    return used;
}

/* All strings/output buffers here are kernel-owned, except ctl0 output and
 * the loader reserved field, which retain their original userspace ABI. */
static long dispatch(unsigned long op, unsigned long a1, unsigned long a2,
                     unsigned long a3, unsigned long a4)
{
    char *out;
    long capacity, rc;
    switch (op) {
    case SUKISU_KPM_PROBE:
        if (a1 || a2 || a3 || a4) return -EINVAL;
        return SUKISU_KPM_ABI_VERSION;
    case SUKISU_KPM_HOOKS:
        if (a3 || a4) return -EINVAL;
        out = (char *)a1;
        capacity = (long)a2;
        if (!out || capacity <= 0 || capacity > SUKISU_KPM_OUTPUT_MAX) return -EINVAL;
        out[0] = '\0';
        return hook_list_text(out, capacity);
    case SUKISU_KPM_LOAD:
        if (a4) return -EINVAL;
        if (!a1) return -EINVAL;
        return load_module_path((const char *)a1, (const char *)a2, (void *)a3);
    case SUKISU_KPM_LOAD_EVENT:
        if (a4) return -EINVAL;
        if (!a1 || !a3) return -EINVAL;
        return load_module_path_event((const char *)a1, (const char *)a2,
                                      (const char *)a3, (void *)a4);
    case SUKISU_KPM_UNLOAD:
        if (a3 || a4) return -EINVAL;
        if (!a1) return -EINVAL;
        return unload_module((const char *)a1, (void *)a2);
    case SUKISU_KPM_NUM:
        if (a1 || a2 || a3 || a4) return -EINVAL;
        return get_module_nums();
    case SUKISU_KPM_CONTROL:
        if (!a1 || !a2 || a4 > SUKISU_KPM_OUTPUT_MAX || (a4 && !a3)) return -EINVAL;
        return module_control0((const char *)a1, (const char *)a2, (char __user *)a3, a4);
    case SUKISU_KPM_LIST:
    case SUKISU_KPM_INFO:
    case SUKISU_KPM_VERSION:
        if (a4 || (op != SUKISU_KPM_INFO && a3)) return -EINVAL;
        out = (char *)(op == SUKISU_KPM_INFO ? a2 : a1);
        capacity = op == SUKISU_KPM_INFO ? a3 : a2;
        if (!out || capacity <= 0 || capacity > SUKISU_KPM_OUTPUT_MAX) return -EINVAL;
        out[0] = '\0';
        if (op == SUKISU_KPM_LIST) return list_snapshot(out, capacity);
        if (op == SUKISU_KPM_INFO) {
            if (!a1) return -EINVAL;
            rc = get_module_info((const char *)a1, out, capacity);
            /* Legacy INFO has a fixed buffer and no size negotiation. */
            if (rc >= capacity) rc = capacity - 1;
        } else {
            rc = snprintf(out, capacity, "%u (%s) hooks=%u", kpver, get_build_time(),
                          (unsigned)SUKISU_KPM_HOOK_PROTO);
        }
        return rc >= capacity ? -ENOBUFS : rc;
    default:
        return -EOPNOTSUPP;
    }
}

static void before_supercall(hook_fargs1_t *args, void *udata)
{
    struct pt_regs *regs = (struct pt_regs *)args->arg0;
    (void)udata;
    /* Check the trusted saved mode before the public protocol tag. EL0 calls
     * must run the original truncate even when they supply the tag: a special
     * errno or early return would provide a trivial presence oracle.
     * This is a kernel-call convention, NOT proof of SukiSU caller identity.
     */
    if (!regs || processor_mode(regs) != PSR_MODE_EL1h) return;
    if (regs->regs[0] != SUKISU_KPM_SUPERCALL_MAGIC) return;
    args->skip_origin = 1;
    /* Fail closed before interpreting ANY operation pointer. Keep malformed
     * kernel requests out of the original truncate handler as well. */
    args->ret = -EPROTO;
    if (regs->regs[6] != SUKISU_KPM_SUPERCALL_MAGIC2 ||
        regs->regs[7] != SUKISU_KPM_SUPERCALL_MAGIC3 ||
        regs->regs[8] != SUKISU_KPM_ABI_VERSION) return;
    for (int i = 9; i < 31; i++) {
        if (regs->regs[i]) return;
    }
    args->ret = dispatch(regs->regs[1], regs->regs[2], regs->regs[3],
                         regs->regs[4], regs->regs[5]);
}

int sukisu_kpm_supercall_install(void)
{
    static int installed;
    int rc;
    unsigned long addr;
    if (installed) return 0;
    if (!has_syscall_wrapper) return -EOPNOTSUPP;
    /* Hook the handler itself: a kernel direct call never enters invoke_syscall
     * or the global EL0 dispatcher. Prefer inline over the syscall table. */
    addr = kallsyms_lookup_name("__arm64_sys_truncate");
    if (!addr) addr = kallsyms_lookup_name("__arm64_sys_truncate.cfi_jt");
    if (!addr) addr = kallsyms_lookup_name("__arm64_sys_truncate.cfi");
    if (!addr) return -ENOENT;
    rc = hook_wrap1((void *)addr, before_supercall, 0, 0);
    if (!rc) installed = 1;
    return rc;
}
