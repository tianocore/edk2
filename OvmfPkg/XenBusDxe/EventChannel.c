/** @file
  Event Channel function implementation.

  Event channel are use to notify of an event that happened in a shared
  structure for example.

  Copyright (C) 2014, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "EventChannel.h"

#include <Library/XenHypercallLib.h>

UINT32
XenEventChannelNotify (
  IN XENBUS_DEVICE  *Dev,
  IN evtchn_port_t  Port
  )
{
  INTN           ReturnCode;
  evtchn_send_t  Send;

  Send.port  = Port;
  ReturnCode = XenHypercallEventChannelOp (EVTCHNOP_send, &Send);
  return (UINT32)ReturnCode;
}

UINT32
EFIAPI
XenBusEventChannelAllocate (
  IN  XENBUS_PROTOCOL  *This,
  IN  domid_t          DomainId,
  OUT evtchn_port_t    *Port
  )
{
  evtchn_alloc_unbound_t  Parameter;
  UINT32                  ReturnCode;

  Parameter.dom        = DOMID_SELF;
  Parameter.remote_dom = DomainId;
  ReturnCode           = (UINT32)XenHypercallEventChannelOp (
                                   EVTCHNOP_alloc_unbound,
                                   &Parameter
                                   );
  if (ReturnCode != 0) {
    DEBUG ((DEBUG_ERROR, "ERROR: alloc_unbound failed with rc=%d", ReturnCode));
    return ReturnCode;
  }

  *Port = Parameter.port;
  return ReturnCode;
}

UINT32
EFIAPI
XenBusEventChannelNotify (
  IN XENBUS_PROTOCOL  *This,
  IN evtchn_port_t    Port
  )
{
  XENBUS_PRIVATE_DATA  *Private;

  Private = XENBUS_PRIVATE_DATA_FROM_THIS (This);
  return XenEventChannelNotify (Private->Dev, Port);
}

UINT32
EFIAPI
XenBusEventChannelClose (
  IN XENBUS_PROTOCOL  *This,
  IN evtchn_port_t    Port
  )
{
  evtchn_close_t  Close;

  Close.port = Port;
  return (UINT32)XenHypercallEventChannelOp (EVTCHNOP_close, &Close);
}
