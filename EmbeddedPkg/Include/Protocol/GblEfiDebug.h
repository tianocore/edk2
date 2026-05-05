/** @file

  Copyright (c) 2025, The Android Open Source Project.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  GBL EFI Debug Protocol.
  Allows GBL to notify firmware of fatal errors or debug events.

  Related docs:
  https://cs.android.com/android/kernel/superproject/+/common-android-mainline:bootable/libbootloader/gbl/docs/gbl_efi_debug_protocol.md
*/

#ifndef __GBL_EFI_DEBUG_PROTOCOL_H__
#define __GBL_EFI_DEBUG_PROTOCOL_H__

#include <Uefi/UefiBaseType.h>

//
// {98ca3da1-c1ac-4402-9c16-7558d3ed5705}
//
#define GBL_EFI_DEBUG_PROTOCOL_GUID \
  { 0x98ca3da1, 0xc1ac, 0x4402, { 0x9c, 0x16, 0x75, 0x58, 0xd3, 0xed, 0x57, 0x05 } }

// WARNING: The current API is unstable. While backward compatibility is
// guaranteed for pre-release revisions after 0x00000100, the official
// `1.0` (0x00010000) release may include final breaking changes.
#define GBL_EFI_DEBUG_PROTOCOL_REVISION  0x00000100

typedef struct _GBL_EFI_DEBUG_PROTOCOL GBL_EFI_DEBUG_PROTOCOL;

typedef enum {
  GBL_EFI_DEBUG_ERROR_TAG_ASSERTION_ERROR,
  GBL_EFI_DEBUG_ERROR_TAG_PARTITION,
  GBL_EFI_DEBUG_ERROR_TAG_LOAD_IMAGE,
  GBL_EFI_DEBUG_ERROR_TAG_BOOT_ERROR
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

#endif // __GBL_EFI_DEBUG_PROTOCOL_H__
