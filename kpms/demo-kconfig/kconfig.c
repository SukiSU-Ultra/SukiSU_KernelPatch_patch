/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 bmax121. All Rights Reserved.
 */

#include <compiler.h>
#include <kpmodule.h>
#include <kconfig.h>
#include <linux/printk.h>

KPM_NAME("kpm-kconfig-demo");
KPM_VERSION("1.0.0");
KPM_LICENSE("GPL v2");
KPM_AUTHOR("bmax121");
KPM_DESCRIPTION("Kernel config query demo");

static long kconfig_demo_init(const char *args, const char *event, void *__user reserved)
{
    (void)reserved;

    pr_info("kconfig demo init, event: %s, args: %s\n", event, args);

    if (!kp_kconfig_available()) {
        pr_info("kernel config is not available\n");
        return 0;
    }

    pr_info("CONFIG_KALLSYMS enabled: %d, value: %s\n", kp_kconfig_enabled("KALLSYMS"),
            kp_kconfig_value("KALLSYMS") ?: "null");
    pr_info("CONFIG_KALLSYMS_ALL enabled: %d, value: %s\n", kp_kconfig_enabled("CONFIG_KALLSYMS_ALL"),
            kp_kconfig_value("CONFIG_KALLSYMS_ALL") ?: "null");

    return 0;
}

static long kconfig_demo_exit(void *__user reserved)
{
    (void)reserved;
    pr_info("kconfig demo exit\n");
    return 0;
}

KPM_INIT(kconfig_demo_init);
KPM_EXIT(kconfig_demo_exit);
