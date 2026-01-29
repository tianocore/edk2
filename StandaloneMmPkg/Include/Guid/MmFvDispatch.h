/** @file
  GUIDs for MM Event.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MM_FV_DISPATCH_H__
#define __MM_FV_DISPATCH_H__

#define MM_FV_DISPATCH_GUID \
  { 0xb65694cc, 0x9e3, 0x4c3b, { 0xb5, 0xcd, 0x5, 0xf4, 0x4d, 0x3c, 0xdb, 0xff }}

extern EFI_GUID  gMmFvDispatchGuid;

#pragma pack(1)

typedef struct {
  EFI_PHYSICAL_ADDRESS    Address;
  UINT64                  Size;
} EFI_MM_COMMUNICATE_FV_DISPATCH_DATA;

typedef struct {
  EFI_GUID                               HeaderGuid;
  UINTN                                  MessageLength;
  EFI_MM_COMMUNICATE_FV_DISPATCH_DATA    Data;
} EFI_MM_COMMUNICATE_FV_DISPATCH;
#pragma pack()

#endif
