/** @file

  Copyright (c) 2025, The Android Open Source Project.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  GBL EFI AVB Protocol.
  Delegates Android Verified Boot (AVB) board-specific logic to firmware.

  Related docs:
  https://cs.android.com/android/kernel/superproject/+/common-android-mainline:bootable/libbootloader/gbl/docs/gbl_efi_avb_protocol.md
*/

#ifndef GBL_EFI_AVB_PROTOCOL_H_
#define GBL_EFI_AVB_PROTOCOL_H_

#include <Uefi/UefiBaseType.h>

//
// {6bc66b9a-d5c9-4c02-9da9-50af198d912c}
//
#define GBL_EFI_AVB_PROTOCOL_GUID \
  { 0x6bc66b9a, 0xd5c9, 0x4c02, { 0x9d, 0xa9, 0x50, 0xaf, 0x19, 0x8d, 0x91, 0x2c } }

// Still in progress
#define GBL_EFI_AVB_PROTOCOL_REVISION  0x00000000

typedef struct _GBL_EFI_AVB_PROTOCOL             GBL_EFI_AVB_PROTOCOL;
typedef struct _GBL_EFI_AVB_PARTITION            GBL_EFI_AVB_PARTITION;
typedef struct _GBL_EFI_AVB_VERIFICATION_RESULT  GBL_EFI_AVB_VERIFICATION_RESULT;

/*
  Os-boot state colour per Android Verified Boot.
*/
typedef enum {
  GBL_EFI_AVB_BOOT_STATE_GREEN,
  GBL_EFI_AVB_BOOT_STATE_YELLOW,
  GBL_EFI_AVB_BOOT_STATE_ORANGE,
  GBL_EFI_AVB_BOOT_STATE_RED_EIO,
  GBL_EFI_AVB_BOOT_STATE_RED,
} GBL_EFI_AVB_BOOT_STATE_COLOR;

/*
  Vbmeta key validation status.
*/
typedef enum {
  GBL_EFI_AVB_VALID,
  GBL_EFI_AVB_VALID_CUSTOM_KEY,
  GBL_EFI_AVB_INVALID,
} GBL_EFI_AVB_KEY_VALIDATION_STATUS;

/*
  Result of AVB verification to be consumed by firmware UI / ROT.
*/
struct _GBL_EFI_AVB_VERIFICATION_RESULT {
  UINT32         Color;              // GBL_EFI_AVB_BOOT_STATE_COLOR
  CONST CHAR8    *Digest;            // Hex digest (NULL if verification failed)

  CONST CHAR8    *BootVersion;
  CONST CHAR8    *BootSecurityPatch;
  CONST CHAR8    *SystemVersion;
  CONST CHAR8    *SystemSecurityPatch;
  CONST CHAR8    *VendorVersion;
  CONST CHAR8    *VendorSecurityPatch;
};

/*
  Extra partition name requested for verification.
*/
struct _GBL_EFI_AVB_PARTITION {
  UINTN    NameLen;  // in/out
  CHAR8    *Name;    // caller-allocated
};

/// Get extra partitions to verify.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_READ_PARTITIONS_TO_VERIFY)(
  IN     GBL_EFI_AVB_PROTOCOL        *This,
  IN OUT UINTN                       *NumberOfPartitions,
  IN OUT GBL_EFI_AVB_PARTITION       *Partitions
  );

/// Report dm-verity corruption reboot.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_READ_IS_DM_VERITY_ERROR)(
  IN  GBL_EFI_AVB_PROTOCOL  *This,
  OUT BOOLEAN               *IsDmVerityError
  );

/// Verify that vbmeta public key is trusted.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_VALIDATE_VBMETA_PUBLIC_KEY)(
  IN  GBL_EFI_AVB_PROTOCOL  *This,
  IN  CONST UINT8           *PublicKeyData,
  IN  UINTN                 PublicKeyLength,
  IN  CONST UINT8           *PublicKeyMetadata,
  IN  UINTN                 PublicKeyMetadataLength,
  OUT UINT32                *ValidationStatus   // GBL_EFI_AVB_KEY_VALIDATION_STATUS
  );

/// Query device unlock state.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_READ_IS_DEVICE_UNLOCKED)(
  IN  GBL_EFI_AVB_PROTOCOL  *This,
  OUT BOOLEAN               *IsUnlocked
  );

/// Read rollback-index fuse.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_READ_ROLLBACK_INDEX)(
  IN  GBL_EFI_AVB_PROTOCOL  *This,
  IN  UINTN                 IndexLocation,
  OUT UINT64                *RollbackIndex
  );

/// Program rollback-index fuse.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_WRITE_ROLLBACK_INDEX)(
  IN GBL_EFI_AVB_PROTOCOL  *This,
  IN UINTN                 IndexLocation,
  IN UINT64                RollbackIndex
  );

/// Read persistent key-value pair.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_READ_PERSISTENT_VALUE)(
  IN     GBL_EFI_AVB_PROTOCOL  *This,
  IN     CONST CHAR8           *Name,
  OUT    UINT8                 *Value,
  IN OUT UINTN                 *ValueSize
  );

/// Write or erase persistent key-value pair.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_WRITE_PERSISTENT_VALUE)(
  IN GBL_EFI_AVB_PROTOCOL  *This,
  IN CONST CHAR8           *Name,
  IN CONST UINT8           *Value,
  IN UINTN                 ValueSize
  );

/// Handle overall AVB verification result.
typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_HANDLE_VERIFICATION_RESULT)(
  IN GBL_EFI_AVB_PROTOCOL                        *This,
  IN CONST GBL_EFI_AVB_VERIFICATION_RESULT       *Result
  );

/*
  Firmware-published protocol instance.
*/
struct _GBL_EFI_AVB_PROTOCOL {
  UINT64                                    Revision;
  GBL_EFI_AVB_READ_PARTITIONS_TO_VERIFY     ReadPartitionsToVerify;
  GBL_EFI_AVB_READ_IS_DM_VERITY_ERROR       ReadIsDmVerityError;
  GBL_EFI_AVB_VALIDATE_VBMETA_PUBLIC_KEY    ValidateVbmetaPublicKey;
  GBL_EFI_AVB_READ_IS_DEVICE_UNLOCKED       ReadIsDeviceUnlocked;
  GBL_EFI_AVB_READ_ROLLBACK_INDEX           ReadRollbackIndex;
  GBL_EFI_AVB_WRITE_ROLLBACK_INDEX          WriteRollbackIndex;
  GBL_EFI_AVB_READ_PERSISTENT_VALUE         ReadPersistentValue;
  GBL_EFI_AVB_WRITE_PERSISTENT_VALUE        WritePersistentValue;
  GBL_EFI_AVB_HANDLE_VERIFICATION_RESULT    HandleVerificationResult;
};

#endif // GBL_EFI_AVB_PROTOCOL_H_
