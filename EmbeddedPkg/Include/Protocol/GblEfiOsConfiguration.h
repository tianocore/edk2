/** @file

  Copyright (c) 2025, The Android Open Source Project.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  GBL EFI OS Configuration Protocol.
  Lets firmware fix up kernel bootconfig, and choose device-tree components at
  boot time.

  Related docs:
  https://cs.android.com/android/kernel/superproject/+/common-android-mainline:bootable/libbootloader/gbl/docs/gbl_os_configuration_protocol.md
*/

#ifndef __GBL_EFI_OS_CONFIGURATION_PROTOCOL_H__
#define __GBL_EFI_OS_CONFIGURATION_PROTOCOL_H__

#include <Uefi/UefiBaseType.h>

//
// {dda0d135-aa5b-42ff-85ac-e3ad6efb4619}
//
#define GBL_EFI_OS_CONFIGURATION_PROTOCOL_GUID \
  { 0xdda0d135, 0xaa5b, 0x42ff, { 0x85, 0xac, 0xe3, 0xad, 0x6e, 0xfb, 0x46, 0x19 } }

// WARNING: The current API is unstable. While backward compatibility is
// guaranteed for pre-release revisions after 0x00000100, the official
// `1.0` (0x00010000) release may include final breaking changes.
#define GBL_EFI_OS_CONFIGURATION_PROTOCOL_REVISION  0x00000100

typedef struct _GBL_EFI_OS_CONFIGURATION_PROTOCOL GBL_EFI_OS_CONFIGURATION_PROTOCOL;

typedef enum {
  GBL_EFI_DEVICE_TREE_TYPE_DEVICE_TREE,
  GBL_EFI_DEVICE_TREE_TYPE_OVERLAY,
  GBL_EFI_DEVICE_TREE_TYPE_PVM_DA_OVERLAY,
} GBL_EFI_DEVICE_TREE_TYPE;

typedef enum {
  GBL_EFI_DEVICE_TREE_SOURCE_BOOT,
  GBL_EFI_DEVICE_TREE_SOURCE_VENDOR_BOOT,
  GBL_EFI_DEVICE_TREE_SOURCE_DTBO,
  GBL_EFI_DEVICE_TREE_SOURCE_DTB,
} GBL_EFI_DEVICE_TREE_SOURCE;

typedef struct {
  UINT32    Source; // GBL_EFI_DEVICE_TREE_SOURCE
  UINT32    Type;   // GBL_EFI_DEVICE_TREE_TYPE
  UINT32    Id;
  UINT32    Rev;
  UINT32    Custom[4];
} GBL_EFI_DEVICE_TREE_METADATA;

typedef struct {
  GBL_EFI_DEVICE_TREE_METADATA    Metadata;
  CONST VOID                      *DeviceTree; // 8-byte aligned, never NULL
  BOOLEAN                         Selected;
} GBL_EFI_VERIFIED_DEVICE_TREE;

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_FIXUP_BOOTCONFIG)(
  IN GBL_EFI_OS_CONFIGURATION_PROTOCOL *This,
  IN UINTN                             BootConfigSize,
  IN CONST CHAR8                       *BootConfig,
  IN OUT UINTN                         *FixupBufferSize,
  OUT CHAR8                            *Fixup
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_SELECT_DEVICE_TREES)(
  IN GBL_EFI_OS_CONFIGURATION_PROTOCOL *This,
  IN UINTN                             NumDeviceTrees,
  IN OUT GBL_EFI_VERIFIED_DEVICE_TREE  *DeviceTrees
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_SELECT_FIT_CONFIGURATION)(
  IN GBL_EFI_OS_CONFIGURATION_PROTOCOL *This,
  IN UINTN                             FitSize,
  IN CONST UINT8                       *Fit,
  IN UINTN                             MetadataSize,
  IN CONST UINT8                       *Metadata,
  OUT UINTN                            *SelectedConfigurationOffset
  );

struct _GBL_EFI_OS_CONFIGURATION_PROTOCOL {
  UINT64                              Revision;
  GBL_EFI_FIXUP_BOOTCONFIG            FixupBootConfig;
  GBL_EFI_SELECT_DEVICE_TREES         SelectDeviceTrees;
  GBL_EFI_SELECT_FIT_CONFIGURATION    SelectFitConfiguration;
};

extern EFI_GUID  gGblEfiOsConfigurationProtocolGuid;

#endif // __GBL_EFI_OS_CONFIGURATION_PROTOCOL_H__
