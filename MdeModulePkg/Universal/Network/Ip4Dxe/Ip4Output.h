/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Ip4Output.h

Abstract:


**/

#ifndef __EFI_IP4_OUTPUT_H__
#define __EFI_IP4_OUTPUT_H__

VOID
Ip4SysPacketSent (
  IP4_PROTOCOL              *Ip4Instance,
  NET_BUF                   *Packet,
  EFI_STATUS                IoStatus,
  UINT32                    Flag,
  VOID                      *Context
  );

EFI_STATUS
Ip4Output (
  IN IP4_SERVICE            *IpSb,
  IN IP4_PROTOCOL           *IpInstance,    OPTIONAL
  IN NET_BUF                *Data,
  IN IP4_HEAD               *Head,
  IN UINT8                  *Option,
  IN UINT32                 OptLen,
  IN IP4_ADDR               GateWay,
  IN IP4_FRAME_CALLBACK     Callback,
  IN VOID                   *Context
  );

VOID
Ip4CancelPacket (
  IN IP4_INTERFACE          *IpIf,
  IN NET_BUF                *Packet,
  IN EFI_STATUS             IoStatus
  );

extern UINT16  mIp4Id;
#endif
