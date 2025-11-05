/** @file
  SMM Memory page management functions.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCore.h"
#include <Library/SmmServicesTableLib.h>

#define TRUNCATE_TO_PAGES(a)  ((a) >> EFI_PAGE_SHIFT)

LIST_ENTRY  mSmmMemoryMap = INITIALIZE_LIST_HEAD_VARIABLE (mSmmMemoryMap);

//
// For GetMemoryMap()
//

#define MEMORY_MAP_SIGNATURE  SIGNATURE_32('m','m','a','p')
typedef struct {
  UINTN              Signature;
  LIST_ENTRY         Link;

  BOOLEAN            FromStack;
  EFI_MEMORY_TYPE    Type;
  UINT64             Start;
  UINT64             End;
} MEMORY_MAP;

LIST_ENTRY  gMemoryMap = INITIALIZE_LIST_HEAD_VARIABLE (gMemoryMap);

#define MAX_MAP_DEPTH  6

///
/// mMapDepth - depth of new descriptor stack
///
UINTN  mMapDepth = 0;
///
/// mMapStack - space to use as temp storage to build new map descriptors
///
MEMORY_MAP  mMapStack[MAX_MAP_DEPTH];
UINTN       mFreeMapStack = 0;
///
/// This list maintain the free memory map list
///
LIST_ENTRY  mFreeMemoryMapEntryList = INITIALIZE_LIST_HEAD_VARIABLE (mFreeMemoryMapEntryList);

/**
  Allocates pages from the memory map.

  @param[in]   Type                   The type of allocation to perform.
  @param[in]   MemoryType             The type of memory to turn the allocated pages
                                      into.
  @param[in]   NumberOfPages          The number of pages to allocate.
  @param[out]  Memory                 A pointer to receive the base allocated memory
                                      address.
  @param[in]   AddRegion              If this memory is new added region.
  @param[in]   NeedGuard              Flag to indicate Guard page is needed
                                      or not

  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
SmmInternalAllocatePagesEx (
  IN  EFI_ALLOCATE_TYPE     Type,
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 NumberOfPages,
  OUT EFI_PHYSICAL_ADDRESS  *Memory,
  IN  BOOLEAN               AddRegion,
  IN  BOOLEAN               NeedGuard
  );

/**
  Internal function.  Deque a descriptor entry from the mFreeMemoryMapEntryList.
  If the list is emtry, then allocate a new page to refuel the list.
  Please Note this algorithm to allocate the memory map descriptor has a property
  that the memory allocated for memory entries always grows, and will never really be freed.

  @return The Memory map descriptor dequeued from the mFreeMemoryMapEntryList

**/
MEMORY_MAP *
AllocateMemoryMapEntry (
  VOID
  )
{
  EFI_PHYSICAL_ADDRESS  Mem;
  EFI_STATUS            Status;
  MEMORY_MAP            *FreeDescriptorEntries;
  MEMORY_MAP            *Entry;
  UINTN                 Index;

  // DEBUG((DEBUG_INFO, "AllocateMemoryMapEntry\n"));

  if (IsListEmpty (&mFreeMemoryMapEntryList)) {
    // DEBUG((DEBUG_INFO, "mFreeMemoryMapEntryList is empty\n"));
    //
    // The list is empty, to allocate one page to refuel the list
    //
    Status = SmmInternalAllocatePagesEx (
               AllocateAnyPages,
               EfiRuntimeServicesData,
               EFI_SIZE_TO_PAGES (RUNTIME_PAGE_ALLOCATION_GRANULARITY),
               &Mem,
               TRUE,
               FALSE
               );
    ASSERT_EFI_ERROR (Status);
    if (!EFI_ERROR (Status)) {
      FreeDescriptorEntries = (MEMORY_MAP *)(UINTN)Mem;
      // DEBUG((DEBUG_INFO, "New FreeDescriptorEntries - 0x%x\n", FreeDescriptorEntries));
      //
      // Enqueue the free memory map entries into the list
      //
      for (Index = 0; Index < RUNTIME_PAGE_ALLOCATION_GRANULARITY / sizeof (MEMORY_MAP); Index++) {
        FreeDescriptorEntries[Index].Signature = MEMORY_MAP_SIGNATURE;
        InsertTailList (&mFreeMemoryMapEntryList, &FreeDescriptorEntries[Index].Link);
      }
    } else {
      return NULL;
    }
  }

  //
  // dequeue the first descriptor from the list
  //
  Entry = CR (mFreeMemoryMapEntryList.ForwardLink, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
  RemoveEntryList (&Entry->Link);

  return Entry;
}

/**
  Internal function.  Moves any memory descriptors that are on the
  temporary descriptor stack to heap.

**/
VOID
CoreFreeMemoryMapStack (
  VOID
  )
{
  MEMORY_MAP  *Entry;

  //
  // If already freeing the map stack, then return
  //
  if (mFreeMapStack != 0) {
    ASSERT (FALSE);
    return;
  }

  //
  // Move the temporary memory descriptor stack into pool
  //
  mFreeMapStack += 1;

  while (mMapDepth != 0) {
    //
    // Deque an memory map entry from mFreeMemoryMapEntryList
    //
    Entry = AllocateMemoryMapEntry ();
    ASSERT (Entry);

    //
    // Update to proper entry
    //
    mMapDepth -= 1;

    if (mMapStack[mMapDepth].Link.ForwardLink != NULL) {
      CopyMem (Entry, &mMapStack[mMapDepth], sizeof (MEMORY_MAP));
      Entry->FromStack = FALSE;

      //
      // Move this entry to general memory
      //
      InsertTailList (&mMapStack[mMapDepth].Link, &Entry->Link);
      RemoveEntryList (&mMapStack[mMapDepth].Link);
      mMapStack[mMapDepth].Link.ForwardLink = NULL;
    }
  }

  mFreeMapStack -= 1;
}

/**
  Insert new entry from memory map.

  @param[in]  Link       The old memory map entry to be linked.
  @param[in]  Start      The start address of new memory map entry.
  @param[in]  End        The end address of new memory map entry.
  @param[in]  Type       The type of new memory map entry.
  @param[in]  Next       If new entry is inserted to the next of old entry.
  @param[in]  AddRegion  If this memory is new added region.
**/
VOID
InsertNewEntry (
  IN LIST_ENTRY       *Link,
  IN UINT64           Start,
  IN UINT64           End,
  IN EFI_MEMORY_TYPE  Type,
  IN BOOLEAN          Next,
  IN BOOLEAN          AddRegion
  )
{
  MEMORY_MAP  *Entry;

  Entry      = &mMapStack[mMapDepth];
  mMapDepth += 1;
  ASSERT (mMapDepth < MAX_MAP_DEPTH);
  Entry->FromStack = TRUE;

  Entry->Signature = MEMORY_MAP_SIGNATURE;
  Entry->Type      = Type;
  Entry->Start     = Start;
  Entry->End       = End;
  if (Next) {
    InsertHeadList (Link, &Entry->Link);
  } else {
    InsertTailList (Link, &Entry->Link);
  }
}

/**
  Remove old entry from memory map.

  @param[in] Entry Memory map entry to be removed.
**/
VOID
RemoveOldEntry (
  IN MEMORY_MAP  *Entry
  )
{
  RemoveEntryList (&Entry->Link);
  Entry->Link.ForwardLink = NULL;

  if (!Entry->FromStack) {
    InsertTailList (&mFreeMemoryMapEntryList, &Entry->Link);
  }
}

/**
  Update SMM memory map entry.

  @param[in]  Type                   The type of allocation to perform.
  @param[in]  Memory                 The base of memory address.
  @param[in]  NumberOfPages          The number of pages to allocate.
  @param[in]  AddRegion              If this memory is new added region.
**/
VOID
ConvertSmmMemoryMapEntry (
  IN EFI_MEMORY_TYPE       Type,
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages,
  IN BOOLEAN               AddRegion
  )
{
  LIST_ENTRY            *Link;
  MEMORY_MAP            *Entry;
  MEMORY_MAP            *NextEntry;
  LIST_ENTRY            *NextLink;
  MEMORY_MAP            *PreviousEntry;
  LIST_ENTRY            *PreviousLink;
  EFI_PHYSICAL_ADDRESS  Start;
  EFI_PHYSICAL_ADDRESS  End;

  Start = Memory;
  End   = Memory + EFI_PAGES_TO_SIZE (NumberOfPages) - 1;

  //
  // Exclude memory region
  //
  Link = gMemoryMap.ForwardLink;
  while (Link != &gMemoryMap) {
    Entry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
    Link  = Link->ForwardLink;

    //
    // ---------------------------------------------------
    // |  +----------+   +------+   +------+   +------+  |
    // ---|gMemoryMep|---|Entry1|---|Entry2|---|Entry3|---
    //    +----------+ ^ +------+   +------+   +------+
    //                 |
    //              +------+
    //              |EntryX|
    //              +------+
    //
    if (Entry->Start > End) {
      if ((Entry->Start == End + 1) && (Entry->Type == Type)) {
        Entry->Start = Start;
        return;
      }

      InsertNewEntry (
        &Entry->Link,
        Start,
        End,
        Type,
        FALSE,
        AddRegion
        );
      return;
    }

    if ((Entry->Start <= Start) && (Entry->End >= End)) {
      if (Entry->Type != Type) {
        if (Entry->Start < Start) {
          //
          // ---------------------------------------------------
          // |  +----------+   +------+   +------+   +------+  |
          // ---|gMemoryMep|---|Entry1|---|EntryX|---|Entry3|---
          //    +----------+   +------+ ^ +------+   +------+
          //                            |
          //                         +------+
          //                         |EntryA|
          //                         +------+
          //
          InsertNewEntry (
            &Entry->Link,
            Entry->Start,
            Start - 1,
            Entry->Type,
            FALSE,
            AddRegion
            );
        }

        if (Entry->End > End) {
          //
          // ---------------------------------------------------
          // |  +----------+   +------+   +------+   +------+  |
          // ---|gMemoryMep|---|Entry1|---|EntryX|---|Entry3|---
          //    +----------+   +------+   +------+ ^ +------+
          //                                       |
          //                                    +------+
          //                                    |EntryZ|
          //                                    +------+
          //
          InsertNewEntry (
            &Entry->Link,
            End + 1,
            Entry->End,
            Entry->Type,
            TRUE,
            AddRegion
            );
        }

        //
        // Update this node
        //
        Entry->Start = Start;
        Entry->End   = End;
        Entry->Type  = Type;

        //
        // Check adjacent
        //
        NextLink = Entry->Link.ForwardLink;
        if (NextLink != &gMemoryMap) {
          NextEntry = CR (NextLink, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
          //
          // ---------------------------------------------------
          // |  +----------+   +------+   +-----------------+  |
          // ---|gMemoryMep|---|Entry1|---|EntryX     Entry3|---
          //    +----------+   +------+   +-----------------+
          //
          if ((Entry->Type == NextEntry->Type) && (Entry->End + 1 == NextEntry->Start)) {
            Entry->End = NextEntry->End;
            RemoveOldEntry (NextEntry);
          }
        }

        PreviousLink = Entry->Link.BackLink;
        if (PreviousLink != &gMemoryMap) {
          PreviousEntry = CR (PreviousLink, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
          //
          // ---------------------------------------------------
          // |  +----------+   +-----------------+   +------+  |
          // ---|gMemoryMep|---|Entry1     EntryX|---|Entry3|---
          //    +----------+   +-----------------+   +------+
          //
          if ((PreviousEntry->Type == Entry->Type) && (PreviousEntry->End + 1 == Entry->Start)) {
            PreviousEntry->End = Entry->End;
            RemoveOldEntry (Entry);
          }
        }
      }

      return;
    }
  }

  //
  // ---------------------------------------------------
  // |  +----------+   +------+   +------+   +------+  |
  // ---|gMemoryMep|---|Entry1|---|Entry2|---|Entry3|---
  //    +----------+   +------+   +------+   +------+ ^
  //                                                  |
  //                                               +------+
  //                                               |EntryX|
  //                                               +------+
  //
  Link = gMemoryMap.BackLink;
  if (Link != &gMemoryMap) {
    Entry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
    if ((Entry->End + 1 == Start) && (Entry->Type == Type)) {
      Entry->End = End;
      return;
    }
  }

  InsertNewEntry (
    &gMemoryMap,
    Start,
    End,
    Type,
    FALSE,
    AddRegion
    );
  return;
}

/**
  Return the count of Smm memory map entry.

  @return The count of Smm memory map entry.
**/
UINTN
GetSmmMemoryMapEntryCount (
  VOID
  )
{
  LIST_ENTRY  *Link;
  UINTN       Count;

  Count = 0;
  Link  = gMemoryMap.ForwardLink;
  while (Link != &gMemoryMap) {
    Link = Link->ForwardLink;
    Count++;
  }

  return Count;
}

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

  @param[in]   Type                   The type of allocation to perform.
  @param[in]   MemoryType             The type of memory to turn the allocated pages
                                      into.
  @param[in]   NumberOfPages          The number of pages to allocate.
  @param[out]  Memory                 A pointer to receive the base allocated memory
                                      address.
  @param[in]   AddRegion              If this memory is new added region.
  @param[in]   NeedGuard              Flag to indicate Guard page is needed
                                      or not

  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
SmmInternalAllocatePagesEx (
  IN  EFI_ALLOCATE_TYPE     Type,
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 NumberOfPages,
  OUT EFI_PHYSICAL_ADDRESS  *Memory,
  IN  BOOLEAN               AddRegion,
  IN  BOOLEAN               NeedGuard
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
  // We don't track memory type in SMM
  //
  RequestedAddress = (UINTN)*Memory;
  switch (Type) {
    case AllocateAnyPages:
      RequestedAddress = (UINTN)(-1);
    case AllocateMaxAddress:
      if (NeedGuard) {
        *Memory = InternalAllocMaxAddressWithGuard (
                    &mSmmMemoryMap,
                    NumberOfPages,
                    RequestedAddress,
                    MemoryType
                    );
        if (*Memory == (UINTN)-1) {
          return EFI_OUT_OF_RESOURCES;
        } else {
          ASSERT (VerifyMemoryGuard (*Memory, NumberOfPages) == TRUE);
          return EFI_SUCCESS;
        }
      }

      *Memory = InternalAllocMaxAddress (
                  &mSmmMemoryMap,
                  NumberOfPages,
                  RequestedAddress
                  );
      if (*Memory == (UINTN)-1) {
        return EFI_OUT_OF_RESOURCES;
      }

      break;
    case AllocateAddress:
      *Memory = InternalAllocAddress (
                  &mSmmMemoryMap,
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

  //
  // Update SmmMemoryMap here.
  //
  ConvertSmmMemoryMapEntry (MemoryType, *Memory, NumberOfPages, AddRegion);
  if (!AddRegion) {
    CoreFreeMemoryMapStack ();
  }

  return EFI_SUCCESS;
}

/**
  Allocates pages from the memory map.

  @param[in]   Type                   The type of allocation to perform.
  @param[in]   MemoryType             The type of memory to turn the allocated pages
                                      into.
  @param[in]   NumberOfPages          The number of pages to allocate.
  @param[out]  Memory                 A pointer to receive the base allocated memory
                                      address.
  @param[in]   NeedGuard              Flag to indicate Guard page is needed
                                      or not

  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
EFIAPI
SmmInternalAllocatePages (
  IN  EFI_ALLOCATE_TYPE     Type,
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 NumberOfPages,
  OUT EFI_PHYSICAL_ADDRESS  *Memory,
  IN  BOOLEAN               NeedGuard
  )
{
  return SmmInternalAllocatePagesEx (
           Type,
           MemoryType,
           NumberOfPages,
           Memory,
           FALSE,
           NeedGuard
           );
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
SmmAllocatePages (
  IN  EFI_ALLOCATE_TYPE     Type,
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 NumberOfPages,
  OUT EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  EFI_STATUS  Status;
  BOOLEAN     NeedGuard;

  NeedGuard = IsPageTypeToGuard (MemoryType, Type);
  Status    = SmmInternalAllocatePages (
                Type,
                MemoryType,
                NumberOfPages,
                Memory,
                NeedGuard
                );
  if (!EFI_ERROR (Status)) {
    SmmCoreUpdateProfile (
      (EFI_PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MemoryProfileActionAllocatePages,
      MemoryType,
      EFI_PAGES_TO_SIZE (NumberOfPages),
      (VOID *)(UINTN)*Memory,
      NULL
      );
  }

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

  @param[in]  Memory                 Base address of memory being freed.
  @param[in]  NumberOfPages          The number of pages to free.
  @param[in]  AddRegion              If this memory is new added region.

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range.
  @retval EFI_INVALID_PARAMETER  Address not aligned, Address is zero or NumberOfPages is zero.
  @return EFI_SUCCESS            Pages successfully freed.

**/
EFI_STATUS
SmmInternalFreePagesEx (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages,
  IN BOOLEAN               AddRegion
  )
{
  LIST_ENTRY      *Node;
  FREE_PAGE_LIST  *Pages;

  if (((Memory & EFI_PAGE_MASK) != 0) || (Memory == 0) || (NumberOfPages == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Pages = NULL;
  Node  = mSmmMemoryMap.ForwardLink;
  while (Node != &mSmmMemoryMap) {
    Pages = BASE_CR (Node, FREE_PAGE_LIST, Link);
    if (Memory < (UINTN)Pages) {
      break;
    }

    Node = Node->ForwardLink;
  }

  if ((Node != &mSmmMemoryMap) &&
      (Memory + EFI_PAGES_TO_SIZE (NumberOfPages) > (UINTN)Pages))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Node->BackLink != &mSmmMemoryMap) {
    Pages = BASE_CR (Node->BackLink, FREE_PAGE_LIST, Link);
    if ((UINTN)Pages + EFI_PAGES_TO_SIZE (Pages->NumberOfPages) > Memory) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Pages                = (FREE_PAGE_LIST *)(UINTN)Memory;
  Pages->NumberOfPages = NumberOfPages;
  InsertTailList (Node, &Pages->Link);

  if (Pages->Link.BackLink != &mSmmMemoryMap) {
    Pages = InternalMergeNodes (
              BASE_CR (Pages->Link.BackLink, FREE_PAGE_LIST, Link)
              );
  }

  if (Node != &mSmmMemoryMap) {
    InternalMergeNodes (Pages);
  }

  //
  // Update SmmMemoryMap here.
  //
  ConvertSmmMemoryMapEntry (EfiConventionalMemory, Memory, NumberOfPages, AddRegion);
  if (!AddRegion) {
    CoreFreeMemoryMapStack ();
  }

  return EFI_SUCCESS;
}

/**
  Frees previous allocated pages.

  @param[in]  Memory                 Base address of memory being freed.
  @param[in]  NumberOfPages          The number of pages to free.
  @param[in]  IsGuarded              Is the memory to free guarded or not.

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range.
  @retval EFI_INVALID_PARAMETER  Address not aligned, Address is zero or NumberOfPages is zero.
  @return EFI_SUCCESS            Pages successfully freed.

**/
EFI_STATUS
EFIAPI
SmmInternalFreePages (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages,
  IN BOOLEAN               IsGuarded
  )
{
  if (IsGuarded) {
    return SmmInternalFreePagesExWithGuard (Memory, NumberOfPages, FALSE);
  }

  return SmmInternalFreePagesEx (Memory, NumberOfPages, FALSE);
}

/**
  Check whether the input range is in memory map.

  @param  Memory                 Base address of memory being inputed.
  @param  NumberOfPages          The number of pages.

  @retval TRUE   In memory map.
  @retval FALSE  Not in memory map.

**/
BOOLEAN
InMemMap (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages
  )
{
  LIST_ENTRY            *Link;
  MEMORY_MAP            *Entry;
  EFI_PHYSICAL_ADDRESS  Last;

  Last = Memory + EFI_PAGES_TO_SIZE (NumberOfPages) - 1;

  Link = gMemoryMap.ForwardLink;
  while (Link != &gMemoryMap) {
    Entry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
    Link  = Link->ForwardLink;

    if ((Entry->Start <= Memory) && (Entry->End >= Last)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Frees previous allocated pages.

  @param  Memory                 Base address of memory being freed.
  @param  NumberOfPages          The number of pages to free.

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range.
  @retval EFI_INVALID_PARAMETER  Address not aligned, Address is zero or NumberOfPages is zero.
  @return EFI_SUCCESS            Pages successfully freed.

**/
EFI_STATUS
EFIAPI
SmmFreePages (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages
  )
{
  EFI_STATUS  Status;
  BOOLEAN     IsGuarded;

  if (!InMemMap (Memory, NumberOfPages)) {
    return EFI_NOT_FOUND;
  }

  IsGuarded = IsHeapGuardEnabled () && IsMemoryGuarded (Memory);
  Status    = SmmInternalFreePages (Memory, NumberOfPages, IsGuarded);
  if (!EFI_ERROR (Status)) {
    SmmCoreUpdateProfile (
      (EFI_PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0),
      MemoryProfileActionFreePages,
      EfiMaxMemoryType,
      EFI_PAGES_TO_SIZE (NumberOfPages),
      (VOID *)(UINTN)Memory,
      NULL
      );
  }

  return Status;
}

/**
  Add free SMRAM region for use by memory service.

  @param  MemBase                Base address of memory region.
  @param  MemLength              Length of the memory region.
  @param  Type                   Memory type.
  @param  Attributes             Memory region state.

**/
VOID
SmmAddMemoryRegion (
  IN  EFI_PHYSICAL_ADDRESS  MemBase,
  IN  UINT64                MemLength,
  IN  EFI_MEMORY_TYPE       Type,
  IN  UINT64                Attributes
  )
{
  UINTN  AlignedMemBase;

  //
  // Add EfiRuntimeServicesData for memory regions that is already allocated, needs testing, or needs ECC initialization
  //
  if ((Attributes & (EFI_ALLOCATED | EFI_NEEDS_TESTING | EFI_NEEDS_ECC_INITIALIZATION)) != 0) {
    Type = EfiRuntimeServicesData;
  } else {
    Type = EfiConventionalMemory;
  }

  DEBUG ((DEBUG_INFO, "SmmAddMemoryRegion\n"));
  DEBUG ((DEBUG_INFO, "  MemBase    - 0x%lx\n", MemBase));
  DEBUG ((DEBUG_INFO, "  MemLength  - 0x%lx\n", MemLength));
  DEBUG ((DEBUG_INFO, "  Type       - 0x%x\n", Type));
  DEBUG ((DEBUG_INFO, "  Attributes - 0x%lx\n", Attributes));

  //
  // Align range on an EFI_PAGE_SIZE boundary
  //
  AlignedMemBase = (UINTN)(MemBase + EFI_PAGE_MASK) & ~EFI_PAGE_MASK;
  MemLength     -= AlignedMemBase - MemBase;
  if (Type == EfiConventionalMemory) {
    SmmInternalFreePagesEx (AlignedMemBase, TRUNCATE_TO_PAGES ((UINTN)MemLength), TRUE);
  } else {
    ConvertSmmMemoryMapEntry (EfiRuntimeServicesData, AlignedMemBase, TRUNCATE_TO_PAGES ((UINTN)MemLength), TRUE);
  }

  CoreFreeMemoryMapStack ();
}

/**
  This function returns a copy of the current memory map. The map is an array of
  memory descriptors, each of which describes a contiguous block of memory.

  @param[in, out]  MemoryMapSize          A pointer to the size, in bytes, of the
                                          MemoryMap buffer. On input, this is the size of
                                          the buffer allocated by the caller.  On output,
                                          it is the size of the buffer returned by the
                                          firmware  if the buffer was large enough, or the
                                          size of the buffer needed  to contain the map if
                                          the buffer was too small.
  @param[in, out]  MemoryMap              A pointer to the buffer in which firmware places
                                          the current memory map.
  @param[out]      MapKey                 A pointer to the location in which firmware
                                          returns the key for the current memory map.
  @param[out]      DescriptorSize         A pointer to the location in which firmware
                                          returns the size, in bytes, of an individual
                                          EFI_MEMORY_DESCRIPTOR.
  @param[out]      DescriptorVersion      A pointer to the location in which firmware
                                          returns the version number associated with the
                                          EFI_MEMORY_DESCRIPTOR.

  @retval EFI_SUCCESS            The memory map was returned in the MemoryMap
                                 buffer.
  @retval EFI_BUFFER_TOO_SMALL   The MemoryMap buffer was too small. The current
                                 buffer size needed to hold the memory map is
                                 returned in MemoryMapSize.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.

**/
EFI_STATUS
EFIAPI
SmmCoreGetMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  OUT UINTN                     *MapKey,
  OUT UINTN                     *DescriptorSize,
  OUT UINT32                    *DescriptorVersion
  )
{
  UINTN       Count;
  LIST_ENTRY  *Link;
  MEMORY_MAP  *Entry;
  UINTN       Size;
  UINTN       BufferSize;

  Size = sizeof (EFI_MEMORY_DESCRIPTOR);

  //
  // Make sure Size != sizeof(EFI_MEMORY_DESCRIPTOR). This will
  // prevent people from having pointer math bugs in their code.
  // now you have to use *DescriptorSize to make things work.
  //
  Size += sizeof (UINT64) - (Size % sizeof (UINT64));

  if (DescriptorSize != NULL) {
    *DescriptorSize = Size;
  }

  if (DescriptorVersion != NULL) {
    *DescriptorVersion = EFI_MEMORY_DESCRIPTOR_VERSION;
  }

  Count      = GetSmmMemoryMapEntryCount ();
  BufferSize = Size * Count;
  if (*MemoryMapSize < BufferSize) {
    *MemoryMapSize = BufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  *MemoryMapSize = BufferSize;
  if (MemoryMap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (MemoryMap, BufferSize);
  Link = gMemoryMap.ForwardLink;
  while (Link != &gMemoryMap) {
    Entry = CR (Link, MEMORY_MAP, Link, MEMORY_MAP_SIGNATURE);
    Link  = Link->ForwardLink;

    MemoryMap->Type          = Entry->Type;
    MemoryMap->PhysicalStart = Entry->Start;
    MemoryMap->NumberOfPages = RShiftU64 (Entry->End - Entry->Start + 1, EFI_PAGE_SHIFT);

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, Size);
  }

  return EFI_SUCCESS;
}
