/** @file
  GBL EFI Boot Memory Protocol.

  Provides APIs for retrieving and synchronizing boot and partition buffers.

  Related docs:
  https://android.googlesource.com/platform/bootable/libbootloader/+/refs/heads/gbl-mainline/gbl/docs/gbl_efi_boot_memory_protocol.md

  Copyright (c) 2025, The Android Open Source Project.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi/UefiBaseType.h>

//
// {309f2874-ad59-4fd2-af5e-ce0f4ab401a6}
//
#define GBL_EFI_BOOT_MEMORY_PROTOCOL_GUID \
  { 0x309f2874, 0xad59, 0x4fd2, { 0xaf, 0x5e, 0xce, 0x0f, 0x4a, 0xb4, 0x01, 0xa6 } }

#define GBL_EFI_BOOT_MEMORY_PROTOCOL_REVISION  0x00010000

typedef struct _GBL_EFI_BOOT_MEMORY_PROTOCOL GBL_EFI_BOOT_MEMORY_PROTOCOL;

typedef enum {
  GblEfiBootBufferTypeGeneralLoad = 0,
  GblEfiBootBufferTypeKernel,
  GblEfiBootBufferTypeRamdisk,
  GblEfiBootBufferTypeFdt,
  GblEfiBootBufferTypePvmfwData,
  GblEfiBootBufferTypeFastbootDownload
} GBL_EFI_BOOT_BUFFER_TYPE;

typedef enum {
  GblEfiPartitionBufferFlagPreloaded = BIT0
} GBL_EFI_PARTITION_BUFFER_FLAG;

typedef
EFI_STATUS
(EFIAPI *GBL_EFI_BOOT_MEMORY_GET_PARTITION_BUFFER)(
  IN GBL_EFI_BOOT_MEMORY_PROTOCOL   *This,
  IN CONST CHAR8                    *BaseName,
  OUT UINTN                         *Size,
  OUT VOID                          **Addr,
  OUT UINT32                        *Flag // GBL_EFI_PARTITION_BUFFER_FLAG
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
  IN UINT32                       BufType, // GBL_EFI_BOOT_BUFFER_TYPE
  OUT UINTN                       *Size,
  OUT VOID                        **Addr
  );

struct _GBL_EFI_BOOT_MEMORY_PROTOCOL {
  UINT64                                       Revision;
  GBL_EFI_BOOT_MEMORY_GET_PARTITION_BUFFER     GetPartitionBuffer;
  GBL_EFI_BOOT_MEMORY_SYNC_PARTITION_BUFFER    SyncPartitionBuffer;
  GBL_EFI_BOOT_MEMORY_GET_BOOT_BUFFER          GetBootBuffer;
};

extern EFI_GUID  gGblEfiBootMemoryProtocolGuid;
