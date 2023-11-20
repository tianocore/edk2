/** @file
  UEFI MemoryAttributesTable support

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/ImagePropertiesRecordLib.h>

#include <Guid/EventGroup.h>

#include <Guid/MemoryAttributesTable.h>

#include "DxeMain.h"
#include "HeapGuard.h"

/**
  This function for GetMemoryMap() with properties table capability.

  It calls original GetMemoryMap() to get the original memory map information. Then
  plus the additional memory map entries for PE Code/Data seperation.

  @param  MemoryMapSize          A pointer to the size, in bytes, of the
                                 MemoryMap buffer. On input, this is the size of
                                 the buffer allocated by the caller.  On output,
                                 it is the size of the buffer returned by the
                                 firmware  if the buffer was large enough, or the
                                 size of the buffer needed  to contain the map if
                                 the buffer was too small.
  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  MapKey                 A pointer to the location in which firmware
                                 returns the key for the current memory map.
  @param  DescriptorSize         A pointer to the location in which firmware
                                 returns the size, in bytes, of an individual
                                 EFI_MEMORY_DESCRIPTOR.
  @param  DescriptorVersion      A pointer to the location in which firmware
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
CoreGetMemoryMapWithSeparatedImageSection (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  OUT UINTN                     *MapKey,
  OUT UINTN                     *DescriptorSize,
  OUT UINT32                    *DescriptorVersion
  );

#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

#define IMAGE_PROPERTIES_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('I','P','P','D')

typedef struct {
  UINT32        Signature;
  UINTN         ImageRecordCount;
  UINTN         CodeSegmentCountMax;
  LIST_ENTRY    ImageRecordList;
} IMAGE_PROPERTIES_PRIVATE_DATA;

STATIC IMAGE_PROPERTIES_PRIVATE_DATA  mImagePropertiesPrivateData = {
  IMAGE_PROPERTIES_PRIVATE_DATA_SIGNATURE,
  0,
  0,
  INITIALIZE_LIST_HEAD_VARIABLE (mImagePropertiesPrivateData.ImageRecordList)
};

STATIC EFI_LOCK  mMemoryAttributesTableLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);

BOOLEAN                      mMemoryAttributesTableEnable      = TRUE;
BOOLEAN                      mMemoryAttributesTableEndOfDxe    = FALSE;
EFI_MEMORY_ATTRIBUTES_TABLE  *mMemoryAttributesTable           = NULL;
BOOLEAN                      mMemoryAttributesTableReadyToBoot = FALSE;
BOOLEAN                      gMemoryAttributesTableForwardCfi  = TRUE;

/**
  Install MemoryAttributesTable.

**/
VOID
InstallMemoryAttributesTable (
  VOID
  )
{
  UINTN                        MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR        *MemoryMap;
  EFI_MEMORY_DESCRIPTOR        *MemoryMapStart;
  UINTN                        MapKey;
  UINTN                        DescriptorSize;
  UINT32                       DescriptorVersion;
  UINTN                        Index;
  EFI_STATUS                   Status;
  UINT32                       RuntimeEntryCount;
  EFI_MEMORY_ATTRIBUTES_TABLE  *MemoryAttributesTable;
  EFI_MEMORY_DESCRIPTOR        *MemoryAttributesEntry;

  if (gMemoryMapTerminated) {
    //
    // Directly return after MemoryMap terminated.
    //
    return;
  }

  if (!mMemoryAttributesTableEnable) {
    DEBUG ((DEBUG_VERBOSE, "Cannot install Memory Attributes Table "));
    DEBUG ((DEBUG_VERBOSE, "because Runtime Driver Section Alignment is not %dK.\n", RUNTIME_PAGE_ALLOCATION_GRANULARITY >> 10));
    return;
  }

  if (mMemoryAttributesTable == NULL) {
    //
    // InstallConfigurationTable here to occupy one entry for MemoryAttributesTable
    // before GetMemoryMap below, as InstallConfigurationTable may allocate runtime
    // memory for the new entry.
    //
    Status = gBS->InstallConfigurationTable (&gEfiMemoryAttributesTableGuid, (VOID *)(UINTN)MAX_ADDRESS);
    ASSERT_EFI_ERROR (Status);
  }

  MemoryMapSize = 0;
  MemoryMap     = NULL;
  Status        = CoreGetMemoryMapWithSeparatedImageSection (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  do {
    MemoryMap = AllocatePool (MemoryMapSize);
    ASSERT (MemoryMap != NULL);

    Status = CoreGetMemoryMapWithSeparatedImageSection (
               &MemoryMapSize,
               MemoryMap,
               &MapKey,
               &DescriptorSize,
               &DescriptorVersion
               );
    if (EFI_ERROR (Status)) {
      FreePool (MemoryMap);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  MemoryMapStart    = MemoryMap;
  RuntimeEntryCount = 0;
  for (Index = 0; Index < MemoryMapSize/DescriptorSize; Index++) {
    switch (MemoryMap->Type) {
      case EfiRuntimeServicesCode:
      case EfiRuntimeServicesData:
        RuntimeEntryCount++;
        break;
    }

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
  }

  //
  // Allocate MemoryAttributesTable
  //
  MemoryAttributesTable = AllocatePool (sizeof (EFI_MEMORY_ATTRIBUTES_TABLE) + DescriptorSize * RuntimeEntryCount);
  ASSERT (MemoryAttributesTable != NULL);
  MemoryAttributesTable->Version         = EFI_MEMORY_ATTRIBUTES_TABLE_VERSION;
  MemoryAttributesTable->NumberOfEntries = RuntimeEntryCount;
  MemoryAttributesTable->DescriptorSize  = (UINT32)DescriptorSize;
  if (gMemoryAttributesTableForwardCfi) {
    MemoryAttributesTable->Flags = EFI_MEMORY_ATTRIBUTES_FLAGS_RT_FORWARD_CONTROL_FLOW_GUARD;
  } else {
    MemoryAttributesTable->Flags = 0;
  }

  DEBUG ((DEBUG_VERBOSE, "MemoryAttributesTable:\n"));
  DEBUG ((DEBUG_VERBOSE, "  Version              - 0x%08x\n", MemoryAttributesTable->Version));
  DEBUG ((DEBUG_VERBOSE, "  NumberOfEntries      - 0x%08x\n", MemoryAttributesTable->NumberOfEntries));
  DEBUG ((DEBUG_VERBOSE, "  DescriptorSize       - 0x%08x\n", MemoryAttributesTable->DescriptorSize));
  MemoryAttributesEntry = (EFI_MEMORY_DESCRIPTOR *)(MemoryAttributesTable + 1);
  MemoryMap             = MemoryMapStart;
  for (Index = 0; Index < MemoryMapSize/DescriptorSize; Index++) {
    switch (MemoryMap->Type) {
      case EfiRuntimeServicesCode:
      case EfiRuntimeServicesData:
        CopyMem (MemoryAttributesEntry, MemoryMap, DescriptorSize);
        MemoryAttributesEntry->Attribute &= (EFI_MEMORY_RO|EFI_MEMORY_XP|EFI_MEMORY_RUNTIME);
        DEBUG ((DEBUG_VERBOSE, "Entry (0x%x)\n", MemoryAttributesEntry));
        DEBUG ((DEBUG_VERBOSE, "  Type              - 0x%x\n", MemoryAttributesEntry->Type));
        DEBUG ((DEBUG_VERBOSE, "  PhysicalStart     - 0x%016lx\n", MemoryAttributesEntry->PhysicalStart));
        DEBUG ((DEBUG_VERBOSE, "  VirtualStart      - 0x%016lx\n", MemoryAttributesEntry->VirtualStart));
        DEBUG ((DEBUG_VERBOSE, "  NumberOfPages     - 0x%016lx\n", MemoryAttributesEntry->NumberOfPages));
        DEBUG ((DEBUG_VERBOSE, "  Attribute         - 0x%016lx\n", MemoryAttributesEntry->Attribute));
        MemoryAttributesEntry = NEXT_MEMORY_DESCRIPTOR (MemoryAttributesEntry, DescriptorSize);
        break;
    }

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
  }

  MemoryMap = MemoryMapStart;
  FreePool (MemoryMap);

  //
  // Update configuratoin table for MemoryAttributesTable.
  //
  Status = gBS->InstallConfigurationTable (&gEfiMemoryAttributesTableGuid, MemoryAttributesTable);
  ASSERT_EFI_ERROR (Status);

  if (mMemoryAttributesTable != NULL) {
    FreePool (mMemoryAttributesTable);
  }

  mMemoryAttributesTable = MemoryAttributesTable;
}

/**
  Install MemoryAttributesTable on memory allocation.

  @param[in] MemoryType EFI memory type.
**/
VOID
InstallMemoryAttributesTableOnMemoryAllocation (
  IN EFI_MEMORY_TYPE  MemoryType
  )
{
  //
  // Install MemoryAttributesTable after ReadyToBoot on runtime memory allocation.
  //
  if (mMemoryAttributesTableReadyToBoot &&
      ((MemoryType == EfiRuntimeServicesCode) || (MemoryType == EfiRuntimeServicesData)))
  {
    InstallMemoryAttributesTable ();
  }
}

/**
  Install MemoryAttributesTable on ReadyToBoot.

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
InstallMemoryAttributesTableOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  InstallMemoryAttributesTable ();
  mMemoryAttributesTableReadyToBoot = TRUE;
}

/**
  Install initial MemoryAttributesTable on EndOfDxe.
  Then SMM can consume this information.

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
InstallMemoryAttributesTableOnEndOfDxe (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mMemoryAttributesTableEndOfDxe = TRUE;
  InstallMemoryAttributesTable ();
}

/**
  Initialize MemoryAttrubutesTable support.
**/
VOID
EFIAPI
CoreInitializeMemoryAttributesTable (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   ReadyToBootEvent;
  EFI_EVENT   EndOfDxeEvent;

  //
  // Construct the table at ReadyToBoot.
  //
  Status = CoreCreateEventInternal (
             EVT_NOTIFY_SIGNAL,
             TPL_CALLBACK,
             InstallMemoryAttributesTableOnReadyToBoot,
             NULL,
             &gEfiEventReadyToBootGuid,
             &ReadyToBootEvent
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Construct the initial table at EndOfDxe,
  // then SMM can consume this information.
  // Use TPL_NOTIFY here, as such SMM code (TPL_CALLBACK)
  // can run after it.
  //
  Status = CoreCreateEventInternal (
             EVT_NOTIFY_SIGNAL,
             TPL_NOTIFY,
             InstallMemoryAttributesTableOnEndOfDxe,
             NULL,
             &gEfiEndOfDxeEventGroupGuid,
             &EndOfDxeEvent
             );
  ASSERT_EFI_ERROR (Status);
  return;
}

//
// Below functions are for MemoryMap
//

/**
  Acquire memory lock on mMemoryAttributesTableLock.
**/
STATIC
VOID
CoreAcquiremMemoryAttributesTableLock (
  VOID
  )
{
  CoreAcquireLock (&mMemoryAttributesTableLock);
}

/**
  Release memory lock on mMemoryAttributesTableLock.
**/
STATIC
VOID
CoreReleasemMemoryAttributesTableLock (
  VOID
  )
{
  CoreReleaseLock (&mMemoryAttributesTableLock);
}

/**
  Merge continous memory map entries whose have same attributes.

  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  MemoryMapSize          A pointer to the size, in bytes, of the
                                 MemoryMap buffer. On input, this is the size of
                                 the current memory map.  On output,
                                 it is the size of new memory map after merge.
  @param  DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
VOID
MergeMemoryMap (
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
      MergeGuardPages (NewMemoryMapEntry, NextMemoryMapEntry->PhysicalStart);
      MemoryBlockLength = LShiftU64 (NewMemoryMapEntry->NumberOfPages, EFI_PAGE_SHIFT);
      if (((UINTN)NextMemoryMapEntry < (UINTN)MemoryMapEnd) &&
          (NewMemoryMapEntry->Type == NextMemoryMapEntry->Type) &&
          (NewMemoryMapEntry->Attribute == NextMemoryMapEntry->Attribute) &&
          ((NewMemoryMapEntry->PhysicalStart + MemoryBlockLength) == NextMemoryMapEntry->PhysicalStart))
      {
        NewMemoryMapEntry->NumberOfPages += NextMemoryMapEntry->NumberOfPages;
        NextMemoryMapEntry                = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
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
  Enforce memory map attributes.
  This function will set EfiRuntimeServicesData/EfiMemoryMappedIO/EfiMemoryMappedIOPortSpace to be EFI_MEMORY_XP.

  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  MemoryMapSize          Size, in bytes, of the MemoryMap buffer.
  @param  DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
STATIC
VOID
EnforceMemoryMapAttribute (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      MemoryMapSize,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    switch (MemoryMapEntry->Type) {
      case EfiRuntimeServicesCode:
        // do nothing
        break;
      case EfiRuntimeServicesData:
      case EfiMemoryMappedIO:
      case EfiMemoryMappedIOPortSpace:
        MemoryMapEntry->Attribute |= EFI_MEMORY_XP;
        break;
      case EfiReservedMemoryType:
      case EfiACPIMemoryNVS:
        break;
    }

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }

  return;
}

/**
  This function for GetMemoryMap() with properties table capability.

  It calls original GetMemoryMap() to get the original memory map information. Then
  plus the additional memory map entries for PE Code/Data seperation.

  @param  MemoryMapSize          A pointer to the size, in bytes, of the
                                 MemoryMap buffer. On input, this is the size of
                                 the buffer allocated by the caller.  On output,
                                 it is the size of the buffer returned by the
                                 firmware  if the buffer was large enough, or the
                                 size of the buffer needed  to contain the map if
                                 the buffer was too small.
  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  MapKey                 A pointer to the location in which firmware
                                 returns the key for the current memory map.
  @param  DescriptorSize         A pointer to the location in which firmware
                                 returns the size, in bytes, of an individual
                                 EFI_MEMORY_DESCRIPTOR.
  @param  DescriptorVersion      A pointer to the location in which firmware
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
CoreGetMemoryMapWithSeparatedImageSection (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  OUT UINTN                     *MapKey,
  OUT UINTN                     *DescriptorSize,
  OUT UINT32                    *DescriptorVersion
  )
{
  EFI_STATUS  Status;
  UINTN       OldMemoryMapSize;
  UINTN       AdditionalRecordCount;

  //
  // If PE code/data is not aligned, just return.
  //
  if (!mMemoryAttributesTableEnable) {
    return CoreGetMemoryMap (MemoryMapSize, MemoryMap, MapKey, DescriptorSize, DescriptorVersion);
  }

  if (MemoryMapSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquiremMemoryAttributesTableLock ();

  AdditionalRecordCount = (2 * mImagePropertiesPrivateData.CodeSegmentCountMax + 3) * mImagePropertiesPrivateData.ImageRecordCount;

  OldMemoryMapSize = *MemoryMapSize;
  Status           = CoreGetMemoryMap (MemoryMapSize, MemoryMap, MapKey, DescriptorSize, DescriptorVersion);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    *MemoryMapSize = *MemoryMapSize + (*DescriptorSize) * AdditionalRecordCount;
  } else if (Status == EFI_SUCCESS) {
    ASSERT (MemoryMap != NULL);
    if (OldMemoryMapSize - *MemoryMapSize < (*DescriptorSize) * AdditionalRecordCount) {
      *MemoryMapSize = *MemoryMapSize + (*DescriptorSize) * AdditionalRecordCount;
      //
      // Need update status to buffer too small
      //
      Status = EFI_BUFFER_TOO_SMALL;
    } else {
      //
      // Split PE code/data
      //
      SplitTable (MemoryMapSize, MemoryMap, *DescriptorSize, &mImagePropertiesPrivateData.ImageRecordList, AdditionalRecordCount);

      //
      // Set RuntimeData to XP
      //
      EnforceMemoryMapAttribute (MemoryMap, *MemoryMapSize, *DescriptorSize);

      //
      // Merge same type to save entry size
      //
      MergeMemoryMap (MemoryMap, MemoryMapSize, *DescriptorSize);
    }
  }

  CoreReleasemMemoryAttributesTableLock ();
  return Status;
}

//
// Below functions are for ImageRecord
//

/**
  Set MemoryAttributesTable according to PE/COFF image section alignment.

  @param  SectionAlignment    PE/COFF section alignment
**/
STATIC
VOID
SetMemoryAttributesTableSectionAlignment (
  IN UINT32  SectionAlignment
  )
{
  if (((SectionAlignment & (RUNTIME_PAGE_ALLOCATION_GRANULARITY - 1)) != 0) &&
      mMemoryAttributesTableEnable)
  {
    DEBUG ((DEBUG_VERBOSE, "SetMemoryAttributesTableSectionAlignment - Clear\n"));
    mMemoryAttributesTableEnable = FALSE;
  }
}

/**
  Insert image record.

  @param  RuntimeImage    Runtime image information
**/
VOID
InsertImageRecord (
  IN EFI_RUNTIME_IMAGE_ENTRY  *RuntimeImage
  )
{
  VOID                                  *ImageAddress;
  EFI_IMAGE_DOS_HEADER                  *DosHdr;
  UINT32                                PeCoffHeaderOffset;
  UINT32                                SectionAlignment;
  EFI_IMAGE_SECTION_HEADER              *Section;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;
  UINT8                                 *Name;
  UINTN                                 Index;
  IMAGE_PROPERTIES_RECORD               *ImageRecord;
  CHAR8                                 *PdbPointer;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;

  DEBUG ((DEBUG_VERBOSE, "InsertImageRecord - 0x%x\n", RuntimeImage));
  DEBUG ((DEBUG_VERBOSE, "InsertImageRecord - 0x%016lx - 0x%016lx\n", (EFI_PHYSICAL_ADDRESS)(UINTN)RuntimeImage->ImageBase, RuntimeImage->ImageSize));

  if (mMemoryAttributesTableEndOfDxe) {
    DEBUG ((DEBUG_INFO, "Do not insert runtime image record after EndOfDxe\n"));
    return;
  }

  ImageRecord = AllocatePool (sizeof (*ImageRecord));
  if (ImageRecord == NULL) {
    return;
  }

  ImageRecord->Signature = IMAGE_PROPERTIES_RECORD_SIGNATURE;

  DEBUG ((DEBUG_VERBOSE, "ImageRecordCount - 0x%x\n", mImagePropertiesPrivateData.ImageRecordCount));

  //
  // Step 1: record whole region
  //
  ImageRecord->ImageBase = (EFI_PHYSICAL_ADDRESS)(UINTN)RuntimeImage->ImageBase;
  ImageRecord->ImageSize = RuntimeImage->ImageSize;

  ImageAddress = RuntimeImage->ImageBase;

  PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageAddress);
  if (PdbPointer != NULL) {
    DEBUG ((DEBUG_VERBOSE, "  Image - %a\n", PdbPointer));
  }

  //
  // Check PE/COFF image
  //
  DosHdr             = (EFI_IMAGE_DOS_HEADER *)(UINTN)ImageAddress;
  PeCoffHeaderOffset = 0;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    PeCoffHeaderOffset = DosHdr->e_lfanew;
  }

  Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINT8 *)(UINTN)ImageAddress + PeCoffHeaderOffset);
  if (Hdr.Pe32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    DEBUG ((DEBUG_VERBOSE, "Hdr.Pe32->Signature invalid - 0x%x\n", Hdr.Pe32->Signature));
    // It might be image in SMM.
    goto Finish;
  }

  //
  // Get SectionAlignment
  //
  if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    SectionAlignment = Hdr.Pe32->OptionalHeader.SectionAlignment;
  } else {
    SectionAlignment = Hdr.Pe32Plus->OptionalHeader.SectionAlignment;
  }

  SetMemoryAttributesTableSectionAlignment (SectionAlignment);
  if ((SectionAlignment & (RUNTIME_PAGE_ALLOCATION_GRANULARITY - 1)) != 0) {
    DEBUG ((
      DEBUG_WARN,
      "!!!!!!!!  InsertImageRecord - Section Alignment(0x%x) is not %dK  !!!!!!!!\n",
      SectionAlignment,
      RUNTIME_PAGE_ALLOCATION_GRANULARITY >> 10
      ));
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageAddress);
    if (PdbPointer != NULL) {
      DEBUG ((DEBUG_WARN, "!!!!!!!!  Image - %a  !!!!!!!!\n", PdbPointer));
    }

    goto Finish;
  }

  Section = (EFI_IMAGE_SECTION_HEADER *)(
                                         (UINT8 *)(UINTN)ImageAddress +
                                         PeCoffHeaderOffset +
                                         sizeof (UINT32) +
                                         sizeof (EFI_IMAGE_FILE_HEADER) +
                                         Hdr.Pe32->FileHeader.SizeOfOptionalHeader
                                         );
  ImageRecord->CodeSegmentCount = 0;
  InitializeListHead (&ImageRecord->CodeSegmentList);
  for (Index = 0; Index < Hdr.Pe32->FileHeader.NumberOfSections; Index++) {
    Name = Section[Index].Name;
    DEBUG ((
      DEBUG_VERBOSE,
      "  Section - '%c%c%c%c%c%c%c%c'\n",
      Name[0],
      Name[1],
      Name[2],
      Name[3],
      Name[4],
      Name[5],
      Name[6],
      Name[7]
      ));

    if ((Section[Index].Characteristics & EFI_IMAGE_SCN_CNT_CODE) != 0) {
      DEBUG ((DEBUG_VERBOSE, "  VirtualSize          - 0x%08x\n", Section[Index].Misc.VirtualSize));
      DEBUG ((DEBUG_VERBOSE, "  VirtualAddress       - 0x%08x\n", Section[Index].VirtualAddress));
      DEBUG ((DEBUG_VERBOSE, "  SizeOfRawData        - 0x%08x\n", Section[Index].SizeOfRawData));
      DEBUG ((DEBUG_VERBOSE, "  PointerToRawData     - 0x%08x\n", Section[Index].PointerToRawData));
      DEBUG ((DEBUG_VERBOSE, "  PointerToRelocations - 0x%08x\n", Section[Index].PointerToRelocations));
      DEBUG ((DEBUG_VERBOSE, "  PointerToLinenumbers - 0x%08x\n", Section[Index].PointerToLinenumbers));
      DEBUG ((DEBUG_VERBOSE, "  NumberOfRelocations  - 0x%08x\n", Section[Index].NumberOfRelocations));
      DEBUG ((DEBUG_VERBOSE, "  NumberOfLinenumbers  - 0x%08x\n", Section[Index].NumberOfLinenumbers));
      DEBUG ((DEBUG_VERBOSE, "  Characteristics      - 0x%08x\n", Section[Index].Characteristics));

      //
      // Step 2: record code section
      //
      ImageRecordCodeSection = AllocatePool (sizeof (*ImageRecordCodeSection));
      if (ImageRecordCodeSection == NULL) {
        return;
      }

      ImageRecordCodeSection->Signature = IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE;

      ImageRecordCodeSection->CodeSegmentBase = (UINTN)ImageAddress + Section[Index].VirtualAddress;
      ImageRecordCodeSection->CodeSegmentSize = Section[Index].SizeOfRawData;

      DEBUG ((DEBUG_VERBOSE, "ImageCode: 0x%016lx - 0x%016lx\n", ImageRecordCodeSection->CodeSegmentBase, ImageRecordCodeSection->CodeSegmentSize));

      InsertTailList (&ImageRecord->CodeSegmentList, &ImageRecordCodeSection->Link);
      ImageRecord->CodeSegmentCount++;
    }
  }

  if (ImageRecord->CodeSegmentCount == 0) {
    SetMemoryAttributesTableSectionAlignment (1);
    DEBUG ((DEBUG_ERROR, "!!!!!!!!  InsertImageRecord - CodeSegmentCount is 0  !!!!!!!!\n"));
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageAddress);
    if (PdbPointer != NULL) {
      DEBUG ((DEBUG_ERROR, "!!!!!!!!  Image - %a  !!!!!!!!\n", PdbPointer));
    }

    goto Finish;
  }

  //
  // Final
  //
  SortImageRecordCodeSection (ImageRecord);
  //
  // Check overlap all section in ImageBase/Size
  //
  if (!IsImageRecordCodeSectionValid (ImageRecord)) {
    DEBUG ((DEBUG_ERROR, "IsImageRecordCodeSectionValid - FAIL\n"));
    goto Finish;
  }

  InsertTailList (&mImagePropertiesPrivateData.ImageRecordList, &ImageRecord->Link);
  mImagePropertiesPrivateData.ImageRecordCount++;

  if (mImagePropertiesPrivateData.CodeSegmentCountMax < ImageRecord->CodeSegmentCount) {
    mImagePropertiesPrivateData.CodeSegmentCountMax = ImageRecord->CodeSegmentCount;
  }

  SortImageRecord (&mImagePropertiesPrivateData.ImageRecordList);

Finish:
  return;
}

/**
  Remove Image record.

  @param  RuntimeImage    Runtime image information
**/
VOID
RemoveImageRecord (
  IN EFI_RUNTIME_IMAGE_ENTRY  *RuntimeImage
  )
{
  IMAGE_PROPERTIES_RECORD               *ImageRecord;
  LIST_ENTRY                            *CodeSegmentListHead;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;

  DEBUG ((DEBUG_VERBOSE, "RemoveImageRecord - 0x%x\n", RuntimeImage));
  DEBUG ((DEBUG_VERBOSE, "RemoveImageRecord - 0x%016lx - 0x%016lx\n", (EFI_PHYSICAL_ADDRESS)(UINTN)RuntimeImage->ImageBase, RuntimeImage->ImageSize));

  if (mMemoryAttributesTableEndOfDxe) {
    DEBUG ((DEBUG_INFO, "Do not remove runtime image record after EndOfDxe\n"));
    return;
  }

  ImageRecord = FindImageRecord ((EFI_PHYSICAL_ADDRESS)(UINTN)RuntimeImage->ImageBase, RuntimeImage->ImageSize, &mImagePropertiesPrivateData.ImageRecordList);
  if (ImageRecord == NULL) {
    DEBUG ((DEBUG_ERROR, "!!!!!!!! ImageRecord not found !!!!!!!!\n"));
    return;
  }

  CodeSegmentListHead = &ImageRecord->CodeSegmentList;
  while (!IsListEmpty (CodeSegmentListHead)) {
    ImageRecordCodeSection = CR (
                               CodeSegmentListHead->ForwardLink,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                               Link,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                               );
    RemoveEntryList (&ImageRecordCodeSection->Link);
    FreePool (ImageRecordCodeSection);
  }

  RemoveEntryList (&ImageRecord->Link);
  FreePool (ImageRecord);
  mImagePropertiesPrivateData.ImageRecordCount--;
}
