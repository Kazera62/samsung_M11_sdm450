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
 * data is synchronized after the framebuffer has blanked, before the blank or
 * release request returns to its caller. Deferral is also suspended while the
 * system is entering suspend.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/suspend.h>
#include <linux/dyn_sync_cntrl.h>

bool dyn_fsync_active = false;
bool dyn_fsync_screen_on = true;
bool dyn_fsync_suspending;

static DEFINE_MUTEX(dyn_fsync_mutex);
static struct kobject *dyn_fsync_kobj;
static struct notifier_block dyn_fsync_fb_notifier;
static struct notifier_block dyn_fsync_reboot_notifier;
static struct notifier_block dyn_fsync_pm_notifier;

/*
 * Record that the primary display is off and flush once on that transition.
 * Sleeps; must not be called from a device suspend callback, where writeback
 * workers are already frozen.
 */
void dyn_fsync_screen_off(void)
{
	bool was_screen_on;

	mutex_lock(&dyn_fsync_mutex);
	was_screen_on = READ_ONCE(dyn_fsync_screen_on);
	WRITE_ONCE(dyn_fsync_screen_on, false);

	/* sync(2) and sync_file_range behavior remains unchanged. */
	if (was_screen_on && READ_ONCE(dyn_fsync_active))
		dyn_fsync_sync_all();
	mutex_unlock(&dyn_fsync_mutex);
}

/*
 * PM_SUSPEND_PREPARE runs before the suspend path's filesystem sync. Stop
 * deferring here so that sync covers everything skipped earlier, including
 * when the display driver powers off without a framebuffer blank event.
 */
static int dyn_fsync_pm_callback(struct notifier_block *nb,
				 unsigned long action, void *data)
{
	switch (action) {
	case PM_SUSPEND_PREPARE:
		WRITE_ONCE(dyn_fsync_suspending, true);
		break;
	case PM_POST_SUSPEND:
		WRITE_ONCE(dyn_fsync_suspending, false);
		break;
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static int dyn_fsync_reboot_callback(struct notifier_block *nb,
				     unsigned long action, void *data)
{
	switch (action) {
	/* SYS_RESTART aliases SYS_DOWN in this kernel's reboot.h. */
	case SYS_RESTART:
	case SYS_HALT:
	case SYS_POWER_OFF:
		break;
	default:
		return NOTIFY_DONE;
	}

	mutex_lock(&dyn_fsync_mutex);
	if (READ_ONCE(dyn_fsync_active)) {
		/* Stop deferring fsync requests and flush before shutdown/reboot. */
		WRITE_ONCE(dyn_fsync_active, false);
		dyn_fsync_sync_all();
	}
	mutex_unlock(&dyn_fsync_mutex);

	return NOTIFY_OK;
}

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
	bool was_active;
	int ret;

	ret = kstrtouint(buf, 10, &value);
	if (ret)
		return ret;

	if (value > 1)
		return -EINVAL;

	mutex_lock(&dyn_fsync_mutex);
	was_active = READ_ONCE(dyn_fsync_active);
	WRITE_ONCE(dyn_fsync_active, value != 0);

	/*
	 * If disabling Dynamic Fsync, synchronously flush data whose fsync()
	 * requests may have been deferred while the feature was active.
	 */
	if (was_active && !value)
		dyn_fsync_sync_all();

	mutex_unlock(&dyn_fsync_mutex);

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

	/*
	 * Use the successful blank event rather than the early event so a failed
	 * panel blank cannot leave the state incorrectly marked as screen-off.
	 * fb0 is the primary display on this device; ignore secondary framebuffers.
	 */
	if (action != FB_EVENT_BLANK || !event || !event->data ||
	    !event->info || event->info->node != 0)
		return NOTIFY_DONE;

	blank = event->data;

	if (*blank == FB_BLANK_UNBLANK) {
		mutex_lock(&dyn_fsync_mutex);
		WRITE_ONCE(dyn_fsync_screen_on, true);
		mutex_unlock(&dyn_fsync_mutex);
	} else {
		dyn_fsync_screen_off();
	}

	return NOTIFY_OK;
}

static int __init dyn_fsync_init(void)
{
	int ret;

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

	dyn_fsync_fb_notifier.notifier_call = dyn_fsync_fb_notifier_callback;
	/* Run last so the flush does not delay other blank handlers. */
	dyn_fsync_fb_notifier.priority = INT_MIN;
	ret = fb_register_client(&dyn_fsync_fb_notifier);
	if (ret) {
		pr_err("dynamic_fsync: failed to register framebuffer notifier: %d\n",
		       ret);
		sysfs_remove_group(dyn_fsync_kobj, &dyn_fsync_attr_group);
		kobject_put(dyn_fsync_kobj);
		dyn_fsync_kobj = NULL;
		return ret;
	}

	dyn_fsync_reboot_notifier.notifier_call = dyn_fsync_reboot_callback;
	ret = register_reboot_notifier(&dyn_fsync_reboot_notifier);
	if (ret) {
		pr_err("dynamic_fsync: failed to register reboot notifier: %d\n",
		       ret);
		fb_unregister_client(&dyn_fsync_fb_notifier);
		sysfs_remove_group(dyn_fsync_kobj, &dyn_fsync_attr_group);
		kobject_put(dyn_fsync_kobj);
		dyn_fsync_kobj = NULL;
		return ret;
	}

	dyn_fsync_pm_notifier.notifier_call = dyn_fsync_pm_callback;
	ret = register_pm_notifier(&dyn_fsync_pm_notifier);
	if (ret) {
		pr_err("dynamic_fsync: failed to register PM notifier: %d\n",
		       ret);
		unregister_reboot_notifier(&dyn_fsync_reboot_notifier);
		fb_unregister_client(&dyn_fsync_fb_notifier);
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
	unregister_pm_notifier(&dyn_fsync_pm_notifier);
	unregister_reboot_notifier(&dyn_fsync_reboot_notifier);
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
