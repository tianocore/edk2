/** @file
  Event Channel function declaration.

  Copyright (C) 2014, Citrix Ltd.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef __XENBUS_EVENT_CHANNEL_H
#define __XENBUS_EVENT_CHANNEL_H

#include "XenBusDxe.h"

#include <IndustryStandard/Xen/event_channel.h>

/**
  Send an event to the remote end of the channel whose local endpoint is Port.

  @param Dev    A pointer to XENBUS_DEVICE.
  @param Port   The port to notify.

  @return       Return 0 on success, or return the errno code from the hypercall.
**/
UINT32
XenEventChannelNotify (
  IN XENBUS_DEVICE *Dev,
  IN evtchn_port_t Port
  );

#endif
