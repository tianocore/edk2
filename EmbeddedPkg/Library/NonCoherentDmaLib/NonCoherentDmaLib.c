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
  EFI_PHYSICAL_ADDRESS    HostAddress;
  VOID                    *BufferAddress;
  UINTN                   NumberOfBytes;
  DMA_MAP_OPERATION       Operation;
  BOOLEAN                 DoubleBuffer;
} MAP_INFO_INSTANCE;

typedef struct {
  LIST_ENTRY    Link;
  VOID          *HostAddress;
  UINTN         NumPages;
  UINT64        Attributes;
} UNCACHED_ALLOCATION;

STATIC EFI_CPU_ARCH_PROTOCOL  *mCpu;
STATIC LIST_ENTRY             UncachedAllocationList;

STATIC PHYSICAL_ADDRESS  mDmaHostAddressLimit;

STATIC
PHYSICAL_ADDRESS
HostToDeviceAddress (
  IN  VOID  *Address
  )
{
  return (PHYSICAL_ADDRESS)(UINTN)Address + PcdGet64 (PcdDmaDeviceOffset);
}

/**
  Allocates one or more 4KB pages of a certain memory type at a specified
  alignment.

  Allocates the number of 4KB pages specified by Pages of a certain memory type
  with an alignment specified by Alignment. The allocated buffer is returned.
  If Pages is 0, then NULL is returned. If there is not enough memory at the
  specified alignment remaining to satisfy the request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().
  If Pages plus EFI_SIZE_TO_PAGES (Alignment) overflows, then ASSERT().

  @param  MemoryType            The type of memory to allocate.
  @param  Pages                 The number of 4 KB pages to allocate.
  @param  Alignment             The requested alignment of the allocation.
                                Must be a power of two.
                                If Alignment is zero, then byte alignment is
                                used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
STATIC
VOID *
InternalAllocateAlignedPages (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            Pages,
  IN UINTN            Alignment
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory;
  UINTN                 AlignedMemory;
  UINTN                 AlignmentMask;
  UINTN                 UnalignedPages;
  UINTN                 RealPages;

  //
  // Alignment must be a power of two or zero.
  //
  ASSERT ((Alignment & (Alignment - 1)) == 0);

  if (Pages == 0) {
    return NULL;
  }

  if (Alignment > EFI_PAGE_SIZE) {
    //
    // Calculate the total number of pages since alignment is larger than page
    // size.
    //
    AlignmentMask = Alignment - 1;
    RealPages     = Pages + EFI_SIZE_TO_PAGES (Alignment);
    //
    // Make sure that Pages plus EFI_SIZE_TO_PAGES (Alignment) does not
    // overflow.
    //
    ASSERT (RealPages > Pages);

    Memory = mDmaHostAddressLimit;
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    MemoryType,
                    RealPages,
                    &Memory
                    );
    if (EFI_ERROR (Status)) {
      return NULL;
    }

    AlignedMemory  = ((UINTN)Memory + AlignmentMask) & ~AlignmentMask;
    UnalignedPages = EFI_SIZE_TO_PAGES (AlignedMemory - (UINTN)Memory);
    if (UnalignedPages > 0) {
      //
      // Free first unaligned page(s).
      //
      Status = gBS->FreePages (Memory, UnalignedPages);
      ASSERT_EFI_ERROR (Status);
    }

    Memory         = AlignedMemory + EFI_PAGES_TO_SIZE (Pages);
    UnalignedPages = RealPages - Pages - UnalignedPages;
    if (UnalignedPages > 0) {
      //
      // Free last unaligned page(s).
      //
      Status = gBS->FreePages (Memory, UnalignedPages);
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    //
    // Do not over-allocate pages in this case.
    //
    Memory = mDmaHostAddressLimit;
    Status = gBS->AllocatePages (
                    AllocateMaxAddress,
                    MemoryType,
                    Pages,
                    &Memory
                    );
    if (EFI_ERROR (Status)) {
      return NULL;
    }

    AlignedMemory = (UINTN)Memory;
  }

  return (VOID *)AlignedMemory;
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
  IN     DMA_MAP_OPERATION  Operation,
  IN     VOID               *HostAddress,
  IN OUT UINTN              *NumberOfBytes,
  OUT    PHYSICAL_ADDRESS   *DeviceAddress,
  OUT    VOID               **Mapping
  )
{
  EFI_STATUS                       Status;
  MAP_INFO_INSTANCE                *Map;
  VOID                             *Buffer;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  GcdDescriptor;
  UINTN                            AllocSize;

  if ((HostAddress == NULL) ||
      (NumberOfBytes == NULL) ||
      (DeviceAddress == NULL) ||
      (Mapping == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Operation >= MapOperationMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  *DeviceAddress = HostToDeviceAddress (HostAddress);

  // Remember range so we can flush on the other side
  Map = AllocatePool (sizeof (MAP_INFO_INSTANCE));
  if (Map == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (((UINTN)HostAddress + *NumberOfBytes) > mDmaHostAddressLimit) {
    if (Operation == MapOperationBusMasterCommonBuffer) {
      goto CommonBufferError;
    }

    AllocSize          = ALIGN_VALUE (*NumberOfBytes, mCpu->DmaBufferAlignment);
    Map->BufferAddress = InternalAllocateAlignedPages (
                           EfiBootServicesData,
                           EFI_SIZE_TO_PAGES (AllocSize),
                           mCpu->DmaBufferAlignment
                           );
    if (Map->BufferAddress == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto FreeMapInfo;
    }

    if (Operation == MapOperationBusMasterRead) {
      CopyMem (Map->BufferAddress, (VOID *)(UINTN)HostAddress, *NumberOfBytes);
    }

    mCpu->FlushDataCache (
            mCpu,
            (UINTN)Map->BufferAddress,
            AllocSize,
            EfiCpuFlushTypeWriteBack
            );

    *DeviceAddress = HostToDeviceAddress (Map->BufferAddress);
  } else if ((Operation != MapOperationBusMasterRead) &&
             ((((UINTN)HostAddress & (mCpu->DmaBufferAlignment - 1)) != 0) ||
              ((*NumberOfBytes & (mCpu->DmaBufferAlignment - 1)) != 0)))
  {
    // Get the cacheability of the region
    Status = gDS->GetMemorySpaceDescriptor ((UINTN)HostAddress, &GcdDescriptor);
    if (EFI_ERROR (Status)) {
      goto FreeMapInfo;
    }

    // If the mapped buffer is not an uncached buffer
    if ((GcdDescriptor.Attributes & (EFI_MEMORY_WB | EFI_MEMORY_WT)) != 0) {
      //
      // Operations of type MapOperationBusMasterCommonBuffer are only allowed
      // on uncached buffers.
      //
      if (Operation == MapOperationBusMasterCommonBuffer) {
        goto CommonBufferError;
      }

      //
      // If the buffer does not fill entire cache lines we must double buffer
      // into a suitably aligned allocation that allows us to invalidate the
      // cache without running the risk of corrupting adjacent unrelated data.
      // Note that pool allocations are guaranteed to be 8 byte aligned, so
      // we only have to add (alignment - 8) worth of padding.
      //
      Map->DoubleBuffer = TRUE;
      AllocSize         = ALIGN_VALUE (*NumberOfBytes, mCpu->DmaBufferAlignment) +
                          (mCpu->DmaBufferAlignment - 8);
      Map->BufferAddress = AllocatePool (AllocSize);
      if (Map->BufferAddress == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto FreeMapInfo;
      }

      Buffer         = ALIGN_POINTER (Map->BufferAddress, mCpu->DmaBufferAlignment);
      *DeviceAddress = HostToDeviceAddress (Buffer);

      //
      // Get rid of any dirty cachelines covering the double buffer. This
      // prevents them from being written back unexpectedly, potentially
      // overwriting the data we receive from the device.
      //
      mCpu->FlushDataCache (
              mCpu,
              (UINTN)Buffer,
              *NumberOfBytes,
              EfiCpuFlushTypeWriteBack
              );
    } else {
      Map->DoubleBuffer = FALSE;
    }
  } else {
    Map->DoubleBuffer = FALSE;

    DEBUG_CODE_BEGIN ();

    //
    // The operation type check above only executes if the buffer happens to be
    // misaligned with respect to CWG, but even if it is aligned, we should not
    // allow arbitrary buffers to be used for creating consistent mappings.
    // So duplicate the check here when running in DEBUG mode, just to assert
    // that we are not trying to create a consistent mapping for cached memory.
    //
    Status = gDS->GetMemorySpaceDescriptor ((UINTN)HostAddress, &GcdDescriptor);
    ASSERT_EFI_ERROR (Status);

    ASSERT (
      Operation != MapOperationBusMasterCommonBuffer ||
      (GcdDescriptor.Attributes & (EFI_MEMORY_WB | EFI_MEMORY_WT)) == 0
      );

    DEBUG_CODE_END ();

    // Flush the Data Cache (should not have any effect if the memory region is
    // uncached)
    mCpu->FlushDataCache (
            mCpu,
            (UINTN)HostAddress,
            *NumberOfBytes,
            EfiCpuFlushTypeWriteBackInvalidate
            );
  }

  Map->HostAddress   = (UINTN)HostAddress;
  Map->NumberOfBytes = *NumberOfBytes;
  Map->Operation     = Operation;

  *Mapping = Map;

  return EFI_SUCCESS;

CommonBufferError:
  DEBUG ((
    DEBUG_ERROR,
    "%a: Operation type 'MapOperationBusMasterCommonBuffer' is only "
    "supported\non memory regions that were allocated using "
    "DmaAllocateBuffer ()\n",
    __func__
    ));
  Status = EFI_UNSUPPORTED;
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
  IN  VOID  *Mapping
  )
{
  MAP_INFO_INSTANCE  *Map;
  EFI_STATUS         Status;
  VOID               *Buffer;
  UINTN              AllocSize;

  if (Mapping == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Map = (MAP_INFO_INSTANCE *)Mapping;

  Status = EFI_SUCCESS;
  if (((UINTN)Map->HostAddress + Map->NumberOfBytes) > mDmaHostAddressLimit) {
    AllocSize = ALIGN_VALUE (Map->NumberOfBytes, mCpu->DmaBufferAlignment);
    if (Map->Operation == MapOperationBusMasterWrite) {
      mCpu->FlushDataCache (
              mCpu,
              (UINTN)Map->BufferAddress,
              AllocSize,
              EfiCpuFlushTypeInvalidate
              );
      CopyMem (
        (VOID *)(UINTN)Map->HostAddress,
        Map->BufferAddress,
        Map->NumberOfBytes
        );
    }

    FreePages (Map->BufferAddress, EFI_SIZE_TO_PAGES (AllocSize));
  } else if (Map->DoubleBuffer) {
    ASSERT (Map->Operation == MapOperationBusMasterWrite);

    if (Map->Operation != MapOperationBusMasterWrite) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      Buffer = ALIGN_POINTER (Map->BufferAddress, mCpu->DmaBufferAlignment);

      mCpu->FlushDataCache (
              mCpu,
              (UINTN)Buffer,
              Map->NumberOfBytes,
              EfiCpuFlushTypeInvalidate
              );

      CopyMem ((VOID *)(UINTN)Map->HostAddress, Buffer, Map->NumberOfBytes);

      FreePool (Map->BufferAddress);
    }
  } else {
    if (Map->Operation == MapOperationBusMasterWrite) {
      //
      // Make sure we read buffer from uncached memory and not the cache
      //
      mCpu->FlushDataCache (
              mCpu,
              Map->HostAddress,
              Map->NumberOfBytes,
              EfiCpuFlushTypeInvalidate
              );
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
  IN  EFI_MEMORY_TYPE  MemoryType,
  IN  UINTN            Pages,
  OUT VOID             **HostAddress
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
  IN  EFI_MEMORY_TYPE  MemoryType,
  IN  UINTN            Pages,
  IN  UINTN            Alignment,
  OUT VOID             **HostAddress
  )
{
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  GcdDescriptor;
  VOID                             *Allocation;
  UINT64                           Attributes;
  UNCACHED_ALLOCATION              *Alloc;
  EFI_STATUS                       Status;

  Attributes = EFI_MEMORY_XP;

  if (Alignment == 0) {
    Alignment = EFI_PAGE_SIZE;
  }

  if ((HostAddress == NULL) ||
      ((Alignment & (Alignment - 1)) != 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((MemoryType == EfiBootServicesData) ||
      (MemoryType == EfiRuntimeServicesData))
  {
    Allocation = InternalAllocateAlignedPages (MemoryType, Pages, Alignment);
  } else {
    return EFI_INVALID_PARAMETER;
  }

  if (Allocation == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Get the cacheability of the region
  Status = gDS->GetMemorySpaceDescriptor ((UINTN)Allocation, &GcdDescriptor);
  if (EFI_ERROR (Status)) {
    goto FreeBuffer;
  }

  // Choose a suitable uncached memory type that is supported by the region
  if (GcdDescriptor.Capabilities & EFI_MEMORY_WC) {
    Attributes |= EFI_MEMORY_WC;
  } else if (GcdDescriptor.Capabilities & EFI_MEMORY_UC) {
    Attributes |= EFI_MEMORY_UC;
  } else {
    Status = EFI_UNSUPPORTED;
    goto FreeBuffer;
  }

  Alloc = AllocatePool (sizeof *Alloc);
  if (Alloc == NULL) {
    goto FreeBuffer;
  }

  Alloc->HostAddress = Allocation;
  Alloc->NumPages    = Pages;
  Alloc->Attributes  = GcdDescriptor.Attributes;

  InsertHeadList (&UncachedAllocationList, &Alloc->Link);

  // Ensure that EFI_MEMORY_XP is in the capability set
  if ((GcdDescriptor.Capabilities & EFI_MEMORY_XP) != EFI_MEMORY_XP) {
    Status = gDS->SetMemorySpaceCapabilities (
                    (PHYSICAL_ADDRESS)(UINTN)Allocation,
                    EFI_PAGES_TO_SIZE (Pages),
                    GcdDescriptor.Capabilities | EFI_MEMORY_XP
                    );

    // if we were to fail setting the capability, this would indicate an internal failure of the GCD code. We should
    // assert here to let a platform know something went crazy, but for a release build we can let the allocation occur
    // without the EFI_MEMORY_XP bit set, as that was the existing behavior
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a failed to set EFI_MEMORY_XP capability on 0x%llx for length 0x%llx. Attempting to allocate without XP set.\n",
        __func__,
        Allocation,
        EFI_PAGES_TO_SIZE (Pages)
        ));

      ASSERT_EFI_ERROR (Status);

      Attributes &= ~EFI_MEMORY_XP;
    }
  }

  // Remap the region with the new attributes and mark it non-executable
  Status = gDS->SetMemorySpaceAttributes (
                  (PHYSICAL_ADDRESS)(UINTN)Allocation,
                  EFI_PAGES_TO_SIZE (Pages),
                  Attributes
                  );
  if (EFI_ERROR (Status)) {
    goto FreeAlloc;
  }

  Status = mCpu->FlushDataCache (
                   mCpu,
                   (PHYSICAL_ADDRESS)(UINTN)Allocation,
                   EFI_PAGES_TO_SIZE (Pages),
                   EfiCpuFlushTypeInvalidate
                   );
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
  IN  UINTN  Pages,
  IN  VOID   *HostAddress
  )
{
  LIST_ENTRY           *Link;
  UNCACHED_ALLOCATION  *Alloc;
  BOOLEAN              Found;
  EFI_STATUS           Status;

  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Link = GetFirstNode (&UncachedAllocationList), Found = FALSE;
       !IsNull (&UncachedAllocationList, Link);
       Link = GetNextNode (&UncachedAllocationList, Link))
  {
    Alloc = BASE_CR (Link, UNCACHED_ALLOCATION, Link);
    if ((Alloc->HostAddress == HostAddress) && (Alloc->NumPages == Pages)) {
      Found = TRUE;
      break;
    }
  }

  if (!Found) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  RemoveEntryList (&Alloc->Link);

  Status = gDS->SetMemorySpaceAttributes (
                  (PHYSICAL_ADDRESS)(UINTN)HostAddress,
                  EFI_PAGES_TO_SIZE (Pages),
                  Alloc->Attributes
                  );
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  InitializeListHead (&UncachedAllocationList);

  //
  // Ensure that the combination of DMA addressing offset and limit produces
  // a sane value.
  //
  ASSERT (PcdGet64 (PcdDmaDeviceLimit) > PcdGet64 (PcdDmaDeviceOffset));

  mDmaHostAddressLimit = PcdGet64 (PcdDmaDeviceLimit) -
                         PcdGet64 (PcdDmaDeviceOffset);

  // Get the Cpu protocol for later use
  return gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&mCpu);
}
