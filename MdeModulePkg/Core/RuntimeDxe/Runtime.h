/** @file
  Runtime Architectural Protocol as defined in the DXE CIS.

  This code is used to produce the EFI runtime architectural protocol.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RUNTIME_H_
#define _RUNTIME_H_

#include <PiDxe.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/Runtime.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/PeCoffLib.h>

//
// Function Prototypes
//

/**
  Calculate CRC32 for target data.

  @param Data            The target data.
  @param DataSize        The target data size.
  @param CrcOut          The CRC32 for target data.

  @retval  EFI_SUCCESS            The CRC32 for target data is calculated successfully.
  @retval  EFI_INVALID_PARAMETER  Some parameter is not valid, so the CRC32 is not
                                  calculated.

**/
EFI_STATUS
EFIAPI
RuntimeDriverCalculateCrc32 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT32  *CrcOut
  );

/**
  Determines the new virtual address that is to be used on subsequent memory accesses.


  @param DebugDisposition Supplies type information for the pointer being converted.
  @param ConvertAddress  A pointer to a pointer that is to be fixed to be the value needed
                         for the new virtual address mappings being applied.

  @retval  EFI_SUCCESS              The pointer pointed to by Address was modified.
  @retval  EFI_NOT_FOUND            The pointer pointed to by Address was not found to be part
                                    of the current memory map. This is normally fatal.
  @retval  EFI_INVALID_PARAMETER    One of the parameters has an invalid value.

**/
EFI_STATUS
EFIAPI
RuntimeDriverConvertPointer (
  IN     UINTN  DebugDisposition,
  IN OUT VOID   **ConvertAddress
  );

/**
  Changes the runtime addressing mode of EFI firmware from physical to virtual.

  @param  MemoryMapSize   The size in bytes of VirtualMap.
  @param  DescriptorSize  The size in bytes of an entry in the VirtualMap.
  @param  DescriptorVersion The version of the structure entries in VirtualMap.
  @param  VirtualMap      An array of memory descriptors which contain new virtual
                         address mapping information for all runtime ranges.

  @retval  EFI_SUCCESS            The virtual address map has been applied.
  @retval  EFI_UNSUPPORTED        EFI firmware is not at runtime, or the EFI firmware is already in
                                  virtual address mapped mode.
  @retval  EFI_INVALID_PARAMETER  DescriptorSize or DescriptorVersion is invalid.
  @retval  EFI_NO_MAPPING         A virtual address was not supplied for a range in the memory
                                  map that requires a mapping.
  @retval  EFI_NOT_FOUND          A virtual address was supplied for an address that is not found
                                  in the memory map.

**/
EFI_STATUS
EFIAPI
RuntimeDriverSetVirtualAddressMap (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN UINT32                 DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR  *VirtualMap
  );

/**
  Install Runtime AP. This code includes the EfiRuntimeLib, but it only
  functions at RT in physical mode.

  @param  ImageHandle     Image handle of this driver.
  @param  SystemTable     Pointer to the EFI System Table.

  @retval EFI_SUCEESS  Runtime Driver Architectural Protocol Installed
  @return Other value if gBS->InstallMultipleProtocolInterfaces fails. Check
           gBS->InstallMultipleProtocolInterfaces for details.

**/
EFI_STATUS
EFIAPI
RuntimeDriverInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif
