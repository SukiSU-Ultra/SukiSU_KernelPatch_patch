/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _SUKISU_KPM_SUPERCALL_H_
#define _SUKISU_KPM_SUPERCALL_H_

/* Kernel-to-kernel ABI v2. Keep in sync with SukiSU extension/kpm/bridge.h.
 * x0=magic, x1=op, x2=a1, x3=a2, x4=a3, x5=a4.
 * Called directly through __arm64_sys_truncate with an EL1 pt_regs frame.
 * Never expose kernel pointers through a userspace syscall.
 * The magic is a PUBLIC protocol tag, not a secret or authentication token.
 * EL0 requests always fall through to the original syscall.
 * EL1h is a calling convention, not proof of SukiSU caller identity.
 */
/* Public framing tags: ALL must match. These are not authentication. */
#define SUKISU_KPM_SUPERCALL_MAGIC 0x53554b504d000002UL
#define SUKISU_KPM_SUPERCALL_MAGIC2 0x4b504d4252494447UL
#define SUKISU_KPM_SUPERCALL_MAGIC3 0x53554b494e544552UL
/* x6=magic2, x7=magic3, x8=ABI version; x9..x30 reserved (zero).
 * Internal synthetic pt_regs only; the userspace extension ABI is unchanged.
 * No fallback to v1: rebuild KernelPatch and SukiSU together.
 */
#define SUKISU_KPM_OUTPUT_MAX 4096
#define SUKISU_KPM_LOAD 1
#define SUKISU_KPM_UNLOAD 2
#define SUKISU_KPM_NUM 3
#define SUKISU_KPM_LIST 4
#define SUKISU_KPM_INFO 5
#define SUKISU_KPM_CONTROL 6
#define SUKISU_KPM_VERSION 7
#define SUKISU_KPM_PROBE 0
#define SUKISU_KPM_ABI_VERSION 1

int sukisu_kpm_supercall_install(void);
#endif
