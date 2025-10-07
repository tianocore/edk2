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

#define GBL_EFI_AVB_PROTOCOL_REVISION  0x00000003

typedef struct _GBL_EFI_AVB_PROTOCOL GBL_EFI_AVB_PROTOCOL;

typedef UINT64 GBL_EFI_AVB_DEVICE_STATUS;
STATIC CONST GBL_EFI_AVB_DEVICE_STATUS  GBL_EFI_AVB_DEVICE_STATUS_UNLOCKED         = 0x1 << 0;
STATIC CONST GBL_EFI_AVB_DEVICE_STATUS  GBL_EFI_AVB_DEVICE_STATUS_DM_VERITY_FAILED = 0x1 << 1;

typedef UINT64 GBL_EFI_AVB_BOOT_COLOR;
STATIC CONST GBL_EFI_AVB_BOOT_COLOR  GBL_EFI_AVB_BOOT_COLOR_RED     = 0x1 << 0;
STATIC CONST GBL_EFI_AVB_BOOT_COLOR  GBL_EFI_AVB_BOOT_COLOR_ORANGE  = 0x1 << 1;
STATIC CONST GBL_EFI_AVB_BOOT_COLOR  GBL_EFI_AVB_BOOT_COLOR_YELLOW  = 0x1 << 2;
STATIC CONST GBL_EFI_AVB_BOOT_COLOR  GBL_EFI_AVB_BOOT_COLOR_GREEN   = 0x1 << 3;
STATIC CONST GBL_EFI_AVB_BOOT_COLOR  GBL_EFI_AVB_BOOT_COLOR_RED_EIO = 0x1 << 4;

typedef enum {
  GBL_EFI_AVB_KEY_VALIDATION_STATUS_INVALID = 0,
  GBL_EFI_AVB_KEY_VALIDATION_STATUS_VALID_CUSTOM_KEY,
  GBL_EFI_AVB_KEY_VALIDATION_STATUS_VALID
} GBL_EFI_AVB_KEY_VALIDATION_STATUS;

typedef struct {
  UINTN    BaseNameLen; // in/out
  CHAR8    *BaseName;   // UTF-8 null terminated
} GBL_EFI_AVB_PARTITION;

typedef struct {
  CHAR8    *BaseName; // UTF-8 null terminated
  UINTN    DataSize;
  UINT8    *Data;
} GBL_EFI_AVB_LOADED_PARTITION;

typedef struct {
  CONST CHAR8    *BasePartitionName; // UTF-8, null terminated
  CONST CHAR8    *Key;               // UTF-8, null terminated
  UINTN          ValueSize;
  CONST UINT8    *Value;
} GBL_EFI_AVB_PROPERTY;

typedef struct {
  GBL_EFI_AVB_BOOT_COLOR                ColorFlags;
  // UTF-8, null terminated
  CONST CHAR8                           *Digest;
  UINTN                                 NumLoadedPartitions;
  CONST GBL_EFI_AVB_LOADED_PARTITION    *LoadedPartitions;
  UINTN                                 NumProperties;
  CONST GBL_EFI_AVB_PROPERTY            *Properties;
  UINT64                                Reserved2[8];
} GBL_EFI_AVB_VERIFICATION_RESULT;

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_READ_PARTITIONS_TO_VERIFY)(
  IN GBL_EFI_AVB_PROTOCOL      *This,
  IN OUT UINTN                 *NumberOfPartitions,
  IN OUT GBL_EFI_AVB_PARTITION *Partitions
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_READ_DEVICE_STATUS)(
  IN GBL_EFI_AVB_PROTOCOL       *This,
  OUT GBL_EFI_AVB_DEVICE_STATUS *StatusFlags
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_VALIDATE_VBMETA_PUBLIC_KEY)(
  IN GBL_EFI_AVB_PROTOCOL *This,
  IN UINTN                PublicKeyLength,
  IN CONST UINT8          *PublicKeyData,
  IN UINTN                PublicKeyMetadataLength,
  IN CONST UINT8          *PublicKeyMetadata,
  OUT UINT32              *ValidationStatus        // GBL_EFI_AVB_KEY_VALIDATION_STATUS
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_READ_ROLLBACK_INDEX)(
  IN GBL_EFI_AVB_PROTOCOL *This,
  IN UINTN                IndexLocation,
  OUT UINT64              *RollbackIndex
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_WRITE_ROLLBACK_INDEX)(
  IN GBL_EFI_AVB_PROTOCOL *This,
  IN UINTN                IndexLocation,
  IN UINT64               RollbackIndex
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_READ_PERSISTENT_VALUE)(
  IN GBL_EFI_AVB_PROTOCOL *This,
  IN CONST CHAR8          *Name,
  IN OUT UINTN            *ValueSize,
  OUT UINT8               *Value
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_WRITE_PERSISTENT_VALUE)(
  IN GBL_EFI_AVB_PROTOCOL *This,
  IN CONST CHAR8          *Name,
  IN UINTN                ValueSize,
  IN CONST UINT8          *Value
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_HANDLE_VERIFICATION_RESULT)(
  IN GBL_EFI_AVB_PROTOCOL                  *This,
  IN CONST GBL_EFI_AVB_VERIFICATION_RESULT *Result
  );

struct _GBL_EFI_AVB_PROTOCOL {
  UINT64                                    Revision;
  GBL_EFI_AVB_READ_PARTITIONS_TO_VERIFY     ReadPartitionsToVerify;
  GBL_EFI_AVB_READ_DEVICE_STATUS            ReadDeviceStatus;
  GBL_EFI_AVB_VALIDATE_VBMETA_PUBLIC_KEY    ValidateVbmetaPublicKey;
  GBL_EFI_AVB_READ_ROLLBACK_INDEX           ReadRollbackIndex;
  GBL_EFI_AVB_WRITE_ROLLBACK_INDEX          WriteRollbackIndex;
  GBL_EFI_AVB_READ_PERSISTENT_VALUE         ReadPersistentValue;
  GBL_EFI_AVB_WRITE_PERSISTENT_VALUE        WritePersistentValue;
  GBL_EFI_AVB_HANDLE_VERIFICATION_RESULT    HandleVerificationResult;
};

#endif // GBL_EFI_AVB_PROTOCOL_H_
