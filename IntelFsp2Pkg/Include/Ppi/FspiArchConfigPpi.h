/** @file
  Header file for FSP-I Arch Config PPI for Dispatch mode

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSPI_ARCH_CONFIG_PPI_H_
#define _FSPI_ARCH_CONFIG_PPI_H_

#define FSPI_ARCH_CONFIG_PPI_REVISION  0x1

///
/// Global ID for the FSPI_ARCH_CONFIG_PPI.
///
#define FSPI_ARCH_CONFIG_GUID \
  { \
    0x15735ef9, 0x84ac, 0x4e34, { 0x98, 0x86, 0x56, 0xee, 0xcf, 0x73, 0xec, 0x0f } \
  }

///
/// This PPI provides FSP-I Arch Config PPI.
///
typedef struct {
  ///
  /// Revision of the structure is 1 for this version of the specification.
  ///
  UINT8     Revision;
  UINT8     Reserved[3];
  ///
  /// Pointer to the bootloader SMM firmware volume (FV).
  ///
  VOID      *BootloaderSmmFvBaseAddress;
  ///
  /// The length in bytes of the bootloader SMM firmware volume (FV).
  ///
  UINTN     BootloaderSmmFvLength;
  ///
  /// Pointer to the bootloader SMM FV context data.
  /// This data is provided to bootloader SMM drivers through a HOB by the FSP MM Foundation.
  ///
  VOID      *BootloaderSmmFvContextData;
  ///
  /// The length in bytes of the bootloader SMM FV context data.
  /// This data is provided to bootloader SMM drivers through a HOB by the FSP MM Foundation.
  ///
  UINT16    BootloaderSmmFvContextDataLength;
} FSPI_ARCH_CONFIG_PPI;

extern EFI_GUID  gFspiArchConfigPpiGuid;

#endif // _FSPI_ARCH_CONFIG_PPI_H_
