// SPDX-License-Identifier: GPL-2.0
/*
 * Temporary devfreq boost for latency-sensitive bandwidth requests.
 *
 * The Qualcomm CPU-to-DDR bus devfreq device is raised to its highest
 * supported bandwidth level for a short period, then returned to the
 * governor's control.
 */

#include <linux/devfreq.h>
#include <linux/devfreq_boost.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>

static DEFINE_MUTEX(devfreq_boost_lock);
static struct devfreq *cpu_bw_devfreq;
static unsigned long cpu_bw_max_freq;

static void devfreq_boost_unboost(struct work_struct *work);
static DECLARE_DELAYED_WORK(devfreq_boost_work, devfreq_boost_unboost);

static void devfreq_boost_unboost(struct work_struct *work)
{
	struct devfreq *df;

	mutex_lock(&devfreq_boost_lock);
	df = cpu_bw_devfreq;
	if (df) {
		mutex_lock(&df->lock);
		df->boost_min_freq = 0;
		update_devfreq(df);
		mutex_unlock(&df->lock);
	}
	mutex_unlock(&devfreq_boost_lock);
}

void devfreq_register_boost_device(struct devfreq *df)
{
	unsigned int i;
	unsigned long max_freq = 0;

	if (!df || !df->profile || !df->profile->freq_table ||
	    !df->profile->max_state)
		return;

	for (i = 0; i < df->profile->max_state; i++)
		max_freq = max(max_freq, df->profile->freq_table[i]);

	if (!max_freq)
		return;

	mutex_lock(&devfreq_boost_lock);
	if (cpu_bw_devfreq && cpu_bw_devfreq != df) {
		pr_warn("devfreq_boost: CPU bandwidth device already registered\n");
		mutex_unlock(&devfreq_boost_lock);
		return;
	}

	mutex_lock(&df->lock);
	df->boost_min_freq = 0;
	mutex_unlock(&df->lock);

	cpu_bw_devfreq = df;
	cpu_bw_max_freq = max_freq;
	mutex_unlock(&devfreq_boost_lock);

	pr_info("devfreq_boost: registered CPU DDR bandwidth device (max %lu)\n",
		max_freq);
}

void devfreq_unregister_boost_device(struct devfreq *df)
{
	mutex_lock(&devfreq_boost_lock);
	if (cpu_bw_devfreq != df) {
		mutex_unlock(&devfreq_boost_lock);
		return;
	}

	cpu_bw_devfreq = NULL;
	cpu_bw_max_freq = 0;
	mutex_unlock(&devfreq_boost_lock);

	cancel_delayed_work_sync(&devfreq_boost_work);

	mutex_lock(&df->lock);
	df->boost_min_freq = 0;
	update_devfreq(df);
	mutex_unlock(&df->lock);
}

void devfreq_boost_kick_max(unsigned int duration_ms)
{
	struct devfreq *df;

	if (!duration_ms)
		return;

	mutex_lock(&devfreq_boost_lock);
	df = cpu_bw_devfreq;
	if (df) {
		mutex_lock(&df->lock);
		df->boost_min_freq = cpu_bw_max_freq;
		update_devfreq(df);
		mutex_unlock(&df->lock);

		mod_delayed_work(system_wq, &devfreq_boost_work,
				 msecs_to_jiffies(duration_ms));
	}
	mutex_unlock(&devfreq_boost_lock);
}
