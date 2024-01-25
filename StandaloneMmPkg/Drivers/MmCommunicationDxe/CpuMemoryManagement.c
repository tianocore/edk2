/** @file

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MmCommunicationDxe.h"

//
// attributes for reserved memory before it is promoted to system memory
//
#define EFI_MEMORY_PRESENT      0x0100000000000000ULL
#define EFI_MEMORY_INITIALIZED  0x0200000000000000ULL
#define EFI_MEMORY_TESTED       0x0400000000000000ULL

#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

EFI_MEMORY_DESCRIPTOR            *mUefiMemoryMap;
UINTN                            mUefiMemoryMapSize;
UINTN                            mUefiDescriptorSize;
EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *mGcdMemSpace               = NULL;
UINTN                            mGcdMemNumberOfDesc         = 0;
EFI_MEMORY_ATTRIBUTES_TABLE      *mUefiMemoryAttributesTable = NULL;

/**
  Sort memory map entries based upon PhysicalStart, from low to high.

  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  MemoryMapSize          Size, in bytes, of the MemoryMap buffer.
  @param  DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
STATIC
VOID
SortMemoryMap (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      MemoryMapSize,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *NextMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR  TempMemoryMap;

  MemoryMapEntry     = MemoryMap;
  NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  MemoryMapEnd       = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while (MemoryMapEntry < MemoryMapEnd) {
    while (NextMemoryMapEntry < MemoryMapEnd) {
      if (MemoryMapEntry->PhysicalStart > NextMemoryMapEntry->PhysicalStart) {
        CopyMem (&TempMemoryMap, MemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
        CopyMem (MemoryMapEntry, NextMemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
        CopyMem (NextMemoryMapEntry, &TempMemoryMap, sizeof (EFI_MEMORY_DESCRIPTOR));
      }

      NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
    }

    MemoryMapEntry     = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }
}

/**
  Return if a UEFI memory page should be marked as not present in SMM page table.
  If the memory map entries type is
  EfiLoaderCode/Data, EfiBootServicesCode/Data, EfiConventionalMemory,
  EfiUnusableMemory, EfiACPIReclaimMemory, return TRUE.
  Or return FALSE.

  @param[in]  MemoryMap              A pointer to the memory descriptor.

  @return TRUE  The memory described will be marked as not present in SMM page table.
  @return FALSE The memory described will not be marked as not present in SMM page table.
**/
BOOLEAN
IsUefiPageNotPresent (
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap
  )
{
  switch (MemoryMap->Type) {
    case EfiLoaderCode:
    case EfiLoaderData:
    case EfiBootServicesCode:
    case EfiBootServicesData:
    case EfiConventionalMemory:
    case EfiUnusableMemory:
    case EfiACPIReclaimMemory:
      return TRUE;
    default:
      return FALSE;
  }
}

/**
  Merge continuous memory map entries whose type is
  EfiLoaderCode/Data, EfiBootServicesCode/Data, EfiConventionalMemory,
  EfiUnusableMemory, EfiACPIReclaimMemory, because the memory described by
  these entries will be set as NOT present in SMM page table.

  @param[in, out]  MemoryMap              A pointer to the buffer in which firmware places
                                          the current memory map.
  @param[in, out]  MemoryMapSize          A pointer to the size, in bytes, of the
                                          MemoryMap buffer. On input, this is the size of
                                          the current memory map.  On output,
                                          it is the size of new memory map after merge.
  @param[in]       DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
STATIC
VOID
MergeMemoryMapForNotPresentEntry (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN OUT UINTN                  *MemoryMapSize,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  UINT64                 MemoryBlockLength;
  EFI_MEMORY_DESCRIPTOR  *NewMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *NextMemoryMapEntry;

  MemoryMapEntry    = MemoryMap;
  NewMemoryMapEntry = MemoryMap;
  MemoryMapEnd      = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + *MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    CopyMem (NewMemoryMapEntry, MemoryMapEntry, sizeof (EFI_MEMORY_DESCRIPTOR));
    NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);

    do {
      MemoryBlockLength = (UINT64)(EFI_PAGES_TO_SIZE ((UINTN)MemoryMapEntry->NumberOfPages));
      if (((UINTN)NextMemoryMapEntry < (UINTN)MemoryMapEnd) &&
          IsUefiPageNotPresent (MemoryMapEntry) && IsUefiPageNotPresent (NextMemoryMapEntry) &&
          ((MemoryMapEntry->PhysicalStart + MemoryBlockLength) == NextMemoryMapEntry->PhysicalStart))
      {
        MemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;
        if (NewMemoryMapEntry != MemoryMapEntry) {
          NewMemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;
        }

        NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
        continue;
      } else {
        MemoryMapEntry = PREVIOUS_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
        break;
      }
    } while (TRUE);

    MemoryMapEntry    = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    NewMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapEntry, DescriptorSize);
  }

  *MemoryMapSize = (UINTN)NewMemoryMapEntry - (UINTN)MemoryMap;

  return;
}

/**
  This function caches the GCD memory map information.
**/
VOID
GetGcdMemoryMap (
  VOID
  )
{
  UINTN                            NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;
  EFI_STATUS                       Status;
  UINTN                            Index;

  NumberOfDescriptors = 0;
  Status              = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);

  if (EFI_ERROR (Status) || (NumberOfDescriptors == 0)) {
    return;
  }

  mGcdMemNumberOfDesc = 0;
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if ((MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeReserved) &&
        ((MemorySpaceMap[Index].Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
         (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED))
        )
    {
      mGcdMemNumberOfDesc++;
    }
  }

  mGcdMemSpace = AllocateZeroPool (mGcdMemNumberOfDesc * sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR));
  ASSERT (mGcdMemSpace != NULL);
  if (mGcdMemSpace == NULL) {
    mGcdMemNumberOfDesc = 0;
    return;
  }

  mGcdMemNumberOfDesc = 0;
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if ((MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeReserved) &&
        ((MemorySpaceMap[Index].Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
         (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED))
        )
    {
      CopyMem (
        &mGcdMemSpace[mGcdMemNumberOfDesc],
        &MemorySpaceMap[Index],
        sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR)
        );
      mGcdMemNumberOfDesc++;
    }
  }
}

/**
  Get UEFI MemoryAttributesTable.
**/
VOID
GetUefiMemoryAttributesTable (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_MEMORY_ATTRIBUTES_TABLE  *MemoryAttributesTable;
  UINTN                        MemoryAttributesTableSize;

  Status = EfiGetSystemConfigurationTable (&gEfiMemoryAttributesTableGuid, (VOID **)&MemoryAttributesTable);
  if (!EFI_ERROR (Status) && (MemoryAttributesTable != NULL)) {
    MemoryAttributesTableSize  = sizeof (EFI_MEMORY_ATTRIBUTES_TABLE) + MemoryAttributesTable->DescriptorSize * MemoryAttributesTable->NumberOfEntries;
    mUefiMemoryAttributesTable = AllocateCopyPool (MemoryAttributesTableSize, MemoryAttributesTable);
    ASSERT (mUefiMemoryAttributesTable != NULL);
  }
}

/**
  Communicate UEFI Memory map to PiSmmCpuStandaloneMm driver.
**/
VOID
CommunicateNonMmramMap (
  NON_MMRAM_MAP  *NonMmramMap,
  UINTN          BufferSize
  )
{
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE  *PiSmmCommunicationRegionTable;
  EFI_STATUS                               Status;
  EFI_MEMORY_DESCRIPTOR                    *MmCommMemRegion;
  EFI_MM_COMMUNICATE_HEADER                *CommHeader;
  UINTN                                    CommBufferSize;
  UINTN                                    Index;

  //
  // Communicate to smm cpu driver.
  //
  //
  // 1.Locate the gEdkiiPiSmmCommunicationRegionTableGuid.
  //
  Status = EfiGetSystemConfigurationTable (&gEdkiiPiSmmCommunicationRegionTableGuid, (VOID **)&PiSmmCommunicationRegionTable);
  ASSERT_EFI_ERROR (Status);

  //
  // Step 2: Grab one that is large enough to hold SMM_CPU_PERF_COMM_BUFFER.
  //
  CommBufferSize  = OFFSET_OF (EFI_MM_COMMUNICATE_HEADER, Data) + BufferSize;
  MmCommMemRegion = (EFI_MEMORY_DESCRIPTOR *)(PiSmmCommunicationRegionTable + 1);
  for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++) {
    if (MmCommMemRegion->Type == EfiConventionalMemory) {
      if (EFI_PAGES_TO_SIZE ((UINTN)MmCommMemRegion->NumberOfPages) >= CommBufferSize) {
        break;
      }
    }

    MmCommMemRegion = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MmCommMemRegion + PiSmmCommunicationRegionTable->DescriptorSize);
  }

  ASSERT (Index < PiSmmCommunicationRegionTable->NumberOfEntries);

  //
  // Step3: Start to populate contents.
  //
  CommHeader = (EFI_MM_COMMUNICATE_HEADER *)(UINTN)MmCommMemRegion->PhysicalStart;
  ZeroMem (CommHeader, CommBufferSize);
  CopyGuid (&CommHeader->HeaderGuid, &gNonMmramMapGuid);
  CommHeader->MessageLength = BufferSize;
  CopyMem (CommHeader->Data, NonMmramMap, BufferSize);

  //
  // Step 4: Use the protocol and signal SMI.
  //
  SmmCommunicationMmCommunicate2 (&mMmCommunication2, CommHeader, NULL, &CommBufferSize);
}

/**
  Get UEFI MemoryAttributesTable.
**/
VOID
CreateNonMmramMap (
  NON_MMRAM_MAP  *NonMmramMap,
  UINTN          *NumOfNonMmramEntry
  )
{
  UINTN                  Index;
  UINTN                  EntryIndex;
  EFI_MEMORY_DESCRIPTOR  *Entry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;

  Index      = 0;
  EntryIndex = 0;

  if (mUefiMemoryMap != NULL) {
    MemoryMap = mUefiMemoryMap;
    for ( ; Index < mUefiMemoryMapSize/mUefiDescriptorSize; Index++) {
      if (IsUefiPageNotPresent (MemoryMap)) {
        NonMmramMap->Entry[EntryIndex].PhysicalStart = MemoryMap->PhysicalStart;
        NonMmramMap->Entry[EntryIndex].NumberOfPages = MemoryMap->NumberOfPages;
        NonMmramMap->Entry[EntryIndex].Attribute     = EFI_MEMORY_RP;
        EntryIndex++;
      }

      MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, mUefiDescriptorSize);
    }
  }

  if (mGcdMemSpace != NULL) {
    for (Index = 0; Index < mGcdMemNumberOfDesc; Index++) {
      NonMmramMap->Entry[EntryIndex].PhysicalStart = mGcdMemSpace[Index].BaseAddress;
      NonMmramMap->Entry[EntryIndex].NumberOfPages = EFI_SIZE_TO_PAGES (mGcdMemSpace[Index].Length);
      NonMmramMap->Entry[EntryIndex].Attribute     = EFI_MEMORY_RP;
      EntryIndex++;
    }
  }

  if (mUefiMemoryAttributesTable != NULL) {
    Entry = (EFI_MEMORY_DESCRIPTOR *)(mUefiMemoryAttributesTable + 1);
    for (Index = 0; Index < mUefiMemoryAttributesTable->NumberOfEntries; Index++) {
      if ((Entry->Type == EfiRuntimeServicesCode) || (Entry->Type == EfiRuntimeServicesData)) {
        if ((Entry->Attribute & EFI_MEMORY_RO) != 0) {
          NonMmramMap->Entry[EntryIndex].PhysicalStart = Entry->PhysicalStart;
          NonMmramMap->Entry[EntryIndex].NumberOfPages = Entry->NumberOfPages;
          NonMmramMap->Entry[EntryIndex].Attribute     = EFI_MEMORY_RP;
          EntryIndex++;
        }
      }

      Entry = NEXT_MEMORY_DESCRIPTOR (Entry, mUefiMemoryAttributesTable->DescriptorSize);
    }
  }

  NonMmramMap->NumberOfEntry = EntryIndex;
  *NumOfNonMmramEntry        = EntryIndex;
}

/**
  This function caches the UEFI memory map information.
**/
VOID
CreateAndCommunicateNonMmramMap (
  VOID
  )
{
  EFI_STATUS             Status;
  UINTN                  MapKey;
  UINT32                 DescriptorVersion;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  UINTN                  UefiMemoryMapSize;
  UINTN                  NumOfMapEntry;
  NON_MMRAM_MAP          *NonMmramMap;
  UINTN                  NumOfNonMmramEntry;

  DEBUG ((DEBUG_INFO, "CreateAndCommunicateNonMmramMap\n"));

  NumOfMapEntry      = 0;
  UefiMemoryMapSize  = 0;
  NumOfNonMmramEntry = 0;
  MemoryMap          = NULL;
  Status             = gBS->GetMemoryMap (
                              &UefiMemoryMapSize,
                              MemoryMap,
                              &MapKey,
                              &mUefiDescriptorSize,
                              &DescriptorVersion
                              );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  do {
    Status = gBS->AllocatePool (EfiBootServicesData, UefiMemoryMapSize, (VOID **)&MemoryMap);
    ASSERT (MemoryMap != NULL);
    if (MemoryMap == NULL) {
      return;
    }

    Status = gBS->GetMemoryMap (
                    &UefiMemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &mUefiDescriptorSize,
                    &DescriptorVersion
                    );
    if (EFI_ERROR (Status)) {
      gBS->FreePool (MemoryMap);
      MemoryMap = NULL;
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  if (MemoryMap == NULL) {
    return;
  }

  SortMemoryMap (MemoryMap, UefiMemoryMapSize, mUefiDescriptorSize);
  MergeMemoryMapForNotPresentEntry (MemoryMap, &UefiMemoryMapSize, mUefiDescriptorSize);

  mUefiMemoryMapSize = UefiMemoryMapSize;
  mUefiMemoryMap     = AllocateCopyPool (UefiMemoryMapSize, MemoryMap);
  ASSERT (mUefiMemoryMap != NULL);

  gBS->FreePool (MemoryMap);
  NumOfMapEntry += mUefiMemoryMapSize/mUefiDescriptorSize;

  //
  // Get additional information from GCD memory map.
  //
  GetGcdMemoryMap ();
  NumOfMapEntry += mGcdMemNumberOfDesc;

  //
  // Get UEFI memory attributes table.
  //
  GetUefiMemoryAttributesTable ();
  NumOfMapEntry += mUefiMemoryAttributesTable->NumberOfEntries;

  //
  // Create NON_MMRAM_MAP
  //
  NonMmramMap = AllocateZeroPool (NumOfMapEntry * sizeof (NON_MMRAM_MAP_ENTRY) + sizeof (NON_MMRAM_MAP));
  ASSERT (NonMmramMap != NULL);
  CreateNonMmramMap (NonMmramMap, &NumOfNonMmramEntry);

  //
  // Communicate non-mmram map to PiSmmCpuStandaloneMm
  //
  CommunicateNonMmramMap (NonMmramMap, sizeof (NON_MMRAM_MAP) + NumOfNonMmramEntry * sizeof (NON_MMRAM_MAP_ENTRY));
  FreePool (NonMmramMap);
}
