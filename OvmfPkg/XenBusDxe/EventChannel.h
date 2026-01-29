/** @file
  Event Channel function declaration.

  Copyright (C) 2014, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

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
  IN XENBUS_DEVICE  *Dev,
  IN evtchn_port_t  Port
  );

/*
 * XenBus protocol
 */

/**
  Allocate a port that can be bind from domain DomainId.

  @param This       A pointer to the XENBUS_PROTOCOL.
  @param DomainId   The domain ID that can bind the newly allocated port.
  @param Port       A pointer to a evtchn_port_t that will contain the newly
                    allocated port.

  @retval UINT32    The return value from the hypercall, 0 if success.
**/
UINT32
EFIAPI
XenBusEventChannelAllocate (
  IN  XENBUS_PROTOCOL  *This,
  IN  domid_t          DomainId,
  OUT evtchn_port_t    *Port
  );

/**
  Send an event to the remote end of the channel whose local endpoint is Port.

  @param This       A pointer to the XENBUS_PROTOCOL.
  @param Port       Local port to the event from.

  @retval UINT32    The return value from the hypercall, 0 if success.
**/
UINT32
EFIAPI
XenBusEventChannelNotify (
  IN XENBUS_PROTOCOL  *This,
  IN evtchn_port_t    Port
  );

/**
  Close a local event channel Port.

  @param This       A pointer to the XENBUS_PROTOCOL.
  @param Port       The event channel to close.

  @retval UINT32    The return value from the hypercall, 0 if success.
**/
UINT32
EFIAPI
XenBusEventChannelClose (
  IN XENBUS_PROTOCOL  *This,
  IN evtchn_port_t    Port
  );

#endif
