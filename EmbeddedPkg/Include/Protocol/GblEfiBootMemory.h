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
// {309f2874-ad59-4fd2-af5e-ce0f4ab401a6}
//
#define GBL_EFI_BOOT_MEMORY_PROTOCOL_GUID \
  { 0x309f2874, 0xad59, 0x4fd2, { 0xaf, 0x5e, 0xce, 0x0f, 0x4a, 0xb4, 0x01, 0xa6 } }

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
