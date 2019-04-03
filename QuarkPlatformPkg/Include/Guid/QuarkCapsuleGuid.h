/** @file

Capsule format guid for Quark capsule image.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _QUARK_CAPSULE_GUID_H_
#define _QUARK_CAPSULE_GUID_H_

#define QUARK_CAPSULE_GUID \
  { 0xd400d1e4, 0xa314, 0x442b, { 0x89, 0xed, 0xa9, 0x2e, 0x4c, 0x81, 0x97, 0xcb } }

#define SMI_INPUT_UPDATE_CAP 0x27
#define SMI_INPUT_GET_CAP    0x28

#define SMI_CAP_FUNCTION     0xEF

#pragma pack(1)
typedef struct {
   UINT64  Address;
   UINT32  BufferOffset;
   UINT32  Size;
   UINT32  Flags;
   UINT32  Reserved;
} CAPSULE_FRAGMENT;

typedef struct {
  UINTN         CapsuleLocation;  // Top of the capsule that point to structure CAPSULE_FRAGMENT
  UINTN         CapsuleSize;    // Size of the capsule
  EFI_STATUS   Status;      // Returned status
} CAPSULE_INFO_PACKET;

typedef struct {
  UINTN           BlocksCompleted;  // # of blocks processed
  UINTN           TotalBlocks;      // Total # of blocks to be processed
  EFI_STATUS      Status;            // returned status
} UPDATE_STATUS_PACKET;
#pragma pack()

extern EFI_GUID gEfiQuarkCapsuleGuid;

#endif
