/** @file
  MM Memory page management functions.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmCore.h"

#define NEXT_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) + (Size)))

#define TRUNCATE_TO_PAGES(a)  ((a) >> EFI_PAGE_SHIFT)

LIST_ENTRY  mMmMemoryMap = INITIALIZE_LIST_HEAD_VARIABLE (mMmMemoryMap);

UINTN  mMapKey;

/**
  Internal Function. Allocate n pages from given free page node.

  @param  Pages                  The free page node.
  @param  NumberOfPages          Number of pages to be allocated.
  @param  MaxAddress             Request to allocate memory below this address.

  @return Memory address of allocated pages.

**/
UINTN
InternalAllocPagesOnOneNode (
  IN OUT FREE_PAGE_LIST  *Pages,
  IN     UINTN           NumberOfPages,
  IN     UINTN           MaxAddress
  )
{
  UINTN           Top;
  UINTN           Bottom;
  FREE_PAGE_LIST  *Node;

  Top = TRUNCATE_TO_PAGES (MaxAddress + 1 - (UINTN)Pages);
  if (Top > Pages->NumberOfPages) {
    Top = Pages->NumberOfPages;
  }

  Bottom = Top - NumberOfPages;

  if (Top < Pages->NumberOfPages) {
    Node                = (FREE_PAGE_LIST *)((UINTN)Pages + EFI_PAGES_TO_SIZE (Top));
    Node->NumberOfPages = Pages->NumberOfPages - Top;
    InsertHeadList (&Pages->Link, &Node->Link);
  }

  if (Bottom > 0) {
    Pages->NumberOfPages = Bottom;
  } else {
    RemoveEntryList (&Pages->Link);
  }

  return (UINTN)Pages + EFI_PAGES_TO_SIZE (Bottom);
}

/**
  Internal Function. Allocate n pages from free page list below MaxAddress.

  @param  FreePageList           The free page node.
  @param  NumberOfPages          Number of pages to be allocated.
  @param  MaxAddress             Request to allocate memory below this address.

  @return Memory address of allocated pages.

**/
UINTN
InternalAllocMaxAddress (
  IN OUT LIST_ENTRY  *FreePageList,
  IN     UINTN       NumberOfPages,
  IN     UINTN       MaxAddress
  )
{
  LIST_ENTRY      *Node;
  FREE_PAGE_LIST  *Pages;

  for (Node = FreePageList->BackLink; Node != FreePageList; Node = Node->BackLink) {
    Pages = BASE_CR (Node, FREE_PAGE_LIST, Link);
    if ((Pages->NumberOfPages >= NumberOfPages) &&
        ((UINTN)Pages + EFI_PAGES_TO_SIZE (NumberOfPages) - 1 <= MaxAddress))
    {
      return InternalAllocPagesOnOneNode (Pages, NumberOfPages, MaxAddress);
    }
  }

  return (UINTN)(-1);
}

/**
  Internal Function. Allocate n pages from free page list at given address.

  @param  FreePageList           The free page node.
  @param  NumberOfPages          Number of pages to be allocated.
  @param  MaxAddress             Request to allocate memory below this address.

  @return Memory address of allocated pages.

**/
UINTN
InternalAllocAddress (
  IN OUT LIST_ENTRY  *FreePageList,
  IN     UINTN       NumberOfPages,
  IN     UINTN       Address
  )
{
  UINTN           EndAddress;
  LIST_ENTRY      *Node;
  FREE_PAGE_LIST  *Pages;

  if ((Address & EFI_PAGE_MASK) != 0) {
    return ~Address;
  }

  EndAddress = Address + EFI_PAGES_TO_SIZE (NumberOfPages);
  for (Node = FreePageList->BackLink; Node != FreePageList; Node = Node->BackLink) {
    Pages = BASE_CR (Node, FREE_PAGE_LIST, Link);
    if ((UINTN)Pages <= Address) {
      if ((UINTN)Pages + EFI_PAGES_TO_SIZE (Pages->NumberOfPages) < EndAddress) {
        break;
      }

      return InternalAllocPagesOnOneNode (Pages, NumberOfPages, EndAddress);
    }
  }

  return ~Address;
}

/**
  Allocates pages from the memory map.

  @param  Type                   The type of allocation to perform.
  @param  MemoryType             The type of memory to turn the allocated pages
                                 into.
  @param  NumberOfPages          The number of pages to allocate.
  @param  Memory                 A pointer to receive the base allocated memory
                                 address.

  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
EFIAPI
MmInternalAllocatePages (
  IN  EFI_ALLOCATE_TYPE     Type,
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 NumberOfPages,
  OUT EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  UINTN  RequestedAddress;

  if ((MemoryType != EfiRuntimeServicesCode) &&
      (MemoryType != EfiRuntimeServicesData))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (NumberOfPages > TRUNCATE_TO_PAGES ((UINTN)-1) + 1) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // We don't track memory type in MM
  //
  RequestedAddress = (UINTN)*Memory;
  switch (Type) {
    case AllocateAnyPages:
      RequestedAddress = (UINTN)(-1);
    case AllocateMaxAddress:
      *Memory = InternalAllocMaxAddress (
                  &mMmMemoryMap,
                  NumberOfPages,
                  RequestedAddress
                  );
      if (*Memory == (UINTN)-1) {
        return EFI_OUT_OF_RESOURCES;
      }

      break;
    case AllocateAddress:
      *Memory = InternalAllocAddress (
                  &mMmMemoryMap,
                  NumberOfPages,
                  RequestedAddress
                  );
      if (*Memory != RequestedAddress) {
        return EFI_NOT_FOUND;
      }

      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Allocates pages from the memory map.

  @param  Type                   The type of allocation to perform.
  @param  MemoryType             The type of memory to turn the allocated pages
                                 into.
  @param  NumberOfPages          The number of pages to allocate.
  @param  Memory                 A pointer to receive the base allocated memory
                                 address.

  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
EFIAPI
MmAllocatePages (
  IN  EFI_ALLOCATE_TYPE     Type,
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 NumberOfPages,
  OUT EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  EFI_STATUS  Status;

  Status = MmInternalAllocatePages (Type, MemoryType, NumberOfPages, Memory);
  return Status;
}

/**
  Internal Function. Merge two adjacent nodes.

  @param  First             The first of two nodes to merge.

  @return Pointer to node after merge (if success) or pointer to next node (if fail).

**/
FREE_PAGE_LIST *
InternalMergeNodes (
  IN FREE_PAGE_LIST  *First
  )
{
  FREE_PAGE_LIST  *Next;

  Next = BASE_CR (First->Link.ForwardLink, FREE_PAGE_LIST, Link);
  ASSERT (
    TRUNCATE_TO_PAGES ((UINTN)Next - (UINTN)First) >= First->NumberOfPages
    );

  if (TRUNCATE_TO_PAGES ((UINTN)Next - (UINTN)First) == First->NumberOfPages) {
    First->NumberOfPages += Next->NumberOfPages;
    RemoveEntryList (&Next->Link);
    Next = First;
  }

  return Next;
}

/**
  Frees previous allocated pages.

  @param  Memory                 Base address of memory being freed.
  @param  NumberOfPages          The number of pages to free.

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range.
  @retval EFI_INVALID_PARAMETER  Address not aligned.
  @return EFI_SUCCESS            Pages successfully freed.

**/
EFI_STATUS
EFIAPI
MmInternalFreePages (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages
  )
{
  LIST_ENTRY      *Node;
  FREE_PAGE_LIST  *Pages;

  if ((Memory & EFI_PAGE_MASK) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  Pages = NULL;
  Node  = mMmMemoryMap.ForwardLink;
  while (Node != &mMmMemoryMap) {
    Pages = BASE_CR (Node, FREE_PAGE_LIST, Link);
    if (Memory < (UINTN)Pages) {
      break;
    }

    Node = Node->ForwardLink;
  }

  if ((Node != &mMmMemoryMap) &&
      (Memory + EFI_PAGES_TO_SIZE (NumberOfPages) > (UINTN)Pages))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Node->BackLink != &mMmMemoryMap) {
    Pages = BASE_CR (Node->BackLink, FREE_PAGE_LIST, Link);
    if ((UINTN)Pages + EFI_PAGES_TO_SIZE (Pages->NumberOfPages) > Memory) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Pages                = (FREE_PAGE_LIST *)(UINTN)Memory;
  Pages->NumberOfPages = NumberOfPages;
  InsertTailList (Node, &Pages->Link);

  if (Pages->Link.BackLink != &mMmMemoryMap) {
    Pages = InternalMergeNodes (
              BASE_CR (Pages->Link.BackLink, FREE_PAGE_LIST, Link)
              );
  }

  if (Node != &mMmMemoryMap) {
    InternalMergeNodes (Pages);
  }

  return EFI_SUCCESS;
}

/**
  Frees previous allocated pages.

  @param  Memory                 Base address of memory being freed.
  @param  NumberOfPages          The number of pages to free.

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range.
  @retval EFI_INVALID_PARAMETER  Address not aligned.
  @return EFI_SUCCESS            Pages successfully freed.

**/
EFI_STATUS
EFIAPI
MmFreePages (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages
  )
{
  EFI_STATUS  Status;

  Status = MmInternalFreePages (Memory, NumberOfPages);
  return Status;
}

/**
  Add free MMRAM region for use by memory service.

  @param  MemBase                Base address of memory region.
  @param  MemLength              Length of the memory region.
  @param  Type                   Memory type.
  @param  Attributes             Memory region state.

**/
VOID
MmAddMemoryRegion (
  IN  EFI_PHYSICAL_ADDRESS  MemBase,
  IN  UINT64                MemLength,
  IN  EFI_MEMORY_TYPE       Type,
  IN  UINT64                Attributes
  )
{
  UINTN  AlignedMemBase;

  //
  // Do not add memory regions that is already allocated, needs testing, or needs ECC initialization
  //
  if ((Attributes & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) != 0) {
    return;
  }

  //
  // Align range on an EFI_PAGE_SIZE boundary
  //
  AlignedMemBase = (UINTN)(MemBase + EFI_PAGE_MASK) & ~EFI_PAGE_MASK;
  MemLength     -= AlignedMemBase - MemBase;
  MmFreePages (AlignedMemBase, TRUNCATE_TO_PAGES ((UINTN)MemLength));
}
