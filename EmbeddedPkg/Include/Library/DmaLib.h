/** @file
  DMA abstraction library APIs. Based on UEFI PCI IO protocol DMA abstractions.
  At some point these functions will probably end up in a non PCI protocol
  for embedded systems.

  DMA Bus Master Read Operation:
    Call DmaMap() for MapOperationBusMasterRead.
    Program the DMA Bus Master with the DeviceAddress returned by DmaMap().
    Start the DMA Bus Master.
    Wait for DMA Bus Master to complete the read operation.
    Call DmaUnmap().

  DMA Bus Master Write Operation:
    Call DmaMap() for MapOperationBusMasterWrite.
    Program the DMA Bus Master with the DeviceAddress returned by DmaMap().
    Start the DMA Bus Master.
    Wait for DMA Bus Master to complete the write operation.
    Call DmaUnmap().

  DMA Bus Master Common Buffer Operation:
    Call DmaAllocateBuffer() to allocate a common buffer.
    Call DmaMap() for MapOperationBusMasterCommonBuffer.
    Program the DMA Bus Master with the DeviceAddress returned by DmaMap().
    The common buffer can now be accessed equally by the processor and the DMA bus master.
    Call DmaUnmap().
    Call DmaFreeBuffer().

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DMA_LIB_H__
#define __DMA_LIB_H__

typedef enum {
  ///
  /// A read operation from system memory by a bus master.
  ///
  MapOperationBusMasterRead,
  ///
  /// A write operation from system memory by a bus master.
  ///
  MapOperationBusMasterWrite,
  ///
  /// Provides both read and write access to system memory by both the processor and a
  /// bus master. The buffer is coherent from both the processor's and the bus master's point of view.
  ///
  MapOperationBusMasterCommonBuffer,
  MapOperationMaximum
} DMA_MAP_OPERATION;

/**
  Provides the DMA controller-specific addresses needed to access system memory.

  Operation is relative to the DMA bus master.

  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the DMA controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to DmaUnmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
EFI_STATUS
EFIAPI
DmaMap (
  IN     DMA_MAP_OPERATION  Operation,
  IN     VOID               *HostAddress,
  IN OUT UINTN              *NumberOfBytes,
  OUT    PHYSICAL_ADDRESS   *DeviceAddress,
  OUT    VOID               **Mapping
  );

/**
  Completes the DmaMapBusMasterRead, DmaMapBusMasterWrite, or DmaMapBusMasterCommonBuffer
  operation and releases any corresponding resources.

  @param  Mapping               The mapping value returned from DmaMap().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.

**/
EFI_STATUS
EFIAPI
DmaUnmap (
  IN  VOID  *Mapping
  );

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
  IN  EFI_MEMORY_TYPE  MemoryType,
  IN  UINTN            Pages,
  OUT VOID             **HostAddress
  );

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
  IN  UINTN  Pages,
  IN  VOID   *HostAddress
  );

/**
  Allocates pages that are suitable for an DmaMap() of type
  MapOperationBusMasterCommonBuffer mapping, at the requested alignment.

  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  Alignment             Alignment in bytes of the base of the returned
                                buffer (must be a power of 2)
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
DmaAllocateAlignedBuffer (
  IN  EFI_MEMORY_TYPE  MemoryType,
  IN  UINTN            Pages,
  IN  UINTN            Alignment,
  OUT VOID             **HostAddress
  );

#endif
