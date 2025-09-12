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
  InstallMemoryAttributesTable ();

  DEBUG_CODE_BEGIN ();
  if ( mImagePropertiesPrivateData.ImageRecordCount > 0) {
    DEBUG ((DEBUG_INFO, "DXE - Total Runtime Image Count: 0x%x\n", mImagePropertiesPrivateData.ImageRecordCount));
    DEBUG ((DEBUG_INFO, "DXE - Dump Runtime Image Records:\n"));
    DumpImageRecords (&mImagePropertiesPrivateData.ImageRecordList);
  }

  DEBUG_CODE_END ();
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
    CopyMem (NewMemoryMapEntry, MemoryMapEntry, DescriptorSize);
    NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);

    do {
      if ((UINTN)NextMemoryMapEntry < (UINTN)MemoryMapEnd) {
        MergeGuardPages (NewMemoryMapEntry, NextMemoryMapEntry->PhysicalStart);
      }

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
  This function will set EfiRuntimeServicesData to be EFI_MEMORY_XP.

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
    if ((MemoryMapEntry->Attribute & EFI_MEMORY_ACCESS_MASK) == 0) {
      switch (MemoryMapEntry->Type) {
        case EfiRuntimeServicesCode:
          // If at this point the attributes have not been set on an EfiRuntimeServicesCode
          // region, the memory range must not contain a loaded image. It's possible these
          // non-image EfiRuntimeServicesCode regions are part of the unused memory bucket.
          // It could also be that this region was explicitly allocated outside of the PE
          // loader but the UEFI spec requires that all EfiRuntimeServicesCode regions contain
          // EFI modules. In either case, set the attributes to RO and XP.
          MemoryMapEntry->Attribute |= (EFI_MEMORY_RO | EFI_MEMORY_XP);
          break;
        case EfiRuntimeServicesData:
          MemoryMapEntry->Attribute |= EFI_MEMORY_XP;
          break;
        default:
          break;
      }
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
  Insert image record.

  @param  RuntimeImage    Runtime image information
**/
VOID
InsertImageRecord (
  IN EFI_RUNTIME_IMAGE_ENTRY  *RuntimeImage
  )
{
  EFI_STATUS               Status;
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  CHAR8                    *PdbPointer;
  UINT32                   RequiredAlignment;

  DEBUG ((DEBUG_VERBOSE, "InsertImageRecord - 0x%x\n", RuntimeImage));

  ImageRecord = AllocatePool (sizeof (*ImageRecord));
  if (ImageRecord == NULL) {
    return;
  }

  InitializeListHead (&ImageRecord->Link);
  InitializeListHead (&ImageRecord->CodeSegmentList);

  PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)RuntimeImage->ImageBase);
  if (PdbPointer != NULL) {
    DEBUG ((DEBUG_VERBOSE, "  Image - %a\n", PdbPointer));
  }

  RequiredAlignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
  Status            = CreateImagePropertiesRecord (
                        RuntimeImage->ImageBase,
                        RuntimeImage->ImageSize,
                        &RequiredAlignment,
                        ImageRecord
                        );

  if (EFI_ERROR (Status)) {
    if (Status == EFI_ABORTED) {
      mMemoryAttributesTableEnable = FALSE;
    }

    Status = EFI_ABORTED;
    goto Finish;
  }

  if (ImageRecord->CodeSegmentCount == 0) {
    mMemoryAttributesTableEnable = FALSE;
    DEBUG ((DEBUG_ERROR, "!!!!!!!!  InsertImageRecord - CodeSegmentCount is 0  !!!!!!!!\n"));
    if (PdbPointer != NULL) {
      DEBUG ((DEBUG_ERROR, "!!!!!!!!  Image - %a  !!!!!!!!\n", PdbPointer));
    }

    Status = EFI_ABORTED;
    goto Finish;
  }

  //
  // Check overlap all section in ImageBase/Size
  //
  if (!IsImageRecordCodeSectionValid (ImageRecord)) {
    DEBUG ((DEBUG_ERROR, "IsImageRecordCodeSectionValid - FAIL\n"));
    Status = EFI_ABORTED;
    goto Finish;
  }

  InsertTailList (&mImagePropertiesPrivateData.ImageRecordList, &ImageRecord->Link);
  mImagePropertiesPrivateData.ImageRecordCount++;

  if (mImagePropertiesPrivateData.CodeSegmentCountMax < ImageRecord->CodeSegmentCount) {
    mImagePropertiesPrivateData.CodeSegmentCountMax = ImageRecord->CodeSegmentCount;
  }

  SortImageRecord (&mImagePropertiesPrivateData.ImageRecordList);

Finish:
  if (EFI_ERROR (Status) && (ImageRecord != NULL)) {
    DeleteImagePropertiesRecord (ImageRecord);
  }

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
  IMAGE_PROPERTIES_RECORD  *ImageRecord;

  DEBUG ((DEBUG_VERBOSE, "RemoveImageRecord - 0x%x\n", RuntimeImage));
  DEBUG ((DEBUG_VERBOSE, "RemoveImageRecord - 0x%016lx - 0x%016lx\n", (EFI_PHYSICAL_ADDRESS)(UINTN)RuntimeImage->ImageBase, RuntimeImage->ImageSize));

  ImageRecord = FindImageRecord ((EFI_PHYSICAL_ADDRESS)(UINTN)RuntimeImage->ImageBase, RuntimeImage->ImageSize, &mImagePropertiesPrivateData.ImageRecordList);
  if (ImageRecord == NULL) {
    DEBUG ((DEBUG_ERROR, "!!!!!!!! ImageRecord not found !!!!!!!!\n"));
    return;
  }

  DeleteImagePropertiesRecord (ImageRecord);

  mImagePropertiesPrivateData.ImageRecordCount--;
}
