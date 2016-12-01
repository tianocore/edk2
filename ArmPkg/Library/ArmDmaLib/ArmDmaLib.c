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

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/DmaLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UncachedMemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/Cpu.h>

typedef struct {
  EFI_PHYSICAL_ADDRESS      HostAddress;
  VOID                      *BufferAddress;
  UINTN                     NumberOfBytes;
  DMA_MAP_OPERATION         Operation;
  BOOLEAN                   DoubleBuffer;
} MAP_INFO_INSTANCE;



STATIC EFI_CPU_ARCH_PROTOCOL      *mCpu;

STATIC
PHYSICAL_ADDRESS
HostToDeviceAddress (
  IN  PHYSICAL_ADDRESS  HostAddress
  )
{
  return HostAddress + PcdGet64 (PcdArmDmaDeviceOffset);
}

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
  EFI_STATUS                      Status;
  MAP_INFO_INSTANCE               *Map;
  VOID                            *Buffer;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR GcdDescriptor;

  if (HostAddress == NULL || NumberOfBytes == NULL || DeviceAddress == NULL || Mapping == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  if (Operation >= MapOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The debug implementation of UncachedMemoryAllocationLib in ArmPkg returns
  // a virtual uncached alias, and unmaps the cached ID mapping of the buffer,
  // in order to catch inadvertent references to the cached mapping.
  // Since HostToDeviceAddress () expects ID mapped input addresses, convert
  // the host address to an ID mapped address first.
  //
  *DeviceAddress = HostToDeviceAddress (ConvertToPhysicalAddress (HostAddress));

  // Remember range so we can flush on the other side
  Map = AllocatePool (sizeof (MAP_INFO_INSTANCE));
  if (Map == NULL) {
    return  EFI_OUT_OF_RESOURCES;
  }

  if ((((UINTN)HostAddress & (mCpu->DmaBufferAlignment - 1)) != 0) ||
      ((*NumberOfBytes & (mCpu->DmaBufferAlignment - 1)) != 0)) {

    // Get the cacheability of the region
    Status = gDS->GetMemorySpaceDescriptor ((UINTN)HostAddress, &GcdDescriptor);
    if (EFI_ERROR(Status)) {
      goto FreeMapInfo;
    }

    // If the mapped buffer is not an uncached buffer
    if ((GcdDescriptor.Attributes & (EFI_MEMORY_WB | EFI_MEMORY_WT)) != 0) {
      //
      // Operations of type MapOperationBusMasterCommonBuffer are only allowed
      // on uncached buffers.
      //
      if (Operation == MapOperationBusMasterCommonBuffer) {
        DEBUG ((EFI_D_ERROR,
          "%a: Operation type 'MapOperationBusMasterCommonBuffer' is only supported\n"
          "on memory regions that were allocated using DmaAllocateBuffer ()\n",
          __FUNCTION__));
        Status = EFI_UNSUPPORTED;
        goto FreeMapInfo;
      }

      //
      // If the buffer does not fill entire cache lines we must double buffer into
      // uncached memory. Device (PCI) address becomes uncached page.
      //
      Map->DoubleBuffer  = TRUE;
      Status = DmaAllocateBuffer (EfiBootServicesData, EFI_SIZE_TO_PAGES (*NumberOfBytes), &Buffer);
      if (EFI_ERROR (Status)) {
        goto FreeMapInfo;
      }

      if (Operation == MapOperationBusMasterRead) {
        CopyMem (Buffer, HostAddress, *NumberOfBytes);
      }

      *DeviceAddress = HostToDeviceAddress (ConvertToPhysicalAddress (Buffer));
      Map->BufferAddress = Buffer;
    } else {
      Map->DoubleBuffer  = FALSE;
    }
  } else {
    Map->DoubleBuffer  = FALSE;

    DEBUG_CODE_BEGIN ();

    //
    // The operation type check above only executes if the buffer happens to be
    // misaligned with respect to CWG, but even if it is aligned, we should not
    // allow arbitrary buffers to be used for creating consistent mappings.
    // So duplicate the check here when running in DEBUG mode, just to assert
    // that we are not trying to create a consistent mapping for cached memory.
    //
    Status = gDS->GetMemorySpaceDescriptor ((UINTN)HostAddress, &GcdDescriptor);
    ASSERT_EFI_ERROR(Status);

    ASSERT (Operation != MapOperationBusMasterCommonBuffer ||
            (GcdDescriptor.Attributes & (EFI_MEMORY_WB | EFI_MEMORY_WT)) == 0);

    DEBUG_CODE_END ();

    // Flush the Data Cache (should not have any effect if the memory region is uncached)
    mCpu->FlushDataCache (mCpu, (UINTN)HostAddress, *NumberOfBytes,
            EfiCpuFlushTypeWriteBackInvalidate);
  }

  Map->HostAddress   = (UINTN)HostAddress;
  Map->NumberOfBytes = *NumberOfBytes;
  Map->Operation     = Operation;

  *Mapping = Map;

  return EFI_SUCCESS;

FreeMapInfo:
  FreePool (Map);

  return Status;
}


/**
  Completes the DmaMapBusMasterRead(), DmaMapBusMasterWrite(), or DmaMapBusMasterCommonBuffer()
  operation and releases any corresponding resources.

  @param  Mapping               The mapping value returned from DmaMap*().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.
  @retval EFI_INVALID_PARAMETER An inconsistency was detected between the mapping type
                                and the DoubleBuffer field

**/
EFI_STATUS
EFIAPI
DmaUnmap (
  IN  VOID                         *Mapping
  )
{
  MAP_INFO_INSTANCE *Map;
  EFI_STATUS        Status;

  if (Mapping == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Map = (MAP_INFO_INSTANCE *)Mapping;

  Status = EFI_SUCCESS;
  if (Map->DoubleBuffer) {
    ASSERT (Map->Operation != MapOperationBusMasterCommonBuffer);

    if (Map->Operation == MapOperationBusMasterCommonBuffer) {
      Status = EFI_INVALID_PARAMETER;
    } else if (Map->Operation == MapOperationBusMasterWrite) {
      CopyMem ((VOID *)(UINTN)Map->HostAddress, Map->BufferAddress,
        Map->NumberOfBytes);
    }

    DmaFreeBuffer (EFI_SIZE_TO_PAGES (Map->NumberOfBytes), Map->BufferAddress);

  } else {
    if (Map->Operation == MapOperationBusMasterWrite) {
      //
      // Make sure we read buffer from uncached memory and not the cache
      //
      mCpu->FlushDataCache (mCpu, Map->HostAddress, Map->NumberOfBytes,
              EfiCpuFlushTypeInvalidate);
    }
  }

  FreePool (Map);

  return Status;
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
  VOID    *Allocation;

  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The only valid memory types are EfiBootServicesData and EfiRuntimeServicesData
  //
  // We used uncached memory to keep coherency
  //
  if (MemoryType == EfiBootServicesData) {
    Allocation = UncachedAllocatePages (Pages);
  } else if (MemoryType == EfiRuntimeServicesData) {
    Allocation = UncachedAllocateRuntimePages (Pages);
  } else {
    return EFI_INVALID_PARAMETER;
  }

  if (Allocation == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *HostAddress = Allocation;

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

  UncachedFreePages (HostAddress, Pages);
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
ArmDmaLibConstructor (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS              Status;

  // Get the Cpu protocol for later use
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&mCpu);
  ASSERT_EFI_ERROR(Status);

  return Status;
}

