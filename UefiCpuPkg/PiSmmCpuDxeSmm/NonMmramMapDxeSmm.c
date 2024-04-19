/** @file

Copyright (c) 2016 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

//
// attributes for reserved memory before it is promoted to system memory
//

#define EFI_MEMORY_PRESENT      0x0100000000000000ULL
#define EFI_MEMORY_INITIALIZED  0x0200000000000000ULL
#define EFI_MEMORY_TESTED       0x0400000000000000ULL

#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

EFI_MEMORY_DESCRIPTOR  *mUefiMemoryMap;
UINTN                  mUefiMemoryMapSize;
UINTN                  mUefiDescriptorSize;

EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *mGcdMemSpace       = NULL;
UINTN                            mGcdMemNumberOfDesc = 0;

EFI_MEMORY_ATTRIBUTES_TABLE  *mUefiMemoryAttributesTable = NULL;

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
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemSpaceMap;
  EFI_STATUS                       Status;
  UINTN                            Index;

  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemSpaceMap);
  if (EFI_ERROR (Status)) {
    return;
  }

  mGcdMemNumberOfDesc = 0;
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if ((MemSpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeReserved) &&
        ((MemSpaceMap[Index].Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
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
    gBS->FreePool (MemSpaceMap);
    return;
  }

  mGcdMemNumberOfDesc = 0;
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if ((MemSpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeReserved) &&
        ((MemSpaceMap[Index].Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
         (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED))
        )
    {
      CopyMem (
        &mGcdMemSpace[mGcdMemNumberOfDesc],
        &MemSpaceMap[Index],
        sizeof (EFI_GCD_MEMORY_SPACE_DESCRIPTOR)
        );
      mGcdMemNumberOfDesc++;
    }
  }

  gBS->FreePool (MemSpaceMap);
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
  This function caches the UEFI memory map information.
**/
VOID
GetUefiMemoryMap (
  VOID
  )
{
  EFI_STATUS             Status;
  UINTN                  MapKey;
  UINT32                 DescriptorVersion;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  UINTN                  UefiMemoryMapSize;

  DEBUG ((DEBUG_INFO, "GetUefiMemoryMap\n"));

  UefiMemoryMapSize = 0;
  MemoryMap         = NULL;
  Status            = gBS->GetMemoryMap (
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

  //
  // Get additional information from GCD memory map.
  //
  GetGcdMemoryMap ();

  //
  // Get UEFI memory attributes table.
  //
  GetUefiMemoryAttributesTable ();
}

/**
  This function updates UEFI memory attribute according to UEFI memory map.

**/
VOID
UpdateUefiMemMapAttributes (
  VOID
  )
{
  BOOLEAN                WriteProtect;
  BOOLEAN                CetEnabled;
  EFI_STATUS             Status;
  UINTN                  Index;
  UINT64                 Limit;
  UINT64                 PreviousAddress;
  UINT64                 Base;
  UINT64                 MemoryAttr;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  UINTN                  MemoryMapEntryCount;
  EFI_MEMORY_DESCRIPTOR  *Entry;

  DEBUG ((DEBUG_INFO, "UpdateUefiMemMapAttributes Start...\n"));

  WRITE_UNPROTECT_RO_PAGES (WriteProtect, CetEnabled);

  if (sizeof (UINTN) == sizeof (UINT32)) {
    Limit = BASE_4GB;
  } else {
    Limit = (IsRestrictedMemoryAccess ()) ? LShiftU64 (1, mPhysicalAddressBits) : BASE_4GB;
  }

  //
  // [0, 4k] may be non-present.
  //
  PreviousAddress = ((FixedPcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT1) != 0) ? BASE_4KB : 0;

  MemoryAttr = EFI_MEMORY_XP;
  for (Index = 0; Index < mSmmCpuSmramRangeCount; Index++) {
    Base = mSmmCpuSmramRanges[Index].CpuStart;
    if (Base > PreviousAddress) {
      //
      // Mark the ranges not in mSmmCpuSmramRanges as NX.
      //
      Status = SmmSetMemoryAttributes (PreviousAddress, Base - PreviousAddress, MemoryAttr);
      ASSERT_RETURN_ERROR (Status);
    }

    PreviousAddress = mSmmCpuSmramRanges[Index].CpuStart + mSmmCpuSmramRanges[Index].PhysicalSize;
  }

  if (PreviousAddress < Limit) {
    Status = SmmSetMemoryAttributes (PreviousAddress, Limit - PreviousAddress, MemoryAttr);
    ASSERT_RETURN_ERROR (Status);
  }

  if (IsRestrictedMemoryAccess ()) {
    if (mUefiMemoryMap != NULL) {
      MemoryMapEntryCount = mUefiMemoryMapSize/mUefiDescriptorSize;
      MemoryMap           = mUefiMemoryMap;
      for (Index = 0; Index < MemoryMapEntryCount; Index++) {
        if (IsUefiPageNotPresent (MemoryMap)) {
          Status = SmmSetMemoryAttributes (
                     MemoryMap->PhysicalStart,
                     EFI_PAGES_TO_SIZE ((UINTN)MemoryMap->NumberOfPages),
                     EFI_MEMORY_RP
                     );
          DEBUG ((
            DEBUG_INFO,
            "UefiMemory protection: 0x%lx - 0x%lx %r\n",
            MemoryMap->PhysicalStart,
            MemoryMap->PhysicalStart + (UINT64)EFI_PAGES_TO_SIZE ((UINTN)MemoryMap->NumberOfPages),
            Status
            ));
        }

        MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, mUefiDescriptorSize);
      }
    }

    //
    // Do not free mUefiMemoryMap, it will be checked in IsSmmCommBufferForbiddenAddress().
    //

    //
    // Set untested memory as not present.
    //
    if (mGcdMemSpace != NULL) {
      for (Index = 0; Index < mGcdMemNumberOfDesc; Index++) {
        Status = SmmSetMemoryAttributes (
                   mGcdMemSpace[Index].BaseAddress,
                   mGcdMemSpace[Index].Length,
                   EFI_MEMORY_RP
                   );
        DEBUG ((
          DEBUG_INFO,
          "GcdMemory protection: 0x%lx - 0x%lx %r\n",
          mGcdMemSpace[Index].BaseAddress,
          mGcdMemSpace[Index].BaseAddress + mGcdMemSpace[Index].Length,
          Status
          ));
      }
    }

    //
    // Do not free mGcdMemSpace, it will be checked in IsSmmCommBufferForbiddenAddress().
    //

    //
    // Set UEFI runtime memory with EFI_MEMORY_RO as not present.
    //
    if (mUefiMemoryAttributesTable != NULL) {
      Entry = (EFI_MEMORY_DESCRIPTOR *)(mUefiMemoryAttributesTable + 1);
      for (Index = 0; Index < mUefiMemoryAttributesTable->NumberOfEntries; Index++) {
        if ((Entry->Type == EfiRuntimeServicesCode) || (Entry->Type == EfiRuntimeServicesData)) {
          if ((Entry->Attribute & EFI_MEMORY_RO) != 0) {
            Status = SmmSetMemoryAttributes (
                       Entry->PhysicalStart,
                       EFI_PAGES_TO_SIZE ((UINTN)Entry->NumberOfPages),
                       EFI_MEMORY_RP
                       );
            DEBUG ((
              DEBUG_INFO,
              "UefiMemoryAttribute protection: 0x%lx - 0x%lx %r\n",
              Entry->PhysicalStart,
              Entry->PhysicalStart + (UINT64)EFI_PAGES_TO_SIZE ((UINTN)Entry->NumberOfPages),
              Status
              ));
          }
        }

        Entry = NEXT_MEMORY_DESCRIPTOR (Entry, mUefiMemoryAttributesTable->DescriptorSize);
      }
    }

    //
    // Do not free mUefiMemoryAttributesTable, it will be checked in IsSmmCommBufferForbiddenAddress().
    //
  }

  //
  // Set execute-disable flag
  //
  mXdEnabled = TRUE;

  WRITE_PROTECT_RO_PAGES (WriteProtect, CetEnabled);

  DEBUG ((DEBUG_INFO, "UpdateUefiMemMapAttributes Done.\n"));
}

/**
  Return if the Address is forbidden as SMM communication buffer.

  @param[in] Address the address to be checked

  @return TRUE  The address is forbidden as SMM communication buffer.
  @return FALSE The address is allowed as SMM communication buffer.
**/
BOOLEAN
IsSmmCommBufferForbiddenAddress (
  IN UINT64  Address
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  UINTN                  MemoryMapEntryCount;
  UINTN                  Index;
  EFI_MEMORY_DESCRIPTOR  *Entry;

  if (!FixedPcdGetBool (PcdCpuSmmRestrictedMemoryAccess)) {
    return FALSE;
  }

  if (mUefiMemoryMap != NULL) {
    MemoryMap           = mUefiMemoryMap;
    MemoryMapEntryCount = mUefiMemoryMapSize/mUefiDescriptorSize;
    for (Index = 0; Index < MemoryMapEntryCount; Index++) {
      if (IsUefiPageNotPresent (MemoryMap)) {
        if ((Address >= MemoryMap->PhysicalStart) &&
            (Address < MemoryMap->PhysicalStart + EFI_PAGES_TO_SIZE ((UINTN)MemoryMap->NumberOfPages)))
        {
          return TRUE;
        }
      }

      MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, mUefiDescriptorSize);
    }
  }

  if (mGcdMemSpace != NULL) {
    for (Index = 0; Index < mGcdMemNumberOfDesc; Index++) {
      if ((Address >= mGcdMemSpace[Index].BaseAddress) &&
          (Address < mGcdMemSpace[Index].BaseAddress + mGcdMemSpace[Index].Length))
      {
        return TRUE;
      }
    }
  }

  if (mUefiMemoryAttributesTable != NULL) {
    Entry = (EFI_MEMORY_DESCRIPTOR *)(mUefiMemoryAttributesTable + 1);
    for (Index = 0; Index < mUefiMemoryAttributesTable->NumberOfEntries; Index++) {
      if ((Entry->Type == EfiRuntimeServicesCode) || (Entry->Type == EfiRuntimeServicesData)) {
        if ((Entry->Attribute & EFI_MEMORY_RO) != 0) {
          if ((Address >= Entry->PhysicalStart) &&
              (Address < Entry->PhysicalStart + LShiftU64 (Entry->NumberOfPages, EFI_PAGE_SHIFT)))
          {
            return TRUE;
          }

          Entry = NEXT_MEMORY_DESCRIPTOR (Entry, mUefiMemoryAttributesTable->DescriptorSize);
        }
      }
    }
  }

  return FALSE;
}

/*
  Build MMIO Memory Map.

  The caller is responsible for freeing MemoryMap via FreePool().

  @param[out]     MemoryMap            Returned Non-Mmram Memory Map.
  @param[out]     MemoryMapSize        A pointer to the size, it is the size of new created memory map.
  @param[out]     DescriptorSize       Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.

*/
VOID
BuildMmioMemoryMap (
  OUT EFI_MEMORY_DESCRIPTOR  **MemoryMap,
  OUT UINTN                  *MemoryMapSize,
  OUT UINTN                  *DescriptorSize
  )
{
  UINTN                            Index;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;
  UINTN                            NumberOfSpaceDescriptors;
  UINTN                            MemoryMapIndex;
  UINTN                            Count;

  MemorySpaceMap           = NULL;
  NumberOfSpaceDescriptors = 0;
  Count                    = 0;

  ASSERT (MemoryMap != NULL && MemoryMapSize != NULL && DescriptorSize != NULL);

  *MemoryMap      = NULL;
  *MemoryMapSize  = 0;
  *DescriptorSize = 0;

  //
  // Get MMIO ranges from GCD.
  //
  gDS->GetMemorySpaceMap (
         &NumberOfSpaceDescriptors,
         &MemorySpaceMap
         );
  for (Index = 0; Index < NumberOfSpaceDescriptors; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo) {
      Count++;
    }
  }

  *DescriptorSize = sizeof (EFI_MEMORY_DESCRIPTOR);
  *MemoryMapSize  =  *DescriptorSize * Count;

  *MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocateZeroPool (*MemoryMapSize);
  ASSERT (*MemoryMap != NULL);

  MemoryMapIndex = 0;
  for (Index = 0; Index < NumberOfSpaceDescriptors; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType != EfiGcdMemoryTypeMemoryMappedIo) {
      continue;
    }

    (*MemoryMap)[MemoryMapIndex].PhysicalStart = MemorySpaceMap[Index].BaseAddress;
    (*MemoryMap)[MemoryMapIndex].NumberOfPages = EFI_SIZE_TO_PAGES (MemorySpaceMap[Index].Length);
    (*MemoryMap)[MemoryMapIndex].Type          = EfiMemoryMappedIO;

    MemoryMapIndex++;
  }

  return;
}

/*
  Create extended protection MemoryMap.

  The caller is responsible for freeing MemoryMap via FreePool().

  @param[out]     MemoryMap            Returned Non-Mmram Memory Map.
  @param[out]     MemoryMapSize        A pointer to the size, it is the size of new created memory map.
  @param[out]     DescriptorSize       Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.

*/
VOID
CreateExtendedProtectionRange (
  OUT EFI_MEMORY_DESCRIPTOR  **MemoryMap,
  OUT UINTN                  *MemoryMapSize,
  OUT UINTN                  *DescriptorSize
  )
{
  //
  // Build MMIO
  //
  BuildMmioMemoryMap (MemoryMap, MemoryMapSize, DescriptorSize);

  //
  // Future extended range could be added here.
  //
}

/*
  Create the Non-Mmram Memory Map.
  The Memory Map shall be within the Range of [0, 2^PhysicalAddressBits] Ranges.

  The caller is responsible for freeing MemoryMap via FreePool().

  @param[in]      PhysicalAddressBits  The bits of physical address to map.
  @param[out]     MemoryMap            Returned Non-Mmram Memory Map.
  @param[out]     MemoryMapSize        A pointer to the size, it is the size of new created memory map.
  @param[out]     DescriptorSize       Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.

*/
VOID
CreateNonMmramMemMap (
  IN  UINT8                  PhysicalAddressBits,
  OUT EFI_MEMORY_DESCRIPTOR  **MemoryMap,
  OUT UINTN                  *MemoryMapSize,
  OUT UINTN                  *DescriptorSize
  )
{
  UINT64  MaxLength;
  UINTN   Count;
  UINTN   Index;
  UINT64  PreviousAddress;
  UINT64  Base;
  UINT64  Length;

  ASSERT (MemoryMap != NULL && MemoryMapSize != NULL && DescriptorSize != NULL);

  *MemoryMap      = NULL;
  *MemoryMapSize  = 0;
  *DescriptorSize = 0;

  MaxLength = LShiftU64 (1, PhysicalAddressBits);

  //
  // Build MemoryMap to cover [0, PhysicalAddressBits] by excluding all Smram range
  //
  Count = mSmmCpuSmramRangeCount + 1;

  *DescriptorSize = sizeof (EFI_MEMORY_DESCRIPTOR);
  *MemoryMapSize  =  *DescriptorSize * Count;

  *MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocateZeroPool (*MemoryMapSize);
  ASSERT (*MemoryMap != NULL);

  PreviousAddress = 0;
  for (Index = 0; Index < mSmmCpuSmramRangeCount; Index++) {
    Base   = mSmmCpuSmramRanges[Index].CpuStart;
    Length = mSmmCpuSmramRanges[Index].PhysicalSize;

    ASSERT (MaxLength > Base +  Length);

    if (Base > PreviousAddress) {
      (*MemoryMap)[Index].PhysicalStart = PreviousAddress;
      (*MemoryMap)[Index].NumberOfPages = EFI_SIZE_TO_PAGES (Base - PreviousAddress);
      (*MemoryMap)[Index].Attribute     = EFI_MEMORY_XP;
    }

    PreviousAddress = Base + Length;
  }

  //
  // Set the last remaining range
  //
  if (PreviousAddress < MaxLength) {
    (*MemoryMap)[Index].PhysicalStart = PreviousAddress;
    (*MemoryMap)[Index].NumberOfPages = EFI_SIZE_TO_PAGES (MaxLength - PreviousAddress);
    (*MemoryMap)[Index].Attribute     = EFI_MEMORY_XP;
  }
}
