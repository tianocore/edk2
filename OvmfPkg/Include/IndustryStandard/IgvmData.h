/** @file
  IGVM Data hobs

  Copyright (c) 2025, Red Hat. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __IGVM_DATA_H__
#define __IGVM_DATA_H__

/* secure boot databases */
#define EFI_IGVM_DATA_TYPE_PK   0x100
#define EFI_IGVM_DATA_TYPE_KEK  0x101
#define EFI_IGVM_DATA_TYPE_DB   0x102
#define EFI_IGVM_DATA_TYPE_DBX  0x103

/* efi binaries for direct kernel boot */
#define EFI_IGVM_DATA_TYPE_SHIM    0x200
#define EFI_IGVM_DATA_TYPE_KERNEL  0x201

typedef struct {
  UINT64    Address;
  UINT64    Length;
  UINT32    DataType;
  UINT32    DataFlags;
} EFI_IGVM_DATA_HOB;

#endif /* __IGVM_DATA_H__ */
