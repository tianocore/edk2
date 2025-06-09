/** @file

  Copyright (c) 2025, The Android Open Source Project.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  GBL EFI AB Slot Protocol.
  Offers firmware helpers for Android A/B slot metadata and boot-reason handling.

  Related docs:
  https://cs.android.com/android/kernel/superproject/+/common-android-mainline:bootable/libbootloader/gbl/docs/gbl_efi_ab_slot_protocol.md
*/

#ifndef GBL_EFI_AB_SLOT_PROTOCOL_H_
#define GBL_EFI_AB_SLOT_PROTOCOL_H_

#include <Uefi/UefiBaseType.h>

//
// {9a7a7db4-614b-4a08-3df9-006f49b0d80c}
//
#define GBL_EFI_AB_SLOT_PROTOCOL_GUID \
  { 0x9a7a7db4, 0x614b, 0x4a08, { 0x3d, 0xf9, 0x00, 0x6f, 0x49, 0xb0, 0xd8, 0x0c } }

#define GBL_EFI_AB_SLOT_PROTOCOL_VERSION  0x00000000

typedef struct _GBL_EFI_AB_SLOT_PROTOCOL     GBL_EFI_AB_SLOT_PROTOCOL;
typedef struct _GBL_EFI_SLOT_INFO            GBL_EFI_SLOT_INFO;
typedef struct _GBL_EFI_SLOT_METADATA_BLOCK  GBL_EFI_SLOT_METADATA_BLOCK;

/*
  Snapshot-merge state (Virtual A/B).
*/
typedef enum {
  GBL_EFI_SLOT_MERGE_STATUS_NONE = 0,
  GBL_EFI_SLOT_MERGE_STATUS_UNKNOWN,
  GBL_EFI_SLOT_MERGE_STATUS_SNAPSHOTTED,
  GBL_EFI_SLOT_MERGE_STATUS_MERGING,
  GBL_EFI_SLOT_MERGE_STATUS_CANCELLED
} GBL_EFI_SLOT_MERGE_STATUS;

/*
  Why a slot became unbootable.
*/
typedef enum {
  GBL_EFI_UNBOOTABLE_REASON_UNKNOWN = 0,
  GBL_EFI_UNBOOTABLE_REASON_NO_MORE_TRIES,
  GBL_EFI_UNBOOTABLE_REASON_SYSTEM_UPDATE,
  GBL_EFI_UNBOOTABLE_REASON_USER_REQUESTED,
  GBL_EFI_UNBOOTABLE_REASON_VERIFICATION_FAILURE
} GBL_EFI_UNBOOTABLE_REASON;

/*
  Android boot-reason codes.
*/
typedef enum {
  GBL_EFI_BOOT_REASON_EMPTY        = 0,
  GBL_EFI_BOOT_REASON_UNKNOWN      = 1,
  GBL_EFI_BOOT_REASON_WATCHDOG     = 14,
  GBL_EFI_BOOT_REASON_KERNEL_PANIC = 15,
  GBL_EFI_BOOT_REASON_RECOVERY     = 3,
  GBL_EFI_BOOT_REASON_BOOTLOADER   = 55,
  GBL_EFI_BOOT_REASON_COLD         = 56,
  GBL_EFI_BOOT_REASON_HARD         = 57,
  GBL_EFI_BOOT_REASON_WARM         = 58,
  GBL_EFI_BOOT_REASON_SHUTDOWN     = 59,
  GBL_EFI_BOOT_REASON_REBOOT       = 18,
  GBL_EFI_BOOT_REASON_FASTBOOTD    = 196
} GBL_EFI_BOOT_REASON;

/*
  Per-slot state.
*/
struct _GBL_EFI_SLOT_INFO {
  UINT32    Suffix;           // UTF-8 code-point of slot letter
  UINT32    UnbootableReason; // GBL_EFI_UNBOOTABLE_REASON
  UINT8     Priority;
  UINT8     Tries;
  UINT8     Successful;      // 1 if slot booted once
};

/*
  Global slot-metadata block.
*/
struct _GBL_EFI_SLOT_METADATA_BLOCK {
  UINT8    UnbootableMetadata; // 1 if reasons tracked
  UINT8    MaxRetries;
  UINT8    SlotCount;
  UINT8    MergeStatus;      // GBL_EFI_SLOT_MERGE_STATUS
};

/// Load immutable slot metadata.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AB_SLOT_LOAD_BOOT_DATA)(
  IN  GBL_EFI_AB_SLOT_PROTOCOL       *This,
  OUT GBL_EFI_SLOT_METADATA_BLOCK    *Metadata
  );

/// Get info for slot by index.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AB_SLOT_GET_SLOT_INFO)(
  IN  GBL_EFI_AB_SLOT_PROTOCOL  *This,
  IN  UINT8                     Index,
  OUT GBL_EFI_SLOT_INFO         *Info
  );

/// Get info for current slot.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AB_SLOT_GET_CURRENT_SLOT)(
  IN  GBL_EFI_AB_SLOT_PROTOCOL  *This,
  OUT GBL_EFI_SLOT_INFO         *Info
  );

/// Decide next slot; optionally mark boot attempt.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AB_SLOT_GET_NEXT_SLOT)(
  IN  GBL_EFI_AB_SLOT_PROTOCOL  *This,
  IN  BOOLEAN                   MarkBootAttempt,
  OUT GBL_EFI_SLOT_INFO         *Info
  );

/// Make slot active.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AB_SLOT_SET_ACTIVE_SLOT)(
  IN GBL_EFI_AB_SLOT_PROTOCOL  *This,
  IN UINT8                     Index
  );

/// Mark slot unbootable with reason.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AB_SLOT_SET_SLOT_UNBOOTABLE)(
  IN GBL_EFI_AB_SLOT_PROTOCOL  *This,
  IN UINT8                     Index,
  IN UINT32                    UnbootableReason   // GBL_EFI_UNBOOTABLE_REASON
  );

/// Re-initialise all slot metadata.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AB_SLOT_REINITIALIZE)(
  IN GBL_EFI_AB_SLOT_PROTOCOL  *This
  );

/// Read boot reason and sub-reason string.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AB_SLOT_GET_BOOT_REASON)(
  IN  GBL_EFI_AB_SLOT_PROTOCOL  *This,
  OUT UINT32                    *Reason,           // GBL_EFI_BOOT_REASON
  IN  OUT UINTN                 *SubreasonLength,
  OUT CHAR8                     *Subreason
  );

/// Set boot reason and sub-reason string.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AB_SLOT_SET_BOOT_REASON)(
  IN GBL_EFI_AB_SLOT_PROTOCOL  *This,
  IN UINT32                    Reason,             // GBL_EFI_BOOT_REASON
  IN UINTN                     SubreasonLength,
  IN CONST CHAR8               *Subreason
  );

/// Flush metadata to persistent storage.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AB_SLOT_FLUSH)(
  IN GBL_EFI_AB_SLOT_PROTOCOL  *This
  );

/*
  Firmware-published protocol instance.
*/
struct _GBL_EFI_AB_SLOT_PROTOCOL {
  UINT32                                 Version;
  GBL_EFI_AB_SLOT_LOAD_BOOT_DATA         LoadBootData;
  GBL_EFI_AB_SLOT_GET_SLOT_INFO          GetSlotInfo;
  GBL_EFI_AB_SLOT_GET_CURRENT_SLOT       GetCurrentSlot;
  GBL_EFI_AB_SLOT_GET_NEXT_SLOT          GetNextSlot;
  GBL_EFI_AB_SLOT_SET_ACTIVE_SLOT        SetActiveSlot;
  GBL_EFI_AB_SLOT_SET_SLOT_UNBOOTABLE    SetSlotUnbootable;
  GBL_EFI_AB_SLOT_REINITIALIZE           Reinitialize;
  GBL_EFI_AB_SLOT_GET_BOOT_REASON        GetBootReason;
  GBL_EFI_AB_SLOT_SET_BOOT_REASON        SetBootReason;
  GBL_EFI_AB_SLOT_FLUSH                  Flush;
};

#endif // GBL_EFI_AB_SLOT_PROTOCOL_H_
