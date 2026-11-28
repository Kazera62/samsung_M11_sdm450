// SPDX-License-Identifier: GPL-2.0
/*
 * KCAL: sysfs color control on top of the MDSS post-processing blocks.
 *
 * Copyright (c) 2013, LGE Inc. All rights reserved
 * Copyright (c) 2014 savoca <adeddo27@gmail.com>
 * Copyright (c) 2014 Paul Reioux <reioux@gmail.com>
 * Copyright (c) 2016 Pal Zoltan Illes <palilles@gmail.com>
 *
 * Based on https://github.com/savoca/kcal and the Linux 4.4 variant in
 * https://github.com/PrimoDev23/kernel_xiaomi_whyred, reworked for the
 * MDP v1.7 post-processing interface of this 4.9 tree.
 *
 * Sysfs: /sys/devices/platform/kcal_ctrl.0/
 *
 * Nothing is programmed until userspace writes a value, so the display keeps
 * its stock color configuration by default. Writes only mark the hardware
 * state dirty; the MDSS driver applies it with the next frame.
 */

#include <linux/err.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "mdss_fb.h"
#include "mdss_mdp.h"

#define KCAL_PCC_MAX	256
#define KCAL_PCC_SCALE	0x80	/* 256 * 0x80 = 0x8000, unity gain */
#define KCAL_MIN_DEF	35
#define KCAL_PA_DEF	255

#define KCAL_BLOCK_PCC	BIT(0)
#define KCAL_BLOCK_PA	BIT(1)

enum kcal_pa_field {
	KCAL_SAT,
	KCAL_HUE,
	KCAL_VAL,
	KCAL_CONT,
	KCAL_PA_FIELDS,
};

static const u32 kcal_pa_flags[KCAL_PA_FIELDS] = {
	[KCAL_SAT]  = MDP_PP_PA_SAT_ENABLE | MDP_PP_PA_SAT_MASK,
	[KCAL_HUE]  = MDP_PP_PA_HUE_ENABLE | MDP_PP_PA_HUE_MASK,
	[KCAL_VAL]  = MDP_PP_PA_VAL_ENABLE | MDP_PP_PA_VAL_MASK,
	[KCAL_CONT] = MDP_PP_PA_CONT_ENABLE | MDP_PP_PA_CONT_MASK,
};

struct kcal_data {
	int red;
	int green;
	int blue;
	int minimum;
	int enable;
	int pa[KCAL_PA_FIELDS];
	/* PA fields userspace has set; only these are enabled in hardware. */
	unsigned long pa_used;
	/* Blocks userspace has configured through KCAL. */
	unsigned int used;
	/* Blocks waiting for the display to become available. */
	unsigned int pending;
};

static struct kcal_data kcal = {
	.red = KCAL_PCC_MAX,
	.green = KCAL_PCC_MAX,
	.blue = KCAL_PCC_MAX,
	.minimum = KCAL_MIN_DEF,
	.enable = 1,
	.pa = { KCAL_PA_DEF, 0, KCAL_PA_DEF, KCAL_PA_DEF },
};

/* Lock order: fb_info lock (when taken), then kcal_lock. */
static DEFINE_MUTEX(kcal_lock);
static struct platform_device *kcal_pdev;

static struct fb_info *kcal_primary_fb(void)
{
	struct fb_info *info = registered_fb[0];

	if (!info || strncmp(info->fix.id, "mdssfb", 6))
		return NULL;

	return info;
}

static int kcal_program_pcc(struct msm_fb_data_type *mfd)
{
	struct mdp_pcc_data_v1_7 payload;
	struct mdp_pcc_cfg_data config;
	mm_segment_t old_fs;
	u32 copyback = 0;
	int ret;

	memset(&payload, 0, sizeof(payload));
	payload.r.r = kcal.red * KCAL_PCC_SCALE;
	payload.g.g = kcal.green * KCAL_PCC_SCALE;
	payload.b.b = kcal.blue * KCAL_PCC_SCALE;

	memset(&config, 0, sizeof(config));
	config.version = mdp_pcc_v1_7;
	config.block = MDP_LOGICAL_BLOCK_DISP_0;
	config.ops = MDP_PP_OPS_WRITE |
		(kcal.enable ? MDP_PP_OPS_ENABLE : MDP_PP_OPS_DISABLE);
	config.cfg_payload = &payload;

	/* The PP cache layer reads the payload with copy_from_user(). */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	ret = mdss_mdp_pcc_config(mfd, &config, &copyback);
	set_fs(old_fs);

	return ret;
}

static int kcal_program_pa(struct msm_fb_data_type *mfd)
{
	struct mdp_pa_data_v1_7 payload;
	struct mdp_pa_v2_cfg_data config;
	mm_segment_t old_fs;
	u32 copyback = 0;
	u32 flags;
	int i, ret;

	flags = MDP_PP_OPS_WRITE |
		(kcal.enable ? MDP_PP_OPS_ENABLE : MDP_PP_OPS_DISABLE);
	for_each_set_bit(i, &kcal.pa_used, KCAL_PA_FIELDS)
		flags |= kcal_pa_flags[i];

	memset(&payload, 0, sizeof(payload));
	payload.mode = flags;
	payload.global_sat_adj = kcal.pa[KCAL_SAT];
	payload.global_hue_adj = kcal.pa[KCAL_HUE];
	payload.global_val_adj = kcal.pa[KCAL_VAL];
	payload.global_cont_adj = kcal.pa[KCAL_CONT];

	memset(&config, 0, sizeof(config));
	config.version = mdp_pa_v1_7;
	config.block = MDP_LOGICAL_BLOCK_DISP_0;
	config.flags = flags;
	config.cfg_payload = &payload;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	ret = mdss_mdp_pa_v2_config(mfd, &config, &copyback);
	set_fs(old_fs);

	return ret;
}

/*
 * Program the requested blocks, or remember them if the display pipeline is not
 * set up yet. Called with kcal_lock held.
 */
static int kcal_apply(struct fb_info *info, unsigned int blocks)
{
	struct msm_fb_data_type *mfd = info ? info->par : NULL;
	int ret;

	if (!blocks)
		return 0;

	if (!mfd || !mdss_mdp_mfd_valid_dspp(mfd)) {
		kcal.pending |= blocks;
		return 0;
	}

	if (blocks & KCAL_BLOCK_PCC) {
		ret = kcal_program_pcc(mfd);
		if (ret)
			return ret;
		kcal.pending &= ~KCAL_BLOCK_PCC;
	}

	if (blocks & KCAL_BLOCK_PA) {
		ret = kcal_program_pa(mfd);
		if (ret)
			return ret;
		kcal.pending &= ~KCAL_BLOCK_PA;
	}

	return 0;
}

/* Take the locks needed by a sysfs write. */
static struct fb_info *kcal_begin(void)
{
	struct fb_info *info = kcal_primary_fb();

	/* The fb_info lock keeps blank and release away while programming. */
	if (info && !lock_fb_info(info))
		info = NULL;
	mutex_lock(&kcal_lock);

	return info;
}

static void kcal_end(struct fb_info *info)
{
	mutex_unlock(&kcal_lock);
	if (info)
		unlock_fb_info(info);
}

static ssize_t kcal_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	ssize_t len;

	mutex_lock(&kcal_lock);
	len = scnprintf(buf, PAGE_SIZE, "%d %d %d\n",
			kcal.red, kcal.green, kcal.blue);
	mutex_unlock(&kcal_lock);

	return len;
}

static ssize_t kcal_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct kcal_data old;
	struct fb_info *info;
	int r, g, b, ret = 0;

	if (sscanf(buf, "%d %d %d", &r, &g, &b) != 3 ||
	    r < 0 || r > KCAL_PCC_MAX || g < 0 || g > KCAL_PCC_MAX ||
	    b < 0 || b > KCAL_PCC_MAX)
		return -EINVAL;

	info = kcal_begin();
	old = kcal;
	kcal.red = max(r, kcal.minimum);
	kcal.green = max(g, kcal.minimum);
	kcal.blue = max(b, kcal.minimum);
	kcal.used |= KCAL_BLOCK_PCC;
	if (kcal.enable)
		ret = kcal_apply(info, KCAL_BLOCK_PCC);
	if (ret)
		kcal = old;
	kcal_end(info);

	return ret ? ret : count;
}

static ssize_t kcal_min_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", READ_ONCE(kcal.minimum));
}

static ssize_t kcal_min_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct kcal_data old;
	struct fb_info *info;
	int value, ret;

	ret = kstrtoint(buf, 10, &value);
	if (ret)
		return ret;
	if (value < 0 || value > KCAL_PCC_MAX)
		return -EINVAL;

	info = kcal_begin();
	old = kcal;
	kcal.minimum = value;
	kcal.red = max(kcal.red, value);
	kcal.green = max(kcal.green, value);
	kcal.blue = max(kcal.blue, value);
	if (kcal.enable)
		ret = kcal_apply(info, kcal.used & KCAL_BLOCK_PCC);
	if (ret)
		kcal = old;
	kcal_end(info);

	return ret ? ret : count;
}

static ssize_t kcal_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", READ_ONCE(kcal.enable));
}

static ssize_t kcal_enable_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct kcal_data old;
	struct fb_info *info;
	int value, ret;

	ret = kstrtoint(buf, 10, &value);
	if (ret)
		return ret;
	if (value != 0 && value != 1)
		return -EINVAL;

	info = kcal_begin();
	old = kcal;
	if (kcal.enable != value) {
		kcal.enable = value;
		ret = kcal_apply(info, kcal.used);
		if (ret)
			kcal = old;
	}
	kcal_end(info);

	return ret ? ret : count;
}

static ssize_t kcal_pa_show(char *buf, enum kcal_pa_field field)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", READ_ONCE(kcal.pa[field]));
}

static ssize_t kcal_pa_store(const char *buf, size_t count,
			     enum kcal_pa_field field, int value)
{
	struct kcal_data old;
	struct fb_info *info;
	int ret = 0;

	info = kcal_begin();
	old = kcal;
	kcal.pa[field] = value;
	__set_bit(field, &kcal.pa_used);
	kcal.used |= KCAL_BLOCK_PA;
	if (kcal.enable)
		ret = kcal_apply(info, KCAL_BLOCK_PA);
	if (ret)
		kcal = old;
	kcal_end(info);

	return ret ? ret : count;
}

/* Accept values in [min, max], plus one extra value when extra >= 0. */
#define KCAL_PA_ATTR(_name, _field, _min, _max, _extra)			\
static ssize_t _name##_show(struct device *dev,				\
			    struct device_attribute *attr, char *buf)	\
{									\
	return kcal_pa_show(buf, _field);				\
}									\
									\
static ssize_t _name##_store(struct device *dev,			\
			     struct device_attribute *attr,		\
			     const char *buf, size_t count)		\
{									\
	int value;							\
	int ret = kstrtoint(buf, 10, &value);				\
									\
	if (ret)							\
		return ret;						\
	if ((value < (_min) || value > (_max)) && value != (_extra))	\
		return -EINVAL;						\
									\
	return kcal_pa_store(buf, count, _field, value);		\
}									\
static DEVICE_ATTR_RW(_name)

static DEVICE_ATTR_RW(kcal);
static DEVICE_ATTR_RW(kcal_min);
static DEVICE_ATTR_RW(kcal_enable);
KCAL_PA_ATTR(kcal_sat, KCAL_SAT, 224, 383, 128);
KCAL_PA_ATTR(kcal_hue, KCAL_HUE, 0, 1536, 0);
KCAL_PA_ATTR(kcal_val, KCAL_VAL, 128, 383, 128);
KCAL_PA_ATTR(kcal_cont, KCAL_CONT, 128, 383, 128);

static struct attribute *kcal_attrs[] = {
	&dev_attr_kcal.attr,
	&dev_attr_kcal_min.attr,
	&dev_attr_kcal_enable.attr,
	&dev_attr_kcal_sat.attr,
	&dev_attr_kcal_hue.attr,
	&dev_attr_kcal_val.attr,
	&dev_attr_kcal_cont.attr,
	NULL,
};

static const struct attribute_group kcal_attr_group = {
	.attrs = kcal_attrs,
};

/* Apply writes that arrived before the display pipeline was ready. */
static int kcal_fb_notifier_callback(struct notifier_block *nb,
				     unsigned long action, void *data)
{
	struct fb_event *event = data;
	int ret = 0;

	if (action != FB_EVENT_BLANK || !event || !event->data ||
	    !event->info || event->info != kcal_primary_fb() ||
	    *(int *)event->data != FB_BLANK_UNBLANK)
		return NOTIFY_DONE;

	mutex_lock(&kcal_lock);
	if (kcal.pending)
		ret = kcal_apply(event->info, kcal.pending);
	mutex_unlock(&kcal_lock);

	if (ret)
		pr_err("kcal: failed to apply pending settings: %d\n", ret);

	return NOTIFY_OK;
}

static struct notifier_block kcal_fb_notifier = {
	.notifier_call = kcal_fb_notifier_callback,
};

static int __init kcal_ctrl_init(void)
{
	int ret;

	kcal_pdev = platform_device_register_simple("kcal_ctrl", 0, NULL, 0);
	if (IS_ERR(kcal_pdev))
		return PTR_ERR(kcal_pdev);

	ret = fb_register_client(&kcal_fb_notifier);
	if (ret)
		goto err_device;

	ret = sysfs_create_group(&kcal_pdev->dev.kobj, &kcal_attr_group);
	if (ret)
		goto err_notifier;

	return 0;

err_notifier:
	fb_unregister_client(&kcal_fb_notifier);
err_device:
	platform_device_unregister(kcal_pdev);
	return ret;
}
late_initcall(kcal_ctrl_init);
