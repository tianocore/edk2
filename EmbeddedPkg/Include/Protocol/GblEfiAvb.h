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

#ifndef __GBL_EFI_AVB_PROTOCOL_H__
#define __GBL_EFI_AVB_PROTOCOL_H__

#include <Uefi/UefiBaseType.h>

//
// {6bc66b9a-d5c9-4c02-9da9-50af198d912c}
//
#define GBL_EFI_AVB_PROTOCOL_GUID \
  { 0x6bc66b9a, 0xd5c9, 0x4c02, { 0x9d, 0xa9, 0x50, 0xaf, 0x19, 0x8d, 0x91, 0x2c } }

// WARNING: The current API is unstable. While backward compatibility is
// guaranteed for pre-release revisions after 0x00000100, the official
// `1.0` (0x00010000) release may include final breaking changes.
#define GBL_EFI_AVB_PROTOCOL_REVISION  0x00000100

typedef struct _GBL_EFI_AVB_PROTOCOL GBL_EFI_AVB_PROTOCOL;

typedef UINT64 GBL_EFI_AVB_DEVICE_STATUS;
#define GBL_EFI_AVB_DEVICE_STATUS_UNLOCKED           BIT0
#define GBL_EFI_AVB_DEVICE_STATUS_DM_VERITY_FAILED   BIT1
#define GBL_EFI_AVB_DEVICE_STATUS_UNLOCKED_CRITICAL  BIT2
#define GBL_EFI_AVB_DEVICE_STATUS_UNLOCKABLE         BIT3

typedef UINT64 GBL_EFI_AVB_BOOT_COLOR;
#define GBL_EFI_AVB_BOOT_COLOR_RED      BIT0
#define GBL_EFI_AVB_BOOT_COLOR_ORANGE   BIT1
#define GBL_EFI_AVB_BOOT_COLOR_YELLOW   BIT2
#define GBL_EFI_AVB_BOOT_COLOR_GREEN    BIT3
#define GBL_EFI_AVB_BOOT_COLOR_RED_EIO  BIT4

typedef UINT64 GBL_EFI_AVB_PARTITION_FLAGS;
#define GBL_EFI_AVB_PARTITION_FLAG_VERIFY            BIT0
#define GBL_EFI_AVB_PARTITION_FLAG_VERIFY_IF_EXISTS  BIT1
#define GBL_EFI_AVB_PARTITION_FLAG_FLASH_CRITICAL    BIT2
#define GBL_EFI_AVB_PARTITION_FLAG_FDR               BIT3

typedef enum {
  GBL_EFI_AVB_KEY_VALIDATION_STATUS_INVALID = 0,
  GBL_EFI_AVB_KEY_VALIDATION_STATUS_VALID_CUSTOM_KEY,
  GBL_EFI_AVB_KEY_VALIDATION_STATUS_VALID
} GBL_EFI_AVB_KEY_VALIDATION_STATUS;

typedef enum {
  GBL_EFI_AVB_LOCK_TYPE_DEVICE = 0,
  GBL_EFI_AVB_LOCK_TYPE_CRITICAL
} GBL_EFI_AVB_LOCK_TYPE;

typedef enum {
  GBL_EFI_AVB_LOCK_STATE_UNLOCKED = 0,
  GBL_EFI_AVB_LOCK_STATE_LOCKED
} GBL_EFI_AVB_LOCK_STATE;

typedef struct {
  UINTN                          BaseNameLen;
  CHAR8                          *BaseName; // UTF-8, null terminated
  GBL_EFI_AVB_PARTITION_FLAGS    Flags;
} GBL_EFI_AVB_PARTITION_ATTRIBUTES;

typedef struct {
  CONST CHAR8    *BaseName; // UTF-8 null terminated
  UINTN          DataSize;
  CONST UINT8    *Data;
} GBL_EFI_AVB_LOADED_PARTITION;

typedef struct {
  CONST CHAR8    *BasePartitionName; // UTF-8, null terminated
  CONST CHAR8    *Key;               // UTF-8, null terminated
  UINTN          ValueSize;
  CONST UINT8    *Value;
} GBL_EFI_AVB_PROPERTY;

typedef struct {
  GBL_EFI_AVB_BOOT_COLOR                ColorFlags;
  CONST CHAR8                           *Digest; // UTF-8, null terminated
  UINTN                                 NumPartitions;
  CONST GBL_EFI_AVB_LOADED_PARTITION    *Partitions;
  UINTN                                 NumProperties;
  CONST GBL_EFI_AVB_PROPERTY            *Properties;
  UINT64                                Reserved[8];
} GBL_EFI_AVB_VERIFICATION_RESULT;

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_READ_PARTITION_ATTRIBUTES)(
  IN GBL_EFI_AVB_PROTOCOL                 *This,
  IN OUT UINTN                            *NumPartitions,
  IN OUT GBL_EFI_AVB_PARTITION_ATTRIBUTES *Partitions
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
  OUT UINT32              *ValidationStatus // GBL_EFI_AVB_KEY_VALIDATION_STATUS
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

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_WRITE_LOCK_STATE)(
  IN GBL_EFI_AVB_PROTOCOL *This,
  IN UINT8                Type, // GBL_EFI_AVB_LOCK_TYPE
  IN UINT8                State // GBL_EFI_AVB_LOCK_STATE
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_AVB_FACTORY_DATA_RESET)(
  IN GBL_EFI_AVB_PROTOCOL *This
  );

struct _GBL_EFI_AVB_PROTOCOL {
  UINT64                                    Revision;
  GBL_EFI_AVB_READ_PARTITION_ATTRIBUTES     ReadPartitionAttributes;
  GBL_EFI_AVB_READ_DEVICE_STATUS            ReadDeviceStatus;
  GBL_EFI_AVB_VALIDATE_VBMETA_PUBLIC_KEY    ValidateVbmetaPublicKey;
  GBL_EFI_AVB_READ_ROLLBACK_INDEX           ReadRollbackIndex;
  GBL_EFI_AVB_WRITE_ROLLBACK_INDEX          WriteRollbackIndex;
  GBL_EFI_AVB_READ_PERSISTENT_VALUE         ReadPersistentValue;
  GBL_EFI_AVB_WRITE_PERSISTENT_VALUE        WritePersistentValue;
  GBL_EFI_AVB_HANDLE_VERIFICATION_RESULT    HandleVerificationResult;
  GBL_EFI_AVB_WRITE_LOCK_STATE              WriteLockState;
  GBL_EFI_AVB_FACTORY_DATA_RESET            FactoryDataReset;
};

extern EFI_GUID  gGblEfiAvbProtocolGuid;

#endif // __GBL_EFI_AVB_PROTOCOL_H__
