/** @file
  UncachedMemoryAllocation lib that uses DXE Service to change cachability for
  a buffer.

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
#include <Library/DxeServicesTableLib.h>

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
  
  

//
// Assume all of memory has the same cache attributes, unless we do our magic
//
UINT64  gAttributes;

typedef struct {
  VOID        *Allocation;
  UINTN       Pages;
  LIST_ENTRY  Link;
} FREE_PAGE_NODE;

LIST_ENTRY  mPageList = INITIALIZE_LIST_HEAD_VARIABLE (mPageList);

VOID
AddPagesToList (
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
  
  NewNode->Allocation = Allocation;
  NewNode->Pages      = Pages;
  
  InsertTailList (&mPageList, &NewNode->Link);
}


VOID
RemovePagesFromList (
  OUT VOID  *Allocation,
  OUT UINTN *Pages
  )
{
  LIST_ENTRY      *Link;
  FREE_PAGE_NODE  *OldNode;

  *Pages = 0;
  
  for (Link = mPageList.ForwardLink; Link != &mPageList; Link = Link->ForwardLink) {
    OldNode = BASE_CR (Link, FREE_PAGE_NODE, Link);
    if (OldNode->Allocation == Allocation) {
      *Pages = OldNode->Pages;
      
      RemoveEntryList (&OldNode->Link);
      FreePool (OldNode);
      return;
    }
  }

  return;
}


/**
  Converts a cached or uncached address to a physical address suitable for use in SoC registers.

  @param  VirtualAddress                 The pointer to convert.

  @return The physical address of the supplied virtual pointer.

**/
EFI_PHYSICAL_ADDRESS
ConvertToPhysicalAddress (
  IN VOID *VirtualAddress
  )
{
  return (EFI_PHYSICAL_ADDRESS)(UINTN)VirtualAddress;
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
  EFI_STATUS                        Status;
  EFI_PHYSICAL_ADDRESS              Memory;
  EFI_PHYSICAL_ADDRESS              AlignedMemory;
  UINTN                             AlignmentMask;
  UINTN                             UnalignedPages;
  UINTN                             RealPages;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR   Descriptor;

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
  
  Status = gDS->GetMemorySpaceDescriptor (Memory, &Descriptor);
  if (!EFI_ERROR (Status)) {
    // We are making an assumption that all of memory has the same default attributes
    gAttributes = Descriptor.Attributes;
  }
  
  Status = gDS->SetMemorySpaceAttributes (Memory, EFI_PAGES_TO_SIZE (Pages), EFI_MEMORY_WC);
  ASSERT_EFI_ERROR (Status);
  
  return (VOID *)(UINTN)Memory;
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
  
  Memory = (EFI_PHYSICAL_ADDRESS) (UINTN) Buffer;
  Status = gDS->SetMemorySpaceAttributes (Memory, EFI_PAGES_TO_SIZE (Pages), gAttributes);
  
  Status = gBS->FreePages (Memory, Pages);
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

  AddPagesToList ((VOID *)(UINTN)AlignedAddress, EFI_SIZE_TO_PAGES (AllocationSize));

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
  IN VOID   *Allocation
  )
{
  UINTN   Pages;
  
  RemovePagesFromList (Allocation, &Pages);

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

