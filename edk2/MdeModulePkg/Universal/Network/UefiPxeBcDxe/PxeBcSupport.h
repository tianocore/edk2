/** @file

Copyright (c) 2007 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PxeBcSupport.h

Abstract:

  Support routines for PxeBc


**/

#ifndef __EFI_PXEBC_SUPPORT_H__
#define __EFI_PXEBC_SUPPORT_H__

EFI_STATUS
GetSmbiosSystemGuidAndSerialNumber (
  IN  EFI_GUID  *SystemGuid,
  OUT CHAR8     **SystemSerialNumber
  );


/**
  GC_NOTO: Add function description

  @param  Event      GC_NOTO: add argument description
  @param  Context    GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
VOID
PxeBcCommonNotify (
  IN EFI_EVENT           Event,
  IN VOID                *Context
  )
;

EFI_STATUS
PxeBcConfigureUdpWriteInstance (
  IN EFI_UDP4_PROTOCOL  *Udp4,
  IN EFI_IPv4_ADDRESS   *StationIp,
  IN EFI_IPv4_ADDRESS   *SubnetMask,
  IN EFI_IPv4_ADDRESS   *Gateway,
  IN OUT UINT16         *SrcPort
  );
VOID
CvtNum (
  IN UINTN  Number,
  IN UINT8  *Buffer,
  IN INTN   Length
  );


/**
  GC_NOTO: Add function description

  @param  Number     GC_NOTO: add argument description
  @param  BufferPtr  GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
UINTN
UtoA10 (
  UINTN Number,
  CHAR8 *BufferPtr
  )
;


/**
  GC_NOTO: Add function description

  @param  BufferPtr  GC_NOTO: add argument description

  @return GC_NOTO: add return values

**/
UINT64
AtoU64 (
  UINT8 *BufferPtr
  )
;


#endif

