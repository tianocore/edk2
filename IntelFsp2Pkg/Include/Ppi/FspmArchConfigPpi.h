/** @file
  Header file for FSP-M Arch Config PPI for Dispatch mode

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSPM_ARCH_CONFIG_PPI_H_
#define _FSPM_ARCH_CONFIG_PPI_H_

#define FSPM_ARCH_CONFIG_PPI_REVISION  0x1

///
/// Global ID for the FSPM_ARCH_CONFIG_PPI.
///
#define FSPM_ARCH_CONFIG_GUID \
  { \
    0x824d5a3a, 0xaf92, 0x4c0c, { 0x9f, 0x19, 0x19, 0x52, 0x6d, 0xca, 0x4a, 0xbb } \
  }

///
/// This PPI provides FSP-M Arch Config PPI.
///
typedef struct {
  ///
  /// Revision of the structure
  ///
  UINT8     Revision;
  UINT8     Reserved[3];
  ///
  /// Pointer to the non-volatile storage (NVS) data buffer.
  /// If it is NULL it indicates the NVS data is not available.
  ///
  VOID      *NvsBufferPtr;
  ///
  /// Size of memory to be reserved by FSP below "top
  /// of low usable memory" for bootloader usage.
  ///
  UINT32    BootLoaderTolumSize;
  UINT8     Reserved1[4];
} FSPM_ARCH_CONFIG_PPI;

extern EFI_GUID  gFspmArchConfigPpiGuid;

#endif // _FSPM_ARCH_CONFIG_PPI_H_
