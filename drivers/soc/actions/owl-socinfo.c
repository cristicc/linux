// SPDX-License-Identifier: GPL-2.0+
/*
 * Actions Semi Owl SoC information driver
 *
 * Copyright (c) 2021 Cristian Ciocaltea <cristian.ciocaltea@gmail.com>
 */

#include <linux/highmem.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/random.h>
#include <linux/sys_soc.h>

#include <asm/system_info.h>

struct owl_soc_info {
	char *name;
	int (*get_system_serial)(struct device *dev);
};

static int __init owl_read_sn_from_mem(struct device *dev)
{
	int paddrs[2] = {0};
	char *vaddr;
	int ret;

	ret = device_property_read_u32_array(dev,
					     "actions,serial-number-addrs",
					     paddrs, 2);
	if (ret) {
		dev_err(dev, "failed to read SoC S/N addresses: %d\n", ret);
		return ret;
	}

	if (PHYS_PFN(paddrs[0]) != PHYS_PFN(paddrs[1])) {
		dev_err(dev, "invalid SoC S/N addresses\n");
		return -EINVAL;
	}

	vaddr = kmap_local_pfn(PHYS_PFN(paddrs[0]));

	memcpy(&system_serial_low, vaddr + (paddrs[0] & (PAGE_SIZE - 1)),
	       sizeof(system_serial_low));
	memcpy(&system_serial_high, vaddr + (paddrs[1] & (PAGE_SIZE - 1)),
	       sizeof(system_serial_high));

	kunmap_local(vaddr);

	return 0;
}

static int owl_socinfo_probe(struct platform_device *pdev)
{
	const struct owl_soc_info *soc_info;
	struct soc_device_attribute *soc_dev_attr;
	struct soc_device *soc_dev;
	int ret;

	soc_info = of_device_get_match_data(&pdev->dev);
	if (!soc_info)
		return -ENODEV;

	soc_dev_attr = devm_kzalloc(&pdev->dev, sizeof(*soc_dev_attr),
				    GFP_KERNEL);
	if (!soc_dev_attr)
		return -ENOMEM;

	ret = of_property_read_string(of_root, "model", &soc_dev_attr->machine);
	if (ret)
		return ret;

	soc_dev_attr->family = "Actions Semi Owl";
	soc_dev_attr->soc_id = soc_info->name;

	if (soc_info->get_system_serial) {
		ret = soc_info->get_system_serial(&pdev->dev);
		if (!ret) {
			soc_dev_attr->serial_number = devm_kasprintf(&pdev->dev,
						GFP_KERNEL, "%08x%08x",
						system_serial_high,
						system_serial_low);
			/* Feed the SoC unique data into entropy pool. */
			add_device_randomness(soc_dev_attr->serial_number, 16);
		}
	}

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev))
		return dev_err_probe(&pdev->dev, PTR_ERR(soc_dev),
				     "failed to register soc device");

	dev_info(soc_device_to_device(soc_dev),
		 "SoC: %s %s\n",
		 soc_dev_attr->family, soc_dev_attr->soc_id);

	return 0;
}

static const struct owl_soc_info s500_soc_info = {
	.name = "S500",
	.get_system_serial = owl_read_sn_from_mem,
};

static const struct owl_soc_info s700_soc_info = {
	.name = "S700",
	/* FIXME: provide get_system_serial */
};

static const struct owl_soc_info s900_soc_info = {
	.name = "S900",
	/* FIXME: provide get_system_serial */
};

static const struct of_device_id owl_soc_of_match[] = {
	{ .compatible = "actions,s500-soc", .data = &s500_soc_info, },
	{ .compatible = "actions,s700-soc", .data = &s700_soc_info, },
	{ .compatible = "actions,s900-soc", .data = &s900_soc_info, },
	{ }
};

static struct platform_driver owl_socinfo_platform_driver = {
	.probe = owl_socinfo_probe,
	.driver = {
		.name = "owl-socinfo",
		.of_match_table = owl_soc_of_match,
	},
};

static int __init owl_socinfo_init(void)
{
	return platform_driver_register(&owl_socinfo_platform_driver);
}
subsys_initcall(owl_socinfo_init);
