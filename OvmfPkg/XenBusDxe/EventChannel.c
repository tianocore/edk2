/** @file
  Event Channel function implementation.

  Event channel are use to notify of an event that happend in a shared
  structure for example.

  Copyright (C) 2014, Citrix Ltd.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "EventChannel.h"
#include "XenHypercall.h"

UINT32
XenEventChannelNotify (
  IN XENBUS_DEVICE *Dev,
  IN evtchn_port_t Port
  )
{
  INTN ReturnCode;
  evtchn_send_t Send;

  Send.port = Port;
  ReturnCode = XenHypercallEventChannelOp (Dev, EVTCHNOP_send, &Send);
  return ReturnCode;
}
