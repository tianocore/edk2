/** @file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Mtftp4Support.h

Abstract:

  Support routines for MTFTP


**/

#ifndef __EFI_MTFTP4_SUPPORT_H__
#define __EFI_MTFTP4_SUPPORT_H__

//
// The structure representing a range of block numbers, [Start, End].
// It is used to remember the holes in the MTFTP block space. If all
// the holes are filled in, then the download or upload has completed.
//
typedef struct {
  LIST_ENTRY                Link;
  INTN                      Start;
  INTN                      End;
} MTFTP4_BLOCK_RANGE;


EFI_STATUS
Mtftp4InitBlockRange (
  IN LIST_ENTRY             *Head,
  IN UINT16                 Start,
  IN UINT16                 End
  );

INTN
Mtftp4GetNextBlockNum (
  IN LIST_ENTRY             *Head
  );

VOID
Mtftp4SetLastBlockNum (
  IN LIST_ENTRY             *Head,
  IN UINT16                 Last
  );

EFI_STATUS
Mtftp4RemoveBlockNum (
  IN LIST_ENTRY             *Head,
  IN UINT16                 Num
  );

VOID
Mtftp4SetTimeout (
  IN MTFTP4_PROTOCOL        *Instance
  );

EFI_STATUS
Mtftp4SendPacket (
  IN MTFTP4_PROTOCOL        *Instance,
  IN NET_BUF                *Packet
  );

EFI_STATUS
Mtftp4SendRequest (
  IN MTFTP4_PROTOCOL        *Instance
  );

EFI_STATUS
Mtftp4SendError (
  IN MTFTP4_PROTOCOL        *Instance,
  IN UINT16                 ErrCode,
  IN UINT8*                 ErrInfo
  );

EFI_STATUS
Mtftp4Retransmit (
  IN MTFTP4_PROTOCOL        *Instance
  );

VOID
EFIAPI
Mtftp4OnTimerTick (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );
#endif
