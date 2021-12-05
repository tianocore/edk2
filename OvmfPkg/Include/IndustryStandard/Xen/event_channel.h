/******************************************************************************
 * event_channel.h
 *
 * Event channels between domains.
 *
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2003-2004, K A Fraser.
 */

#ifndef __XEN_PUBLIC_EVENT_CHANNEL_H__
#define __XEN_PUBLIC_EVENT_CHANNEL_H__

#include "xen.h"

/*
 * `incontents 150 evtchn Event Channels
 *
 * Event channels are the basic primitive provided by Xen for event
 * notifications. An event is the Xen equivalent of a hardware
 * interrupt. They essentially store one bit of information, the event
 * of interest is signalled by transitioning this bit from 0 to 1.
 *
 * Notifications are received by a guest via an upcall from Xen,
 * indicating when an event arrives (setting the bit). Further
 * notifications are masked until the bit is cleared again (therefore,
 * guests must check the value of the bit after re-enabling event
 * delivery to ensure no missed notifications).
 *
 * Event notifications can be masked by setting a flag; this is
 * equivalent to disabling interrupts and can be used to ensure
 * atomicity of certain operations in the guest kernel.
 *
 * Event channels are represented by the evtchn_* fields in
 * struct shared_info and struct vcpu_info.
 */

/*
 * ` enum neg_errnoval
 * ` HYPERVISOR_event_channel_op(enum event_channel_op cmd, VOID *args)
 * `
 * @cmd  == EVTCHNOP_* (event-channel operation).
 * @args == struct evtchn_* Operation-specific extra arguments (NULL if none).
 */

/* ` enum event_channel_op { // EVTCHNOP_* => struct evtchn_* */
#define EVTCHNOP_close          3
#define EVTCHNOP_send           4
#define EVTCHNOP_alloc_unbound  6
/* ` } */

typedef UINT32 evtchn_port_t;
DEFINE_XEN_GUEST_HANDLE (evtchn_port_t);

/*
 * EVTCHNOP_alloc_unbound: Allocate a port in domain <dom> and mark as
 * accepting interdomain bindings from domain <remote_dom>. A fresh port
 * is allocated in <dom> and returned as <port>.
 * NOTES:
 *  1. If the caller is unprivileged then <dom> must be DOMID_SELF.
 *  2. <rdom> may be DOMID_SELF, allowing loopback connections.
 */
struct evtchn_alloc_unbound {
  /* IN parameters */
  domid_t          dom, remote_dom;
  /* OUT parameters */
  evtchn_port_t    port;
};

typedef struct evtchn_alloc_unbound evtchn_alloc_unbound_t;

/*
 * EVTCHNOP_close: Close a local event channel <port>. If the channel is
 * interdomain then the remote end is placed in the unbound state
 * (EVTCHNSTAT_unbound), awaiting a new connection.
 */
struct evtchn_close {
  /* IN parameters. */
  evtchn_port_t    port;
};

typedef struct evtchn_close evtchn_close_t;

/*
 * EVTCHNOP_send: Send an event to the remote end of the channel whose local
 * endpoint is <port>.
 */
struct evtchn_send {
  /* IN parameters. */
  evtchn_port_t    port;
};

typedef struct evtchn_send evtchn_send_t;

#endif /* __XEN_PUBLIC_EVENT_CHANNEL_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
