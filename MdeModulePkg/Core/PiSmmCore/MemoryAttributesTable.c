/** @file
  PI SMM MemoryAttributes support

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/ImagePropertiesRecordLib.h>

#include <Library/PeCoffLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#include <Guid/PiSmmMemoryAttributesTable.h>

#include "PiSmmCore.h"

#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

#define IMAGE_PROPERTIES_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('I','P','P','D')

typedef struct {
  UINT32        Signature;
  UINTN         ImageRecordCount;
  UINTN         CodeSegmentCountMax;
  LIST_ENTRY    ImageRecordList;
} IMAGE_PROPERTIES_PRIVATE_DATA;

IMAGE_PROPERTIES_PRIVATE_DATA  mImagePropertiesPrivateData = {
  IMAGE_PROPERTIES_PRIVATE_DATA_SIGNATURE,
  0,
  0,
  INITIALIZE_LIST_HEAD_VARIABLE (mImagePropertiesPrivateData.ImageRecordList)
};

#define EFI_MEMORY_ATTRIBUTES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA  BIT0

UINT64  mMemoryProtectionAttribute = EFI_MEMORY_ATTRIBUTES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA;

//
// Below functions are for MemoryMap
//

/**
  Merge continuous memory map entries whose have same attributes.

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
      MemoryBlockLength = LShiftU64 (MemoryMapEntry->NumberOfPages, EFI_PAGE_SHIFT);
      if (((UINTN)NextMemoryMapEntry < (UINTN)MemoryMapEnd) &&
          (MemoryMapEntry->Type == NextMemoryMapEntry->Type) &&
          (MemoryMapEntry->Attribute == NextMemoryMapEntry->Attribute) &&
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
  Enforce memory map attributes.
  This function will set EfiRuntimeServicesData/EfiMemoryMappedIO/EfiMemoryMappedIOPortSpace to be EFI_MEMORY_XP.

  @param[in, out]  MemoryMap              A pointer to the buffer in which firmware places
                                          the current memory map.
  @param[in]       MemoryMapSize          Size, in bytes, of the MemoryMap buffer.
  @param[in]       DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
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
    if (MemoryMapEntry->Attribute != 0) {
      // It is PE image, the attribute is already set.
    } else {
      switch (MemoryMapEntry->Type) {
        case EfiRuntimeServicesCode:
          MemoryMapEntry->Attribute = EFI_MEMORY_RO;
          break;
        case EfiRuntimeServicesData:
        default:
          MemoryMapEntry->Attribute |= EFI_MEMORY_XP;
          break;
      }
    }

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }

  return;
}

/**
  This function for GetMemoryMap() with memory attributes table.

  It calls original GetMemoryMap() to get the original memory map information. Then
  plus the additional memory map entries for PE Code/Data separation.

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
STATIC
EFI_STATUS
EFIAPI
SmmCoreGetMemoryMapMemoryAttributesTable (
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
  if ((mMemoryProtectionAttribute & EFI_MEMORY_ATTRIBUTES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA) == 0) {
    return SmmCoreGetMemoryMap (MemoryMapSize, MemoryMap, MapKey, DescriptorSize, DescriptorVersion);
  }

  if (MemoryMapSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AdditionalRecordCount = (2 * mImagePropertiesPrivateData.CodeSegmentCountMax + 3) * mImagePropertiesPrivateData.ImageRecordCount;

  OldMemoryMapSize = *MemoryMapSize;
  Status           = SmmCoreGetMemoryMap (MemoryMapSize, MemoryMap, MapKey, DescriptorSize, DescriptorVersion);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    *MemoryMapSize = *MemoryMapSize + (*DescriptorSize) * AdditionalRecordCount;
  } else if (Status == EFI_SUCCESS) {
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
      ASSERT (MemoryMap != NULL);
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

  return Status;
}

//
// Below functions are for ImageRecord
//

/**
  Set MemoryProtectionAttribute according to PE/COFF image section alignment.

  @param[in]  SectionAlignment    PE/COFF section alignment
**/
STATIC
VOID
SetMemoryAttributesTableSectionAlignment (
  IN UINT32  SectionAlignment
  )
{
  if (((SectionAlignment & (RUNTIME_PAGE_ALLOCATION_GRANULARITY - 1)) != 0) &&
      ((mMemoryProtectionAttribute & EFI_MEMORY_ATTRIBUTES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA) != 0))
  {
    DEBUG ((DEBUG_VERBOSE, "SMM SetMemoryAttributesTableSectionAlignment - Clear\n"));
    mMemoryProtectionAttribute &= ~((UINT64)EFI_MEMORY_ATTRIBUTES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA);
  }
}

/**
  Insert image record.

  @param[in]  DriverEntry    Driver information
**/
VOID
SmmInsertImageRecord (
  IN EFI_SMM_DRIVER_ENTRY  *DriverEntry
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

  DEBUG ((DEBUG_VERBOSE, "SMM InsertImageRecord - 0x%x\n", DriverEntry));
  DEBUG ((DEBUG_VERBOSE, "SMM InsertImageRecord - 0x%016lx - 0x%08x\n", DriverEntry->ImageBuffer, DriverEntry->NumberOfPage));

  ImageRecord = AllocatePool (sizeof (*ImageRecord));
  if (ImageRecord == NULL) {
    return;
  }

  ImageRecord->Signature = IMAGE_PROPERTIES_RECORD_SIGNATURE;

  DEBUG ((DEBUG_VERBOSE, "SMM ImageRecordCount - 0x%x\n", mImagePropertiesPrivateData.ImageRecordCount));

  //
  // Step 1: record whole region
  //
  ImageRecord->ImageBase = DriverEntry->ImageBuffer;
  ImageRecord->ImageSize = LShiftU64 (DriverEntry->NumberOfPage, EFI_PAGE_SHIFT);

  ImageAddress = (VOID *)(UINTN)DriverEntry->ImageBuffer;

  PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageAddress);
  if (PdbPointer != NULL) {
    DEBUG ((DEBUG_VERBOSE, "SMM   Image - %a\n", PdbPointer));
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
    DEBUG ((DEBUG_VERBOSE, "SMM Hdr.Pe32->Signature invalid - 0x%x\n", Hdr.Pe32->Signature));
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
      "SMM !!!!!!!!  InsertImageRecord - Section Alignment(0x%x) is not %dK  !!!!!!!!\n",
      SectionAlignment,
      RUNTIME_PAGE_ALLOCATION_GRANULARITY >> 10
      ));
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageAddress);
    if (PdbPointer != NULL) {
      DEBUG ((DEBUG_WARN, "SMM !!!!!!!!  Image - %a  !!!!!!!!\n", PdbPointer));
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
      "SMM   Section - '%c%c%c%c%c%c%c%c'\n",
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
      DEBUG ((DEBUG_VERBOSE, "SMM   VirtualSize          - 0x%08x\n", Section[Index].Misc.VirtualSize));
      DEBUG ((DEBUG_VERBOSE, "SMM   VirtualAddress       - 0x%08x\n", Section[Index].VirtualAddress));
      DEBUG ((DEBUG_VERBOSE, "SMM   SizeOfRawData        - 0x%08x\n", Section[Index].SizeOfRawData));
      DEBUG ((DEBUG_VERBOSE, "SMM   PointerToRawData     - 0x%08x\n", Section[Index].PointerToRawData));
      DEBUG ((DEBUG_VERBOSE, "SMM   PointerToRelocations - 0x%08x\n", Section[Index].PointerToRelocations));
      DEBUG ((DEBUG_VERBOSE, "SMM   PointerToLinenumbers - 0x%08x\n", Section[Index].PointerToLinenumbers));
      DEBUG ((DEBUG_VERBOSE, "SMM   NumberOfRelocations  - 0x%08x\n", Section[Index].NumberOfRelocations));
      DEBUG ((DEBUG_VERBOSE, "SMM   NumberOfLinenumbers  - 0x%08x\n", Section[Index].NumberOfLinenumbers));
      DEBUG ((DEBUG_VERBOSE, "SMM   Characteristics      - 0x%08x\n", Section[Index].Characteristics));

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

      DEBUG ((DEBUG_VERBOSE, "SMM ImageCode: 0x%016lx - 0x%016lx\n", ImageRecordCodeSection->CodeSegmentBase, ImageRecordCodeSection->CodeSegmentSize));

      InsertTailList (&ImageRecord->CodeSegmentList, &ImageRecordCodeSection->Link);
      ImageRecord->CodeSegmentCount++;
    }
  }

  if (ImageRecord->CodeSegmentCount == 0) {
    SetMemoryAttributesTableSectionAlignment (1);
    DEBUG ((DEBUG_ERROR, "SMM !!!!!!!!  InsertImageRecord - CodeSegmentCount is 0  !!!!!!!!\n"));
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageAddress);
    if (PdbPointer != NULL) {
      DEBUG ((DEBUG_ERROR, "SMM !!!!!!!!  Image - %a  !!!!!!!!\n", PdbPointer));
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
    DEBUG ((DEBUG_ERROR, "SMM IsImageRecordCodeSectionValid - FAIL\n"));
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
  Publish MemoryAttributesTable to SMM configuration table.
**/
VOID
PublishMemoryAttributesTable (
  VOID
  )
{
  UINTN                                 MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR                 *MemoryMap;
  UINTN                                 MapKey;
  UINTN                                 DescriptorSize;
  UINT32                                DescriptorVersion;
  UINTN                                 Index;
  EFI_STATUS                            Status;
  UINTN                                 RuntimeEntryCount;
  EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE  *MemoryAttributesTable;
  EFI_MEMORY_DESCRIPTOR                 *MemoryAttributesEntry;
  UINTN                                 MemoryAttributesTableSize;

  MemoryMapSize = 0;
  MemoryMap     = NULL;
  Status        = SmmCoreGetMemoryMapMemoryAttributesTable (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  do {
    DEBUG ((DEBUG_VERBOSE, "MemoryMapSize - 0x%x\n", MemoryMapSize));
    MemoryMap = AllocatePool (MemoryMapSize);
    ASSERT (MemoryMap != NULL);
    DEBUG ((DEBUG_VERBOSE, "MemoryMap - 0x%x\n", MemoryMap));

    Status = SmmCoreGetMemoryMapMemoryAttributesTable (
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

  //
  // Allocate MemoryAttributesTable
  //
  RuntimeEntryCount         = MemoryMapSize/DescriptorSize;
  MemoryAttributesTableSize = sizeof (EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE) + DescriptorSize * RuntimeEntryCount;
  MemoryAttributesTable     = AllocatePool (sizeof (EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE) + DescriptorSize * RuntimeEntryCount);
  ASSERT (MemoryAttributesTable != NULL);
  MemoryAttributesTable->Version         = EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE_VERSION;
  MemoryAttributesTable->NumberOfEntries = (UINT32)RuntimeEntryCount;
  MemoryAttributesTable->DescriptorSize  = (UINT32)DescriptorSize;
  MemoryAttributesTable->Reserved        = 0;
  DEBUG ((DEBUG_VERBOSE, "MemoryAttributesTable:\n"));
  DEBUG ((DEBUG_VERBOSE, "  Version              - 0x%08x\n", MemoryAttributesTable->Version));
  DEBUG ((DEBUG_VERBOSE, "  NumberOfEntries      - 0x%08x\n", MemoryAttributesTable->NumberOfEntries));
  DEBUG ((DEBUG_VERBOSE, "  DescriptorSize       - 0x%08x\n", MemoryAttributesTable->DescriptorSize));
  MemoryAttributesEntry = (EFI_MEMORY_DESCRIPTOR *)(MemoryAttributesTable + 1);
  for (Index = 0; Index < MemoryMapSize/DescriptorSize; Index++) {
    CopyMem (MemoryAttributesEntry, MemoryMap, DescriptorSize);
    DEBUG ((DEBUG_VERBOSE, "Entry (0x%x)\n", MemoryAttributesEntry));
    DEBUG ((DEBUG_VERBOSE, "  Type              - 0x%x\n", MemoryAttributesEntry->Type));
    DEBUG ((DEBUG_VERBOSE, "  PhysicalStart     - 0x%016lx\n", MemoryAttributesEntry->PhysicalStart));
    DEBUG ((DEBUG_VERBOSE, "  VirtualStart      - 0x%016lx\n", MemoryAttributesEntry->VirtualStart));
    DEBUG ((DEBUG_VERBOSE, "  NumberOfPages     - 0x%016lx\n", MemoryAttributesEntry->NumberOfPages));
    DEBUG ((DEBUG_VERBOSE, "  Attribute         - 0x%016lx\n", MemoryAttributesEntry->Attribute));
    MemoryAttributesEntry = NEXT_MEMORY_DESCRIPTOR (MemoryAttributesEntry, DescriptorSize);

    MemoryMap = NEXT_MEMORY_DESCRIPTOR (MemoryMap, DescriptorSize);
  }

  Status = gSmst->SmmInstallConfigurationTable (gSmst, &gEdkiiPiSmmMemoryAttributesTableGuid, MemoryAttributesTable, MemoryAttributesTableSize);
  ASSERT_EFI_ERROR (Status);
}

/**
  This function installs all SMM image record information.
**/
VOID
SmmInstallImageRecord (
  VOID
  )
{
  EFI_STATUS                 Status;
  UINTN                      NoHandles;
  EFI_HANDLE                 *HandleBuffer;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  UINTN                      Index;
  EFI_SMM_DRIVER_ENTRY       DriverEntry;

  Status = SmmLocateHandleBuffer (
             ByProtocol,
             &gEfiLoadedImageProtocolGuid,
             NULL,
             &NoHandles,
             &HandleBuffer
             );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < NoHandles; Index++) {
    Status = gSmst->SmmHandleProtocol (
                      HandleBuffer[Index],
                      &gEfiLoadedImageProtocolGuid,
                      (VOID **)&LoadedImage
                      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    DEBUG ((DEBUG_VERBOSE, "LoadedImage - 0x%x 0x%x ", LoadedImage->ImageBase, LoadedImage->ImageSize));
    {
      VOID  *PdbPointer;
      PdbPointer = PeCoffLoaderGetPdbPointer (LoadedImage->ImageBase);
      if (PdbPointer != NULL) {
        DEBUG ((DEBUG_VERBOSE, "(%a) ", PdbPointer));
      }
    }
    DEBUG ((DEBUG_VERBOSE, "\n"));
    ZeroMem (&DriverEntry, sizeof (DriverEntry));
    DriverEntry.ImageBuffer  = (UINTN)LoadedImage->ImageBase;
    DriverEntry.NumberOfPage = EFI_SIZE_TO_PAGES ((UINTN)LoadedImage->ImageSize);
    SmmInsertImageRecord (&DriverEntry);
  }

  FreePool (HandleBuffer);
}

/**
  Install MemoryAttributesTable.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification runs successfully.
**/
EFI_STATUS
EFIAPI
SmmInstallMemoryAttributesTable (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  SmmInstallImageRecord ();

  DEBUG ((DEBUG_VERBOSE, "SMM MemoryProtectionAttribute - 0x%016lx\n", mMemoryProtectionAttribute));
  if ((mMemoryProtectionAttribute & EFI_MEMORY_ATTRIBUTES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA) == 0) {
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_VERBOSE, "SMM Total Image Count - 0x%x\n", mImagePropertiesPrivateData.ImageRecordCount));
  DEBUG ((DEBUG_VERBOSE, "SMM Dump ImageRecord:\n"));
  DumpImageRecord (&mImagePropertiesPrivateData.ImageRecordList);

  PublishMemoryAttributesTable ();

  return EFI_SUCCESS;
}

/**
  Initialize MemoryAttributesTable support.
**/
VOID
EFIAPI
SmmCoreInitializeMemoryAttributesTable (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmEndOfDxeProtocolGuid,
                    SmmInstallMemoryAttributesTable,
                    &Registration
                    );
  ASSERT_EFI_ERROR (Status);

  return;
}
