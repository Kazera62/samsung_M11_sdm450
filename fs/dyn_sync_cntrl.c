// SPDX-License-Identifier: GPL-2.0
/*
 * Dynamic Fsync control for Android kernels.
 *
 * Runtime control:
 *   /sys/kernel/dyn_fsync/Dyn_fsync_active
 *     0 = disabled (normal fsync behavior)
 *     1 = enabled (skip fsync/fdatasync while display is on)
 *
 * Dynamic Fsync is compiled in but disabled by default. When enabled, dirty
 * data is synchronized when the framebuffer blanks, before the display-off
 * transition completes.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include <linux/errno.h>
#include <linux/dyn_sync_cntrl.h>

bool dyn_fsync_active;
bool dyn_fsync_screen_on = true;

static DEFINE_MUTEX(dyn_fsync_mutex);
static struct kobject *dyn_fsync_kobj;
static struct notifier_block dyn_fsync_fb_notifier;

static ssize_t dyn_fsync_active_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n",
			 READ_ONCE(dyn_fsync_active) ? 1 : 0);
}

static ssize_t dyn_fsync_active_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;
	int ret;

	ret = kstrtouint(buf, 10, &value);
	if (ret)
		return ret;

	if (value > 1)
		return -EINVAL;

	WRITE_ONCE(dyn_fsync_active, value != 0);
	pr_info("dynamic_fsync: %s\n",
		value ? "enabled" : "disabled");

	return count;
}

static ssize_t dyn_fsync_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "1.0\n");
}

static struct kobj_attribute dyn_fsync_active_attribute =
	__ATTR(Dyn_fsync_active, 0644,
		dyn_fsync_active_show, dyn_fsync_active_store);

static struct kobj_attribute dyn_fsync_version_attribute =
	__ATTR(Dyn_fsync_version, 0444, dyn_fsync_version_show, NULL);

static struct attribute *dyn_fsync_attrs[] = {
	&dyn_fsync_active_attribute.attr,
	&dyn_fsync_version_attribute.attr,
	NULL,
};

static const struct attribute_group dyn_fsync_attr_group = {
	.attrs = dyn_fsync_attrs,
};

static int dyn_fsync_fb_notifier_callback(struct notifier_block *nb,
					 unsigned long action, void *data)
{
	struct fb_event *event = data;
	int *blank;
	bool was_screen_on;

	if (action != FB_EARLY_EVENT_BLANK || !event || !event->data)
		return NOTIFY_DONE;

	blank = event->data;

	mutex_lock(&dyn_fsync_mutex);
	if (*blank == FB_BLANK_UNBLANK) {
		WRITE_ONCE(dyn_fsync_screen_on, true);
	} else {
		was_screen_on = READ_ONCE(dyn_fsync_screen_on);
		WRITE_ONCE(dyn_fsync_screen_on, false);

		/*
		 * Flush once on the transition to screen-off if Dynamic Fsync
		 * was active. The regular sync syscall and sync_file_range
		 * behavior remains unchanged.
		 */
		if (was_screen_on && READ_ONCE(dyn_fsync_active))
			dyn_fsync_sync_all();
	}
	mutex_unlock(&dyn_fsync_mutex);

	return NOTIFY_OK;
}

static int __init dyn_fsync_init(void)
{
	int ret;

	dyn_fsync_fb_notifier.notifier_call = dyn_fsync_fb_notifier_callback;

	dyn_fsync_kobj = kobject_create_and_add("dyn_fsync", kernel_kobj);
	if (!dyn_fsync_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(dyn_fsync_kobj, &dyn_fsync_attr_group);
	if (ret) {
		pr_err("dynamic_fsync: failed to create sysfs controls: %d\n", ret);
		kobject_put(dyn_fsync_kobj);
		dyn_fsync_kobj = NULL;
		return ret;
	}

	ret = fb_register_client(&dyn_fsync_fb_notifier);
	if (ret) {
		pr_err("dynamic_fsync: failed to register framebuffer notifier: %d\n",
		       ret);
		sysfs_remove_group(dyn_fsync_kobj, &dyn_fsync_attr_group);
		kobject_put(dyn_fsync_kobj);
		dyn_fsync_kobj = NULL;
		return ret;
	}

	pr_info("dynamic_fsync: initialized (runtime default: disabled)\n");
	return 0;
}

static void __exit dyn_fsync_exit(void)
{
	fb_unregister_client(&dyn_fsync_fb_notifier);

	if (dyn_fsync_kobj) {
		sysfs_remove_group(dyn_fsync_kobj, &dyn_fsync_attr_group);
		kobject_put(dyn_fsync_kobj);
		dyn_fsync_kobj = NULL;
	}
}

module_init(dyn_fsync_init);
module_exit(dyn_fsync_exit);

MODULE_AUTHOR("NOCTURNE");
MODULE_DESCRIPTION("Dynamic Fsync control");
MODULE_LICENSE("GPL v2");
