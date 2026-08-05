/** @file
  GBL EFI Boot Control Protocol.

  Delegates boot target manipulation logic to firmware.

  Related docs:
  https://android.googlesource.com/platform/bootable/libbootloader/+/refs/heads/gbl-mainline/gbl/docs/gbl_efi_boot_control_protocol.md

  Copyright (c) 2025, The Android Open Source Project.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi/UefiBaseType.h>

//
// {d382db1b-9ac2-11f0-84c7-047bcba96019}
//
#define GBL_EFI_BOOT_CONTROL_PROTOCOL_GUID \
  { 0xd382db1b, 0x9ac2, 0x11f0, { 0x84, 0xc7, 0x04, 0x7b, 0xcb, 0xa9, 0x60, 0x19 } }

#define GBL_EFI_BOOT_CONTROL_PROTOCOL_REVISION  0x00010000

typedef struct _GBL_EFI_BOOT_CONTROL_PROTOCOL GBL_EFI_BOOT_CONTROL_PROTOCOL;

typedef enum {
  GblEfiUnbootableReasonUnknownReason,
  GblEfiUnbootableReasonNoMoreTries,
  GblEfiUnbootableReasonSystemUpdate,
  GblEfiUnbootableReasonUserRequested,
  GblEfiUnbootableReasonVerificationFailure
} GBL_EFI_UNBOOTABLE_REASON;

typedef enum {
  GblEfiOneShotBootModeNone,
  GblEfiOneShotBootModeBootloader,
  GblEfiOneShotBootModeRecovery
} GBL_EFI_ONE_SHOT_BOOT_MODE;

typedef struct {
  UINT32    Suffix;           // One UTF-8 encoded single character.
  UINT8     UnbootableReason; // GBL_EFI_UNBOOTABLE_REASON
  UINT8     Priority;
  UINT8     RemainingTries;
  UINT8     Successful;
} GBL_EFI_SLOT_INFO;

typedef struct {
  UINTN                   KernelSize;
  EFI_PHYSICAL_ADDRESS    Kernel;
  UINTN                   RamdiskSize;
  EFI_PHYSICAL_ADDRESS    Ramdisk;
  UINTN                   DeviceTreeSize;
  EFI_PHYSICAL_ADDRESS    DeviceTree;
  UINT64                  Reserved[8];
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
