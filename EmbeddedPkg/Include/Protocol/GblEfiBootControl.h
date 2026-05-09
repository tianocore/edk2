/** @file

  Copyright (c) 2025, The Android Open Source Project.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  GBL EFI Boot Control Protocol.
  Delegates boot target manipulation logic to firmware.

  Related docs:
  https://cs.android.com/android/kernel/superproject/+/common-android-mainline:bootable/libbootloader/gbl/docs/gbl_efi_boot_control_protocol.md
*/

#ifndef __GBL_EFI_BOOT_CONTROL_PROTOCOL_H__
#define __GBL_EFI_BOOT_CONTROL_PROTOCOL_H__

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

//
// {d382db1b-9ac2-11f0-84c7-047bcba96019}
//
#define GBL_EFI_BOOT_CONTROL_PROTOCOL_GUID \
  { 0xd382db1b, 0x9ac2, 0x11f0, { 0x84, 0xc7, 0x04, 0x7b, 0xcb, 0xa9, 0x60, 0x19 } }

// WARNING: The current API is unstable. While backward compatibility is
// guaranteed for pre-release revisions after 0x00000100, the official
// `1.0` (0x00010000) release may include final breaking changes.
#define GBL_EFI_BOOT_CONTROL_PROTOCOL_REVISION  0x00000100

typedef struct _GBL_EFI_BOOT_CONTROL_PROTOCOL GBL_EFI_BOOT_CONTROL_PROTOCOL;

typedef enum {
  GBL_EFI_UNBOOTABLE_REASON_UNKNOWN_REASON,
  GBL_EFI_UNBOOTABLE_REASON_NO_MORE_TRIES,
  GBL_EFI_UNBOOTABLE_REASON_SYSTEM_UPDATE,
  GBL_EFI_UNBOOTABLE_REASON_USER_REQUESTED,
  GBL_EFI_UNBOOTABLE_REASON_VERIFICATION_FAILURE
} GBL_EFI_UNBOOTABLE_REASON;

typedef enum {
  GBL_EFI_ONE_SHOT_BOOT_MODE_NONE,
  GBL_EFI_ONE_SHOT_BOOT_MODE_BOOTLOADER,
  GBL_EFI_ONE_SHOT_BOOT_MODE_RECOVERY
} GBL_EFI_ONE_SHOT_BOOT_MODE;

typedef struct {
  UINT32    Suffix;           // One UTF-8 encoded single character.
  UINT8     UnbootableReason; // GBL_EFI_UNBOOTABLE_REASON
  UINT8     Priority;
  UINT8     RemainingTries;
  UINT8     Successful;
} GBL_EFI_SLOT_INFO;

typedef struct {
  UINTN               KernelSize;
  PHYSICAL_ADDRESS    Kernel;
  UINTN               RamdiskSize;
  PHYSICAL_ADDRESS    Ramdisk;
  UINTN               DeviceTreeSize;
  PHYSICAL_ADDRESS    DeviceTree;
  UINT64              Reserved[8];
} GBL_EFI_LOADED_OS;

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_BOOT_CONTROL_GET_SLOT_COUNT)(
  IN GBL_EFI_BOOT_CONTROL_PROTOCOL *This,
  OUT UINT8                        *SlotCount
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_BOOT_CONTROL_GET_SLOT_INFO)(
  IN GBL_EFI_BOOT_CONTROL_PROTOCOL *This,
  IN UINT8                         Index,
  OUT GBL_EFI_SLOT_INFO            *Info
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_BOOT_CONTROL_GET_CURRENT_SLOT)(
  IN GBL_EFI_BOOT_CONTROL_PROTOCOL *This,
  OUT GBL_EFI_SLOT_INFO            *Info
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_BOOT_CONTROL_SET_ACTIVE_SLOT)(
  IN GBL_EFI_BOOT_CONTROL_PROTOCOL *This,
  IN UINT8                         Index
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_BOOT_CONTROL_GET_ONE_SHOT_BOOT_MODE)(
  IN GBL_EFI_BOOT_CONTROL_PROTOCOL *This,
  OUT UINT32                       *Mode // GBL_EFI_ONE_SHOT_BOOT_MODE
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_BOOT_CONTROL_HANDLE_LOADED_OS)(
  IN GBL_EFI_BOOT_CONTROL_PROTOCOL *This,
  IN CONST GBL_EFI_LOADED_OS       *Os
  );

struct _GBL_EFI_BOOT_CONTROL_PROTOCOL {
  UINT64                                         Revision;
  GBL_EFI_BOOT_CONTROL_GET_SLOT_COUNT            GetSlotCount;
  GBL_EFI_BOOT_CONTROL_GET_SLOT_INFO             GetSlotInfo;
  GBL_EFI_BOOT_CONTROL_GET_CURRENT_SLOT          GetCurrentSlot;
  GBL_EFI_BOOT_CONTROL_SET_ACTIVE_SLOT           SetActiveSlot;
  GBL_EFI_BOOT_CONTROL_GET_ONE_SHOT_BOOT_MODE    GetOneShotBootMode;
  GBL_EFI_BOOT_CONTROL_HANDLE_LOADED_OS          HandleLoadedOs;
};

extern EFI_GUID  gGblEfiBootControlProtocolGuid;

#endif // __GBL_EFI_BOOT_CONTROL_PROTOCOL_H__
