/** @file
  UncachedMemoryAllocation lib that uses DXE Service to change cachability for
  a buffer.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2014, AMR Ltd. All rights reserved.<BR>

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
  EFI_PHYSICAL_ADDRESS  Base;
  VOID                  *Allocation;
  UINTN                 Pages;
  EFI_MEMORY_TYPE       MemoryType;
  BOOLEAN               Allocated;
  LIST_ENTRY            Link;
} FREE_PAGE_NODE;

STATIC LIST_ENTRY  mPageList = INITIALIZE_LIST_HEAD_VARIABLE (mPageList);
// Track the size of the non-allocated buffer in the linked-list
STATIC UINTN   mFreedBufferSize = 0;

/**
 * This function firstly checks if the requested allocation can fit into one
 * of the previously allocated buffer.
 * If the requested allocation does not fit in the existing pool then
 * the function makes a new allocation.
 *
 * @param MemoryType    Type of memory requested for the new allocation
 * @param Pages         Number of requested page
 * @param Alignment     Required alignment
 * @param Allocation    Address of the newly allocated buffer
 *
 * @return EFI_SUCCESS  If the function manage to allocate a buffer
 * @return !EFI_SUCCESS If the function did not manage to allocate a buffer
 */
STATIC
EFI_STATUS
AllocatePagesFromList (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            Pages,
  IN UINTN            Alignment,
  OUT VOID            **Allocation
  )
{
  EFI_STATUS       Status;
  LIST_ENTRY      *Link;
  FREE_PAGE_NODE  *Node;
  FREE_PAGE_NODE  *NewNode;
  UINTN            AlignmentMask;
  EFI_PHYSICAL_ADDRESS Memory;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR Descriptor;

  // Alignment must be a power of two or zero.
  ASSERT ((Alignment & (Alignment - 1)) == 0);

  //
  // Look in our list for the smallest page that could satisfy the new allocation
  //
  NewNode = NULL;
  for (Link = mPageList.ForwardLink; Link != &mPageList; Link = Link->ForwardLink) {
    Node = BASE_CR (Link, FREE_PAGE_NODE, Link);
    if ((Node->Allocated == FALSE) && (Node->MemoryType == MemoryType)) {
      // We have a node that fits our requirements
      if (((UINTN)Node->Base & (Alignment - 1)) == 0) {
        // We found a page that matches the page size
        if (Node->Pages == Pages) {
          Node->Allocated  = TRUE;
          Node->Allocation = (VOID*)(UINTN)Node->Base;
          *Allocation      = Node->Allocation;

          // Update the size of the freed buffer
          mFreedBufferSize  -= Pages * EFI_PAGE_SIZE;
          return EFI_SUCCESS;
        } else if (Node->Pages > Pages) {
          if (NewNode == NULL) {
            // It is the first node that could contain our new allocation
            NewNode = Node;
          } else if (NewNode->Pages > Node->Pages) {
            // This node offers a smaller number of page.
            NewNode = Node;
          }
        }
      }
    }
  }
  // Check if we have found a node that could contain our new allocation
  if (NewNode != NULL) {
    NewNode->Allocated = TRUE;
    Node->Allocation   = (VOID*)(UINTN)Node->Base;
    *Allocation        = Node->Allocation;
    return EFI_SUCCESS;
  }

  //
  // Otherwise, we need to allocate a new buffer
  //

  // We do not want to over-allocate in case the alignment requirement does not
  // require extra pages
  if (Alignment > EFI_PAGE_SIZE) {
    AlignmentMask  = Alignment - 1;
    Pages          += EFI_SIZE_TO_PAGES (Alignment);
  } else {
    AlignmentMask  = 0;
  }

  Status = gBS->AllocatePages (AllocateAnyPages, MemoryType, Pages, &Memory);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gDS->GetMemorySpaceDescriptor (Memory, &Descriptor);
  if (!EFI_ERROR (Status)) {
    // We are making an assumption that all of memory has the same default attributes
    gAttributes = Descriptor.Attributes;
  } else {
    gBS->FreePages (Memory, Pages);
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (Memory, EFI_PAGES_TO_SIZE (Pages), EFI_MEMORY_WC);
  if (EFI_ERROR (Status)) {
    gBS->FreePages (Memory, Pages);
    return Status;
  }

  NewNode = AllocatePool (sizeof (FREE_PAGE_NODE));
  if (NewNode == NULL) {
    ASSERT (FALSE);
    gBS->FreePages (Memory, Pages);
    return EFI_OUT_OF_RESOURCES;
  }

  NewNode->Base       = Memory;
  NewNode->Allocation = (VOID*)(((UINTN)Memory + AlignmentMask) & ~AlignmentMask);
  NewNode->Pages      = Pages;
  NewNode->Allocated  = TRUE;
  NewNode->MemoryType = MemoryType;

  InsertTailList (&mPageList, &NewNode->Link);

  *Allocation = NewNode->Allocation;
  return EFI_SUCCESS;
}

/**
 * Free the memory allocation
 *
 * This function will actually try to find the allocation in the linked list.
 * And it will then mark the entry as freed.
 *
 * @param  Allocation  Base address of the buffer to free
 *
 * @return EFI_SUCCESS            The allocation has been freed
 * @return EFI_NOT_FOUND          The allocation was not found in the pool.
 * @return EFI_INVALID_PARAMETER  If Allocation is NULL
 *
 */
STATIC
EFI_STATUS
FreePagesFromList (
  IN  VOID  *Allocation
  )
{
  LIST_ENTRY      *Link;
  FREE_PAGE_NODE  *Node;

  if (Allocation == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Link = mPageList.ForwardLink; Link != &mPageList; Link = Link->ForwardLink) {
    Node = BASE_CR (Link, FREE_PAGE_NODE, Link);
    if ((UINTN)Node->Allocation == (UINTN)Allocation) {
      Node->Allocated = FALSE;

      // Update the size of the freed buffer
      mFreedBufferSize  += Node->Pages * EFI_PAGE_SIZE;

      // If the size of the non-allocated reaches the threshold we raise a warning.
      // It might be an expected behaviour in some cases.
      // We might device to free some of these buffers later on.
      if (mFreedBufferSize > PcdGet64 (PcdArmFreeUncachedMemorySizeThreshold)) {
        DEBUG ((EFI_D_WARN, "Warning: The list of non-allocated buffer has reach the threshold.\n"));
      }
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
 * This function is automatically invoked when the driver exits
 * It frees all the non-allocated memory buffer.
 * This function is not responsible to free allocated buffer (eg: case of memory leak,
 * runtime allocation).
 */
EFI_STATUS
EFIAPI
UncachedMemoryAllocationLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  LIST_ENTRY      *Link;
  FREE_PAGE_NODE  *OldNode;

  // Test if the list is empty
  Link = mPageList.ForwardLink;
  if (Link == &mPageList) {
    return EFI_SUCCESS;
  }

  // Free all the pages and nodes
  do {
    OldNode = BASE_CR (Link, FREE_PAGE_NODE, Link);
    // Point to the next entry
    Link = Link->ForwardLink;

    // We only free the non-allocated buffer
    if (OldNode->Allocated == FALSE) {
      gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)OldNode->Base, OldNode->Pages);
      RemoveEntryList (&OldNode->Link);
      FreePool (OldNode);
    }
  } while (Link != &mPageList);

  return EFI_SUCCESS;
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
  EFI_STATUS Status;
  VOID   *Allocation;

  if (Pages == 0) {
    return NULL;
  }

  Allocation = NULL;
  Status = AllocatePagesFromList (MemoryType, Pages, Alignment, &Allocation);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return NULL;
  } else {
    return Allocation;
  }
}


VOID
EFIAPI
UncachedFreeAlignedPages (
  IN VOID   *Buffer,
  IN UINTN  Pages
  )
{
  FreePagesFromList (Buffer);
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
  UncachedFreePages (Allocation, 0);
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

