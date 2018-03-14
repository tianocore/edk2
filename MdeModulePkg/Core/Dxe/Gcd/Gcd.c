/** @file
  The file contains the GCD related services in the EFI Boot Services Table.
  The GCD services are used to manage the memory and I/O regions that
  are accessible to the CPU that is executing the DXE core.

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"
#include "Gcd.h"
#include "Mem/HeapGuard.h"

#define MINIMUM_INITIAL_MEMORY_SIZE 0x10000

#define MEMORY_ATTRIBUTE_MASK         (EFI_RESOURCE_ATTRIBUTE_PRESENT             | \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED         | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED              | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED      | \
                                       EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED     | \
                                       EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_16_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_32_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_64_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_PERSISTENT          )

#define TESTED_MEMORY_ATTRIBUTES      (EFI_RESOURCE_ATTRIBUTE_PRESENT     | \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED      )

#define INITIALIZED_MEMORY_ATTRIBUTES (EFI_RESOURCE_ATTRIBUTE_PRESENT     | \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED )

#define PRESENT_MEMORY_ATTRIBUTES     (EFI_RESOURCE_ATTRIBUTE_PRESENT)

#define EXCLUSIVE_MEMORY_ATTRIBUTES   (EFI_MEMORY_UC | EFI_MEMORY_WC | \
                                       EFI_MEMORY_WT | EFI_MEMORY_WB | \
                                       EFI_MEMORY_WP | EFI_MEMORY_UCE)

#define NONEXCLUSIVE_MEMORY_ATTRIBUTES (EFI_MEMORY_XP | EFI_MEMORY_RP | \
                                        EFI_MEMORY_RO)

#define INVALID_CPU_ARCH_ATTRIBUTES   0xffffffff

//
// Module Variables
//
EFI_LOCK           mGcdMemorySpaceLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);
EFI_LOCK           mGcdIoSpaceLock     = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);
LIST_ENTRY         mGcdMemorySpaceMap  = INITIALIZE_LIST_HEAD_VARIABLE (mGcdMemorySpaceMap);
LIST_ENTRY         mGcdIoSpaceMap      = INITIALIZE_LIST_HEAD_VARIABLE (mGcdIoSpaceMap);

EFI_GCD_MAP_ENTRY mGcdMemorySpaceMapEntryTemplate = {
  EFI_GCD_MAP_SIGNATURE,
  {
    NULL,
    NULL
  },
  0,
  0,
  0,
  0,
  EfiGcdMemoryTypeNonExistent,
  (EFI_GCD_IO_TYPE) 0,
  NULL,
  NULL
};

EFI_GCD_MAP_ENTRY mGcdIoSpaceMapEntryTemplate = {
  EFI_GCD_MAP_SIGNATURE,
  {
    NULL,
    NULL
  },
  0,
  0,
  0,
  0,
  (EFI_GCD_MEMORY_TYPE) 0,
  EfiGcdIoTypeNonExistent,
  NULL,
  NULL
};

GCD_ATTRIBUTE_CONVERSION_ENTRY mAttributeConversionTable[] = {
  { EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE,             EFI_MEMORY_UC,              TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_UNCACHED_EXPORTED,       EFI_MEMORY_UCE,             TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE,       EFI_MEMORY_WC,              TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE, EFI_MEMORY_WT,              TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE,    EFI_MEMORY_WB,              TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_READ_PROTECTABLE,        EFI_MEMORY_RP,              TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE,       EFI_MEMORY_WP,              TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTABLE,   EFI_MEMORY_XP,              TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTABLE,   EFI_MEMORY_RO,              TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_PRESENT,                 EFI_MEMORY_PRESENT,         FALSE },
  { EFI_RESOURCE_ATTRIBUTE_INITIALIZED,             EFI_MEMORY_INITIALIZED,     FALSE },
  { EFI_RESOURCE_ATTRIBUTE_TESTED,                  EFI_MEMORY_TESTED,          FALSE },
  { EFI_RESOURCE_ATTRIBUTE_PERSISTABLE,             EFI_MEMORY_NV,              TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE,           EFI_MEMORY_MORE_RELIABLE,   TRUE  },
  { 0,                                              0,                          FALSE }
};

///
/// Lookup table used to print GCD Memory Space Map
///
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *mGcdMemoryTypeNames[] = {
  "NonExist ",  // EfiGcdMemoryTypeNonExistent
  "Reserved ",  // EfiGcdMemoryTypeReserved
  "SystemMem",  // EfiGcdMemoryTypeSystemMemory
  "MMIO     ",  // EfiGcdMemoryTypeMemoryMappedIo
  "PersisMem",  // EfiGcdMemoryTypePersistent
  "MoreRelia",  // EfiGcdMemoryTypeMoreReliable
  "Unknown  "   // EfiGcdMemoryTypeMaximum
};

///
/// Lookup table used to print GCD I/O Space Map
///
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *mGcdIoTypeNames[] = {
  "NonExist",  // EfiGcdIoTypeNonExistent
  "Reserved",  // EfiGcdIoTypeReserved
  "I/O     ",  // EfiGcdIoTypeIo
  "Unknown "   // EfiGcdIoTypeMaximum 
};

///
/// Lookup table used to print GCD Allocation Types
///
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 *mGcdAllocationTypeNames[] = {
  "AnySearchBottomUp        ",  // EfiGcdAllocateAnySearchBottomUp
  "MaxAddressSearchBottomUp ",  // EfiGcdAllocateMaxAddressSearchBottomUp
  "AtAddress                ",  // EfiGcdAllocateAddress
  "AnySearchTopDown         ",  // EfiGcdAllocateAnySearchTopDown
  "MaxAddressSearchTopDown  ",  // EfiGcdAllocateMaxAddressSearchTopDown
  "Unknown                  "   // EfiGcdMaxAllocateType
};

/**
  Dump the entire contents if the GCD Memory Space Map using DEBUG() macros when
  PcdDebugPrintErrorLevel has the DEBUG_GCD bit set.

  @param  InitialMap  TRUE if the initial GCD Memory Map is being dumped.  Otherwise, FALSE.
  
**/
VOID
EFIAPI
CoreDumpGcdMemorySpaceMap (
  BOOLEAN  InitialMap
  )
{
  DEBUG_CODE (
    EFI_STATUS                       Status;
    UINTN                            NumberOfDescriptors;
    EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;
    UINTN                            Index;
   
    Status = CoreGetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
    ASSERT (Status == EFI_SUCCESS && MemorySpaceMap != NULL);

    if (InitialMap) {
      DEBUG ((DEBUG_GCD, "GCD:Initial GCD Memory Space Map\n"));
    }
    DEBUG ((DEBUG_GCD, "GCDMemType Range                             Capabilities     Attributes      \n"));
    DEBUG ((DEBUG_GCD, "========== ================================= ================ ================\n"));
    for (Index = 0; Index < NumberOfDescriptors; Index++) {
      DEBUG ((DEBUG_GCD, "%a  %016lx-%016lx %016lx %016lx%c\n", 
        mGcdMemoryTypeNames[MIN (MemorySpaceMap[Index].GcdMemoryType, EfiGcdMemoryTypeMaximum)],
        MemorySpaceMap[Index].BaseAddress, 
        MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length - 1,
        MemorySpaceMap[Index].Capabilities, 
        MemorySpaceMap[Index].Attributes,
        MemorySpaceMap[Index].ImageHandle == NULL ? ' ' : '*'
        ));
    }
    DEBUG ((DEBUG_GCD, "\n"));
    FreePool (MemorySpaceMap);
  );
}

/**
  Dump the entire contents if the GCD I/O Space Map using DEBUG() macros when 
  PcdDebugPrintErrorLevel has the DEBUG_GCD bit set.

  @param  InitialMap  TRUE if the initial GCD I/O Map is being dumped.  Otherwise, FALSE.
  
**/
VOID
EFIAPI
CoreDumpGcdIoSpaceMap (
  BOOLEAN  InitialMap
  )
{
  DEBUG_CODE (
    EFI_STATUS                   Status;
    UINTN                        NumberOfDescriptors;
    EFI_GCD_IO_SPACE_DESCRIPTOR  *IoSpaceMap;
    UINTN                        Index;
    
    Status = CoreGetIoSpaceMap (&NumberOfDescriptors, &IoSpaceMap);
    ASSERT (Status == EFI_SUCCESS && IoSpaceMap != NULL);
    
    if (InitialMap) {
      DEBUG ((DEBUG_GCD, "GCD:Initial GCD I/O Space Map\n"));
    }  
    
    DEBUG ((DEBUG_GCD, "GCDIoType  Range                            \n"));
    DEBUG ((DEBUG_GCD, "========== =================================\n"));
    for (Index = 0; Index < NumberOfDescriptors; Index++) {
      DEBUG ((DEBUG_GCD, "%a   %016lx-%016lx%c\n", 
        mGcdIoTypeNames[MIN (IoSpaceMap[Index].GcdIoType, EfiGcdIoTypeMaximum)],
        IoSpaceMap[Index].BaseAddress, 
        IoSpaceMap[Index].BaseAddress + IoSpaceMap[Index].Length - 1,
        IoSpaceMap[Index].ImageHandle == NULL ? ' ' : '*'
        ));
    }
    DEBUG ((DEBUG_GCD, "\n"));
    FreePool (IoSpaceMap);
  );
}
  
/**
  Validate resource descriptor HOB's attributes.

  If Attributes includes some memory resource's settings, it should include 
  the corresponding capabilites also.

  @param  Attributes  Resource descriptor HOB attributes.

**/
VOID
CoreValidateResourceDescriptorHobAttributes (
  IN UINT64  Attributes
  )
{
  ASSERT (((Attributes & EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED) == 0) ||
          ((Attributes & EFI_RESOURCE_ATTRIBUTE_READ_PROTECTABLE) != 0));
  ASSERT (((Attributes & EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED) == 0) ||
          ((Attributes & EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE) != 0));
  ASSERT (((Attributes & EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED) == 0) ||
          ((Attributes & EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTABLE) != 0));
  ASSERT (((Attributes & EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED) == 0) ||
          ((Attributes & EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTABLE) != 0));
  ASSERT (((Attributes & EFI_RESOURCE_ATTRIBUTE_PERSISTENT) == 0) ||
          ((Attributes & EFI_RESOURCE_ATTRIBUTE_PERSISTABLE) != 0));
}

/**
  Acquire memory lock on mGcdMemorySpaceLock.

**/
VOID
CoreAcquireGcdMemoryLock (
  VOID
  )
{
  CoreAcquireLock (&mGcdMemorySpaceLock);
}



/**
  Release memory lock on mGcdMemorySpaceLock.

**/
VOID
CoreReleaseGcdMemoryLock (
  VOID
  )
{
  CoreReleaseLock (&mGcdMemorySpaceLock);
}



/**
  Acquire memory lock on mGcdIoSpaceLock.

**/
VOID
CoreAcquireGcdIoLock (
  VOID
  )
{
  CoreAcquireLock (&mGcdIoSpaceLock);
}


/**
  Release memory lock on mGcdIoSpaceLock.

**/
VOID
CoreReleaseGcdIoLock (
  VOID
  )
{
  CoreReleaseLock (&mGcdIoSpaceLock);
}



//
// GCD Initialization Worker Functions
//
/**
  Aligns a value to the specified boundary.

  @param  Value                  64 bit value to align
  @param  Alignment              Log base 2 of the boundary to align Value to
  @param  RoundUp                TRUE if Value is to be rounded up to the nearest
                                 aligned boundary.  FALSE is Value is to be
                                 rounded down to the nearest aligned boundary.

  @return A 64 bit value is the aligned to the value nearest Value with an alignment by Alignment.

**/
UINT64
AlignValue (
  IN UINT64   Value,
  IN UINTN    Alignment,
  IN BOOLEAN  RoundUp
  )
{
  UINT64  AlignmentMask;

  AlignmentMask = LShiftU64 (1, Alignment) - 1;
  if (RoundUp) {
    Value += AlignmentMask;
  }
  return Value & (~AlignmentMask);
}


/**
  Aligns address to the page boundary.

  @param  Value                  64 bit address to align

  @return A 64 bit value is the aligned to the value nearest Value with an alignment by Alignment.

**/
UINT64
PageAlignAddress (
  IN UINT64 Value
  )
{
  return AlignValue (Value, EFI_PAGE_SHIFT, TRUE);
}


/**
  Aligns length to the page boundary.

  @param  Value                  64 bit length to align

  @return A 64 bit value is the aligned to the value nearest Value with an alignment by Alignment.

**/
UINT64
PageAlignLength (
  IN UINT64 Value
  )
{
  return AlignValue (Value, EFI_PAGE_SHIFT, FALSE);
}

//
// GCD Memory Space Worker Functions
//

/**
  Allocate pool for two entries.

  @param  TopEntry               An entry of GCD map
  @param  BottomEntry            An entry of GCD map

  @retval EFI_OUT_OF_RESOURCES   No enough buffer to be allocated.
  @retval EFI_SUCCESS            Both entries successfully allocated.

**/
EFI_STATUS
CoreAllocateGcdMapEntry (
  IN OUT EFI_GCD_MAP_ENTRY  **TopEntry,
  IN OUT EFI_GCD_MAP_ENTRY  **BottomEntry
  )
{
  //
  // Set to mOnGuarding to TRUE before memory allocation. This will make sure
  // that the entry memory is not "guarded" by HeapGuard. Otherwise it might
  // cause problem when it's freed (if HeapGuard is enabled).
  //
  mOnGuarding = TRUE;
  *TopEntry = AllocateZeroPool (sizeof (EFI_GCD_MAP_ENTRY));
  mOnGuarding = FALSE;
  if (*TopEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mOnGuarding = TRUE;
  *BottomEntry = AllocateZeroPool (sizeof (EFI_GCD_MAP_ENTRY));
  mOnGuarding = FALSE;
  if (*BottomEntry == NULL) {
    CoreFreePool (*TopEntry);
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}


/**
  Internal function.  Inserts a new descriptor into a sorted list

  @param  Link                   The linked list to insert the range BaseAddress
                                 and Length into
  @param  Entry                  A pointer to the entry that is inserted
  @param  BaseAddress            The base address of the new range
  @param  Length                 The length of the new range in bytes
  @param  TopEntry               Top pad entry to insert if needed.
  @param  BottomEntry            Bottom pad entry to insert if needed.

  @retval EFI_SUCCESS            The new range was inserted into the linked list

**/
EFI_STATUS
CoreInsertGcdMapEntry (
  IN LIST_ENTRY           *Link,
  IN EFI_GCD_MAP_ENTRY     *Entry,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN EFI_GCD_MAP_ENTRY     *TopEntry,
  IN EFI_GCD_MAP_ENTRY     *BottomEntry
  )
{
  ASSERT (Length != 0);

  if (BaseAddress > Entry->BaseAddress) {
    ASSERT (BottomEntry->Signature == 0);

    CopyMem (BottomEntry, Entry, sizeof (EFI_GCD_MAP_ENTRY));
    Entry->BaseAddress      = BaseAddress;
    BottomEntry->EndAddress = BaseAddress - 1;
    InsertTailList (Link, &BottomEntry->Link);
  }

  if ((BaseAddress + Length - 1) < Entry->EndAddress) {
    ASSERT (TopEntry->Signature == 0);

    CopyMem (TopEntry, Entry, sizeof (EFI_GCD_MAP_ENTRY));
    TopEntry->BaseAddress = BaseAddress + Length;
    Entry->EndAddress     = BaseAddress + Length - 1;
    InsertHeadList (Link, &TopEntry->Link);
  }

  return EFI_SUCCESS;
}


/**
  Merge the Gcd region specified by Link and its adjacent entry.

  @param  Link                   Specify the entry to be merged (with its
                                 adjacent entry).
  @param  Forward                Direction (forward or backward).
  @param  Map                    Boundary.

  @retval EFI_SUCCESS            Successfully returned.
  @retval EFI_UNSUPPORTED        These adjacent regions could not merge.

**/
EFI_STATUS
CoreMergeGcdMapEntry (
  IN LIST_ENTRY      *Link,
  IN BOOLEAN         Forward,
  IN LIST_ENTRY      *Map
  )
{
  LIST_ENTRY         *AdjacentLink;
  EFI_GCD_MAP_ENTRY  *Entry;
  EFI_GCD_MAP_ENTRY  *AdjacentEntry;

  //
  // Get adjacent entry
  //
  if (Forward) {
    AdjacentLink = Link->ForwardLink;
  } else {
    AdjacentLink = Link->BackLink;
  }

  //
  // If AdjacentLink is the head of the list, then no merge can be performed
  //
  if (AdjacentLink == Map) {
    return EFI_SUCCESS;
  }

  Entry         = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
  AdjacentEntry = CR (AdjacentLink, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);

  if (Entry->Capabilities != AdjacentEntry->Capabilities) {
    return EFI_UNSUPPORTED;
  }
  if (Entry->Attributes != AdjacentEntry->Attributes) {
    return EFI_UNSUPPORTED;
  }
  if (Entry->GcdMemoryType != AdjacentEntry->GcdMemoryType) {
    return EFI_UNSUPPORTED;
  }
  if (Entry->GcdIoType != AdjacentEntry->GcdIoType) {
    return EFI_UNSUPPORTED;
  }
  if (Entry->ImageHandle != AdjacentEntry->ImageHandle) {
    return EFI_UNSUPPORTED;
  }
  if (Entry->DeviceHandle != AdjacentEntry->DeviceHandle) {
    return EFI_UNSUPPORTED;
  }

  if (Forward) {
    Entry->EndAddress  = AdjacentEntry->EndAddress;
  } else {
    Entry->BaseAddress = AdjacentEntry->BaseAddress;
  }
  RemoveEntryList (AdjacentLink);
  CoreFreePool (AdjacentEntry);

  return EFI_SUCCESS;
}


/**
  Merge adjacent entries on total chain.

  @param  TopEntry               Top entry of GCD map.
  @param  BottomEntry            Bottom entry of GCD map.
  @param  StartLink              Start link of the list for this loop.
  @param  EndLink                End link of the list for this loop.
  @param  Map                    Boundary.

  @retval EFI_SUCCESS            GCD map successfully cleaned up.

**/
EFI_STATUS
CoreCleanupGcdMapEntry (
  IN EFI_GCD_MAP_ENTRY  *TopEntry,
  IN EFI_GCD_MAP_ENTRY  *BottomEntry,
  IN LIST_ENTRY         *StartLink,
  IN LIST_ENTRY         *EndLink,
  IN LIST_ENTRY         *Map
  )
{
  LIST_ENTRY  *Link;

  if (TopEntry->Signature == 0) {
    CoreFreePool (TopEntry);
  }
  if (BottomEntry->Signature == 0) {
    CoreFreePool (BottomEntry);
  }

  Link = StartLink;
  while (Link != EndLink->ForwardLink) {
    CoreMergeGcdMapEntry (Link, FALSE, Map);
    Link = Link->ForwardLink;
  }
  CoreMergeGcdMapEntry (EndLink, TRUE, Map);

  return EFI_SUCCESS;
}


/**
  Search a segment of memory space in GCD map. The result is a range of GCD entry list.

  @param  BaseAddress            The start address of the segment.
  @param  Length                 The length of the segment.
  @param  StartLink              The first GCD entry involves this segment of
                                 memory space.
  @param  EndLink                The first GCD entry involves this segment of
                                 memory space.
  @param  Map                    Points to the start entry to search.

  @retval EFI_SUCCESS            Successfully found the entry.
  @retval EFI_NOT_FOUND          Not found.

**/
EFI_STATUS
CoreSearchGcdMapEntry (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  OUT LIST_ENTRY            **StartLink,
  OUT LIST_ENTRY            **EndLink,
  IN  LIST_ENTRY            *Map
  )
{
  LIST_ENTRY         *Link;
  EFI_GCD_MAP_ENTRY  *Entry;

  ASSERT (Length != 0);

  *StartLink = NULL;
  *EndLink   = NULL;

  Link = Map->ForwardLink;
  while (Link != Map) {
    Entry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    if (BaseAddress >= Entry->BaseAddress && BaseAddress <= Entry->EndAddress) {
      *StartLink = Link;
    }
    if (*StartLink != NULL) {
      if ((BaseAddress + Length - 1) >= Entry->BaseAddress &&
          (BaseAddress + Length - 1) <= Entry->EndAddress     ) {
        *EndLink = Link;
        return EFI_SUCCESS;
      }
    }
    Link = Link->ForwardLink;
  }

  return EFI_NOT_FOUND;
}


/**
  Count the amount of GCD map entries.

  @param  Map                    Points to the start entry to do the count loop.

  @return The count.

**/
UINTN
CoreCountGcdMapEntry (
  IN LIST_ENTRY  *Map
  )
{
  UINTN           Count;
  LIST_ENTRY      *Link;

  Count = 0;
  Link = Map->ForwardLink;
  while (Link != Map) {
    Count++;
    Link = Link->ForwardLink;
  }

  return Count;
}



/**
  Return the memory attribute specified by Attributes

  @param  Attributes             A num with some attribute bits on.

  @return The enum value of memory attribute.

**/
UINT64
ConverToCpuArchAttributes (
  UINT64 Attributes
  )
{
  UINT64      CpuArchAttributes;

  if ((Attributes & ~(EXCLUSIVE_MEMORY_ATTRIBUTES |
                      NONEXCLUSIVE_MEMORY_ATTRIBUTES)) != 0) {
    return INVALID_CPU_ARCH_ATTRIBUTES;
  }

  CpuArchAttributes = Attributes & NONEXCLUSIVE_MEMORY_ATTRIBUTES;

  if ( (Attributes & EFI_MEMORY_UC) == EFI_MEMORY_UC) {
    CpuArchAttributes |= EFI_MEMORY_UC;
  } else if ( (Attributes & EFI_MEMORY_WC ) == EFI_MEMORY_WC) {
    CpuArchAttributes |= EFI_MEMORY_WC;
  } else if ( (Attributes & EFI_MEMORY_WT ) == EFI_MEMORY_WT) {
    CpuArchAttributes |= EFI_MEMORY_WT;
  } else if ( (Attributes & EFI_MEMORY_WB) == EFI_MEMORY_WB) {
    CpuArchAttributes |= EFI_MEMORY_WB;
  } else if ( (Attributes & EFI_MEMORY_UCE) == EFI_MEMORY_UCE) {
    CpuArchAttributes |= EFI_MEMORY_UCE;
  } else if ( (Attributes & EFI_MEMORY_WP) == EFI_MEMORY_WP) {
    CpuArchAttributes |= EFI_MEMORY_WP;
  }

  return CpuArchAttributes;
}


/**
  Do operation on a segment of memory space specified (add, free, remove, change attribute ...).

  @param  Operation              The type of the operation
  @param  GcdMemoryType          Additional information for the operation
  @param  GcdIoType              Additional information for the operation
  @param  BaseAddress            Start address of the segment
  @param  Length                 length of the segment
  @param  Capabilities           The alterable attributes of a newly added entry
  @param  Attributes             The attributes needs to be set

  @retval EFI_INVALID_PARAMETER  Length is 0 or address (length) not aligned when
                                 setting attribute.
  @retval EFI_SUCCESS            Action successfully done.
  @retval EFI_UNSUPPORTED        Could not find the proper descriptor on this
                                 segment or  set an upsupported attribute.
  @retval EFI_ACCESS_DENIED      Operate on an space non-exist or is used for an
                                 image.
  @retval EFI_NOT_FOUND          Free a non-using space or remove a non-exist
                                 space, and so on.
  @retval EFI_OUT_OF_RESOURCES   No buffer could be allocated.
  @retval EFI_NOT_AVAILABLE_YET  The attributes cannot be set because CPU architectural protocol
                                 is not available yet.
**/
EFI_STATUS
CoreConvertSpace (
  IN UINTN                 Operation,
  IN EFI_GCD_MEMORY_TYPE   GcdMemoryType,
  IN EFI_GCD_IO_TYPE       GcdIoType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Capabilities,
  IN UINT64                Attributes
  )
{
  EFI_STATUS         Status;
  LIST_ENTRY         *Map;
  LIST_ENTRY         *Link;
  EFI_GCD_MAP_ENTRY  *Entry;
  EFI_GCD_MAP_ENTRY  *TopEntry;
  EFI_GCD_MAP_ENTRY  *BottomEntry;
  LIST_ENTRY         *StartLink;
  LIST_ENTRY         *EndLink;
  UINT64             CpuArchAttributes;

  if (Length == 0) {
    DEBUG ((DEBUG_GCD, "  Status = %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }

  Map = NULL;
  if ((Operation & GCD_MEMORY_SPACE_OPERATION) != 0) {
    CoreAcquireGcdMemoryLock ();
    Map = &mGcdMemorySpaceMap;
  } else if ((Operation & GCD_IO_SPACE_OPERATION) != 0) {
    CoreAcquireGcdIoLock ();
    Map = &mGcdIoSpaceMap;
  } else {
    ASSERT (FALSE);
  }

  //
  // Search for the list of descriptors that cover the range BaseAddress to BaseAddress+Length
  //
  Status = CoreSearchGcdMapEntry (BaseAddress, Length, &StartLink, &EndLink, Map);
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;

    goto Done;
  }
  ASSERT (StartLink != NULL && EndLink != NULL);

  //
  // Verify that the list of descriptors are unallocated non-existent memory.
  //
  Link = StartLink;
  while (Link != EndLink->ForwardLink) {
    Entry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    switch (Operation) {
    //
    // Add operations
    //
    case GCD_ADD_MEMORY_OPERATION:
      if (Entry->GcdMemoryType != EfiGcdMemoryTypeNonExistent ||
          Entry->ImageHandle   != NULL                           ) {
        Status = EFI_ACCESS_DENIED;
        goto Done;
      }
      break;
    case GCD_ADD_IO_OPERATION:
      if (Entry->GcdIoType   != EfiGcdIoTypeNonExistent ||
          Entry->ImageHandle != NULL                       ) {
        Status = EFI_ACCESS_DENIED;
        goto Done;
      }
      break;
    //
    // Free operations
    //
    case GCD_FREE_MEMORY_OPERATION:
    case GCD_FREE_IO_OPERATION:
      if (Entry->ImageHandle == NULL) {
        Status = EFI_NOT_FOUND;
        goto Done;
      }
      break;
    //
    // Remove operations
    //
    case GCD_REMOVE_MEMORY_OPERATION:
      if (Entry->GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
        Status = EFI_NOT_FOUND;
        goto Done;
      }
      if (Entry->ImageHandle != NULL) {
        Status = EFI_ACCESS_DENIED;
        goto Done;
      }
      break;
    case GCD_REMOVE_IO_OPERATION:
      if (Entry->GcdIoType == EfiGcdIoTypeNonExistent) {
        Status = EFI_NOT_FOUND;
        goto Done;
      }
      if (Entry->ImageHandle != NULL) {
        Status = EFI_ACCESS_DENIED;
        goto Done;
      }
      break;
    //
    // Set attributes operation
    //
    case GCD_SET_ATTRIBUTES_MEMORY_OPERATION:
      if ((Attributes & EFI_MEMORY_RUNTIME) != 0) {
        if ((BaseAddress & EFI_PAGE_MASK) != 0 || (Length & EFI_PAGE_MASK) != 0) {
          Status = EFI_INVALID_PARAMETER;
          goto Done;
        }
      }
      if ((Entry->Capabilities & Attributes) != Attributes) {
        Status = EFI_UNSUPPORTED;
        goto Done;
      }
      break;
    //
    // Set capabilities operation
    //
    case GCD_SET_CAPABILITIES_MEMORY_OPERATION:
      if ((BaseAddress & EFI_PAGE_MASK) != 0 || (Length & EFI_PAGE_MASK) != 0) {
        Status = EFI_INVALID_PARAMETER;

        goto Done;
      }
      //
      // Current attributes must still be supported with new capabilities
      //
      if ((Capabilities & Entry->Attributes) != Entry->Attributes) {
        Status = EFI_UNSUPPORTED;
        goto Done;
      }
      break;
    }
    Link = Link->ForwardLink;
  }

  //
  // Allocate work space to perform this operation
  //
  Status = CoreAllocateGcdMapEntry (&TopEntry, &BottomEntry);
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  ASSERT (TopEntry != NULL && BottomEntry != NULL);

  if (Operation == GCD_SET_ATTRIBUTES_MEMORY_OPERATION) {
    //
    // Call CPU Arch Protocol to attempt to set attributes on the range
    //
    CpuArchAttributes = ConverToCpuArchAttributes (Attributes);
    if (CpuArchAttributes != INVALID_CPU_ARCH_ATTRIBUTES) {
      if (gCpu == NULL) {
        Status = EFI_NOT_AVAILABLE_YET;
      } else {
        Status = gCpu->SetMemoryAttributes (
                         gCpu,
                         BaseAddress,
                         Length,
                         CpuArchAttributes
                         );
      }
      if (EFI_ERROR (Status)) {
        CoreFreePool (TopEntry);
        CoreFreePool (BottomEntry);
        goto Done;
      }
    }
  }

  //
  // Convert/Insert the list of descriptors from StartLink to EndLink
  //
  Link = StartLink;
  while (Link != EndLink->ForwardLink) {
    Entry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    CoreInsertGcdMapEntry (Link, Entry, BaseAddress, Length, TopEntry, BottomEntry);
    switch (Operation) {
    //
    // Add operations
    //
    case GCD_ADD_MEMORY_OPERATION:
      Entry->GcdMemoryType = GcdMemoryType;
      if (GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo) {
        Entry->Capabilities  = Capabilities | EFI_MEMORY_RUNTIME | EFI_MEMORY_PORT_IO;
      } else {
        Entry->Capabilities  = Capabilities | EFI_MEMORY_RUNTIME;
      }
      break;
    case GCD_ADD_IO_OPERATION:
      Entry->GcdIoType = GcdIoType;
      break;
    //
    // Free operations
    //
    case GCD_FREE_MEMORY_OPERATION:
    case GCD_FREE_IO_OPERATION:
      Entry->ImageHandle  = NULL;
      Entry->DeviceHandle = NULL;
      break;
    //
    // Remove operations
    //
    case GCD_REMOVE_MEMORY_OPERATION:
      Entry->GcdMemoryType = EfiGcdMemoryTypeNonExistent;
      Entry->Capabilities  = 0;
      break;
    case GCD_REMOVE_IO_OPERATION:
      Entry->GcdIoType = EfiGcdIoTypeNonExistent;
      break;
    //
    // Set attributes operation
    //
    case GCD_SET_ATTRIBUTES_MEMORY_OPERATION:
      Entry->Attributes = Attributes;
      break;
    //
    // Set capabilities operation
    //
    case GCD_SET_CAPABILITIES_MEMORY_OPERATION:
      Entry->Capabilities = Capabilities;
      break;
    }
    Link = Link->ForwardLink;
  }

  //
  // Cleanup
  //
  Status = CoreCleanupGcdMapEntry (TopEntry, BottomEntry, StartLink, EndLink, Map);

Done:
  DEBUG ((DEBUG_GCD, "  Status = %r\n", Status));

  if ((Operation & GCD_MEMORY_SPACE_OPERATION) != 0) {
    CoreReleaseGcdMemoryLock ();
    CoreDumpGcdMemorySpaceMap (FALSE);
  }
  if ((Operation & GCD_IO_SPACE_OPERATION) != 0) {
    CoreReleaseGcdIoLock ();
    CoreDumpGcdIoSpaceMap (FALSE);
  }

  return Status;
}


/**
  Check whether an entry could be used to allocate space.

  @param  Operation              Allocate memory or IO
  @param  Entry                  The entry to be tested
  @param  GcdMemoryType          The desired memory type
  @param  GcdIoType              The desired IO type

  @retval EFI_NOT_FOUND          The memory type does not match or there's an
                                 image handle on the entry.
  @retval EFI_UNSUPPORTED        The operation unsupported.
  @retval EFI_SUCCESS            It's ok for this entry to be used to allocate
                                 space.

**/
EFI_STATUS
CoreAllocateSpaceCheckEntry (
  IN UINTN                Operation,
  IN EFI_GCD_MAP_ENTRY    *Entry,
  IN EFI_GCD_MEMORY_TYPE  GcdMemoryType,
  IN EFI_GCD_IO_TYPE      GcdIoType
  )
{
  if (Entry->ImageHandle != NULL) {
    return EFI_NOT_FOUND;
  }
  switch (Operation) {
  case GCD_ALLOCATE_MEMORY_OPERATION:
    if (Entry->GcdMemoryType != GcdMemoryType) {
      return EFI_NOT_FOUND;
    }
    break;
  case GCD_ALLOCATE_IO_OPERATION:
    if (Entry->GcdIoType != GcdIoType) {
      return EFI_NOT_FOUND;
    }
    break;
  default:
    return EFI_UNSUPPORTED;
  }
  return EFI_SUCCESS;
}


/**
  Allocate space on specified address and length.

  @param  Operation              The type of operation (memory or IO)
  @param  GcdAllocateType        The type of allocate operation
  @param  GcdMemoryType          The desired memory type
  @param  GcdIoType              The desired IO type
  @param  Alignment              Align with 2^Alignment
  @param  Length                 Length to allocate
  @param  BaseAddress            Base address to allocate
  @param  ImageHandle            The image handle consume the allocated space.
  @param  DeviceHandle           The device handle consume the allocated space.

  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_NOT_FOUND          No descriptor for the desired space exists.
  @retval EFI_SUCCESS            Space successfully allocated.

**/
EFI_STATUS
CoreAllocateSpace (
  IN     UINTN                  Operation,
  IN     EFI_GCD_ALLOCATE_TYPE  GcdAllocateType,
  IN     EFI_GCD_MEMORY_TYPE    GcdMemoryType,
  IN     EFI_GCD_IO_TYPE        GcdIoType,
  IN     UINTN                  Alignment,
  IN     UINT64                 Length,
  IN OUT EFI_PHYSICAL_ADDRESS   *BaseAddress,
  IN     EFI_HANDLE             ImageHandle,
  IN     EFI_HANDLE             DeviceHandle OPTIONAL
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  AlignmentMask;
  EFI_PHYSICAL_ADDRESS  MaxAddress;
  LIST_ENTRY            *Map;
  LIST_ENTRY            *Link;
  LIST_ENTRY            *SubLink;
  EFI_GCD_MAP_ENTRY     *Entry;
  EFI_GCD_MAP_ENTRY     *TopEntry;
  EFI_GCD_MAP_ENTRY     *BottomEntry;
  LIST_ENTRY            *StartLink;
  LIST_ENTRY            *EndLink;
  BOOLEAN               Found;

  //
  // Make sure parameters are valid
  //
  if ((UINT32)GcdAllocateType >= EfiGcdMaxAllocateType) {
    DEBUG ((DEBUG_GCD, "  Status = %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }
  if ((UINT32)GcdMemoryType >= EfiGcdMemoryTypeMaximum) {
    DEBUG ((DEBUG_GCD, "  Status = %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }
  if ((UINT32)GcdIoType >= EfiGcdIoTypeMaximum) {
    DEBUG ((DEBUG_GCD, "  Status = %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }
  if (BaseAddress == NULL) {
    DEBUG ((DEBUG_GCD, "  Status = %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }
  if (ImageHandle == NULL) {
    DEBUG ((DEBUG_GCD, "  Status = %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }
  if (Alignment >= 64) {
    DEBUG ((DEBUG_GCD, "  Status = %r\n", EFI_NOT_FOUND));
    return EFI_NOT_FOUND;
  }
  if (Length == 0) {
    DEBUG ((DEBUG_GCD, "  Status = %r\n", EFI_INVALID_PARAMETER));
    return EFI_INVALID_PARAMETER;
  }

  Map = NULL;
  if ((Operation & GCD_MEMORY_SPACE_OPERATION) != 0) {
    CoreAcquireGcdMemoryLock ();
    Map = &mGcdMemorySpaceMap;
  } else if ((Operation & GCD_IO_SPACE_OPERATION) != 0) {
    CoreAcquireGcdIoLock ();
    Map = &mGcdIoSpaceMap;
  } else {
    ASSERT (FALSE);
  }

  Found     = FALSE;
  StartLink = NULL;
  EndLink   = NULL;
  //
  // Compute alignment bit mask
  //
  AlignmentMask = LShiftU64 (1, Alignment) - 1;

  if (GcdAllocateType == EfiGcdAllocateAddress) {
    //
    // Verify that the BaseAddress passed in is aligned correctly
    //
    if ((*BaseAddress & AlignmentMask) != 0) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    //
    // Search for the list of descriptors that cover the range BaseAddress to BaseAddress+Length
    //
    Status = CoreSearchGcdMapEntry (*BaseAddress, Length, &StartLink, &EndLink, Map);
    if (EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
    ASSERT (StartLink != NULL && EndLink != NULL);

    //
    // Verify that the list of descriptors are unallocated memory matching GcdMemoryType.
    //
    Link = StartLink;
    while (Link != EndLink->ForwardLink) {
      Entry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
      Link = Link->ForwardLink;
      Status = CoreAllocateSpaceCheckEntry (Operation, Entry, GcdMemoryType, GcdIoType);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }
    Found = TRUE;
  } else {

    Entry = CR (Map->BackLink, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);

    //
    // Compute the maximum address to use in the search algorithm
    //
    if (GcdAllocateType == EfiGcdAllocateMaxAddressSearchBottomUp ||
        GcdAllocateType == EfiGcdAllocateMaxAddressSearchTopDown     ) {
      MaxAddress = *BaseAddress;
    } else {
      MaxAddress = Entry->EndAddress;
    }

    //
    // Verify that the list of descriptors are unallocated memory matching GcdMemoryType.
    //
    if (GcdAllocateType == EfiGcdAllocateMaxAddressSearchTopDown ||
        GcdAllocateType == EfiGcdAllocateAnySearchTopDown ) {
      Link = Map->BackLink;
    } else {
      Link = Map->ForwardLink;
    }
    while (Link != Map) {
      Entry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);

      if (GcdAllocateType == EfiGcdAllocateMaxAddressSearchTopDown ||
          GcdAllocateType == EfiGcdAllocateAnySearchTopDown           ) {
        Link = Link->BackLink;
      } else {
        Link = Link->ForwardLink;
      }

      Status = CoreAllocateSpaceCheckEntry (Operation, Entry, GcdMemoryType, GcdIoType);
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (GcdAllocateType == EfiGcdAllocateMaxAddressSearchTopDown ||
          GcdAllocateType == EfiGcdAllocateAnySearchTopDown) {
        if ((Entry->BaseAddress + Length) > MaxAddress) {
          continue;
        }
        if (Length > (Entry->EndAddress + 1)) {
          Status = EFI_NOT_FOUND;
          goto Done;
        }
        if (Entry->EndAddress > MaxAddress) {
          *BaseAddress = MaxAddress;
        } else {
          *BaseAddress = Entry->EndAddress;
        }
        *BaseAddress = (*BaseAddress + 1 - Length) & (~AlignmentMask);
      } else {
        *BaseAddress = (Entry->BaseAddress + AlignmentMask) & (~AlignmentMask);
        if ((*BaseAddress + Length - 1) > MaxAddress) {
          Status = EFI_NOT_FOUND;
          goto Done;
        }
      }

      //
      // Search for the list of descriptors that cover the range BaseAddress to BaseAddress+Length
      //
      Status = CoreSearchGcdMapEntry (*BaseAddress, Length, &StartLink, &EndLink, Map);
      if (EFI_ERROR (Status)) {
        Status = EFI_NOT_FOUND;
        goto Done;
      }
      ASSERT (StartLink != NULL && EndLink != NULL);

      Link = StartLink;
      //
      // Verify that the list of descriptors are unallocated memory matching GcdMemoryType.
      //
      Found = TRUE;
      SubLink = StartLink;
      while (SubLink != EndLink->ForwardLink) {
        Entry = CR (SubLink, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
        Status = CoreAllocateSpaceCheckEntry (Operation, Entry, GcdMemoryType, GcdIoType);
        if (EFI_ERROR (Status)) {
          Link = SubLink;
          Found = FALSE;
          break;
        }
        SubLink = SubLink->ForwardLink;
      }
      if (Found) {
        break;
      }
    }
  }
  if (!Found) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  //
  // Allocate work space to perform this operation
  //
  Status = CoreAllocateGcdMapEntry (&TopEntry, &BottomEntry);
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  ASSERT (TopEntry != NULL && BottomEntry != NULL);

  //
  // Convert/Insert the list of descriptors from StartLink to EndLink
  //
  Link = StartLink;
  while (Link != EndLink->ForwardLink) {
    Entry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    CoreInsertGcdMapEntry (Link, Entry, *BaseAddress, Length, TopEntry, BottomEntry);
    Entry->ImageHandle  = ImageHandle;
    Entry->DeviceHandle = DeviceHandle;
    Link = Link->ForwardLink;
  }

  //
  // Cleanup
  //
  Status = CoreCleanupGcdMapEntry (TopEntry, BottomEntry, StartLink, EndLink, Map);

Done:
  DEBUG ((DEBUG_GCD, "  Status = %r", Status));
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_GCD, "  (BaseAddress = %016lx)", *BaseAddress));
  }
  DEBUG ((DEBUG_GCD, "\n"));
  
  if ((Operation & GCD_MEMORY_SPACE_OPERATION) != 0) {
    CoreReleaseGcdMemoryLock ();
    CoreDumpGcdMemorySpaceMap (FALSE);
  }
  if ((Operation & GCD_IO_SPACE_OPERATION) !=0) {
    CoreReleaseGcdIoLock ();
    CoreDumpGcdIoSpaceMap (FALSE);
  }

  return Status;
}


/**
  Add a segment of memory to GCD map.

  @param  GcdMemoryType          Memory type of the segment.
  @param  BaseAddress            Base address of the segment.
  @param  Length                 Length of the segment.
  @param  Capabilities           alterable attributes of the segment.

  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval EFI_SUCCESS            Successfully add a segment of memory space.

**/
EFI_STATUS
CoreInternalAddMemorySpace (
  IN EFI_GCD_MEMORY_TYPE   GcdMemoryType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Capabilities
  )
{
  DEBUG ((DEBUG_GCD, "GCD:AddMemorySpace(Base=%016lx,Length=%016lx)\n", BaseAddress, Length));
  DEBUG ((DEBUG_GCD, "  GcdMemoryType   = %a\n", mGcdMemoryTypeNames[MIN (GcdMemoryType, EfiGcdMemoryTypeMaximum)]));
  DEBUG ((DEBUG_GCD, "  Capabilities    = %016lx\n", Capabilities));

  //
  // Make sure parameters are valid
  //
  if (GcdMemoryType <= EfiGcdMemoryTypeNonExistent || GcdMemoryType >= EfiGcdMemoryTypeMaximum) {
    return EFI_INVALID_PARAMETER;
  }

  return CoreConvertSpace (GCD_ADD_MEMORY_OPERATION, GcdMemoryType, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, Capabilities, 0);
}

//
// GCD Core Services
//

/**
  Allocates nonexistent memory, reserved memory, system memory, or memorymapped
  I/O resources from the global coherency domain of the processor.

  @param  GcdAllocateType        The type of allocate operation
  @param  GcdMemoryType          The desired memory type
  @param  Alignment              Align with 2^Alignment
  @param  Length                 Length to allocate
  @param  BaseAddress            Base address to allocate
  @param  ImageHandle            The image handle consume the allocated space.
  @param  DeviceHandle           The device handle consume the allocated space.

  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_NOT_FOUND          No descriptor contains the desired space.
  @retval EFI_SUCCESS            Memory space successfully allocated.

**/
EFI_STATUS
EFIAPI
CoreAllocateMemorySpace (
  IN     EFI_GCD_ALLOCATE_TYPE  GcdAllocateType,
  IN     EFI_GCD_MEMORY_TYPE    GcdMemoryType,
  IN     UINTN                  Alignment,
  IN     UINT64                 Length,
  IN OUT EFI_PHYSICAL_ADDRESS   *BaseAddress,
  IN     EFI_HANDLE             ImageHandle,
  IN     EFI_HANDLE             DeviceHandle OPTIONAL
  )
{
  if (BaseAddress != NULL) {
    DEBUG ((DEBUG_GCD, "GCD:AllocateMemorySpace(Base=%016lx,Length=%016lx)\n", *BaseAddress, Length));
  } else {
    DEBUG ((DEBUG_GCD, "GCD:AllocateMemorySpace(Base=<NULL>,Length=%016lx)\n", Length));
  }
  DEBUG ((DEBUG_GCD, "  GcdAllocateType = %a\n", mGcdAllocationTypeNames[MIN (GcdAllocateType, EfiGcdMaxAllocateType)]));
  DEBUG ((DEBUG_GCD, "  GcdMemoryType   = %a\n", mGcdMemoryTypeNames[MIN (GcdMemoryType, EfiGcdMemoryTypeMaximum)]));
  DEBUG ((DEBUG_GCD, "  Alignment       = %016lx\n", LShiftU64 (1, Alignment)));
  DEBUG ((DEBUG_GCD, "  ImageHandle     = %p\n", ImageHandle));
  DEBUG ((DEBUG_GCD, "  DeviceHandle    = %p\n", DeviceHandle));
  
  return CoreAllocateSpace (
           GCD_ALLOCATE_MEMORY_OPERATION,
           GcdAllocateType,
           GcdMemoryType,
           (EFI_GCD_IO_TYPE) 0,
           Alignment,
           Length,
           BaseAddress,
           ImageHandle,
           DeviceHandle
           );
}


/**
  Adds reserved memory, system memory, or memory-mapped I/O resources to the
  global coherency domain of the processor.

  @param  GcdMemoryType          Memory type of the memory space.
  @param  BaseAddress            Base address of the memory space.
  @param  Length                 Length of the memory space.
  @param  Capabilities           alterable attributes of the memory space.

  @retval EFI_SUCCESS            Merged this memory space into GCD map.

**/
EFI_STATUS
EFIAPI
CoreAddMemorySpace (
  IN EFI_GCD_MEMORY_TYPE   GcdMemoryType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Capabilities
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PageBaseAddress;
  UINT64                PageLength;

  Status = CoreInternalAddMemorySpace (GcdMemoryType, BaseAddress, Length, Capabilities);

  if (!EFI_ERROR (Status) && ((GcdMemoryType == EfiGcdMemoryTypeSystemMemory) || (GcdMemoryType == EfiGcdMemoryTypeMoreReliable))) {

    PageBaseAddress = PageAlignAddress (BaseAddress);
    PageLength      = PageAlignLength (BaseAddress + Length - PageBaseAddress);

    Status = CoreAllocateMemorySpace (
               EfiGcdAllocateAddress,
               GcdMemoryType,
               EFI_PAGE_SHIFT,
               PageLength,
               &PageBaseAddress,
               gDxeCoreImageHandle,
               NULL
               );

    if (!EFI_ERROR (Status)) {
      CoreAddMemoryDescriptor (
        EfiConventionalMemory,
        PageBaseAddress,
        RShiftU64 (PageLength, EFI_PAGE_SHIFT),
        Capabilities
        );
    } else {
      for (; PageLength != 0; PageLength -= EFI_PAGE_SIZE, PageBaseAddress += EFI_PAGE_SIZE) {
        Status = CoreAllocateMemorySpace (
                   EfiGcdAllocateAddress,
                   GcdMemoryType,
                   EFI_PAGE_SHIFT,
                   EFI_PAGE_SIZE,
                   &PageBaseAddress,
                   gDxeCoreImageHandle,
                   NULL
                   );

        if (!EFI_ERROR (Status)) {
          CoreAddMemoryDescriptor (
            EfiConventionalMemory,
            PageBaseAddress,
            1,
            Capabilities
            );
        }
      }
    }
  }
  return Status;
}


/**
  Frees nonexistent memory, reserved memory, system memory, or memory-mapped
  I/O resources from the global coherency domain of the processor.

  @param  BaseAddress            Base address of the memory space.
  @param  Length                 Length of the memory space.

  @retval EFI_SUCCESS            Space successfully freed.

**/
EFI_STATUS
EFIAPI
CoreFreeMemorySpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  DEBUG ((DEBUG_GCD, "GCD:FreeMemorySpace(Base=%016lx,Length=%016lx)\n", BaseAddress, Length));

  return CoreConvertSpace (GCD_FREE_MEMORY_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, 0, 0);
}


/**
  Removes reserved memory, system memory, or memory-mapped I/O resources from
  the global coherency domain of the processor.

  @param  BaseAddress            Base address of the memory space.
  @param  Length                 Length of the memory space.

  @retval EFI_SUCCESS            Successfully remove a segment of memory space.

**/
EFI_STATUS
EFIAPI
CoreRemoveMemorySpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  DEBUG ((DEBUG_GCD, "GCD:RemoveMemorySpace(Base=%016lx,Length=%016lx)\n", BaseAddress, Length));
  
  return CoreConvertSpace (GCD_REMOVE_MEMORY_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, 0, 0);
}


/**
  Build a memory descriptor according to an entry.

  @param  Descriptor             The descriptor to be built
  @param  Entry                  According to this entry

**/
VOID
BuildMemoryDescriptor (
  IN OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor,
  IN EFI_GCD_MAP_ENTRY                *Entry
  )
{
  Descriptor->BaseAddress   = Entry->BaseAddress;
  Descriptor->Length        = Entry->EndAddress - Entry->BaseAddress + 1;
  Descriptor->Capabilities  = Entry->Capabilities;
  Descriptor->Attributes    = Entry->Attributes;
  Descriptor->GcdMemoryType = Entry->GcdMemoryType;
  Descriptor->ImageHandle   = Entry->ImageHandle;
  Descriptor->DeviceHandle  = Entry->DeviceHandle;
}


/**
  Retrieves the descriptor for a memory region containing a specified address.

  @param  BaseAddress            Specified start address
  @param  Descriptor             Specified length

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCCESS            Successfully get memory space descriptor.

**/
EFI_STATUS
EFIAPI
CoreGetMemorySpaceDescriptor (
  IN  EFI_PHYSICAL_ADDRESS             BaseAddress,
  OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor
  )
{
  EFI_STATUS         Status;
  LIST_ENTRY         *StartLink;
  LIST_ENTRY         *EndLink;
  EFI_GCD_MAP_ENTRY  *Entry;

  //
  // Make sure parameters are valid
  //
  if (Descriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquireGcdMemoryLock ();

  //
  // Search for the list of descriptors that contain BaseAddress
  //
  Status = CoreSearchGcdMapEntry (BaseAddress, 1, &StartLink, &EndLink, &mGcdMemorySpaceMap);
  if (EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
  } else {
    ASSERT (StartLink != NULL && EndLink != NULL);
    //
    // Copy the contents of the found descriptor into Descriptor
    //
    Entry = CR (StartLink, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    BuildMemoryDescriptor (Descriptor, Entry);
  }

  CoreReleaseGcdMemoryLock ();

  return Status;
}


/**
  Modifies the attributes for a memory region in the global coherency domain of the
  processor.

  @param  BaseAddress            Specified start address
  @param  Length                 Specified length
  @param  Attributes             Specified attributes

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero. 
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
  @retval EFI_UNSUPPORTED       The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.
  @retval EFI_ACCESS_DEFINED    The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_NOT_AVAILABLE_YET The attributes cannot be set because CPU architectural protocol is
                                not available yet.

**/
EFI_STATUS
EFIAPI
CoreSetMemorySpaceAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes
  )
{
  DEBUG ((DEBUG_GCD, "GCD:SetMemorySpaceAttributes(Base=%016lx,Length=%016lx)\n", BaseAddress, Length));
  DEBUG ((DEBUG_GCD, "  Attributes  = %016lx\n", Attributes));

  return CoreConvertSpace (GCD_SET_ATTRIBUTES_MEMORY_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, 0, Attributes);
}


/**
  Modifies the capabilities for a memory region in the global coherency domain of the
  processor.

  @param  BaseAddress      The physical address that is the start address of a memory region.
  @param  Length           The size in bytes of the memory region.
  @param  Capabilities     The bit mask of capabilities that the memory region supports.

  @retval EFI_SUCCESS           The capabilities were set for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
  @retval EFI_UNSUPPORTED       The capabilities specified by Capabilities do not include the
                                memory region attributes currently in use.
  @retval EFI_ACCESS_DENIED     The capabilities for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the capabilities
                                of the memory resource range.
**/
EFI_STATUS
EFIAPI
CoreSetMemorySpaceCapabilities (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Capabilities
  )
{
  EFI_STATUS    Status;

  DEBUG ((DEBUG_GCD, "GCD:CoreSetMemorySpaceCapabilities(Base=%016lx,Length=%016lx)\n", BaseAddress, Length));
  DEBUG ((DEBUG_GCD, "  Capabilities  = %016lx\n", Capabilities));

  Status = CoreConvertSpace (GCD_SET_CAPABILITIES_MEMORY_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, Capabilities, 0);
  if (!EFI_ERROR(Status)) {
    CoreUpdateMemoryAttributes(BaseAddress, RShiftU64(Length, EFI_PAGE_SHIFT), Capabilities & (~EFI_MEMORY_RUNTIME));
  }

  return Status;
}


/**
  Returns a map of the memory resources in the global coherency domain of the
  processor.

  @param  NumberOfDescriptors    Number of descriptors.
  @param  MemorySpaceMap         Descriptor array

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_OUT_OF_RESOURCES   No enough buffer to allocate
  @retval EFI_SUCCESS            Successfully get memory space map.

**/
EFI_STATUS
EFIAPI
CoreGetMemorySpaceMap (
  OUT UINTN                            *NumberOfDescriptors,
  OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  **MemorySpaceMap
  )
{
  EFI_STATUS                       Status;
  LIST_ENTRY                       *Link;
  EFI_GCD_MAP_ENTRY                *Entry;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor;

  //
  // Make sure parameters are valid
  //
  if (NumberOfDescriptors == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (MemorySpaceMap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquireGcdMemoryLock ();

  //
  // Count the number of descriptors
  //
  *NumberOfDescriptors = CoreCountGcdMapEntry (&mGcdMemorySpaceMap);

  //
  // Allocate the MemorySpaceMap
  //
  *MemorySpaceMap = AllocatePool (*NumberOfDescriptors * sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR));
  if (*MemorySpaceMap == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Fill in the MemorySpaceMap
  //
  Descriptor = *MemorySpaceMap;
  Link = mGcdMemorySpaceMap.ForwardLink;
  while (Link != &mGcdMemorySpaceMap) {
    Entry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    BuildMemoryDescriptor (Descriptor, Entry);
    Descriptor++;
    Link = Link->ForwardLink;
  }
  Status = EFI_SUCCESS;

Done:
  CoreReleaseGcdMemoryLock ();
  return Status;
}


/**
  Adds reserved I/O or I/O resources to the global coherency domain of the processor.

  @param  GcdIoType              IO type of the segment.
  @param  BaseAddress            Base address of the segment.
  @param  Length                 Length of the segment.

  @retval EFI_SUCCESS            Merged this segment into GCD map.
  @retval EFI_INVALID_PARAMETER  Parameter not valid

**/
EFI_STATUS
EFIAPI
CoreAddIoSpace (
  IN EFI_GCD_IO_TYPE       GcdIoType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  DEBUG ((DEBUG_GCD, "GCD:AddIoSpace(Base=%016lx,Length=%016lx)\n", BaseAddress, Length));
  DEBUG ((DEBUG_GCD, "  GcdIoType    = %a\n", mGcdIoTypeNames[MIN (GcdIoType, EfiGcdIoTypeMaximum)]));
  
  //
  // Make sure parameters are valid
  //
  if (GcdIoType <= EfiGcdIoTypeNonExistent || GcdIoType >= EfiGcdIoTypeMaximum) {
    return EFI_INVALID_PARAMETER;
  }
  return CoreConvertSpace (GCD_ADD_IO_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, GcdIoType, BaseAddress, Length, 0, 0);
}


/**
  Allocates nonexistent I/O, reserved I/O, or I/O resources from the global coherency
  domain of the processor.

  @param  GcdAllocateType        The type of allocate operation
  @param  GcdIoType              The desired IO type
  @param  Alignment              Align with 2^Alignment
  @param  Length                 Length to allocate
  @param  BaseAddress            Base address to allocate
  @param  ImageHandle            The image handle consume the allocated space.
  @param  DeviceHandle           The device handle consume the allocated space.

  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_NOT_FOUND          No descriptor contains the desired space.
  @retval EFI_SUCCESS            IO space successfully allocated.

**/
EFI_STATUS
EFIAPI
CoreAllocateIoSpace (
  IN     EFI_GCD_ALLOCATE_TYPE  GcdAllocateType,
  IN     EFI_GCD_IO_TYPE        GcdIoType,
  IN     UINTN                  Alignment,
  IN     UINT64                 Length,
  IN OUT EFI_PHYSICAL_ADDRESS   *BaseAddress,
  IN     EFI_HANDLE             ImageHandle,
  IN     EFI_HANDLE             DeviceHandle OPTIONAL
  )
{
  if (BaseAddress != NULL) {
    DEBUG ((DEBUG_GCD, "GCD:AllocateIoSpace(Base=%016lx,Length=%016lx)\n", *BaseAddress, Length));
  } else {
    DEBUG ((DEBUG_GCD, "GCD:AllocateIoSpace(Base=<NULL>,Length=%016lx)\n", Length));
  }
  DEBUG ((DEBUG_GCD, "  GcdAllocateType = %a\n", mGcdAllocationTypeNames[MIN (GcdAllocateType, EfiGcdMaxAllocateType)]));
  DEBUG ((DEBUG_GCD, "  GcdIoType       = %a\n", mGcdIoTypeNames[MIN (GcdIoType, EfiGcdIoTypeMaximum)]));
  DEBUG ((DEBUG_GCD, "  Alignment       = %016lx\n", LShiftU64 (1, Alignment)));
  DEBUG ((DEBUG_GCD, "  ImageHandle     = %p\n", ImageHandle));
  DEBUG ((DEBUG_GCD, "  DeviceHandle    = %p\n", DeviceHandle));
  
  return CoreAllocateSpace (
           GCD_ALLOCATE_IO_OPERATION,
           GcdAllocateType,
           (EFI_GCD_MEMORY_TYPE) 0,
           GcdIoType,
           Alignment,
           Length,
           BaseAddress,
           ImageHandle,
           DeviceHandle
           );
}


/**
  Frees nonexistent I/O, reserved I/O, or I/O resources from the global coherency
  domain of the processor.

  @param  BaseAddress            Base address of the segment.
  @param  Length                 Length of the segment.

  @retval EFI_SUCCESS            Space successfully freed.

**/
EFI_STATUS
EFIAPI
CoreFreeIoSpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  DEBUG ((DEBUG_GCD, "GCD:FreeIoSpace(Base=%016lx,Length=%016lx)\n", BaseAddress, Length));

  return CoreConvertSpace (GCD_FREE_IO_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, 0, 0);
}


/**
  Removes reserved I/O or I/O resources from the global coherency domain of the
  processor.

  @param  BaseAddress            Base address of the segment.
  @param  Length                 Length of the segment.

  @retval EFI_SUCCESS            Successfully removed a segment of IO space.

**/
EFI_STATUS
EFIAPI
CoreRemoveIoSpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
{
  DEBUG ((DEBUG_GCD, "GCD:RemoveIoSpace(Base=%016lx,Length=%016lx)\n", BaseAddress, Length));
  
  return CoreConvertSpace (GCD_REMOVE_IO_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, 0, 0);
}


/**
  Build a IO descriptor according to an entry.

  @param  Descriptor             The descriptor to be built
  @param  Entry                  According to this entry

**/
VOID
BuildIoDescriptor (
  IN EFI_GCD_IO_SPACE_DESCRIPTOR  *Descriptor,
  IN EFI_GCD_MAP_ENTRY            *Entry
  )
{
  Descriptor->BaseAddress  = Entry->BaseAddress;
  Descriptor->Length       = Entry->EndAddress - Entry->BaseAddress + 1;
  Descriptor->GcdIoType    = Entry->GcdIoType;
  Descriptor->ImageHandle  = Entry->ImageHandle;
  Descriptor->DeviceHandle = Entry->DeviceHandle;
}


/**
  Retrieves the descriptor for an I/O region containing a specified address.

  @param  BaseAddress            Specified start address
  @param  Descriptor             Specified length

  @retval EFI_INVALID_PARAMETER  Descriptor is NULL.
  @retval EFI_SUCCESS            Successfully get the IO space descriptor.

**/
EFI_STATUS
EFIAPI
CoreGetIoSpaceDescriptor (
  IN  EFI_PHYSICAL_ADDRESS         BaseAddress,
  OUT EFI_GCD_IO_SPACE_DESCRIPTOR  *Descriptor
  )
{
  EFI_STATUS         Status;
  LIST_ENTRY         *StartLink;
  LIST_ENTRY         *EndLink;
  EFI_GCD_MAP_ENTRY  *Entry;

  //
  // Make sure parameters are valid
  //
  if (Descriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquireGcdIoLock ();

  //
  // Search for the list of descriptors that contain BaseAddress
  //
  Status = CoreSearchGcdMapEntry (BaseAddress, 1, &StartLink, &EndLink, &mGcdIoSpaceMap);
  if (EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
  } else {
    ASSERT (StartLink != NULL && EndLink != NULL);
    //
    // Copy the contents of the found descriptor into Descriptor
    //
    Entry = CR (StartLink, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    BuildIoDescriptor (Descriptor, Entry);
  }

  CoreReleaseGcdIoLock ();

  return Status;
}


/**
  Returns a map of the I/O resources in the global coherency domain of the processor.

  @param  NumberOfDescriptors    Number of descriptors.
  @param  IoSpaceMap             Descriptor array

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_OUT_OF_RESOURCES   No enough buffer to allocate
  @retval EFI_SUCCESS            Successfully get IO space map.

**/
EFI_STATUS
EFIAPI
CoreGetIoSpaceMap (
  OUT UINTN                        *NumberOfDescriptors,
  OUT EFI_GCD_IO_SPACE_DESCRIPTOR  **IoSpaceMap
  )
{
  EFI_STATUS                   Status;
  LIST_ENTRY                   *Link;
  EFI_GCD_MAP_ENTRY            *Entry;
  EFI_GCD_IO_SPACE_DESCRIPTOR  *Descriptor;

  //
  // Make sure parameters are valid
  //
  if (NumberOfDescriptors == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (IoSpaceMap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquireGcdIoLock ();

  //
  // Count the number of descriptors
  //
  *NumberOfDescriptors = CoreCountGcdMapEntry (&mGcdIoSpaceMap);

  //
  // Allocate the IoSpaceMap
  //
  *IoSpaceMap = AllocatePool (*NumberOfDescriptors * sizeof (EFI_GCD_IO_SPACE_DESCRIPTOR));
  if (*IoSpaceMap == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Fill in the IoSpaceMap
  //
  Descriptor = *IoSpaceMap;
  Link = mGcdIoSpaceMap.ForwardLink;
  while (Link != &mGcdIoSpaceMap) {
    Entry = CR (Link, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    BuildIoDescriptor (Descriptor, Entry);
    Descriptor++;
    Link = Link->ForwardLink;
  }
  Status = EFI_SUCCESS;

Done:
  CoreReleaseGcdIoLock ();
  return Status;
}


/**
  Converts a Resource Descriptor HOB attributes mask to an EFI Memory Descriptor
  capabilities mask

  @param  GcdMemoryType          Type of resource in the GCD memory map.
  @param  Attributes             The attribute mask in the Resource Descriptor
                                 HOB.

  @return The capabilities mask for an EFI Memory Descriptor.

**/
UINT64
CoreConvertResourceDescriptorHobAttributesToCapabilities (
  EFI_GCD_MEMORY_TYPE  GcdMemoryType,
  UINT64               Attributes
  )
{
  UINT64                          Capabilities;
  GCD_ATTRIBUTE_CONVERSION_ENTRY  *Conversion;

  //
  // Convert the Resource HOB Attributes to an EFI Memory Capabilities mask
  //
  for (Capabilities = 0, Conversion = mAttributeConversionTable; Conversion->Attribute != 0; Conversion++) {
    if (Conversion->Memory || ((GcdMemoryType != EfiGcdMemoryTypeSystemMemory) && (GcdMemoryType != EfiGcdMemoryTypeMoreReliable))) {
      if (Attributes & Conversion->Attribute) {
        Capabilities |= Conversion->Capability;
      }
    }
  }

  return Capabilities;
}

/**
  Calculate total memory bin size neeeded.

  @return The total memory bin size neeeded.

**/
UINT64
CalculateTotalMemoryBinSizeNeeded (
  VOID
  )
{
  UINTN     Index;
  UINT64    TotalSize;

  //
  // Loop through each memory type in the order specified by the gMemoryTypeInformation[] array
  //
  TotalSize = 0;
  for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    TotalSize += LShiftU64 (gMemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT);
  }

  return TotalSize;
}

/**
  External function. Initializes memory services based on the memory
  descriptor HOBs.  This function is responsible for priming the memory
  map, so memory allocations and resource allocations can be made.
  The first part of this function can not depend on any memory services
  until at least one memory descriptor is provided to the memory services.

  @param  HobStart               The start address of the HOB.
  @param  MemoryBaseAddress      Start address of memory region found to init DXE
                                 core.
  @param  MemoryLength           Length of memory region found to init DXE core.

  @retval EFI_SUCCESS            Memory services successfully initialized.

**/
EFI_STATUS
CoreInitializeMemoryServices (
  IN  VOID                  **HobStart,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryBaseAddress,
  OUT UINT64                *MemoryLength
  )
{
  EFI_PEI_HOB_POINTERS               Hob;
  EFI_MEMORY_TYPE_INFORMATION        *EfiMemoryTypeInformation;
  UINTN                              DataSize;
  BOOLEAN                            Found;
  EFI_HOB_HANDOFF_INFO_TABLE         *PhitHob;
  EFI_HOB_RESOURCE_DESCRIPTOR        *ResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR        *PhitResourceHob;
  EFI_PHYSICAL_ADDRESS               BaseAddress;
  UINT64                             Length;
  UINT64                             Attributes;
  UINT64                             Capabilities;
  EFI_PHYSICAL_ADDRESS               TestedMemoryBaseAddress;
  UINT64                             TestedMemoryLength;
  EFI_PHYSICAL_ADDRESS               HighAddress;
  EFI_HOB_GUID_TYPE                  *GuidHob;
  UINT32                             ReservedCodePageNumber;
  UINT64                             MinimalMemorySizeNeeded;

  //
  // Point at the first HOB.  This must be the PHIT HOB.
  //
  Hob.Raw = *HobStart;
  ASSERT (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_HANDOFF);

  //
  // Initialize the spin locks and maps in the memory services.
  // Also fill in the memory services into the EFI Boot Services Table
  //
  CoreInitializePool ();

  //
  // Initialize Local Variables
  //
  PhitResourceHob       = NULL;
  ResourceHob           = NULL;
  BaseAddress           = 0;
  Length                = 0;
  Attributes            = 0;

  //
  // Cache the PHIT HOB for later use
  //
  PhitHob = Hob.HandoffInformationTable;
  
  if (PcdGet64(PcdLoadModuleAtFixAddressEnable) != 0) {
  	ReservedCodePageNumber = PcdGet32(PcdLoadFixAddressRuntimeCodePageNumber);
  	ReservedCodePageNumber += PcdGet32(PcdLoadFixAddressBootTimeCodePageNumber);
   
  	//
  	// cache the Top address for loading modules at Fixed Address 
  	//
    gLoadModuleAtFixAddressConfigurationTable.DxeCodeTopAddress = PhitHob->EfiMemoryTop 
                                                                   + EFI_PAGES_TO_SIZE(ReservedCodePageNumber);
  }
  //
  // See if a Memory Type Information HOB is available
  //
  GuidHob = GetFirstGuidHob (&gEfiMemoryTypeInformationGuid);
  if (GuidHob != NULL) {
    EfiMemoryTypeInformation = GET_GUID_HOB_DATA (GuidHob);
    DataSize                 = GET_GUID_HOB_DATA_SIZE (GuidHob);
    if (EfiMemoryTypeInformation != NULL && DataSize > 0 && DataSize <= (EfiMaxMemoryType + 1) * sizeof (EFI_MEMORY_TYPE_INFORMATION)) {
      CopyMem (&gMemoryTypeInformation, EfiMemoryTypeInformation, DataSize);
    }
  }

  //
  // Include the total memory bin size needed to make sure memory bin could be allocated successfully.
  //
  MinimalMemorySizeNeeded = MINIMUM_INITIAL_MEMORY_SIZE + CalculateTotalMemoryBinSizeNeeded ();

  //
  // Find the Resource Descriptor HOB that contains PHIT range EfiFreeMemoryBottom..EfiFreeMemoryTop
  //
  Found  = FALSE;
  for (Hob.Raw = *HobStart; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
    //
    // Skip all HOBs except Resource Descriptor HOBs
    //
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not describe tested system memory
    //
    ResourceHob = Hob.ResourceDescriptor;
    if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) {
      continue;
    }
    if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) != TESTED_MEMORY_ATTRIBUTES) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not contain the PHIT range EfiFreeMemoryBottom..EfiFreeMemoryTop
    //
    if (PhitHob->EfiFreeMemoryBottom < ResourceHob->PhysicalStart) {
      continue;
    }
    if (PhitHob->EfiFreeMemoryTop > (ResourceHob->PhysicalStart + ResourceHob->ResourceLength)) {
      continue;
    }

    //
    // Cache the resource descriptor HOB for the memory region described by the PHIT HOB
    //
    PhitResourceHob = ResourceHob;
    Found = TRUE;

    //
    // Compute range between PHIT EfiMemoryTop and the end of the Resource Descriptor HOB
    //
    Attributes  = PhitResourceHob->ResourceAttribute;
    BaseAddress = PageAlignAddress (PhitHob->EfiMemoryTop);
    Length      = PageAlignLength  (ResourceHob->PhysicalStart + ResourceHob->ResourceLength - BaseAddress);
    if (Length < MinimalMemorySizeNeeded) {
      //
      // If that range is not large enough to intialize the DXE Core, then 
      // Compute range between PHIT EfiFreeMemoryBottom and PHIT EfiFreeMemoryTop
      //
      BaseAddress = PageAlignAddress (PhitHob->EfiFreeMemoryBottom);
      Length      = PageAlignLength  (PhitHob->EfiFreeMemoryTop - BaseAddress);
      if (Length < MinimalMemorySizeNeeded) {
        //
        // If that range is not large enough to intialize the DXE Core, then 
        // Compute range between the start of the Resource Descriptor HOB and the start of the HOB List
        //
        BaseAddress = PageAlignAddress (ResourceHob->PhysicalStart);
        Length      = PageAlignLength  ((UINT64)((UINTN)*HobStart - BaseAddress));
      }
    }
    break;
  }

  //
  // Assert if a resource descriptor HOB for the memory region described by the PHIT was not found
  //
  ASSERT (Found);

  //
  // Take the range in the resource descriptor HOB for the memory region described
  // by the PHIT as higher priority if it is big enough. It can make the memory bin
  // allocated to be at the same memory region with PHIT that has more better compatibility
  // to avoid memory fragmentation for some code practices assume and allocate <4G ACPI memory.
  //
  if (Length < MinimalMemorySizeNeeded) {
    //
    // Search all the resource descriptor HOBs from the highest possible addresses down for a memory
    // region that is big enough to initialize the DXE core.  Always skip the PHIT Resource HOB.
    // The max address must be within the physically addressible range for the processor.
    //
    HighAddress = MAX_ADDRESS;
    for (Hob.Raw = *HobStart; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
      //
      // Skip the Resource Descriptor HOB that contains the PHIT
      //
      if (Hob.ResourceDescriptor == PhitResourceHob) {
        continue;
      }
      //
      // Skip all HOBs except Resource Descriptor HOBs
      //
      if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
        continue;
      }

      //
      // Skip Resource Descriptor HOBs that do not describe tested system memory below MAX_ADDRESS
      //
      ResourceHob = Hob.ResourceDescriptor;
      if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) {
        continue;
      }
      if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) != TESTED_MEMORY_ATTRIBUTES) {
        continue;
      }
      if ((ResourceHob->PhysicalStart + ResourceHob->ResourceLength) > (EFI_PHYSICAL_ADDRESS)MAX_ADDRESS) {
        continue;
      }

      //
      // Skip Resource Descriptor HOBs that are below a previously found Resource Descriptor HOB
      //
      if (HighAddress != (EFI_PHYSICAL_ADDRESS)MAX_ADDRESS && ResourceHob->PhysicalStart <= HighAddress) {
        continue;
      }

      //
      // Skip Resource Descriptor HOBs that are not large enough to initilize the DXE Core
      //
      TestedMemoryBaseAddress = PageAlignAddress (ResourceHob->PhysicalStart);
      TestedMemoryLength      = PageAlignLength  (ResourceHob->PhysicalStart + ResourceHob->ResourceLength - TestedMemoryBaseAddress);
      if (TestedMemoryLength < MinimalMemorySizeNeeded) {
        continue;
      }

      //
      // Save the range described by the Resource Descriptor that is large enough to initilize the DXE Core
      //
      BaseAddress = TestedMemoryBaseAddress;
      Length      = TestedMemoryLength;
      Attributes  = ResourceHob->ResourceAttribute; 
      HighAddress = ResourceHob->PhysicalStart;
    }
  }

  DEBUG ((EFI_D_INFO, "CoreInitializeMemoryServices:\n"));
  DEBUG ((EFI_D_INFO, "  BaseAddress - 0x%lx Length - 0x%lx MinimalMemorySizeNeeded - 0x%lx\n", BaseAddress, Length, MinimalMemorySizeNeeded));

  //
  // If no memory regions are found that are big enough to initialize the DXE core, then ASSERT().
  //
  ASSERT (Length >= MinimalMemorySizeNeeded);

  //
  // Convert the Resource HOB Attributes to an EFI Memory Capabilities mask
  //
  if ((Attributes & EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE) == EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE) {
    Capabilities = CoreConvertResourceDescriptorHobAttributesToCapabilities (EfiGcdMemoryTypeMoreReliable, Attributes);
  } else {
    Capabilities = CoreConvertResourceDescriptorHobAttributesToCapabilities (EfiGcdMemoryTypeSystemMemory, Attributes);
  }

  //
  // Declare the very first memory region, so the EFI Memory Services are available.
  //
  CoreAddMemoryDescriptor (
    EfiConventionalMemory,
    BaseAddress,
    RShiftU64 (Length, EFI_PAGE_SHIFT),
    Capabilities
    );

  *MemoryBaseAddress = BaseAddress;
  *MemoryLength      = Length;

  return EFI_SUCCESS;
}


/**
  External function. Initializes the GCD and memory services based on the memory
  descriptor HOBs.  This function is responsible for priming the GCD map and the
  memory map, so memory allocations and resource allocations can be made. The
  HobStart will be relocated to a pool buffer.

  @param  HobStart               The start address of the HOB
  @param  MemoryBaseAddress      Start address of memory region found to init DXE
                                 core.
  @param  MemoryLength           Length of memory region found to init DXE core.

  @retval EFI_SUCCESS            GCD services successfully initialized.

**/
EFI_STATUS
CoreInitializeGcdServices (
  IN OUT VOID              **HobStart,
  IN EFI_PHYSICAL_ADDRESS  MemoryBaseAddress,
  IN UINT64                MemoryLength
  )
{
  EFI_PEI_HOB_POINTERS               Hob;
  VOID                               *NewHobList;
  EFI_HOB_HANDOFF_INFO_TABLE         *PhitHob;
  UINT8                              SizeOfMemorySpace;
  UINT8                              SizeOfIoSpace;
  EFI_HOB_RESOURCE_DESCRIPTOR        *ResourceHob;
  EFI_PHYSICAL_ADDRESS               BaseAddress;
  UINT64                             Length;
  EFI_STATUS                         Status;
  EFI_GCD_MAP_ENTRY                  *Entry;
  EFI_GCD_MEMORY_TYPE                GcdMemoryType;
  EFI_GCD_IO_TYPE                    GcdIoType;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR    Descriptor;
  EFI_HOB_MEMORY_ALLOCATION          *MemoryHob;
  EFI_HOB_FIRMWARE_VOLUME            *FirmwareVolumeHob;
  UINTN                              NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR    *MemorySpaceMap;
  UINTN                              Index;
  UINT64                             Capabilities;
  EFI_HOB_CPU *                      CpuHob;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR    *MemorySpaceMapHobList;

  //
  // Cache the PHIT HOB for later use
  //
  PhitHob = (EFI_HOB_HANDOFF_INFO_TABLE *)(*HobStart);

  //
  // Get the number of address lines in the I/O and Memory space for the CPU
  //
  CpuHob = GetFirstHob (EFI_HOB_TYPE_CPU);
  ASSERT (CpuHob != NULL);
  SizeOfMemorySpace = CpuHob->SizeOfMemorySpace;
  SizeOfIoSpace     = CpuHob->SizeOfIoSpace;

  //
  // Initialize the GCD Memory Space Map
  //
  Entry = AllocateCopyPool (sizeof (EFI_GCD_MAP_ENTRY), &mGcdMemorySpaceMapEntryTemplate);
  ASSERT (Entry != NULL);

  Entry->EndAddress = LShiftU64 (1, SizeOfMemorySpace) - 1;

  InsertHeadList (&mGcdMemorySpaceMap, &Entry->Link);

  CoreDumpGcdMemorySpaceMap (TRUE);
  
  //
  // Initialize the GCD I/O Space Map
  //
  Entry = AllocateCopyPool (sizeof (EFI_GCD_MAP_ENTRY), &mGcdIoSpaceMapEntryTemplate);
  ASSERT (Entry != NULL);

  Entry->EndAddress = LShiftU64 (1, SizeOfIoSpace) - 1;

  InsertHeadList (&mGcdIoSpaceMap, &Entry->Link);

  CoreDumpGcdIoSpaceMap (TRUE);
  
  //
  // Walk the HOB list and add all resource descriptors to the GCD
  //
  for (Hob.Raw = *HobStart; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {

    GcdMemoryType = EfiGcdMemoryTypeNonExistent;
    GcdIoType     = EfiGcdIoTypeNonExistent;

    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {

      ResourceHob = Hob.ResourceDescriptor;

      switch (ResourceHob->ResourceType) {
      case EFI_RESOURCE_SYSTEM_MEMORY:
        if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) == TESTED_MEMORY_ATTRIBUTES) {
          if ((ResourceHob->ResourceAttribute & EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE) == EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE) {
            GcdMemoryType = EfiGcdMemoryTypeMoreReliable;
          } else {
            GcdMemoryType = EfiGcdMemoryTypeSystemMemory;
          }
        }
        if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) == INITIALIZED_MEMORY_ATTRIBUTES) {
          GcdMemoryType = EfiGcdMemoryTypeReserved;
        }
        if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) == PRESENT_MEMORY_ATTRIBUTES) {
          GcdMemoryType = EfiGcdMemoryTypeReserved;
        }
        if ((ResourceHob->ResourceAttribute & EFI_RESOURCE_ATTRIBUTE_PERSISTENT) == EFI_RESOURCE_ATTRIBUTE_PERSISTENT) {
          GcdMemoryType = EfiGcdMemoryTypePersistent;
        }
        break;
      case EFI_RESOURCE_MEMORY_MAPPED_IO:
      case EFI_RESOURCE_FIRMWARE_DEVICE:
        GcdMemoryType = EfiGcdMemoryTypeMemoryMappedIo;
        break;
      case EFI_RESOURCE_MEMORY_MAPPED_IO_PORT:
      case EFI_RESOURCE_MEMORY_RESERVED:
        GcdMemoryType = EfiGcdMemoryTypeReserved;
        break;
      case EFI_RESOURCE_IO:
        GcdIoType = EfiGcdIoTypeIo;
        break;
      case EFI_RESOURCE_IO_RESERVED:
        GcdIoType = EfiGcdIoTypeReserved;
        break;
      }

      if (GcdMemoryType != EfiGcdMemoryTypeNonExistent) {
        //
        // Validate the Resource HOB Attributes
        //
        CoreValidateResourceDescriptorHobAttributes (ResourceHob->ResourceAttribute);

        //
        // Convert the Resource HOB Attributes to an EFI Memory Capabilities mask
        //
        Capabilities = CoreConvertResourceDescriptorHobAttributesToCapabilities (
                         GcdMemoryType,
                         ResourceHob->ResourceAttribute
                         );

        Status = CoreInternalAddMemorySpace (
                   GcdMemoryType,
                   ResourceHob->PhysicalStart,
                   ResourceHob->ResourceLength,
                   Capabilities
                   );
      }

      if (GcdIoType != EfiGcdIoTypeNonExistent) {
        Status = CoreAddIoSpace (
                   GcdIoType,
                   ResourceHob->PhysicalStart,
                   ResourceHob->ResourceLength
                   );
      }
    }
  }

  //
  // Allocate first memory region from the GCD by the DXE core
  //
  Status = CoreGetMemorySpaceDescriptor (MemoryBaseAddress, &Descriptor);
  if (!EFI_ERROR (Status)) {
    ASSERT ((Descriptor.GcdMemoryType == EfiGcdMemoryTypeSystemMemory) ||
            (Descriptor.GcdMemoryType == EfiGcdMemoryTypeMoreReliable));
    Status = CoreAllocateMemorySpace (
               EfiGcdAllocateAddress,
               Descriptor.GcdMemoryType,
               0,
               MemoryLength,
               &MemoryBaseAddress,
               gDxeCoreImageHandle,
               NULL
               );
  }

  //
  // Walk the HOB list and allocate all memory space that is consumed by memory allocation HOBs,
  // and Firmware Volume HOBs.  Also update the EFI Memory Map with the memory allocation HOBs.
  //
  for (Hob.Raw = *HobStart; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      MemoryHob = Hob.MemoryAllocation;
      BaseAddress = MemoryHob->AllocDescriptor.MemoryBaseAddress;
      Status = CoreGetMemorySpaceDescriptor  (BaseAddress, &Descriptor);
      if (!EFI_ERROR (Status)) {
        Status = CoreAllocateMemorySpace (
                   EfiGcdAllocateAddress,
                   Descriptor.GcdMemoryType,
                   0,
                   MemoryHob->AllocDescriptor.MemoryLength,
                   &BaseAddress,
                   gDxeCoreImageHandle,
                   NULL
                   );
        if (!EFI_ERROR (Status) &&
            ((Descriptor.GcdMemoryType == EfiGcdMemoryTypeSystemMemory) ||
             (Descriptor.GcdMemoryType == EfiGcdMemoryTypeMoreReliable))) {
          CoreAddMemoryDescriptor (
            MemoryHob->AllocDescriptor.MemoryType,
            MemoryHob->AllocDescriptor.MemoryBaseAddress,
            RShiftU64 (MemoryHob->AllocDescriptor.MemoryLength, EFI_PAGE_SHIFT),
            Descriptor.Capabilities & (~EFI_MEMORY_RUNTIME)
            );
        }
      }
    }

    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_FV) {
      FirmwareVolumeHob = Hob.FirmwareVolume;
      BaseAddress = FirmwareVolumeHob->BaseAddress;
      Status = CoreAllocateMemorySpace (
                 EfiGcdAllocateAddress,
                 EfiGcdMemoryTypeMemoryMappedIo,
                 0,
                 FirmwareVolumeHob->Length,
                 &BaseAddress,
                 gDxeCoreImageHandle,
                 NULL
                 );
    }
  }

  //
  // Add and allocate the remaining unallocated system memory to the memory services.
  //
  Status = CoreGetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  ASSERT (Status == EFI_SUCCESS);

  MemorySpaceMapHobList = NULL;
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if ((MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeSystemMemory) ||
        (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeMoreReliable)) {
      if (MemorySpaceMap[Index].ImageHandle == NULL) {
        BaseAddress  = PageAlignAddress (MemorySpaceMap[Index].BaseAddress);
        Length       = PageAlignLength  (MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length - BaseAddress);
        if (Length == 0 || MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length < BaseAddress) {
          continue;
        }
        if (((UINTN) MemorySpaceMap[Index].BaseAddress <= (UINTN) (*HobStart)) &&
            ((UINTN) (MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length) >= (UINTN) PhitHob->EfiFreeMemoryBottom)) {
          //
          // Skip the memory space that covers HOB List, it should be processed
          // after HOB List relocation to avoid the resources allocated by others
          // to corrupt HOB List before its relocation.
          //
          MemorySpaceMapHobList = &MemorySpaceMap[Index];
          continue;
        }
        CoreAddMemoryDescriptor (
          EfiConventionalMemory,
          BaseAddress,
          RShiftU64 (Length, EFI_PAGE_SHIFT),
          MemorySpaceMap[Index].Capabilities & (~EFI_MEMORY_RUNTIME)
          );
        Status = CoreAllocateMemorySpace (
                   EfiGcdAllocateAddress,
                   MemorySpaceMap[Index].GcdMemoryType,
                   0,
                   Length,
                   &BaseAddress,
                   gDxeCoreImageHandle,
                   NULL
                   );
      }
    }
  }

  //
  // Relocate HOB List to an allocated pool buffer.
  // The relocation should be at after all the tested memory resources added
  // (except the memory space that covers HOB List) to the memory services,
  // because the memory resource found in CoreInitializeMemoryServices()
  // may have not enough remaining resource for HOB List.
  //
  NewHobList = AllocateCopyPool (
                 (UINTN) PhitHob->EfiFreeMemoryBottom - (UINTN) (*HobStart),
                 *HobStart
                 );
  ASSERT (NewHobList != NULL);

  *HobStart = NewHobList;
  gHobList  = NewHobList;

  if (MemorySpaceMapHobList != NULL) {
    //
    // Add and allocate the memory space that covers HOB List to the memory services
    // after HOB List relocation.
    //
    BaseAddress = PageAlignAddress (MemorySpaceMapHobList->BaseAddress);
    Length      = PageAlignLength  (MemorySpaceMapHobList->BaseAddress + MemorySpaceMapHobList->Length - BaseAddress);
    CoreAddMemoryDescriptor (
      EfiConventionalMemory,
      BaseAddress,
      RShiftU64 (Length, EFI_PAGE_SHIFT),
      MemorySpaceMapHobList->Capabilities & (~EFI_MEMORY_RUNTIME)
      );
    Status = CoreAllocateMemorySpace (
               EfiGcdAllocateAddress,
               MemorySpaceMapHobList->GcdMemoryType,
               0,
               Length,
               &BaseAddress,
               gDxeCoreImageHandle,
               NULL
               );
  }

  CoreFreePool (MemorySpaceMap);

  return EFI_SUCCESS;
}
