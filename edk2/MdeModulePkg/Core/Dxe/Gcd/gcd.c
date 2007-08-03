/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    gcd.c

Abstract:
    The file contains the GCD related services in the EFI Boot Services Table.
    The GCD services are used to manage the memory and I/O regions that 
    are accessible to the CPU that is executing the DXE core.

--*/

#include <DxeMain.h>

#define MINIMUM_INITIAL_MEMORY_SIZE 0x10000

#define MEMORY_ATTRIBUTE_MASK         (EFI_RESOURCE_ATTRIBUTE_PRESENT             | \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED         | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED              | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED      | \
                                       EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED     | \
                                       EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_16_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_32_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_64_BIT_IO           ) 

#define TESTED_MEMORY_ATTRIBUTES      (EFI_RESOURCE_ATTRIBUTE_PRESENT     | \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED      )

#define INITIALIZED_MEMORY_ATTRIBUTES (EFI_RESOURCE_ATTRIBUTE_PRESENT     | \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED )

#define PRESENT_MEMORY_ATTRIBUTES     (EFI_RESOURCE_ATTRIBUTE_PRESENT)

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
  { NULL, NULL },
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
  { NULL, NULL },
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
  { EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE,             EFI_MEMORY_UC,          TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_UNCACHED_EXPORTED,       EFI_MEMORY_UCE,         TRUE  },   
  { EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE,       EFI_MEMORY_WC,          TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE, EFI_MEMORY_WT,          TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE,    EFI_MEMORY_WB,          TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED,          EFI_MEMORY_RP,          TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED,         EFI_MEMORY_WP,          TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED,     EFI_MEMORY_XP,          TRUE  },
  { EFI_RESOURCE_ATTRIBUTE_PRESENT,                 EFI_MEMORY_PRESENT,     FALSE },
  { EFI_RESOURCE_ATTRIBUTE_INITIALIZED,             EFI_MEMORY_INITIALIZED, FALSE },
  { EFI_RESOURCE_ATTRIBUTE_TESTED,                  EFI_MEMORY_TESTED,      FALSE },
  { 0, 0, FALSE }
};

VOID
CoreAcquireGcdMemoryLock (
  VOID
  )
/*++

Routine Description:
    Acquire memory lock on mGcdMemorySpaceLock

Arguments:
    None

Returns:
    None

--*/
{
  CoreAcquireLock (&mGcdMemorySpaceLock);
}


VOID
CoreReleaseGcdMemoryLock (
  VOID
  )
/*++

Routine Description:
    Release memory lock on mGcdMemorySpaceLock

Arguments:
    None

Returns:
    None

--*/
{
  CoreReleaseLock (&mGcdMemorySpaceLock);
}


STATIC
VOID
CoreAcquireGcdIoLock (
  VOID
  )
/*++

Routine Description:
    Acquire memory lock on mGcdIoSpaceLock

Arguments:
    None

Returns:
    None

--*/
{
  CoreAcquireLock (&mGcdIoSpaceLock);
}

STATIC
VOID
CoreReleaseGcdIoLock (
  VOID
  )
/*++

Routine Description:
    Release memory lock on mGcdIoSpaceLock

Arguments:
    None

Returns:
    None

--*/
{
  CoreReleaseLock (&mGcdIoSpaceLock);
}



//
// GCD Initialization Worker Functions
//
STATIC
UINT64
AlignValue (
  IN UINT64   Value,
  IN UINTN    Alignment,
  IN BOOLEAN  RoundUp
  )
/*++

Routine Description:

  Aligns a value to the specified boundary.

Arguments:

  Value     - 64 bit value to align
  Alignment - Log base 2 of the boundary to align Value to
  RoundUp   - TRUE if Value is to be rounded up to the nearest aligned boundary. 
              FALSE is Value is to be rounded down to the nearest aligned boundary.

Returns:

  A 64 bit value is the aligned to the value nearest Value with an alignment by Alignment.

--*/
{
  UINT64  AlignmentMask;

  AlignmentMask = LShiftU64 (1, Alignment) - 1;
  if (RoundUp) {
    Value += AlignmentMask;
  }
  return Value & (~AlignmentMask);
}

STATIC
UINT64
PageAlignAddress (
  IN UINT64 Value
  )
/*++

Routine Description:

  Aligns address to the page boundary.

Arguments:

  Value     - 64 bit address to align

Returns:

  A 64 bit value is the aligned to the value nearest Value with an alignment by Alignment.

--*/
{
  return AlignValue (Value, EFI_PAGE_SHIFT, TRUE);
}

STATIC
UINT64
PageAlignLength (
  IN UINT64 Value
  )
/*++

Routine Description:

  Aligns length to the page boundary.

Arguments:

  Value     - 64 bit length to align

Returns:

  A 64 bit value is the aligned to the value nearest Value with an alignment by Alignment.

--*/
{
  return AlignValue (Value, EFI_PAGE_SHIFT, FALSE);
}

//
// GCD Memory Space Worker Functions
//
STATIC
EFI_STATUS
CoreAllocateGcdMapEntry (
  IN OUT EFI_GCD_MAP_ENTRY  **TopEntry,
  IN OUT EFI_GCD_MAP_ENTRY  **BottomEntry
  )
/*++

Routine Description:

  Allocate pool for two entries.

Arguments:

  TopEntry      - An entry of GCD map
  BottomEntry   - An entry of GCD map

Returns:

  EFI_OUT_OF_RESOURCES    - No enough buffer to be allocated.
  EFI_SUCCESS             - Both entries successfully allocated.

--*/
{
  *TopEntry = CoreAllocateZeroBootServicesPool (sizeof (EFI_GCD_MAP_ENTRY));
  if (*TopEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *BottomEntry = CoreAllocateZeroBootServicesPool (sizeof (EFI_GCD_MAP_ENTRY));
  if (*BottomEntry == NULL) {
    CoreFreePool (*TopEntry);
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
CoreInsertGcdMapEntry (
  IN LIST_ENTRY           *Link,
  IN EFI_GCD_MAP_ENTRY     *Entry,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN EFI_GCD_MAP_ENTRY     *TopEntry,
  IN EFI_GCD_MAP_ENTRY     *BottomEntry
  )
/*++

Routine Description:

  Internal function.  Inserts a new descriptor into a sorted list

Arguments:

  Link        - The linked list to insert the range BaseAddress and Length into

  Entry     -   A pointer to the entry that is inserted

  BaseAddress - The base address of the new range
  
  Length      - The length of the new range in bytes
  
  TopEntry    - Top pad entry to insert if needed.

  BottomEntry - Bottom pad entry to insert if needed.

Returns:

  EFI_SUCCESS - The new range was inserted into the linked list
  
--*/
{
  ASSERT (Length != 0);
  ASSERT (TopEntry->Signature == 0);
  ASSERT (BottomEntry->Signature == 0);

  if (BaseAddress > Entry->BaseAddress) {
    CopyMem (BottomEntry, Entry, sizeof (EFI_GCD_MAP_ENTRY));
    Entry->BaseAddress      = BaseAddress;
    BottomEntry->EndAddress = BaseAddress - 1;
    InsertTailList (Link, &BottomEntry->Link);
  } 

  if ((BaseAddress + Length - 1) < Entry->EndAddress) {
    CopyMem (TopEntry, Entry, sizeof (EFI_GCD_MAP_ENTRY));
    TopEntry->BaseAddress = BaseAddress + Length;
    Entry->EndAddress     = BaseAddress + Length - 1;
    InsertHeadList (Link, &TopEntry->Link);
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
CoreMergeGcdMapEntry (
  IN LIST_ENTRY      *Link,
  IN BOOLEAN         Forward,
  IN LIST_ENTRY      *Map
  )
/*++

Routine Description:

  Merge the Gcd region specified by Link and its adjacent entry

Arguments:

  Link      - Specify the entry to be merged (with its adjacent entry).
  
  Forward   - Direction (forward or backward).
  
  Map       - Boundary.

Returns:

  EFI_SUCCESS     - Successfully returned.
  
  EFI_UNSUPPORTED - These adjacent regions could not merge.

--*/
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

STATIC
EFI_STATUS
CoreCleanupGcdMapEntry (
  IN EFI_GCD_MAP_ENTRY  *TopEntry,
  IN EFI_GCD_MAP_ENTRY  *BottomEntry,
  IN LIST_ENTRY         *StartLink,
  IN LIST_ENTRY         *EndLink,
  IN LIST_ENTRY         *Map
  )
/*++

Routine Description:

  Merge adjacent entries on total chain.

Arguments:

  TopEntry      - Top entry of GCD map.
  
  BottomEntry   - Bottom entry of GCD map.
  
  StartLink     - Start link of the list for this loop.
  
  EndLink       - End link of the list for this loop.
  
  Map           - Boundary.

Returns:

  EFI_SUCCESS   - GCD map successfully cleaned up.

--*/
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

STATIC
EFI_STATUS
CoreSearchGcdMapEntry (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length,
  OUT LIST_ENTRY            **StartLink,
  OUT LIST_ENTRY            **EndLink,
  IN  LIST_ENTRY            *Map
  )
/*++

Routine Description:

  Search a segment of memory space in GCD map. The result is a range of GCD entry list.

Arguments:

  BaseAddress       - The start address of the segment.
  
  Length            - The length of the segment.
  
  StartLink         - The first GCD entry involves this segment of memory space.
  
  EndLink           - The first GCD entry involves this segment of memory space.
  
  Map               - Points to the start entry to search.

Returns:

  EFI_SUCCESS       - Successfully found the entry.
  
  EFI_NOT_FOUND     - Not found.

--*/
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

STATIC
UINTN
CoreCountGcdMapEntry (
  IN LIST_ENTRY  *Map
  )
/*++

Routine Description:

  Count the amount of GCD map entries.

Arguments:

  Map       - Points to the start entry to do the count loop.

Returns:

  The count.

--*/
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


STATIC
UINT64
ConverToCpuArchAttributes (
  UINT64 Attributes
  ) 
/*++

Routine Description:

  Return the memory attribute specified by Attributes

Arguments:

  Attributes        - A num with some attribute bits on.

Returns:

  The enum value of memory attribute.

--*/
{
  if ( (Attributes & EFI_MEMORY_UC) == EFI_MEMORY_UC) {
    return EFI_MEMORY_UC;
  }

  if ( (Attributes & EFI_MEMORY_WC ) == EFI_MEMORY_WC) {
    return EFI_MEMORY_WC;
  }

  if ( (Attributes & EFI_MEMORY_WT ) == EFI_MEMORY_WT) {
    return EFI_MEMORY_WT;
  }

  if ( (Attributes & EFI_MEMORY_WB) == EFI_MEMORY_WB) {
    return EFI_MEMORY_WB;
  }

  if ( (Attributes & EFI_MEMORY_WP) == EFI_MEMORY_WP) {
    return EFI_MEMORY_WP;
  }

  return INVALID_CPU_ARCH_ATTRIBUTES;

}

STATIC
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
/*++

Routine Description:

  Do operation on a segment of memory space specified (add, free, remove, change attribute ...).

Arguments:

  Operation       - The type of the operation
  
  GcdMemoryType   - Additional information for the operation
  
  GcdIoType       - Additional information for the operation
  
  BaseAddress     - Start address of the segment
  
  Length          - length of the segment
  
  Capabilities    - The alterable attributes of a newly added entry
  
  Attributes      - The attributes needs to be set
  
Returns:

  EFI_INVALID_PARAMETER       - Length is 0 or address (length) not aligned when setting attribute.
  
  EFI_SUCCESS                 - Action successfully done.
  
  EFI_UNSUPPORTED             - Could not find the proper descriptor on this segment or 
                                set an upsupported attribute.
  
  EFI_ACCESS_DENIED           - Operate on an space non-exist or is used for an image.
  
  EFI_NOT_FOUND               - Free a non-using space or remove a non-exist space, and so on.
  
  EFI_OUT_OF_RESOURCES        - No buffer could be allocated.

Returns:

--*/
{
  EFI_STATUS         Status;
  LIST_ENTRY         *Map;
  LIST_ENTRY         *Link;
  EFI_GCD_MAP_ENTRY  *Entry;
  EFI_GCD_MAP_ENTRY  *TopEntry;
  EFI_GCD_MAP_ENTRY  *BottomEntry;
  LIST_ENTRY         *StartLink;
  LIST_ENTRY         *EndLink;
  
  EFI_CPU_ARCH_PROTOCOL           *CpuArch;
  UINT64                          CpuArchAttributes;

  if (Length == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Map = NULL;
  if (Operation & GCD_MEMORY_SPACE_OPERATION) {
    CoreAcquireGcdMemoryLock ();
    Map = &mGcdMemorySpaceMap;
  }
  if (Operation & GCD_IO_SPACE_OPERATION) {
    CoreAcquireGcdIoLock ();
    Map = &mGcdIoSpaceMap;
  }

  //
  // Search for the list of descriptors that cover the range BaseAddress to BaseAddress+Length
  //
  Status = CoreSearchGcdMapEntry (BaseAddress, Length, &StartLink, &EndLink, Map);
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;

    goto Done;
  }

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
    // Set attribute operations
    //
    case GCD_SET_ATTRIBUTES_MEMORY_OPERATION:
      if (Attributes & EFI_MEMORY_RUNTIME) {
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

  //
  //
  //
  if (Operation == GCD_SET_ATTRIBUTES_MEMORY_OPERATION) {
    //
    // Call CPU Arch Protocol to attempt to set attributes on the range
    //
    CpuArchAttributes = ConverToCpuArchAttributes (Attributes);
    if ( CpuArchAttributes != INVALID_CPU_ARCH_ATTRIBUTES ) {
      Status = CoreLocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&CpuArch);
      if (EFI_ERROR (Status)) {
        Status = EFI_ACCESS_DENIED;
        goto Done;
      }

      Status = CpuArch->SetMemoryAttributes (
                          CpuArch,
                          BaseAddress,
                          Length,
                          CpuArchAttributes
                          );
      if (EFI_ERROR (Status)) {
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
    // Set attribute operations
    //
    case GCD_SET_ATTRIBUTES_MEMORY_OPERATION:
      Entry->Attributes = Attributes;
      break;
    }
    Link = Link->ForwardLink;
  }

  //
  // Cleanup
  //
  Status = CoreCleanupGcdMapEntry (TopEntry, BottomEntry, StartLink, EndLink, Map);

Done:
  if (Operation & GCD_MEMORY_SPACE_OPERATION) {
    CoreReleaseGcdMemoryLock ();
  }
  if (Operation & GCD_IO_SPACE_OPERATION) {
    CoreReleaseGcdIoLock ();
  }

  return Status;
}

STATIC
EFI_STATUS
CoreAllocateSpaceCheckEntry (
  IN UINTN                Operation,
  IN EFI_GCD_MAP_ENTRY    *Entry,
  IN EFI_GCD_MEMORY_TYPE  GcdMemoryType,
  IN EFI_GCD_IO_TYPE      GcdIoType
  )
/*++

Routine Description:

  Check whether an entry could be used to allocate space.

Arguments:

  Operation       - Allocate memory or IO
  
  Entry           - The entry to be tested
  
  GcdMemoryType   - The desired memory type
  
  GcdIoType       - The desired IO type
  
Returns:

  EFI_NOT_FOUND   - The memory type does not match or there's an image handle on the entry.
  
  EFI_UNSUPPORTED - The operation unsupported.
  
  EFI_SUCCESS     - It's ok for this entry to be used to allocate space.

--*/
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

STATIC
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
/*++

Routine Description:

  Allocate space on specified address and length.

Arguments:

  Operation         - The type of operation (memory or IO)
  
  GcdAllocateType   - The type of allocate operation
  
  GcdMemoryType     - The desired memory type
  
  GcdIoType         - The desired IO type
  
  Alignment         - Align with 2^Alignment
  
  Length            - Length to allocate
  
  BaseAddress       - Base address to allocate
  
  ImageHandle       - The image handle consume the allocated space.
  
  DeviceHandle      - The device handle consume the allocated space.

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter.
  
  EFI_NOT_FOUND               - No descriptor for the desired space exists.
  
  EFI_SUCCESS                 - Space successfully allocated.

--*/
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
  if (GcdAllocateType < 0 || GcdAllocateType >= EfiGcdMaxAllocateType) {
    return EFI_INVALID_PARAMETER;
  }
  if (GcdMemoryType < 0 || GcdMemoryType >= EfiGcdMemoryTypeMaximum) {
    return EFI_INVALID_PARAMETER;
  }
  if (GcdIoType < 0 || GcdIoType >= EfiGcdIoTypeMaximum) {
    return EFI_INVALID_PARAMETER;
  }
  if (BaseAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (ImageHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (Alignment >= 64) {
    return EFI_NOT_FOUND;
  }
  if (Length == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Map = NULL;
  if (Operation & GCD_MEMORY_SPACE_OPERATION) {
    CoreAcquireGcdMemoryLock ();
    Map = &mGcdMemorySpaceMap;
  }
  if (Operation & GCD_IO_SPACE_OPERATION) {
    CoreAcquireGcdIoLock ();
    Map = &mGcdIoSpaceMap;
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
          GcdAllocateType == EfiGcdAllocateAnySearchTopDown           ) {
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
  if (Operation & GCD_MEMORY_SPACE_OPERATION) {
    CoreReleaseGcdMemoryLock ();
  }
  if (Operation & GCD_IO_SPACE_OPERATION) {
    CoreReleaseGcdIoLock ();
  }

  return Status;
}

STATIC
EFI_STATUS
CoreInternalAddMemorySpace (
  IN EFI_GCD_MEMORY_TYPE   GcdMemoryType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Capabilities
  )
/*++

Routine Description:

  Add a segment of memory to GCD map.

Arguments:

  GcdMemoryType     - Memory type of the segment.
  
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.
  
  Capabilities      - alterable attributes of the segment.

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameters.
  
  EFI_SUCCESS                 - Successfully add a segment of memory space.

--*/
{
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
EFI_STATUS
CoreAllocateMemorySpace (
  IN     EFI_GCD_ALLOCATE_TYPE  GcdAllocateType,
  IN     EFI_GCD_MEMORY_TYPE    GcdMemoryType,
  IN     UINTN                  Alignment,
  IN     UINT64                 Length,
  IN OUT EFI_PHYSICAL_ADDRESS   *BaseAddress,
  IN     EFI_HANDLE             ImageHandle,
  IN     EFI_HANDLE             DeviceHandle OPTIONAL
  )
/*++

Routine Description:

  Allocates nonexistent memory, reserved memory, system memory, or memorymapped
I/O resources from the global coherency domain of the processor.

Arguments:

  GcdAllocateType   - The type of allocate operation
  
  GcdMemoryType     - The desired memory type
  
  Alignment         - Align with 2^Alignment
  
  Length            - Length to allocate
  
  BaseAddress       - Base address to allocate
  
  ImageHandle       - The image handle consume the allocated space.
  
  DeviceHandle      - The device handle consume the allocated space.

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter.
  
  EFI_NOT_FOUND               - No descriptor contains the desired space.
  
  EFI_SUCCESS                 - Memory space successfully allocated.

--*/
{
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

EFI_STATUS
CoreAddMemorySpace (
  IN EFI_GCD_MEMORY_TYPE   GcdMemoryType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Capabilities
  )
/*++

Routine Description:

  Adds reserved memory, system memory, or memory-mapped I/O resources to the
global coherency domain of the processor.

Arguments:

  GcdMemoryType     - Memory type of the memory space.
  
  BaseAddress       - Base address of the memory space.
  
  Length            - Length of the memory space.
  
  Capabilities      - alterable attributes of the memory space.

Returns:

  EFI_SUCCESS       - Merged this memory space into GCD map.  

--*/
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PageBaseAddress;
  UINT64                PageLength;

  Status = CoreInternalAddMemorySpace (GcdMemoryType, BaseAddress, Length, Capabilities);

  if (!EFI_ERROR (Status) && GcdMemoryType == EfiGcdMemoryTypeSystemMemory) {

    PageBaseAddress = PageAlignLength (BaseAddress);
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

EFI_STATUS
CoreFreeMemorySpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:

  Frees nonexistent memory, reserved memory, system memory, or memory-mapped
I/O resources from the global coherency domain of the processor.

Arguments:

  BaseAddress       - Base address of the memory space.
  
  Length            - Length of the memory space.
  
Returns:

  EFI_SUCCESS       - Space successfully freed.

--*/
{
  return CoreConvertSpace (GCD_FREE_MEMORY_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, 0, 0);
}

EFI_STATUS
CoreRemoveMemorySpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:

  Removes reserved memory, system memory, or memory-mapped I/O resources from
the global coherency domain of the processor.

Arguments:

  BaseAddress       - Base address of the memory space.
  
  Length            - Length of the memory space.
  
Returns:

  EFI_SUCCESS       - Successfully remove a segment of memory space.

--*/
{
  return CoreConvertSpace (GCD_REMOVE_MEMORY_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, 0, 0);
}

STATIC
VOID
BuildMemoryDescriptor (
  IN OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor,
  IN EFI_GCD_MAP_ENTRY                *Entry
  )
/*++

Routine Description:

  Build a memory descriptor according to an entry.

Arguments:

  Descriptor          - The descriptor to be built
  
  Entry               - According to this entry

Returns:

  None

--*/
{
  Descriptor->BaseAddress   = Entry->BaseAddress;
  Descriptor->Length        = Entry->EndAddress - Entry->BaseAddress + 1;
  Descriptor->Capabilities  = Entry->Capabilities;
  Descriptor->Attributes    = Entry->Attributes;
  Descriptor->GcdMemoryType = Entry->GcdMemoryType;
  Descriptor->ImageHandle   = Entry->ImageHandle;
  Descriptor->DeviceHandle  = Entry->DeviceHandle;
}

EFI_STATUS
CoreGetMemorySpaceDescriptor (
  IN  EFI_PHYSICAL_ADDRESS             BaseAddress,
  OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor
  )
/*++

Routine Description:

  Retrieves the descriptor for a memory region containing a specified address.

Arguments:

  BaseAddress       - Specified start address
  
  Descriptor        - Specified length

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter
  
  EFI_SUCCESS                 - Successfully get memory space descriptor.

--*/
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
    //
    // Copy the contents of the found descriptor into Descriptor
    //
    Entry = CR (StartLink, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    BuildMemoryDescriptor (Descriptor, Entry);
  }

  CoreReleaseGcdMemoryLock ();

  return Status;
}

EFI_STATUS
CoreSetMemorySpaceAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes
  )
/*++

Routine Description:

  Modifies the attributes for a memory region in the global coherency domain of the
processor.

Arguments:

  BaseAddress       - Specified start address
  
  Length            - Specified length
  
  Attributes        - Specified attributes

Returns:

  EFI_SUCCESS       - Successfully set attribute of a segment of memory space.

--*/
{
  return CoreConvertSpace (GCD_SET_ATTRIBUTES_MEMORY_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, 0, Attributes);
}

EFI_STATUS
CoreGetMemorySpaceMap (
  OUT UINTN                            *NumberOfDescriptors,
  OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  **MemorySpaceMap
  )
/*++

Routine Description:

  Returns a map of the memory resources in the global coherency domain of the
processor.

Arguments:

  NumberOfDescriptors       - Number of descriptors.
  
  MemorySpaceMap            - Descriptor array

Returns:

  EFI_INVALID_PARAMETER     - Invalid parameter
  
  EFI_OUT_OF_RESOURCES      - No enough buffer to allocate
  
  EFI_SUCCESS               - Successfully get memory space map.

--*/
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
  *MemorySpaceMap = CoreAllocateBootServicesPool (*NumberOfDescriptors * sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR));
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

EFI_STATUS
CoreAddIoSpace (
  IN EFI_GCD_IO_TYPE       GcdIoType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:

  Adds reserved I/O or I/O resources to the global coherency domain of the processor.

Arguments:

  GcdIoType         - IO type of the segment.
  
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.

Returns:

  EFI_SUCCESS       - Merged this segment into GCD map.
  EFI_INVALID_PARAMETER    - Parameter not valid

--*/
{
  //
  // Make sure parameters are valid
  //
  if (GcdIoType <= EfiGcdIoTypeNonExistent || GcdIoType >= EfiGcdIoTypeMaximum) {
    return EFI_INVALID_PARAMETER;
  }
  return CoreConvertSpace (GCD_ADD_IO_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, GcdIoType, BaseAddress, Length, 0, 0);
}

EFI_STATUS
CoreAllocateIoSpace (
  IN     EFI_GCD_ALLOCATE_TYPE  GcdAllocateType,
  IN     EFI_GCD_IO_TYPE        GcdIoType,
  IN     UINTN                  Alignment,
  IN     UINT64                 Length,
  IN OUT EFI_PHYSICAL_ADDRESS   *BaseAddress,
  IN     EFI_HANDLE             ImageHandle,
  IN     EFI_HANDLE             DeviceHandle OPTIONAL
  )
/*++

Routine Description:

  Allocates nonexistent I/O, reserved I/O, or I/O resources from the global coherency
domain of the processor.

Arguments:

  GcdAllocateType   - The type of allocate operation
  
  GcdIoType         - The desired IO type
  
  Alignment         - Align with 2^Alignment
  
  Length            - Length to allocate
  
  BaseAddress       - Base address to allocate
  
  ImageHandle       - The image handle consume the allocated space.
  
  DeviceHandle      - The device handle consume the allocated space.

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter.
  
  EFI_NOT_FOUND               - No descriptor contains the desired space.
  
  EFI_SUCCESS                 - IO space successfully allocated.

--*/
{
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

EFI_STATUS
CoreFreeIoSpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:

  Frees nonexistent I/O, reserved I/O, or I/O resources from the global coherency
domain of the processor.

Arguments:

  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.
  
Returns:

  EFI_SUCCESS       - Space successfully freed.

--*/
{
  return CoreConvertSpace (GCD_FREE_IO_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, 0, 0);
}

EFI_STATUS
CoreRemoveIoSpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:

  Removes reserved I/O or I/O resources from the global coherency domain of the
processor.

Arguments:

  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.
  
Returns:

  EFI_SUCCESS       - Successfully removed a segment of IO space.

--*/
{
  return CoreConvertSpace (GCD_REMOVE_IO_OPERATION, (EFI_GCD_MEMORY_TYPE) 0, (EFI_GCD_IO_TYPE) 0, BaseAddress, Length, 0, 0);
}

STATIC
VOID
BuildIoDescriptor (
  IN EFI_GCD_IO_SPACE_DESCRIPTOR  *Descriptor,
  IN EFI_GCD_MAP_ENTRY            *Entry
  )
/*++

Routine Description:

  Build a IO descriptor according to an entry.

Arguments:

  Descriptor          - The descriptor to be built
  
  Entry               - According to this entry

Returns:

  None

--*/
{
  Descriptor->BaseAddress  = Entry->BaseAddress;
  Descriptor->Length       = Entry->EndAddress - Entry->BaseAddress + 1;
  Descriptor->GcdIoType    = Entry->GcdIoType;
  Descriptor->ImageHandle  = Entry->ImageHandle;
  Descriptor->DeviceHandle = Entry->DeviceHandle;
}

EFI_STATUS
CoreGetIoSpaceDescriptor (
  IN  EFI_PHYSICAL_ADDRESS         BaseAddress,
  OUT EFI_GCD_IO_SPACE_DESCRIPTOR  *Descriptor
  )
/*++

Routine Description:

  Retrieves the descriptor for an I/O region containing a specified address.

Arguments:

  BaseAddress       - Specified start address
  
  Descriptor        - Specified length

Returns:

  EFI_INVALID_PARAMETER       - Descriptor is NULL.
  
  EFI_SUCCESS                 - Successfully get the IO space descriptor.

--*/
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
    //
    // Copy the contents of the found descriptor into Descriptor
    //
    Entry = CR (StartLink, EFI_GCD_MAP_ENTRY, Link, EFI_GCD_MAP_SIGNATURE);
    BuildIoDescriptor (Descriptor, Entry);
  }

  CoreReleaseGcdIoLock ();

  return Status;
}

EFI_STATUS
CoreGetIoSpaceMap (
  OUT UINTN                        *NumberOfDescriptors,
  OUT EFI_GCD_IO_SPACE_DESCRIPTOR  **IoSpaceMap
  )
/*++

Routine Description:

  Returns a map of the I/O resources in the global coherency domain of the processor.

Arguments:

  NumberOfDescriptors       - Number of descriptors.
  
  IoSpaceMap                - Descriptor array

Returns:

  EFI_INVALID_PARAMETER     - Invalid parameter
  
  EFI_OUT_OF_RESOURCES      - No enough buffer to allocate
  
  EFI_SUCCESS               - Successfully get IO space map.

--*/
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
  *IoSpaceMap = CoreAllocateBootServicesPool (*NumberOfDescriptors * sizeof (EFI_GCD_IO_SPACE_DESCRIPTOR));
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

STATIC
UINT64
CoreConvertResourceDescriptorHobAttributesToCapabilities (
  EFI_GCD_MEMORY_TYPE  GcdMemoryType,
  UINT64               Attributes
  )
/*++

Routine Description:

  Converts a Resource Descriptor HOB attributes mask to an EFI Memory Descriptor 
  capabilities mask

Arguments:

  GcdMemoryType   - Type of resource in the GCD memory map.
  Attributes      - The attribute mask in the Resource Descriptor HOB.

Returns:

  The capabilities mask for an EFI Memory Descriptor.

--*/
{
  UINT64                          Capabilities;
  GCD_ATTRIBUTE_CONVERSION_ENTRY  *Conversion;
  
  //
  // Convert the Resource HOB Attributes to an EFI Memory Capabilities mask
  //
  for (Capabilities = 0, Conversion = mAttributeConversionTable; Conversion->Attribute != 0; Conversion++) {
    if (Conversion->Memory || (GcdMemoryType != EfiGcdMemoryTypeSystemMemory)) {
      if (Attributes & Conversion->Attribute) {
        Capabilities |= Conversion->Capability;
      }
    }
  }
  
  return Capabilities;
}

EFI_STATUS
CoreInitializeMemoryServices (
  IN  VOID                  **HobStart,
  OUT EFI_PHYSICAL_ADDRESS  *MemoryBaseAddress,
  OUT UINT64                *MemoryLength
  )
/*++

Routine Description:

  External function. Initializes the GCD and memory services based on the memory 
  descriptor HOBs.  This function is responsible for priming the GCD map and the
  memory map, so memory allocations and resource allocations can be made.  The first
  part of this function can not depend on any memory services until at least one
  memory descriptor is provided to the memory services.  Then the memory services
  can be used to intialize the GCD map.

Arguments:

  HobStart            - The start address of the HOB.
  MemoryBaseAddress   - Start address of memory region found to init DXE core.
  MemoryLength        - Length of memory region found to init DXE core.

Returns:

  EFI_SUCCESS   - Memory services successfully initialized.

--*/
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
  EFI_PHYSICAL_ADDRESS               MaxMemoryBaseAddress;
  UINT64                             MaxMemoryLength;
  UINT64                             MaxMemoryAttributes;
  EFI_PHYSICAL_ADDRESS               MaxAddress;
  EFI_PHYSICAL_ADDRESS               HighAddress;
  EFI_HOB_RESOURCE_DESCRIPTOR        *MaxResourceHob;
  EFI_HOB_GUID_TYPE                  *GuidHob;

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
  MaxResourceHob        = NULL;
  ResourceHob           = NULL;
  BaseAddress           = 0;
  Length                = 0;
  Attributes            = 0;
  MaxMemoryBaseAddress  = 0;
  MaxMemoryLength       = 0;
  MaxMemoryAttributes   = 0;

  //
  // Cache the PHIT HOB for later use
  //
  PhitHob = Hob.HandoffInformationTable;

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
  // Find the Resource Descriptor HOB that contains range FreeMemoryBaseAddress..FreeMemoryLength
  //
  Length = 0;
  Found  = FALSE;
  for (Hob.Raw = *HobStart; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {

    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {

      ResourceHob = Hob.ResourceDescriptor;

      if (ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY                                       &&
          (ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) == TESTED_MEMORY_ATTRIBUTES    ) {

        if (PhitHob->EfiFreeMemoryBottom >= ResourceHob->PhysicalStart                         && 
            PhitHob->EfiFreeMemoryTop    <= (ResourceHob->PhysicalStart + ResourceHob->ResourceLength)    ) {

          //
          // Cache the resource descriptor HOB for the memory region described by the PHIT HOB
          //
          PhitResourceHob = ResourceHob;
          Found = TRUE;

          Attributes  = PhitResourceHob->ResourceAttribute;
          BaseAddress = PageAlignAddress (PhitHob->EfiMemoryTop);
          Length      = PageAlignLength  (ResourceHob->PhysicalStart + ResourceHob->ResourceLength - BaseAddress);
          if (Length < MINIMUM_INITIAL_MEMORY_SIZE) {
            BaseAddress = PageAlignAddress (PhitHob->EfiFreeMemoryBottom);
            Length      = PageAlignLength  (PhitHob->EfiFreeMemoryTop - BaseAddress);
            if (Length < MINIMUM_INITIAL_MEMORY_SIZE) {
              BaseAddress = PageAlignAddress (ResourceHob->PhysicalStart);
              Length      = PageAlignLength  ((UINT64)((UINTN)*HobStart - BaseAddress));
            }
          }
          break;
        }
      }
    }
  }

  //
  // Assert if a resource descriptor HOB for the memory region described by the PHIT was not found
  //
  ASSERT (Found);

  //
  // Search all the resource descriptor HOBs from the highest possible addresses down for a memory
  // region that is big enough to initialize the DXE core.  Always skip the PHIT Resource HOB.
  // The max address must be within the physically addressible range for the processor.
  //
  MaxMemoryLength = 0;
  MaxAddress      = EFI_MAX_ADDRESS;
  do {
    HighAddress = 0;
    Found       = FALSE;
    //
    // Search for a tested memory region that is below MaxAddress
    //
    for (Hob.Raw = *HobStart; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {

      //
      // See if this is a resource descriptor HOB that does not contain the PHIT.
      //
      if (Hob.ResourceDescriptor != PhitResourceHob && GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {

        ResourceHob = Hob.ResourceDescriptor;
        //
        // See if this resource descrior HOB describes tested system memory below MaxAddress
        //
        if (ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY                                       &&
            (ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) == TESTED_MEMORY_ATTRIBUTES &&
            ResourceHob->PhysicalStart + ResourceHob->ResourceLength <= MaxAddress                            ) {

          //
          // See if this is the highest tested system memory region below MaxAddress
          //
          if (ResourceHob->PhysicalStart > HighAddress) {

            MaxResourceHob = ResourceHob;
            HighAddress = MaxResourceHob->PhysicalStart;
            Found = TRUE;
          }
        }
      }
    }
    if (Found) {
      //
      // Compute the size of the tested memory region below MaxAddrees
      //
      MaxMemoryBaseAddress = PageAlignAddress (MaxResourceHob->PhysicalStart);
      MaxMemoryLength      = PageAlignLength  (MaxResourceHob->PhysicalStart + MaxResourceHob->ResourceLength - MaxMemoryBaseAddress);
      MaxMemoryAttributes  = MaxResourceHob->ResourceAttribute;
    }
    MaxAddress = ResourceHob->PhysicalStart;
  } while (Found && MaxMemoryLength < MINIMUM_INITIAL_MEMORY_SIZE);

  //
  //
  //
  if ((Length < MINIMUM_INITIAL_MEMORY_SIZE)                                                 ||
      (MaxMemoryBaseAddress > BaseAddress && MaxMemoryLength >= MINIMUM_INITIAL_MEMORY_SIZE)    ) {
    BaseAddress = MaxMemoryBaseAddress;
    Length      = MaxMemoryLength;
    Attributes  = MaxMemoryAttributes;
  }

  //
  // If no memory regions are found that are big enough to initialize the DXE core, then ASSERT().
  //
  ASSERT (Length >= MINIMUM_INITIAL_MEMORY_SIZE);

  //
  // Convert the Resource HOB Attributes to an EFI Memory Capabilities mask
  //
  Capabilities = CoreConvertResourceDescriptorHobAttributesToCapabilities (EfiGcdMemoryTypeSystemMemory, Attributes);

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

EFI_STATUS
CoreInitializeGcdServices (
  IN VOID                  **HobStart,
  IN EFI_PHYSICAL_ADDRESS  MemoryBaseAddress,
  IN UINT64                MemoryLength
  )
/*++

Routine Description:

  External function. Initializes the GCD and memory services based on the memory 
  descriptor HOBs.  This function is responsible for priming the GCD map and the
  memory map, so memory allocations and resource allocations can be made.  The first
  part of this function can not depend on any memory services until at least one
  memory descriptor is provided to the memory services.  Then the memory services
  can be used to intialize the GCD map.

Arguments:

  HobStart - The start address of the HOB

  MemoryBaseAddress   - Start address of memory region found to init DXE core.
  
  MemoryLength        - Length of memory region found to init DXE core.


Returns:

  EFI_SUCCESS         - GCD services successfully initialized.

--*/
{
  EFI_PEI_HOB_POINTERS                   Hob;
  VOID                               *NewHobList;
  EFI_HOB_HANDOFF_INFO_TABLE  *PhitHob;
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
  Entry = CoreAllocateCopyPool (sizeof (EFI_GCD_MAP_ENTRY), &mGcdMemorySpaceMapEntryTemplate);
  ASSERT (Entry != NULL);

  Entry->EndAddress = LShiftU64 (1, SizeOfMemorySpace) - 1;

  InsertHeadList (&mGcdMemorySpaceMap, &Entry->Link);

  //
  // Initialize the GCD I/O Space Map
  //
  Entry = CoreAllocateCopyPool (sizeof (EFI_GCD_MAP_ENTRY), &mGcdIoSpaceMapEntryTemplate);
  ASSERT (Entry != NULL);

  Entry->EndAddress = LShiftU64 (1, SizeOfIoSpace) - 1;

  InsertHeadList (&mGcdIoSpaceMap, &Entry->Link);

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
          GcdMemoryType = EfiGcdMemoryTypeSystemMemory;
        }
        if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) == INITIALIZED_MEMORY_ATTRIBUTES) {
          GcdMemoryType = EfiGcdMemoryTypeReserved;
        }
        if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) == PRESENT_MEMORY_ATTRIBUTES) {
          GcdMemoryType = EfiGcdMemoryTypeReserved;
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
  Status = CoreAllocateMemorySpace (
             EfiGcdAllocateAddress,
             EfiGcdMemoryTypeSystemMemory,
             0,
             MemoryLength,
             &MemoryBaseAddress,
             gDxeCoreImageHandle,
             NULL
             );

  //
  // Walk the HOB list and allocate all memory space that is consumed by memory allocation HOBs,
  // and Firmware Volume HOBs.  Also update the EFI Memory Map with the memory allocation HOBs.
  //
  for (Hob.Raw = *HobStart; !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      MemoryHob = Hob.MemoryAllocation;
      BaseAddress = MemoryHob->AllocDescriptor.MemoryBaseAddress;
      Status = CoreAllocateMemorySpace (
                 EfiGcdAllocateAddress,
                 EfiGcdMemoryTypeSystemMemory, 
                 0,
                 MemoryHob->AllocDescriptor.MemoryLength,
                 &BaseAddress,
                 gDxeCoreImageHandle,
                 NULL
                 );
      if (!EFI_ERROR (Status)) {
        Status = CoreGetMemorySpaceDescriptor (MemoryHob->AllocDescriptor.MemoryBaseAddress, &Descriptor);
        if (!EFI_ERROR (Status)) {
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
  // Relocate HOB List to an allocated pool buffer.
  //
  NewHobList = CoreAllocateCopyPool (
                 (UINTN)PhitHob->EfiFreeMemoryBottom - (UINTN)(*HobStart), 
                 *HobStart
                 );
  ASSERT (NewHobList != NULL);

  *HobStart = NewHobList;
  gHobList  = NewHobList;

  //
  // Add and allocate the remaining unallocated system memory to the memory services.
  //
  Status = CoreGetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeSystemMemory) {
      if (MemorySpaceMap[Index].ImageHandle == NULL) {
        BaseAddress  = PageAlignAddress (MemorySpaceMap[Index].BaseAddress);
        Length       = PageAlignLength  (MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length - BaseAddress);
        CoreAddMemoryDescriptor (
          EfiConventionalMemory,
          BaseAddress,
          RShiftU64 (Length, EFI_PAGE_SHIFT),
          MemorySpaceMap[Index].Capabilities & (~EFI_MEMORY_RUNTIME)
          );
        Status = CoreAllocateMemorySpace (
                   EfiGcdAllocateAddress,
                   EfiGcdMemoryTypeSystemMemory,
                   0,
                   Length,
                   &BaseAddress,
                   gDxeCoreImageHandle,
                   NULL
                   );
      }
    }
  }
  CoreFreePool (MemorySpaceMap);

  return EFI_SUCCESS;
}
