/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_DEVFREQ_BOOST_H_
#define _LINUX_DEVFREQ_BOOST_H_

#include <linux/devfreq.h>

#ifdef CONFIG_DEVFREQ_BOOST
void devfreq_register_boost_device(struct devfreq *df);
void devfreq_unregister_boost_device(struct devfreq *df);
void devfreq_boost_kick_max(unsigned int duration_ms);
#else
static inline void devfreq_register_boost_device(struct devfreq *df) { }
static inline void devfreq_unregister_boost_device(struct devfreq *df) { }
static inline void devfreq_boost_kick_max(unsigned int duration_ms) { }
#endif

#endif /* _LINUX_DEVFREQ_BOOST_H_ */
