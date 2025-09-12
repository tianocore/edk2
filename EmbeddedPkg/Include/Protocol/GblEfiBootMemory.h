/** @file

  Copyright (c) 2025, The Android Open Source Project.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
  GBL EFI Boot Memory Protocol.
  Provides APIs for retrieving and synchronizing boot and partition buffers.

  Related docs:
  https://cs.android.com/android/kernel/superproject/+/common-android-mainline:bootable/libbootloader/gbl/docs/gbl_efi_boot_memory_protocol.md
*/

#ifndef GBL_EFI_BOOT_MEMORY_PROTOCOL_H_
#define GBL_EFI_BOOT_MEMORY_PROTOCOL_H_

#include <Uefi/UefiBaseType.h>

//
// {6f4e49e0-07c4-45a1-b6e5-39df55ff2f3e}
//
#define GBL_EFI_BOOT_MEMORY_PROTOCOL_GUID \
  { 0x6f4e49e0, 0x07c4, 0x45a1, { 0xb6, 0xe5, 0x39, 0xdf, 0x55, 0xff, 0x2f, 0x3e } }

#define GBL_EFI_BOOT_MEMORY_PROTOCOL_REVISION  0x00000001

typedef struct _GBL_EFI_BOOT_MEMORY_PROTOCOL GBL_EFI_BOOT_MEMORY_PROTOCOL;

typedef enum {
  GBL_EFI_BOOT_BUFFER_TYPE_GENERAL_LOAD = 0,
  GBL_EFI_BOOT_BUFFER_TYPE_KERNEL,
  GBL_EFI_BOOT_BUFFER_TYPE_RAMDISK,
  GBL_EFI_BOOT_BUFFER_TYPE_FDT,
  GBL_EFI_BOOT_BUFFER_TYPE_PVMFW_DATA,
  GBL_EFI_BOOT_BUFFER_TYPE_FASTBOOT_DOWNLOAD,
} GBL_EFI_BOOT_BUFFER_TYPE;

typedef enum {
  GBL_EFI_PARTITION_BUFFER_FLAG_PRELOADED = 1u << 0,
} GBL_EFI_PARTITION_BUFFER_FLAG;

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_BOOT_MEMORY_GET_PARTITION_BUFFER)(
  IN GBL_EFI_BOOT_MEMORY_PROTOCOL   *This,
  IN CONST CHAR8                    *BaseName,
  OUT UINTN                         *Size,
  OUT VOID                          **Addr,
  OUT GBL_EFI_PARTITION_BUFFER_FLAG *Flag
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_BOOT_MEMORY_SYNC_PARTITION_BUFFER)(
  IN GBL_EFI_BOOT_MEMORY_PROTOCOL *This,
  IN BOOLEAN                      SyncPreloaded
  );

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_BOOT_MEMORY_GET_BOOT_BUFFER)(
  IN GBL_EFI_BOOT_MEMORY_PROTOCOL *This,
  IN GBL_EFI_BOOT_BUFFER_TYPE     BufType,
  OUT UINTN                       *Size,
  OUT VOID                        **Addr
  );

struct _GBL_EFI_BOOT_MEMORY_PROTOCOL {
  UINT64                                       Revision;
  GBL_EFI_BOOT_MEMORY_GET_PARTITION_BUFFER     GetPartitionBuffer;
  GBL_EFI_BOOT_MEMORY_SYNC_PARTITION_BUFFER    SyncPartitionBuffer;
  GBL_EFI_BOOT_MEMORY_GET_BOOT_BUFFER          GetBootBuffer;
};

#endif // GBL_EFI_BOOT_MEMORY_PROTOCOL_H_
