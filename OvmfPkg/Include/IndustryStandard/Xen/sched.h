/******************************************************************************
 * sched.h
 *
 * Scheduler state interactions
 *
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2005, Keir Fraser <keir@xensource.com>
 */

#ifndef __XEN_PUBLIC_SCHED_H__
#define __XEN_PUBLIC_SCHED_H__

#include "event_channel.h"

/*
 * Halt execution of this domain (all VCPUs) and notify the system controller.
 * @arg == pointer to sched_shutdown_t structure.
 *
 * If the sched_shutdown_t reason is SHUTDOWN_suspend then
 * x86 PV guests must also set RDX (EDX for 32-bit guests) to the MFN
 * of the guest's start info page.  RDX/EDX is the third hypercall
 * argument.
 *
 * In addition, which reason is SHUTDOWN_suspend this hypercall
 * returns 1 if suspend was cancelled or the domain was merely
 * checkpointed, and 0 if it is resuming in a new domain.
 */
#define XEN_SCHEDOP_SHUTDOWN  2

struct _XEN_SCHED_SHUTDOWN {
  UINT32    Reason; /* SHUTDOWN_* => enum sched_shutdown_reason */
};

typedef struct _XEN_SCHED_SHUTDOWN XEN_SCHED_SHUTDOWN;
DEFINE_XEN_GUEST_HANDLE (XEN_SCHED_SHUTDOWN);

/*
 * Reason codes for SCHEDOP_shutdown. These may be interpreted by control
 * software to determine the appropriate action. For the most part, Xen does
 * not care about the shutdown code.
 */
/* ` enum sched_shutdown_reason { */
#define XEN_SHED_SHUTDOWN_POWEROFF  0  /* Domain exited normally. Clean up and kill. */
#define XEN_SHED_SHUTDOWN_REBOOT    1  /* Clean up, kill, and then restart.          */
#define XEN_SHED_SHUTDOWN_SUSPEND   2  /* Clean up, save suspend info, kill.         */
#define XEN_SHED_SHUTDOWN_CRASH     3  /* Tell controller we've crashed.             */
#define XEN_SHED_SHUTDOWN_WATCHDOG  4  /* Restart because watchdog time expired.     */

#endif /* __XEN_PUBLIC_SCHED_H__ */
