/** @file
  Runtime Architectural Protocol as defined in the DXE CIS.

Copyright (c) 2006, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  Runtime.h

Abstract:

  This code is used to produce the EFI runtime architectural protocol.

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
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PcdLib.h>


//
// Function Prototypes
//
EFI_STATUS
EFIAPI
RuntimeDriverCalculateCrc32 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT32  *CrcOut
  )
/*++

Routine Description:

  Calculate CRC32 for target data

Arguments:

  Data     - The target data.
  DataSize - The target data size.
  CrcOut   - The CRC32 for target data.

Returns:

  EFI_SUCCESS           - The CRC32 for target data is calculated successfully.
  EFI_INVALID_PARAMETER - Some parameter is not valid, so the CRC32 is not
                          calculated.

--*/
;

EFI_STATUS
EFIAPI
RuntimeDriverConvertPointer (
  IN     UINTN  DebugDisposition,
  IN OUT VOID   **ConvertAddress
  )
/*++

Routine Description:

  Determines the new virtual address that is to be used on subsequent memory accesses.

Arguments:

  DebugDisposition    - Supplies type information for the pointer being converted.
  ConvertAddress      - A pointer to a pointer that is to be fixed to be the value needed
                        for the new virtual address mappings being applied.

Returns:

  EFI_SUCCESS             - The pointer pointed to by Address was modified.
  EFI_NOT_FOUND           - The pointer pointed to by Address was not found to be part
                            of the current memory map. This is normally fatal.
  EFI_INVALID_PARAMETER   - One of the parameters has an invalid value.

--*/
;

EFI_STATUS
EFIAPI
RuntimeDriverSetVirtualAddressMap (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN UINT32                 DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR  *VirtualMap
  )
/*++

Routine Description:

  Changes the runtime addressing mode of EFI firmware from physical to virtual.

Arguments:

  MemoryMapSize     - The size in bytes of VirtualMap.
  DescriptorSize    - The size in bytes of an entry in the VirtualMap.
  DescriptorVersion - The version of the structure entries in VirtualMap.
  VirtualMap        - An array of memory descriptors which contain new virtual
                      address mapping information for all runtime ranges.

Returns:

  EFI_SUCCESS           - The virtual address map has been applied.
  EFI_UNSUPPORTED       - EFI firmware is not at runtime, or the EFI firmware is already in
                          virtual address mapped mode.
  EFI_INVALID_PARAMETER - DescriptorSize or DescriptorVersion is invalid.
  EFI_NO_MAPPING        - A virtual address was not supplied for a range in the memory
                          map that requires a mapping.
  EFI_NOT_FOUND         - A virtual address was supplied for an address that is not found
                          in the memory map.

--*/
;

VOID
RuntimeDriverInitializeCrc32Table (
  VOID
  )
/*++

Routine Description:

  Initialize CRC32 table.

Arguments:

  None.

Returns:

  None.

--*/
;

EFI_STATUS
EFIAPI
RuntimeDriverInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
/*++

Routine Description:

  Install Runtime AP. This code includes the EfiRuntimeLib, but it only
  functions at RT in physical mode.

Arguments:

  ImageHandle   - Image handle of this driver.
  SystemTable   - Pointer to the EFI System Table.

Returns:

  EFI_SUCEESS - Runtime Driver Architectural Protocol installed.

--*/
;

#endif
