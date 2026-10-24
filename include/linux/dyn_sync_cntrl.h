/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_DYN_SYNC_CNTRL_H
#define _LINUX_DYN_SYNC_CNTRL_H

#include <linux/types.h>

extern bool dyn_fsync_active;
extern bool dyn_fsync_screen_on;
void dyn_fsync_sync_all(void);

#endif /* _LINUX_DYN_SYNC_CNTRL_H */
