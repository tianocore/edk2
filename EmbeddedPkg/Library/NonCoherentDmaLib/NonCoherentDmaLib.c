/** @file

  Generic non-coherent implementation of DmaLib.h

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2015 - 2017, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DmaLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
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


typedef struct {
  LIST_ENTRY          Link;
  VOID                *HostAddress;
  UINTN               NumPages;
  UINT64              Attributes;
} UNCACHED_ALLOCATION;

STATIC EFI_CPU_ARCH_PROTOCOL      *mCpu;
STATIC LIST_ENTRY                 UncachedAllocationList;

STATIC
PHYSICAL_ADDRESS
HostToDeviceAddress (
  IN  VOID      *Address
  )
{
  return (PHYSICAL_ADDRESS)(UINTN)Address + PcdGet64 (PcdDmaDeviceOffset);
}

/**
  Provides the DMA controller-specific addresses needed to access system memory.

  Operation is relative to the DMA bus master.

  @param  Operation             Indicates if the bus master is going to read or
                                write to system memory.
  @param  HostAddress           The system memory address to map to the DMA
                                controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output
                                the number of bytes that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master
                                controller to use to access the host's
                                HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned
                                NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common
                                buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested
                                address.

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
  UINTN                           AllocSize;

  if (HostAddress == NULL ||
      NumberOfBytes == NULL ||
      DeviceAddress == NULL ||
      Mapping == NULL ) {
    return EFI_INVALID_PARAMETER;
  }

  if (Operation >= MapOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  *DeviceAddress = HostToDeviceAddress (HostAddress);

  // Remember range so we can flush on the other side
  Map = AllocatePool (sizeof (MAP_INFO_INSTANCE));
  if (Map == NULL) {
    return  EFI_OUT_OF_RESOURCES;
  }

  if (Operation != MapOperationBusMasterRead &&
      ((((UINTN)HostAddress & (mCpu->DmaBufferAlignment - 1)) != 0) ||
       ((*NumberOfBytes & (mCpu->DmaBufferAlignment - 1)) != 0))) {

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
        DEBUG ((DEBUG_ERROR,
          "%a: Operation type 'MapOperationBusMasterCommonBuffer' is only "
          "supported\non memory regions that were allocated using "
          "DmaAllocateBuffer ()\n", __FUNCTION__));
        Status = EFI_UNSUPPORTED;
        goto FreeMapInfo;
      }

      //
      // If the buffer does not fill entire cache lines we must double buffer
      // into a suitably aligned allocation that allows us to invalidate the
      // cache without running the risk of corrupting adjacent unrelated data.
      // Note that pool allocations are guaranteed to be 8 byte aligned, so
      // we only have to add (alignment - 8) worth of padding.
      //
      Map->DoubleBuffer = TRUE;
      AllocSize = ALIGN_VALUE (*NumberOfBytes, mCpu->DmaBufferAlignment) +
                  (mCpu->DmaBufferAlignment - 8);
      Map->BufferAddress = AllocatePool (AllocSize);
      if (Map->BufferAddress == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto FreeMapInfo;
      }

      Buffer = ALIGN_POINTER (Map->BufferAddress, mCpu->DmaBufferAlignment);
      *DeviceAddress = HostToDeviceAddress (Buffer);

      //
      // Get rid of any dirty cachelines covering the double buffer. This
      // prevents them from being written back unexpectedly, potentially
      // overwriting the data we receive from the device.
      //
      mCpu->FlushDataCache (mCpu, (UINTN)Buffer, *NumberOfBytes,
              EfiCpuFlushTypeWriteBack);
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

    // Flush the Data Cache (should not have any effect if the memory region is
    // uncached)
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
  Completes the DmaMapBusMasterRead(), DmaMapBusMasterWrite(), or
  DmaMapBusMasterCommonBuffer() operation and releases any corresponding
  resources.

  @param  Mapping               The mapping value returned from DmaMap*().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system
                                memory.
  @retval EFI_INVALID_PARAMETER An inconsistency was detected between the
                                mapping type and the DoubleBuffer field

**/
EFI_STATUS
EFIAPI
DmaUnmap (
  IN  VOID                         *Mapping
  )
{
  MAP_INFO_INSTANCE *Map;
  EFI_STATUS        Status;
  VOID              *Buffer;

  if (Mapping == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Map = (MAP_INFO_INSTANCE *)Mapping;

  Status = EFI_SUCCESS;
  if (Map->DoubleBuffer) {
    ASSERT (Map->Operation == MapOperationBusMasterWrite);

    if (Map->Operation != MapOperationBusMasterWrite) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      Buffer = ALIGN_POINTER (Map->BufferAddress, mCpu->DmaBufferAlignment);

      mCpu->FlushDataCache (mCpu, (UINTN)Buffer, Map->NumberOfBytes,
              EfiCpuFlushTypeInvalidate);

      CopyMem ((VOID *)(UINTN)Map->HostAddress, Buffer, Map->NumberOfBytes);

      FreePool (Map->BufferAddress);
    }
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
  Allocates pages that are suitable for an DmaMap() of type
  MapOperationBusMasterCommonBuffer mapping.

  @param  MemoryType            The type of memory to allocate,
                                EfiBootServicesData or EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory
                                address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
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
  return DmaAllocateAlignedBuffer (MemoryType, Pages, 0, HostAddress);
}

/**
  Allocates pages that are suitable for an DmaMap() of type
  MapOperationBusMasterCommonBuffer mapping, at the requested alignment.

  @param  MemoryType            The type of memory to allocate,
                                EfiBootServicesData or EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  Alignment             Alignment in bytes of the base of the returned
                                buffer (must be a power of 2)
  @param  HostAddress           A pointer to store the base system memory
                                address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
EFIAPI
DmaAllocateAlignedBuffer (
  IN  EFI_MEMORY_TYPE              MemoryType,
  IN  UINTN                        Pages,
  IN  UINTN                        Alignment,
  OUT VOID                         **HostAddress
  )
{
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR   GcdDescriptor;
  VOID                              *Allocation;
  UINT64                            MemType;
  UNCACHED_ALLOCATION               *Alloc;
  EFI_STATUS                        Status;

  if (Alignment == 0) {
    Alignment = EFI_PAGE_SIZE;
  }

  if (HostAddress == NULL ||
      (Alignment & (Alignment - 1)) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (MemoryType == EfiBootServicesData) {
    Allocation = AllocateAlignedPages (Pages, Alignment);
  } else if (MemoryType == EfiRuntimeServicesData) {
    Allocation = AllocateAlignedRuntimePages (Pages, Alignment);
  } else {
    return EFI_INVALID_PARAMETER;
  }

  if (Allocation == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Get the cacheability of the region
  Status = gDS->GetMemorySpaceDescriptor ((UINTN)Allocation, &GcdDescriptor);
  if (EFI_ERROR(Status)) {
    goto FreeBuffer;
  }

  // Choose a suitable uncached memory type that is supported by the region
  if (GcdDescriptor.Capabilities & EFI_MEMORY_WC) {
    MemType = EFI_MEMORY_WC;
  } else if (GcdDescriptor.Capabilities & EFI_MEMORY_UC) {
    MemType = EFI_MEMORY_UC;
  } else {
    Status = EFI_UNSUPPORTED;
    goto FreeBuffer;
  }

  Alloc = AllocatePool (sizeof *Alloc);
  if (Alloc == NULL) {
    goto FreeBuffer;
  }

  Alloc->HostAddress = Allocation;
  Alloc->NumPages = Pages;
  Alloc->Attributes = GcdDescriptor.Attributes;

  InsertHeadList (&UncachedAllocationList, &Alloc->Link);

  // Remap the region with the new attributes
  Status = gDS->SetMemorySpaceAttributes ((PHYSICAL_ADDRESS)(UINTN)Allocation,
                                          EFI_PAGES_TO_SIZE (Pages),
                                          MemType);
  if (EFI_ERROR (Status)) {
    goto FreeAlloc;
  }

  Status = mCpu->FlushDataCache (mCpu,
                                 (PHYSICAL_ADDRESS)(UINTN)Allocation,
                                 EFI_PAGES_TO_SIZE (Pages),
                                 EfiCpuFlushTypeInvalidate);
  if (EFI_ERROR (Status)) {
    goto FreeAlloc;
  }

  *HostAddress = Allocation;

  return EFI_SUCCESS;

FreeAlloc:
  RemoveEntryList (&Alloc->Link);
  FreePool (Alloc);

FreeBuffer:
  FreePages (Allocation, Pages);
  return Status;
}


/**
  Frees memory that was allocated with DmaAllocateBuffer().

  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated
                                range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and
                                Pages was not allocated with
                                DmaAllocateBuffer().

**/
EFI_STATUS
EFIAPI
DmaFreeBuffer (
  IN  UINTN                        Pages,
  IN  VOID                         *HostAddress
  )
{
  LIST_ENTRY                       *Link;
  UNCACHED_ALLOCATION              *Alloc;
  BOOLEAN                          Found;
  EFI_STATUS                       Status;

  if (HostAddress == NULL) {
     return EFI_INVALID_PARAMETER;
  }

  for (Link = GetFirstNode (&UncachedAllocationList), Found = FALSE;
       !IsNull (&UncachedAllocationList, Link);
       Link = GetNextNode (&UncachedAllocationList, Link)) {

    Alloc = BASE_CR (Link, UNCACHED_ALLOCATION, Link);
    if (Alloc->HostAddress == HostAddress && Alloc->NumPages == Pages) {
      Found = TRUE;
      break;
    }
  }

  if (!Found) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  RemoveEntryList (&Alloc->Link);

  Status = gDS->SetMemorySpaceAttributes ((PHYSICAL_ADDRESS)(UINTN)HostAddress,
                                          EFI_PAGES_TO_SIZE (Pages),
                                          Alloc->Attributes);
  if (EFI_ERROR (Status)) {
    goto FreeAlloc;
  }

  //
  // If we fail to restore the original attributes, it is better to leak the
  // memory than to return it to the heap
  //
  FreePages (HostAddress, Pages);

FreeAlloc:
  FreePool (Alloc);
  return Status;
}


EFI_STATUS
EFIAPI
NonCoherentDmaLibConstructor (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  InitializeListHead (&UncachedAllocationList);

  // Get the Cpu protocol for later use
  return gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&mCpu);
}
