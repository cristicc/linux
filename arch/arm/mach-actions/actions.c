// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Actions Semi Owl SoCs
 *
 * Copyright (c) 2012 Actions Semi Inc.
 * Copyright (c) 2021 Cristian Ciocaltea <cristian.ciocaltea@gmail.com>
 */

#include <linux/highmem.h>
#include <linux/of_platform.h>

#include <asm/mach/arch.h>
#include <asm/system_info.h>

#define OWL_S500_SERIAL_LOW_PAGE_OFF		0x800
#define OWL_S500_SERIAL_HIGH_PAGE_OFF		0x804

static const char *const owl_s500_dt_compat[] __initconst = {
	"actions,s500",
	NULL,
};

static void __init owl_s500_get_system_serial(void)
{
	char *vddr = kmap_local_page(pfn_to_page(PFN_DOWN(0)));

	memcpy(&system_serial_low, vddr + OWL_S500_SERIAL_LOW_PAGE_OFF,
	       sizeof(system_serial_low));
	memcpy(&system_serial_high, vddr + OWL_S500_SERIAL_HIGH_PAGE_OFF,
	       sizeof(system_serial_high));

	kunmap_local(vddr);
}

static void __init owl_s500_init_early(void)

{
	owl_s500_get_system_serial();
}

DT_MACHINE_START(ACTIONS, "Actions Semi Owl S500 SoC")
	.dt_compat	= owl_s500_dt_compat,
	.init_early	= owl_s500_init_early,
MACHINE_END
