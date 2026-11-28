// SPDX-License-Identifier: GPL-2.0
/*
 * Maple I/O Scheduler
 * Based on Zen and SIO.
 *
 * Copyright (C) 2016 Joe Maples <joe@frap129.org>
 *           (C) 2012 Brandon Berhent <bbedward@gmail.com>
 *           (C) 2012 Miguel Boton <mboton@gmail.com>
 *
 * Maple uses a first come first serve style algorithm with separated read/write
 * handling to allow for read biases. By prioritizing reads, simple tasks should
 * improve in performance. While the display is off, expiration times are
 * stretched and writes are no longer allowed to be starved for long.
 *
 * Ported from https://github.com/xellDart/Maple-IO-Scheduler (commit 2efe369),
 * a hisi6220 adaptation. Display state now comes from the framebuffer notifier.
 * See Documentation/FEATURES.md for the other changes.
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/notifier.h>

enum { ASYNC, SYNC };

/* Tunables */
static const int sync_read_expire = 100;	/* max time before a read sync is submitted. */
static const int sync_write_expire = 350;	/* max time before a write sync is submitted. */
static const int async_read_expire = 200;	/* ditto for read async, these limits are SOFT! */
static const int async_write_expire = 500;	/* ditto for write async, these limits are SOFT! */
static const int fifo_batch = 16;		/* # of sequential requests treated as one by the above parameters. */
static const int writes_starved = 3;		/* max times reads can starve a write */
static const int sleep_latency_multiple = 5;	/* multiple for expire time when the display is off */

/* Upper bounds for sysfs; they keep expire * multiple from overflowing. */
#define MAPLE_MAX_EXPIRE_MS	60000
#define MAPLE_MAX_BATCH		1024
#define MAPLE_MAX_MULTIPLE	100

/* State of the primary display (fb0), shared by all queues. */
static bool maple_display_on = true;

/* Elevator data */
struct maple_data {
	/* Request queues */
	struct list_head fifo_list[2][2];

	/* Attributes */
	unsigned int batched;
	unsigned int starved;

	/* Settings */
	int fifo_expire[2][2];
	int fifo_batch;
	int writes_starved;
	int sleep_latency_multiple;
};

static inline struct maple_data *maple_get_data(struct request_queue *q)
{
	return q->elevator->elevator_data;
}

static void maple_merged_requests(struct request_queue *q, struct request *rq,
				  struct request *next)
{
	/*
	 * If next expires before rq, assign its expire time to rq
	 * and move into next position (next will be deleted) in fifo.
	 */
	if (!list_empty(&rq->queuelist) && !list_empty(&next->queuelist)) {
		if (time_before((unsigned long)next->fifo_time,
				(unsigned long)rq->fifo_time)) {
			list_move(&rq->queuelist, &next->queuelist);
			rq->fifo_time = next->fifo_time;
		}
	}

	/* Delete next request */
	rq_fifo_clear(next);
}

static void maple_add_request(struct request_queue *q, struct request *rq)
{
	struct maple_data *mdata = maple_get_data(q);
	const int sync = rq_is_sync(rq);
	const int dir = rq_data_dir(rq);
	unsigned long expire = mdata->fifo_expire[sync][dir];

	/* Increase expiration while the display is off */
	if (!READ_ONCE(maple_display_on))
		expire *= mdata->sleep_latency_multiple;

	/*
	 * Always queue the request, even with a zero expire time, so that it
	 * can never be lost.
	 */
	rq->fifo_time = jiffies + expire;
	list_add_tail(&rq->queuelist, &mdata->fifo_list[sync][dir]);
}

static struct request *maple_expired_request(struct maple_data *mdata,
					     int sync, int data_dir)
{
	struct list_head *list = &mdata->fifo_list[sync][data_dir];
	struct request *rq;

	if (list_empty(list))
		return NULL;

	/* Retrieve request */
	rq = rq_entry_fifo(list->next);

	/* Request has expired */
	if (time_after_eq(jiffies, (unsigned long)rq->fifo_time))
		return rq;

	return NULL;
}

/* Return whichever request expired first; either may be NULL. */
static struct request *maple_older_request(struct request *async,
					   struct request *sync)
{
	if (!async)
		return sync;
	if (!sync)
		return async;

	if (time_after((unsigned long)sync->fifo_time,
		       (unsigned long)async->fifo_time))
		return async;

	return sync;
}

static struct request *maple_choose_expired_request(struct maple_data *mdata)
{
	struct request *rq;

	/* Reset (non-expired-)batch-counter */
	mdata->batched = 0;

	/* Check expired requests. Reads have priority over writes. */
	rq = maple_older_request(maple_expired_request(mdata, ASYNC, READ),
				 maple_expired_request(mdata, SYNC, READ));
	if (rq)
		return rq;

	return maple_older_request(maple_expired_request(mdata, ASYNC, WRITE),
				   maple_expired_request(mdata, SYNC, WRITE));
}

static struct request *maple_choose_request(struct maple_data *mdata,
					    int data_dir)
{
	struct list_head *sync = mdata->fifo_list[SYNC];
	struct list_head *async = mdata->fifo_list[ASYNC];

	/* Increase (non-expired-)batch-counter */
	mdata->batched++;

	/*
	 * Retrieve request from available fifo list. The preferred data
	 * direction goes first, and asynchronous before synchronous.
	 */
	if (!list_empty(&async[data_dir]))
		return rq_entry_fifo(async[data_dir].next);
	if (!list_empty(&sync[data_dir]))
		return rq_entry_fifo(sync[data_dir].next);

	if (!list_empty(&async[!data_dir]))
		return rq_entry_fifo(async[!data_dir].next);
	if (!list_empty(&sync[!data_dir]))
		return rq_entry_fifo(sync[!data_dir].next);

	return NULL;
}

static void maple_dispatch_request(struct maple_data *mdata,
				   struct request *rq)
{
	/*
	 * Remove the request from the fifo list
	 * and dispatch it.
	 */
	rq_fifo_clear(rq);
	elv_dispatch_add_tail(rq->q, rq);

	if (rq_data_dir(rq)) {
		mdata->starved = 0;
	} else {
		if (!list_empty(&mdata->fifo_list[SYNC][WRITE]) ||
		    !list_empty(&mdata->fifo_list[ASYNC][WRITE]))
			mdata->starved++;
	}
}

static int maple_dispatch_requests(struct request_queue *q, int force)
{
	struct maple_data *mdata = maple_get_data(q);
	struct request *rq = NULL;
	int data_dir = READ;
	int starve_limit;

	/*
	 * Retrieve any expired request after a batch of
	 * sequential requests.
	 */
	if (mdata->batched >= mdata->fifo_batch)
		rq = maple_choose_expired_request(mdata);

	/* Retrieve request */
	if (!rq) {
		/* Treat writes fairly while the display is off */
		starve_limit = READ_ONCE(maple_display_on) ?
			       mdata->writes_starved : 1;
		if (mdata->starved >= starve_limit)
			data_dir = WRITE;

		rq = maple_choose_request(mdata, data_dir);
		if (!rq)
			return 0;
	}

	/* Dispatch request */
	maple_dispatch_request(mdata, rq);

	return 1;
}

/*
 * A merge can move a request onto a list that does not match its own sync flag,
 * so every list head has to be recognised when walking to a neighbour.
 */
static bool maple_is_list_head(struct maple_data *mdata,
			       struct list_head *entry)
{
	return entry == &mdata->fifo_list[SYNC][READ] ||
	       entry == &mdata->fifo_list[SYNC][WRITE] ||
	       entry == &mdata->fifo_list[ASYNC][READ] ||
	       entry == &mdata->fifo_list[ASYNC][WRITE];
}

static struct request *maple_former_request(struct request_queue *q,
					    struct request *rq)
{
	struct maple_data *mdata = maple_get_data(q);

	if (maple_is_list_head(mdata, rq->queuelist.prev))
		return NULL;

	/* Return former request */
	return list_prev_entry(rq, queuelist);
}

static struct request *maple_latter_request(struct request_queue *q,
					    struct request *rq)
{
	struct maple_data *mdata = maple_get_data(q);

	if (maple_is_list_head(mdata, rq->queuelist.next))
		return NULL;

	/* Return latter request */
	return list_next_entry(rq, queuelist);
}

static int maple_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct maple_data *mdata;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	/* Allocate structure */
	mdata = kmalloc_node(sizeof(*mdata), GFP_KERNEL, q->node);
	if (!mdata) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = mdata;

	/* Initialize fifo lists */
	INIT_LIST_HEAD(&mdata->fifo_list[SYNC][READ]);
	INIT_LIST_HEAD(&mdata->fifo_list[SYNC][WRITE]);
	INIT_LIST_HEAD(&mdata->fifo_list[ASYNC][READ]);
	INIT_LIST_HEAD(&mdata->fifo_list[ASYNC][WRITE]);

	/* Initialize data */
	mdata->batched = 0;
	mdata->starved = 0;
	mdata->fifo_expire[SYNC][READ] = msecs_to_jiffies(sync_read_expire);
	mdata->fifo_expire[SYNC][WRITE] = msecs_to_jiffies(sync_write_expire);
	mdata->fifo_expire[ASYNC][READ] = msecs_to_jiffies(async_read_expire);
	mdata->fifo_expire[ASYNC][WRITE] = msecs_to_jiffies(async_write_expire);
	mdata->fifo_batch = fifo_batch;
	mdata->writes_starved = writes_starved;
	mdata->sleep_latency_multiple = sleep_latency_multiple;

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void maple_exit_queue(struct elevator_queue *e)
{
	struct maple_data *mdata = e->elevator_data;

	BUG_ON(!list_empty(&mdata->fifo_list[SYNC][READ]));
	BUG_ON(!list_empty(&mdata->fifo_list[SYNC][WRITE]));
	BUG_ON(!list_empty(&mdata->fifo_list[ASYNC][READ]));
	BUG_ON(!list_empty(&mdata->fifo_list[ASYNC][WRITE]));

	/* Free structure */
	kfree(mdata);
}

/*
 * sysfs code
 */

static ssize_t maple_var_show(int var, char *page)
{
	return scnprintf(page, PAGE_SIZE, "%d\n", var);
}

static ssize_t maple_var_store(int *var, const char *page, size_t count)
{
	int ret = kstrtoint(page, 10, var);

	return ret ? ret : count;
}

#define SHOW_FUNCTION(__FUNC, __VAR, __CONV)				\
static ssize_t __FUNC(struct elevator_queue *e, char *page)		\
{									\
	struct maple_data *mdata = e->elevator_data;			\
	int __data = __VAR;						\
	if (__CONV)							\
		__data = jiffies_to_msecs(__data);			\
	return maple_var_show(__data, (page));				\
}
SHOW_FUNCTION(maple_sync_read_expire_show, mdata->fifo_expire[SYNC][READ], 1);
SHOW_FUNCTION(maple_sync_write_expire_show, mdata->fifo_expire[SYNC][WRITE], 1);
SHOW_FUNCTION(maple_async_read_expire_show, mdata->fifo_expire[ASYNC][READ], 1);
SHOW_FUNCTION(maple_async_write_expire_show, mdata->fifo_expire[ASYNC][WRITE], 1);
SHOW_FUNCTION(maple_fifo_batch_show, mdata->fifo_batch, 0);
SHOW_FUNCTION(maple_writes_starved_show, mdata->writes_starved, 0);
SHOW_FUNCTION(maple_sleep_latency_multiple_show, mdata->sleep_latency_multiple, 0);
#undef SHOW_FUNCTION

#define STORE_FUNCTION(__FUNC, __PTR, MIN, MAX, __CONV)			\
static ssize_t __FUNC(struct elevator_queue *e, const char *page,	\
		      size_t count)					\
{									\
	struct maple_data *mdata = e->elevator_data;			\
	int __data;							\
	ssize_t ret = maple_var_store(&__data, (page), count);		\
									\
	if (ret < 0)							\
		return ret;						\
	__data = clamp_t(int, __data, (MIN), (MAX));			\
	if (__CONV)							\
		*(__PTR) = msecs_to_jiffies(__data);			\
	else								\
		*(__PTR) = __data;					\
	return ret;							\
}
STORE_FUNCTION(maple_sync_read_expire_store, &mdata->fifo_expire[SYNC][READ], 0, MAPLE_MAX_EXPIRE_MS, 1);
STORE_FUNCTION(maple_sync_write_expire_store, &mdata->fifo_expire[SYNC][WRITE], 0, MAPLE_MAX_EXPIRE_MS, 1);
STORE_FUNCTION(maple_async_read_expire_store, &mdata->fifo_expire[ASYNC][READ], 0, MAPLE_MAX_EXPIRE_MS, 1);
STORE_FUNCTION(maple_async_write_expire_store, &mdata->fifo_expire[ASYNC][WRITE], 0, MAPLE_MAX_EXPIRE_MS, 1);
STORE_FUNCTION(maple_fifo_batch_store, &mdata->fifo_batch, 1, MAPLE_MAX_BATCH, 0);
STORE_FUNCTION(maple_writes_starved_store, &mdata->writes_starved, 1, MAPLE_MAX_BATCH, 0);
STORE_FUNCTION(maple_sleep_latency_multiple_store, &mdata->sleep_latency_multiple, 1, MAPLE_MAX_MULTIPLE, 0);
#undef STORE_FUNCTION

#define DD_ATTR(name) \
	__ATTR(name, 0644, maple_##name##_show, maple_##name##_store)

static struct elv_fs_entry maple_attrs[] = {
	DD_ATTR(sync_read_expire),
	DD_ATTR(sync_write_expire),
	DD_ATTR(async_read_expire),
	DD_ATTR(async_write_expire),
	DD_ATTR(fifo_batch),
	DD_ATTR(writes_starved),
	DD_ATTR(sleep_latency_multiple),
	__ATTR_NULL
};

static struct elevator_type iosched_maple = {
	.ops = {
		.elevator_merge_req_fn		= maple_merged_requests,
		.elevator_dispatch_fn		= maple_dispatch_requests,
		.elevator_add_req_fn		= maple_add_request,
		.elevator_former_req_fn		= maple_former_request,
		.elevator_latter_req_fn		= maple_latter_request,
		.elevator_init_fn		= maple_init_queue,
		.elevator_exit_fn		= maple_exit_queue,
	},

	.elevator_attrs = maple_attrs,
	.elevator_name = "maple",
	.elevator_owner = THIS_MODULE,
};

static int maple_fb_notifier_callback(struct notifier_block *nb,
				      unsigned long action, void *data)
{
	struct fb_event *event = data;

	if (action != FB_EVENT_BLANK || !event || !event->data ||
	    !event->info || event->info->node != 0)
		return NOTIFY_DONE;

	WRITE_ONCE(maple_display_on,
		   *(int *)event->data == FB_BLANK_UNBLANK);

	return NOTIFY_OK;
}

static struct notifier_block maple_fb_notifier = {
	.notifier_call = maple_fb_notifier_callback,
};

static int __init maple_init(void)
{
	int ret;

	ret = fb_register_client(&maple_fb_notifier);
	if (ret)
		return ret;

	/* Register elevator */
	ret = elv_register(&iosched_maple);
	if (ret)
		fb_unregister_client(&maple_fb_notifier);

	return ret;
}

static void __exit maple_exit(void)
{
	/* Unregister elevator */
	elv_unregister(&iosched_maple);
	fb_unregister_client(&maple_fb_notifier);
}

module_init(maple_init);
module_exit(maple_exit);

MODULE_AUTHOR("Joe Maples");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Maple I/O Scheduler");
MODULE_VERSION("1.0");
