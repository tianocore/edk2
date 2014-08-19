/** @file
  Generic ARM implementation of DmaLib.h

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/DmaLib.h>
#include <Library/MemoryAllocationLib.h>



/**
  Provides the DMA controller-specific addresses needed to access system memory.

  Operation is relative to the DMA bus master.

  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the DMA controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
EFI_STATUS
EFIAPI
DmaMap (
  IN     DMA_MAP_OPERATION              Operation,
  IN     VOID                           *HostAddress,
  IN OUT UINTN                          *NumberOfBytes,
  OUT    PHYSICAL_ADDRESS               *DeviceAddress,
  OUT    VOID                           **Mapping
  )
{
  *DeviceAddress = (PHYSICAL_ADDRESS)(UINTN)HostAddress;
  *Mapping = NULL;
  return EFI_SUCCESS;
}


/**
  Completes the DmaMapBusMasterRead(), DmaMapBusMasterWrite(), or DmaMapBusMasterCommonBuffer()
  operation and releases any corresponding resources.

  @param  Mapping               The mapping value returned from DmaMap*().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.

**/
EFI_STATUS
EFIAPI
DmaUnmap (
  IN  VOID                         *Mapping
  )
{
  return EFI_SUCCESS;
}

/**
  Allocates pages that are suitable for an DmaMap() of type MapOperationBusMasterCommonBuffer.
  mapping.

  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.

                                @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
EFIAPI
DmaAllocateBuffer (
  IN  EFI_MEMORY_TYPE              MemoryType,
  IN  UINTN                        Pages,
  OUT VOID                         **HostAddress
  )
{
  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The only valid memory types are EfiBootServicesData and EfiRuntimeServicesData
  //
  // We used uncached memory to keep coherency
  //
  if (MemoryType == EfiBootServicesData) {
    *HostAddress = AllocatePages (Pages);
  } else if (MemoryType != EfiRuntimeServicesData) {
    *HostAddress = AllocateRuntimePages (Pages);
  } else {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}


/**
  Frees memory that was allocated with DmaAllocateBuffer().

  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with DmaAllocateBuffer().

**/
EFI_STATUS
EFIAPI
DmaFreeBuffer (
  IN  UINTN                        Pages,
  IN  VOID                         *HostAddress
  )
{
  if (HostAddress == NULL) {
     return EFI_INVALID_PARAMETER;
  }

  FreePages (HostAddress, Pages);
  return EFI_SUCCESS;
}

