/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2023 bmax121. All Rights Reserved.
 */

#ifndef _KP_KCONFIG_H_
#define _KP_KCONFIG_H_

#include <ktypes.h>

extern bool has_config_compat;

int kp_kconfig_available(void);
int kp_kconfig_enabled(const char *name);
const char *kp_kconfig_value(const char *name);
const char *kp_kconfig_data(void);
int kp_kconfig_size(void);

#endif
