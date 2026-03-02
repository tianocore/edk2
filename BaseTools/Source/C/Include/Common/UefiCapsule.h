/** @file
  Defines for the EFI Capsule functionality.
  @par Revision Reference:
  These definitions are from Uefi Spec.

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

typedef struct {
  EFI_GUID          CapsuleGuid;
  UINT32            HeaderSize;
  UINT32            Flags;
  UINT32            CapsuleImageSize;
} EFI_CAPSULE_HEADER;

#define CAPSULE_FLAGS_PERSIST_ACROSS_RESET          0x00010000
#define CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE         0x00020000
#define CAPSULE_FLAGS_INITIATE_RESET                0x00040000
