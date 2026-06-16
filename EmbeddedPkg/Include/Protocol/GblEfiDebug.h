/** @file
  GBL EFI Debug Protocol.

  Allows GBL to notify firmware of fatal errors or debug events.

  Related docs:
  https://android.googlesource.com/platform/bootable/libbootloader/+/refs/heads/gbl-mainline/gbl/docs/gbl_efi_debug_protocol.md

  Copyright (c) 2025, The Android Open Source Project.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi/UefiBaseType.h>

//
// {98ca3da1-c1ac-4402-9c16-7558d3ed5705}
//
#define GBL_EFI_DEBUG_PROTOCOL_GUID \
  { 0x98ca3da1, 0xc1ac, 0x4402, { 0x9c, 0x16, 0x75, 0x58, 0xd3, 0xed, 0x57, 0x05 } }

#define GBL_EFI_DEBUG_PROTOCOL_REVISION  0x00010000

typedef struct _GBL_EFI_DEBUG_PROTOCOL GBL_EFI_DEBUG_PROTOCOL;

typedef enum {
  GblEfiDebugErrorTagAssertionError,
  GblEfiDebugErrorTagPartition,
  GblEfiDebugErrorTagLoadImage,
  GblEfiDebugErrorTagBootError
} GBL_EFI_DEBUG_ERROR_TAG;

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_DEBUG_FATAL_ERROR)(
  IN GBL_EFI_DEBUG_PROTOCOL  *This,
  IN CONST VOID              *FramePtr,
  IN UINT64                  Tag // GBL_EFI_DEBUG_ERROR_TAG
  );

struct _GBL_EFI_DEBUG_PROTOCOL {
  UINT64                       Revision;
  GBL_EFI_DEBUG_FATAL_ERROR    FatalError;
};

extern EFI_GUID  gGblEfiDebugProtocolGuid;
