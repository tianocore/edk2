/** @file
  Debug version of the UncachedMemoryAllocation lib that uses the VirtualUncachedPages
  protocol, produced by the DXE CPU driver, to produce debuggable uncached memory buffers.

  The DMA rules for EFI contain the concept of a PCI (DMA master) address for memory and
  a CPU (C code) address for the memory buffer that don't have to be the same.  There seem to
  be common errors out there with folks mixing up the two addresses.  This library causes
  the PCI (DMA master) address to not be mapped into system memory so if the CPU (C code)
  uses the wrong pointer it will generate a page fault. The CPU (C code) version of the buffer
  has a virtual address that does not match the physical address. The virtual address has
  PcdArmUncachedMemoryMask ored into the physical address.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UncachedMemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ArmLib.h>

#include <Protocol/Cpu.h>
#include <Protocol/VirtualUncachedPages.h>

VOID *
UncachedInternalAllocatePages (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            Pages
  );

VOID *
UncachedInternalAllocateAlignedPages (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            Pages,
  IN UINTN            Alignment
  );



EFI_CPU_ARCH_PROTOCOL           *gDebugUncachedCpu;
VIRTUAL_UNCACHED_PAGES_PROTOCOL *gVirtualUncachedPages;

//
// Assume all of memory has the same cache attributes, unless we do our magic
//
UINT64  gAttributes;

typedef struct {
  VOID        *Buffer;
  VOID        *Allocation;
  UINTN       Pages;
  LIST_ENTRY  Link;
} FREE_PAGE_NODE;

LIST_ENTRY  mPageList = INITIALIZE_LIST_HEAD_VARIABLE (mPageList);

VOID
AddPagesToList (
  IN VOID   *Buffer,
  IN VOID   *Allocation,
  UINTN     Pages
  )
{
  FREE_PAGE_NODE  *NewNode;

  NewNode = AllocatePool (sizeof (LIST_ENTRY));
  if (NewNode == NULL) {
    ASSERT (FALSE);
    return;
  }

  NewNode->Buffer     = Buffer;
  NewNode->Allocation = Allocation;
  NewNode->Pages      = Pages;

  InsertTailList (&mPageList, &NewNode->Link);
}


VOID
RemovePagesFromList (
  IN VOID   *Buffer,
  OUT VOID  **Allocation,
  OUT UINTN *Pages
  )
{
  LIST_ENTRY      *Link;
  FREE_PAGE_NODE  *OldNode;

  *Allocation = NULL;
  *Pages = 0;

  for (Link = mPageList.ForwardLink; Link != &mPageList; Link = Link->ForwardLink) {
    OldNode = BASE_CR (Link, FREE_PAGE_NODE, Link);
    if (OldNode->Buffer == Buffer) {
      *Allocation = OldNode->Allocation;
      *Pages = OldNode->Pages;

      RemoveEntryList (&OldNode->Link);
      FreePool (OldNode);
      return;
    }
  }

  return;
}



EFI_PHYSICAL_ADDRESS
ConvertToPhysicalAddress (
  IN VOID *VirtualAddress
  )
{
  UINTN UncachedMemoryMask = (UINTN)PcdGet64 (PcdArmUncachedMemoryMask);
  UINTN PhysicalAddress;

  PhysicalAddress = (UINTN)VirtualAddress & ~UncachedMemoryMask;

  return (EFI_PHYSICAL_ADDRESS)PhysicalAddress;
}


VOID *
ConvertToUncachedAddress (
  IN VOID *Address
  )
{
  UINTN UncachedMemoryMask = (UINTN)PcdGet64 (PcdArmUncachedMemoryMask);
  UINTN UncachedAddress;

  UncachedAddress = (UINTN)Address | UncachedMemoryMask;

  return (VOID *)UncachedAddress;
}



VOID *
UncachedInternalAllocatePages (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            Pages
  )
{
  return UncachedInternalAllocateAlignedPages (MemoryType, Pages, EFI_PAGE_SIZE);
}


VOID *
EFIAPI
UncachedAllocatePages (
  IN UINTN  Pages
  )
{
  return UncachedInternalAllocatePages (EfiBootServicesData, Pages);
}

VOID *
EFIAPI
UncachedAllocateRuntimePages (
  IN UINTN  Pages
  )
{
  return UncachedInternalAllocatePages (EfiRuntimeServicesData, Pages);
}

VOID *
EFIAPI
UncachedAllocateReservedPages (
  IN UINTN  Pages
  )
{
  return UncachedInternalAllocatePages (EfiReservedMemoryType, Pages);
}



VOID
EFIAPI
UncachedFreePages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  UncachedFreeAlignedPages (Buffer, Pages);
  return;
}


VOID *
UncachedInternalAllocateAlignedPages (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            Pages,
  IN UINTN            Alignment
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory;
  EFI_PHYSICAL_ADDRESS  AlignedMemory;
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
    // Caculate the total number of pages since alignment is larger than page size.
    //
    AlignmentMask  = Alignment - 1;
    RealPages      = Pages + EFI_SIZE_TO_PAGES (Alignment);
    //
    // Make sure that Pages plus EFI_SIZE_TO_PAGES (Alignment) does not overflow.
    //
    ASSERT (RealPages > Pages);

    Status         = gBS->AllocatePages (AllocateAnyPages, MemoryType, RealPages, &Memory);
    if (EFI_ERROR (Status)) {
      return NULL;
    }
    AlignedMemory  = ((UINTN) Memory + AlignmentMask) & ~AlignmentMask;
    UnalignedPages = EFI_SIZE_TO_PAGES (AlignedMemory - (UINTN) Memory);
    if (UnalignedPages > 0) {
      //
      // Free first unaligned page(s).
      //
      Status = gBS->FreePages (Memory, UnalignedPages);
      ASSERT_EFI_ERROR (Status);
    }
    Memory         = (EFI_PHYSICAL_ADDRESS) (AlignedMemory + EFI_PAGES_TO_SIZE (Pages));
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
    Status = gBS->AllocatePages (AllocateAnyPages, MemoryType, Pages, &Memory);
    if (EFI_ERROR (Status)) {
      return NULL;
    }
    AlignedMemory  = (UINTN) Memory;
  }

  Status = gVirtualUncachedPages->ConvertPages (gVirtualUncachedPages, AlignedMemory, Pages * EFI_PAGE_SIZE, PcdGet64 (PcdArmUncachedMemoryMask), &gAttributes);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  AlignedMemory = (EFI_PHYSICAL_ADDRESS)(UINTN)ConvertToUncachedAddress ((VOID *)(UINTN)AlignedMemory);

  return (VOID *)(UINTN)AlignedMemory;
}


VOID
EFIAPI
UncachedFreeAlignedPages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Memory;

  ASSERT (Pages != 0);

  Memory = ConvertToPhysicalAddress (Buffer);

  Status = gVirtualUncachedPages->RevertPages (gVirtualUncachedPages, Memory, Pages * EFI_PAGE_SIZE, PcdGet64 (PcdArmUncachedMemoryMask), gAttributes);


  Status = gBS->FreePages ((EFI_PHYSICAL_ADDRESS) (UINTN) Memory, Pages);
  ASSERT_EFI_ERROR (Status);
}




VOID *
UncachedInternalAllocateAlignedPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN UINTN            Alignment
  )
{
  VOID      *AlignedAddress;

  //
  // Alignment must be a power of two or zero.
  //
  ASSERT ((Alignment & (Alignment - 1)) == 0);

  if (Alignment < EFI_PAGE_SIZE) {
    Alignment = EFI_PAGE_SIZE;
  }

  AlignedAddress = UncachedInternalAllocateAlignedPages (PoolType, EFI_SIZE_TO_PAGES (AllocationSize), Alignment);
  if (AlignedAddress == NULL) {
    return NULL;
  }

  AddPagesToList ((VOID *)(UINTN)ConvertToPhysicalAddress (AlignedAddress), (VOID *)(UINTN)AlignedAddress, EFI_SIZE_TO_PAGES (AllocationSize));

  return (VOID *) AlignedAddress;
}

VOID *
EFIAPI
UncachedAllocateAlignedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedPool (EfiBootServicesData, AllocationSize, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedRuntimePool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedPool (EfiRuntimeServicesData, AllocationSize, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedReservedPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedPool (EfiReservedMemoryType, AllocationSize, Alignment);
}

VOID *
UncachedInternalAllocateAlignedZeroPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN UINTN            Alignment
  )
{
  VOID    *Memory;
  Memory = UncachedInternalAllocateAlignedPool (PoolType, AllocationSize, Alignment);
  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }
  return Memory;
}

VOID *
EFIAPI
UncachedAllocateAlignedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedZeroPool (EfiBootServicesData, AllocationSize, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedRuntimeZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedZeroPool (EfiRuntimeServicesData, AllocationSize, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedReservedZeroPool (
  IN UINTN  AllocationSize,
  IN UINTN  Alignment
  )
{
  return UncachedInternalAllocateAlignedZeroPool (EfiReservedMemoryType, AllocationSize, Alignment);
}

VOID *
UncachedInternalAllocateAlignedCopyPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN CONST VOID       *Buffer,
  IN UINTN            Alignment
  )
{
  VOID  *Memory;

  ASSERT (Buffer != NULL);
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN) Buffer + 1));

  Memory = UncachedInternalAllocateAlignedPool (PoolType, AllocationSize, Alignment);
  if (Memory != NULL) {
    Memory = CopyMem (Memory, Buffer, AllocationSize);
  }
  return Memory;
}

VOID *
EFIAPI
UncachedAllocateAlignedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  return UncachedInternalAllocateAlignedCopyPool (EfiBootServicesData, AllocationSize, Buffer, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  return UncachedInternalAllocateAlignedCopyPool (EfiRuntimeServicesData, AllocationSize, Buffer, Alignment);
}

VOID *
EFIAPI
UncachedAllocateAlignedReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer,
  IN UINTN       Alignment
  )
{
  return UncachedInternalAllocateAlignedCopyPool (EfiReservedMemoryType, AllocationSize, Buffer, Alignment);
}

VOID
EFIAPI
UncachedFreeAlignedPool (
  IN VOID   *Buffer
  )
{
  VOID    *Allocation;
  UINTN   Pages;

  RemovePagesFromList (Buffer, &Allocation, &Pages);

  UncachedFreePages (Allocation, Pages);
}

VOID *
UncachedInternalAllocatePool (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            AllocationSize
  )
{
  UINTN CacheLineLength = ArmDataCacheLineLength ();
  return UncachedInternalAllocateAlignedPool (MemoryType, AllocationSize, CacheLineLength);
}

VOID *
EFIAPI
UncachedAllocatePool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocatePool (EfiBootServicesData, AllocationSize);
}

VOID *
EFIAPI
UncachedAllocateRuntimePool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocatePool (EfiRuntimeServicesData, AllocationSize);
}

VOID *
EFIAPI
UncachedAllocateReservedPool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocatePool (EfiReservedMemoryType, AllocationSize);
}

VOID *
UncachedInternalAllocateZeroPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize
  )
{
  VOID  *Memory;

  Memory = UncachedInternalAllocatePool (PoolType, AllocationSize);
  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }
  return Memory;
}

VOID *
EFIAPI
UncachedAllocateZeroPool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocateZeroPool (EfiBootServicesData, AllocationSize);
}

VOID *
EFIAPI
UncachedAllocateRuntimeZeroPool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocateZeroPool (EfiRuntimeServicesData, AllocationSize);
}

VOID *
EFIAPI
UncachedAllocateReservedZeroPool (
  IN UINTN  AllocationSize
  )
{
  return UncachedInternalAllocateZeroPool (EfiReservedMemoryType, AllocationSize);
}

VOID *
UncachedInternalAllocateCopyPool (
  IN EFI_MEMORY_TYPE  PoolType,
  IN UINTN            AllocationSize,
  IN CONST VOID       *Buffer
  )
{
  VOID  *Memory;

  ASSERT (Buffer != NULL);
  ASSERT (AllocationSize <= (MAX_ADDRESS - (UINTN) Buffer + 1));

  Memory = UncachedInternalAllocatePool (PoolType, AllocationSize);
  if (Memory != NULL) {
     Memory = CopyMem (Memory, Buffer, AllocationSize);
  }
  return Memory;
}

VOID *
EFIAPI
UncachedAllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return UncachedInternalAllocateCopyPool (EfiBootServicesData, AllocationSize, Buffer);
}

VOID *
EFIAPI
UncachedAllocateRuntimeCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return UncachedInternalAllocateCopyPool (EfiRuntimeServicesData, AllocationSize, Buffer);
}

VOID *
EFIAPI
UncachedAllocateReservedCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  return UncachedInternalAllocateCopyPool (EfiReservedMemoryType, AllocationSize, Buffer);
}

VOID
EFIAPI
UncachedFreePool (
  IN VOID   *Buffer
  )
{
  UncachedFreeAlignedPool (Buffer);
}

VOID
EFIAPI
UncachedSafeFreePool (
  IN VOID   *Buffer
  )
{
  if (Buffer != NULL) {
    UncachedFreePool (Buffer);
    Buffer = NULL;
  }
}

/**
  The constructor function caches the pointer of DXE Services Table.

  The constructor function caches the pointer of DXE Services Table.
  It will ASSERT() if that operation fails.
  It will ASSERT() if the pointer of DXE Services Table is NULL.
  It will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DebugUncachedMemoryAllocationLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;

  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&gDebugUncachedCpu);
  ASSERT_EFI_ERROR(Status);

  Status = gBS->LocateProtocol (&gVirtualUncachedPagesProtocolGuid, NULL, (VOID **)&gVirtualUncachedPages);
  ASSERT_EFI_ERROR(Status);

  return Status;
}



