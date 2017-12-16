/** @file
  PI SMM MemoryAttributes support

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <Library/PeCoffLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#include <Guid/PiSmmMemoryAttributesTable.h>

#include "PiSmmCore.h"

#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

#define IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE SIGNATURE_32 ('I','P','R','C')

typedef struct {
  UINT32                 Signature;
  LIST_ENTRY             Link;
  EFI_PHYSICAL_ADDRESS   CodeSegmentBase;
  UINT64                 CodeSegmentSize;
} IMAGE_PROPERTIES_RECORD_CODE_SECTION;

#define IMAGE_PROPERTIES_RECORD_SIGNATURE SIGNATURE_32 ('I','P','R','D')

typedef struct {
  UINT32                 Signature;
  LIST_ENTRY             Link;
  EFI_PHYSICAL_ADDRESS   ImageBase;
  UINT64                 ImageSize;
  UINTN                  CodeSegmentCount;
  LIST_ENTRY             CodeSegmentList;
} IMAGE_PROPERTIES_RECORD;

#define IMAGE_PROPERTIES_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ('I','P','P','D')

typedef struct {
  UINT32                 Signature;
  UINTN                  ImageRecordCount;
  UINTN                  CodeSegmentCountMax;
  LIST_ENTRY             ImageRecordList;
} IMAGE_PROPERTIES_PRIVATE_DATA;

IMAGE_PROPERTIES_PRIVATE_DATA  mImagePropertiesPrivateData = {
  IMAGE_PROPERTIES_PRIVATE_DATA_SIGNATURE,
  0,
  0,
  INITIALIZE_LIST_HEAD_VARIABLE (mImagePropertiesPrivateData.ImageRecordList)
};

#define EFI_MEMORY_ATTRIBUTES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA  BIT0

UINT64 mMemoryProtectionAttribute = EFI_MEMORY_ATTRIBUTES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA;

//
// Below functions are for MemoryMap
//

/**
  Converts a number of EFI_PAGEs to a size in bytes.

  NOTE: Do not use EFI_PAGES_TO_SIZE because it handles UINTN only.

  @param[in]  Pages     The number of EFI_PAGES.

  @return  The number of bytes associated with the number of EFI_PAGEs specified
           by Pages.
**/
STATIC
UINT64
EfiPagesToSize (
  IN UINT64 Pages
  )
{
  return LShiftU64 (Pages, EFI_PAGE_SHIFT);
}

/**
  Converts a size, in bytes, to a number of EFI_PAGESs.

  NOTE: Do not use EFI_SIZE_TO_PAGES because it handles UINTN only.

  @param[in]  Size      A size in bytes.

  @return  The number of EFI_PAGESs associated with the number of bytes specified
           by Size.

**/
STATIC
UINT64
EfiSizeToPages (
  IN UINT64 Size
  )
{
  return RShiftU64 (Size, EFI_PAGE_SHIFT) + ((((UINTN)Size) & EFI_PAGE_MASK) ? 1 : 0);
}

/**
  Check the consistency of Smm memory attributes table.

  @param[in] MemoryAttributesTable  PI SMM memory attributes table
**/
VOID
SmmMemoryAttributesTableConsistencyCheck (
  IN EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE *MemoryAttributesTable
  )
{
  EFI_MEMORY_DESCRIPTOR                     *MemoryMap;
  UINTN                                     MemoryMapEntryCount;
  UINTN                                     DescriptorSize;
  UINTN                                     Index;
  UINT64                                    Address;

  Address = 0;
  MemoryMapEntryCount = MemoryAttributesTable->NumberOfEntries;
  DescriptorSize = MemoryAttributesTable->DescriptorSize;
  MemoryMap = (EFI_MEMORY_DESCRIPTOR *)(MemoryAttributesTable + 1);
  for (Index = 0; Index < MemoryMapEntryCount; Index++) {
    if (Address != 0) {
      ASSERT (Address == MemoryMap->PhysicalStart);
    }
    Address = MemoryMap->PhysicalStart + EfiPagesToSize(MemoryMap->NumberOfPages);
    MemoryMap = NEXT_MEMORY_DESCRIPTOR(MemoryMap, DescriptorSize);
  }
}

/**
  Sort memory map entries based upon PhysicalStart, from low to high.

  @param[in,out]  MemoryMap         A pointer to the buffer in which firmware places
                                    the current memory map.
  @param[in]      MemoryMapSize     Size, in bytes, of the MemoryMap buffer.
  @param[in]      DescriptorSize    Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
STATIC
VOID
SortMemoryMap (
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      MemoryMapSize,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR       *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR       *NextMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR       *MemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR       TempMemoryMap;

  MemoryMapEntry = MemoryMap;
  NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  MemoryMapEnd = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) MemoryMap + MemoryMapSize);
  while (MemoryMapEntry < MemoryMapEnd) {
    while (NextMemoryMapEntry < MemoryMapEnd) {
      if (MemoryMapEntry->PhysicalStart > NextMemoryMapEntry->PhysicalStart) {
        CopyMem (&TempMemoryMap, MemoryMapEntry, sizeof(EFI_MEMORY_DESCRIPTOR));
        CopyMem (MemoryMapEntry, NextMemoryMapEntry, sizeof(EFI_MEMORY_DESCRIPTOR));
        CopyMem (NextMemoryMapEntry, &TempMemoryMap, sizeof(EFI_MEMORY_DESCRIPTOR));
      }

      NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NextMemoryMapEntry, DescriptorSize);
    }

    MemoryMapEntry      = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    NextMemoryMapEntry  = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }

  return ;
}

/**
  Merge continous memory map entries whose have same attributes.

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
  EFI_MEMORY_DESCRIPTOR       *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR       *MemoryMapEnd;
  UINT64                      MemoryBlockLength;
  EFI_MEMORY_DESCRIPTOR       *NewMemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR       *NextMemoryMapEntry;

  MemoryMapEntry = MemoryMap;
  NewMemoryMapEntry = MemoryMap;
  MemoryMapEnd = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) MemoryMap + *MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    CopyMem (NewMemoryMapEntry, MemoryMapEntry, sizeof(EFI_MEMORY_DESCRIPTOR));
    NextMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);

    do {
      MemoryBlockLength = (UINT64) (EfiPagesToSize (MemoryMapEntry->NumberOfPages));
      if (((UINTN)NextMemoryMapEntry < (UINTN)MemoryMapEnd) &&
          (MemoryMapEntry->Type == NextMemoryMapEntry->Type) &&
          (MemoryMapEntry->Attribute == NextMemoryMapEntry->Attribute) &&
          ((MemoryMapEntry->PhysicalStart + MemoryBlockLength) == NextMemoryMapEntry->PhysicalStart)) {
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

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
    NewMemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (NewMemoryMapEntry, DescriptorSize);
  }

  *MemoryMapSize = (UINTN)NewMemoryMapEntry - (UINTN)MemoryMap;

  return ;
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
  EFI_MEMORY_DESCRIPTOR       *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR       *MemoryMapEnd;

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) MemoryMap + MemoryMapSize);
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

  return ;
}

/**
  Return the first image record, whose [ImageBase, ImageSize] covered by [Buffer, Length].

  @param[in] Buffer  Start Address
  @param[in] Length  Address length

  @return first image record covered by [buffer, length]
**/
STATIC
IMAGE_PROPERTIES_RECORD *
GetImageRecordByAddress (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  IMAGE_PROPERTIES_RECORD    *ImageRecord;
  LIST_ENTRY                 *ImageRecordLink;
  LIST_ENTRY                 *ImageRecordList;

  ImageRecordList = &mImagePropertiesPrivateData.ImageRecordList;

  for (ImageRecordLink = ImageRecordList->ForwardLink;
       ImageRecordLink != ImageRecordList;
       ImageRecordLink = ImageRecordLink->ForwardLink) {
    ImageRecord = CR (
                    ImageRecordLink,
                    IMAGE_PROPERTIES_RECORD,
                    Link,
                    IMAGE_PROPERTIES_RECORD_SIGNATURE
                    );

    if ((Buffer <= ImageRecord->ImageBase) &&
        (Buffer + Length >= ImageRecord->ImageBase + ImageRecord->ImageSize)) {
      return ImageRecord;
    }
  }

  return NULL;
}

/**
  Set the memory map to new entries, according to one old entry,
  based upon PE code section and data section in image record

  @param[in]       ImageRecord            An image record whose [ImageBase, ImageSize] covered
                                          by old memory map entry.
  @param[in, out]  NewRecord              A pointer to several new memory map entries.
                                          The caller gurantee the buffer size be 1 +
                                          (SplitRecordCount * DescriptorSize) calculated
                                          below.
  @param[in]       OldRecord              A pointer to one old memory map entry.
  @param[in]       DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
STATIC
UINTN
SetNewRecord (
  IN IMAGE_PROPERTIES_RECORD       *ImageRecord,
  IN OUT EFI_MEMORY_DESCRIPTOR     *NewRecord,
  IN EFI_MEMORY_DESCRIPTOR         *OldRecord,
  IN UINTN                         DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR                     TempRecord;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION      *ImageRecordCodeSection;
  LIST_ENTRY                                *ImageRecordCodeSectionLink;
  LIST_ENTRY                                *ImageRecordCodeSectionEndLink;
  LIST_ENTRY                                *ImageRecordCodeSectionList;
  UINTN                                     NewRecordCount;
  UINT64                                    PhysicalEnd;
  UINT64                                    ImageEnd;

  CopyMem (&TempRecord, OldRecord, sizeof(EFI_MEMORY_DESCRIPTOR));
  PhysicalEnd = TempRecord.PhysicalStart + EfiPagesToSize(TempRecord.NumberOfPages);
  NewRecordCount = 0;

  //
  // Always create a new entry for non-PE image record
  //
  if (ImageRecord->ImageBase > TempRecord.PhysicalStart) {
    NewRecord->Type = TempRecord.Type;
    NewRecord->PhysicalStart = TempRecord.PhysicalStart;
    NewRecord->VirtualStart  = 0;
    NewRecord->NumberOfPages = EfiSizeToPages(ImageRecord->ImageBase - TempRecord.PhysicalStart);
    NewRecord->Attribute     = TempRecord.Attribute;
    NewRecord = NEXT_MEMORY_DESCRIPTOR (NewRecord, DescriptorSize);
    NewRecordCount ++;
    TempRecord.PhysicalStart = ImageRecord->ImageBase;
    TempRecord.NumberOfPages = EfiSizeToPages(PhysicalEnd - TempRecord.PhysicalStart);
  }

  ImageRecordCodeSectionList = &ImageRecord->CodeSegmentList;

  ImageRecordCodeSectionLink = ImageRecordCodeSectionList->ForwardLink;
  ImageRecordCodeSectionEndLink = ImageRecordCodeSectionList;
  while (ImageRecordCodeSectionLink != ImageRecordCodeSectionEndLink) {
    ImageRecordCodeSection = CR (
                               ImageRecordCodeSectionLink,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                               Link,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                               );
    ImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink;

    if (TempRecord.PhysicalStart <= ImageRecordCodeSection->CodeSegmentBase) {
      //
      // DATA
      //
      NewRecord->Type = EfiRuntimeServicesData;
      NewRecord->PhysicalStart = TempRecord.PhysicalStart;
      NewRecord->VirtualStart  = 0;
      NewRecord->NumberOfPages = EfiSizeToPages(ImageRecordCodeSection->CodeSegmentBase - NewRecord->PhysicalStart);
      NewRecord->Attribute     = TempRecord.Attribute | EFI_MEMORY_XP;
      if (NewRecord->NumberOfPages != 0) {
        NewRecord = NEXT_MEMORY_DESCRIPTOR (NewRecord, DescriptorSize);
        NewRecordCount ++;
      }

      //
      // CODE
      //
      NewRecord->Type = EfiRuntimeServicesCode;
      NewRecord->PhysicalStart = ImageRecordCodeSection->CodeSegmentBase;
      NewRecord->VirtualStart  = 0;
      NewRecord->NumberOfPages = EfiSizeToPages(ImageRecordCodeSection->CodeSegmentSize);
      NewRecord->Attribute     = (TempRecord.Attribute & (~EFI_MEMORY_XP)) | EFI_MEMORY_RO;
      if (NewRecord->NumberOfPages != 0) {
        NewRecord = NEXT_MEMORY_DESCRIPTOR (NewRecord, DescriptorSize);
        NewRecordCount ++;
      }

      TempRecord.PhysicalStart = ImageRecordCodeSection->CodeSegmentBase + EfiPagesToSize (EfiSizeToPages(ImageRecordCodeSection->CodeSegmentSize));
      TempRecord.NumberOfPages = EfiSizeToPages(PhysicalEnd - TempRecord.PhysicalStart);
      if (TempRecord.NumberOfPages == 0) {
        break;
      }
    }
  }

  ImageEnd = ImageRecord->ImageBase + ImageRecord->ImageSize;

  //
  // Final DATA
  //
  if (TempRecord.PhysicalStart < ImageEnd) {
    NewRecord->Type = EfiRuntimeServicesData;
    NewRecord->PhysicalStart = TempRecord.PhysicalStart;
    NewRecord->VirtualStart  = 0;
    NewRecord->NumberOfPages = EfiSizeToPages (ImageEnd - TempRecord.PhysicalStart);
    NewRecord->Attribute     = TempRecord.Attribute | EFI_MEMORY_XP;
    NewRecordCount ++;
  }

  return NewRecordCount;
}

/**
  Return the max number of new splitted entries, according to one old entry,
  based upon PE code section and data section.

  @param[in]  OldRecord              A pointer to one old memory map entry.

  @retval  0 no entry need to be splitted.
  @return  the max number of new splitted entries
**/
STATIC
UINTN
GetMaxSplitRecordCount (
  IN EFI_MEMORY_DESCRIPTOR *OldRecord
  )
{
  IMAGE_PROPERTIES_RECORD *ImageRecord;
  UINTN                   SplitRecordCount;
  UINT64                  PhysicalStart;
  UINT64                  PhysicalEnd;

  SplitRecordCount = 0;
  PhysicalStart = OldRecord->PhysicalStart;
  PhysicalEnd = OldRecord->PhysicalStart + EfiPagesToSize(OldRecord->NumberOfPages);

  do {
    ImageRecord = GetImageRecordByAddress (PhysicalStart, PhysicalEnd - PhysicalStart);
    if (ImageRecord == NULL) {
      break;
    }
    SplitRecordCount += (2 * ImageRecord->CodeSegmentCount + 2);
    PhysicalStart = ImageRecord->ImageBase + ImageRecord->ImageSize;
  } while ((ImageRecord != NULL) && (PhysicalStart < PhysicalEnd));

  return SplitRecordCount;
}

/**
  Split the memory map to new entries, according to one old entry,
  based upon PE code section and data section.

  @param[in]       OldRecord              A pointer to one old memory map entry.
  @param[in, out]  NewRecord              A pointer to several new memory map entries.
                                          The caller gurantee the buffer size be 1 +
                                          (SplitRecordCount * DescriptorSize) calculated
                                          below.
  @param[in]       MaxSplitRecordCount    The max number of splitted entries
  @param[in]       DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.

  @retval  0 no entry is splitted.
  @return  the real number of splitted record.
**/
STATIC
UINTN
SplitRecord (
  IN EFI_MEMORY_DESCRIPTOR     *OldRecord,
  IN OUT EFI_MEMORY_DESCRIPTOR *NewRecord,
  IN UINTN                     MaxSplitRecordCount,
  IN UINTN                     DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR   TempRecord;
  IMAGE_PROPERTIES_RECORD *ImageRecord;
  IMAGE_PROPERTIES_RECORD *NewImageRecord;
  UINT64                  PhysicalStart;
  UINT64                  PhysicalEnd;
  UINTN                   NewRecordCount;
  UINTN                   TotalNewRecordCount;

  if (MaxSplitRecordCount == 0) {
    CopyMem (NewRecord, OldRecord, DescriptorSize);
    return 0;
  }

  TotalNewRecordCount = 0;

  //
  // Override previous record
  //
  CopyMem (&TempRecord, OldRecord, sizeof(EFI_MEMORY_DESCRIPTOR));
  PhysicalStart = TempRecord.PhysicalStart;
  PhysicalEnd = TempRecord.PhysicalStart + EfiPagesToSize(TempRecord.NumberOfPages);

  ImageRecord = NULL;
  do {
    NewImageRecord = GetImageRecordByAddress (PhysicalStart, PhysicalEnd - PhysicalStart);
    if (NewImageRecord == NULL) {
      //
      // No more image covered by this range, stop
      //
      if (PhysicalEnd > PhysicalStart) {
        //
        // Always create a new entry for non-PE image record
        //
        NewRecord->Type = TempRecord.Type;
        NewRecord->PhysicalStart = TempRecord.PhysicalStart;
        NewRecord->VirtualStart  = 0;
        NewRecord->NumberOfPages = TempRecord.NumberOfPages;
        NewRecord->Attribute     = TempRecord.Attribute;
        TotalNewRecordCount ++;
      }
      break;
    }
    ImageRecord = NewImageRecord;

    //
    // Set new record
    //
    NewRecordCount = SetNewRecord (ImageRecord, NewRecord, &TempRecord, DescriptorSize);
    TotalNewRecordCount += NewRecordCount;
    NewRecord = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)NewRecord + NewRecordCount * DescriptorSize);

    //
    // Update PhysicalStart, in order to exclude the image buffer already splitted.
    //
    PhysicalStart = ImageRecord->ImageBase + ImageRecord->ImageSize;
    TempRecord.PhysicalStart = PhysicalStart;
    TempRecord.NumberOfPages = EfiSizeToPages (PhysicalEnd - PhysicalStart);
  } while ((ImageRecord != NULL) && (PhysicalStart < PhysicalEnd));

  return TotalNewRecordCount - 1;
}

/**
  Split the original memory map, and add more entries to describe PE code section and data section.
  This function will set EfiRuntimeServicesData to be EFI_MEMORY_XP.
  This function will merge entries with same attributes finally.

  NOTE: It assumes PE code/data section are page aligned.
  NOTE: It assumes enough entry is prepared for new memory map.

  Split table:
   +---------------+
   | Record X      |
   +---------------+
   | Record RtCode |
   +---------------+
   | Record Y      |
   +---------------+
   ==>
   +---------------+
   | Record X      |
   +---------------+
   | Record RtCode |
   +---------------+ ----
   | Record RtData |     |
   +---------------+     |
   | Record RtCode |     |-> PE/COFF1
   +---------------+     |
   | Record RtData |     |
   +---------------+ ----
   | Record RtCode |
   +---------------+ ----
   | Record RtData |     |
   +---------------+     |
   | Record RtCode |     |-> PE/COFF2
   +---------------+     |
   | Record RtData |     |
   +---------------+ ----
   | Record RtCode |
   +---------------+
   | Record Y      |
   +---------------+

  @param[in, out]  MemoryMapSize          A pointer to the size, in bytes, of the
                                          MemoryMap buffer. On input, this is the size of
                                          old MemoryMap before split. The actual buffer
                                          size of MemoryMap is MemoryMapSize +
                                          (AdditionalRecordCount * DescriptorSize) calculated
                                          below. On output, it is the size of new MemoryMap
                                          after split.
  @param[in, out]  MemoryMap              A pointer to the buffer in which firmware places
                                          the current memory map.
  @param[in]       DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
STATIC
VOID
SplitTable (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                      DescriptorSize
  )
{
  INTN        IndexOld;
  INTN        IndexNew;
  UINTN       MaxSplitRecordCount;
  UINTN       RealSplitRecordCount;
  UINTN       TotalSplitRecordCount;
  UINTN       AdditionalRecordCount;

  AdditionalRecordCount = (2 * mImagePropertiesPrivateData.CodeSegmentCountMax + 2) * mImagePropertiesPrivateData.ImageRecordCount;

  TotalSplitRecordCount = 0;
  //
  // Let old record point to end of valid MemoryMap buffer.
  //
  IndexOld = ((*MemoryMapSize) / DescriptorSize) - 1;
  //
  // Let new record point to end of full MemoryMap buffer.
  //
  IndexNew = ((*MemoryMapSize) / DescriptorSize) - 1 + AdditionalRecordCount;
  for (; IndexOld >= 0; IndexOld--) {
    MaxSplitRecordCount = GetMaxSplitRecordCount ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + IndexOld * DescriptorSize));
    //
    // Split this MemoryMap record
    //
    IndexNew -= MaxSplitRecordCount;
    RealSplitRecordCount = SplitRecord (
                             (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + IndexOld * DescriptorSize),
                             (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + IndexNew * DescriptorSize),
                             MaxSplitRecordCount,
                             DescriptorSize
                             );
    //
    // Adjust IndexNew according to real split.
    //
    if (MaxSplitRecordCount != RealSplitRecordCount) {
      CopyMem (
        ((UINT8 *)MemoryMap + (IndexNew + MaxSplitRecordCount - RealSplitRecordCount) * DescriptorSize),
        ((UINT8 *)MemoryMap + IndexNew * DescriptorSize),
        (RealSplitRecordCount + 1) * DescriptorSize
        );
    }
    IndexNew = IndexNew + MaxSplitRecordCount - RealSplitRecordCount;
    TotalSplitRecordCount += RealSplitRecordCount;
    IndexNew --;
  }
  //
  // Move all records to the beginning.
  //
  CopyMem (
    MemoryMap,
    (UINT8 *)MemoryMap + (AdditionalRecordCount - TotalSplitRecordCount) * DescriptorSize,
    (*MemoryMapSize) + TotalSplitRecordCount * DescriptorSize
    );

  *MemoryMapSize = (*MemoryMapSize) + DescriptorSize * TotalSplitRecordCount;

  //
  // Sort from low to high (Just in case)
  //
  SortMemoryMap (MemoryMap, *MemoryMapSize, DescriptorSize);

  //
  // Set RuntimeData to XP
  //
  EnforceMemoryMapAttribute (MemoryMap, *MemoryMapSize, DescriptorSize);

  //
  // Merge same type to save entry size
  //
  MergeMemoryMap (MemoryMap, MemoryMapSize, DescriptorSize);

  return ;
}

/**
  This function for GetMemoryMap() with memory attributes table.

  It calls original GetMemoryMap() to get the original memory map information. Then
  plus the additional memory map entries for PE Code/Data seperation.

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

  AdditionalRecordCount = (2 * mImagePropertiesPrivateData.CodeSegmentCountMax + 2) * mImagePropertiesPrivateData.ImageRecordCount;

  OldMemoryMapSize = *MemoryMapSize;
  Status = SmmCoreGetMemoryMap (MemoryMapSize, MemoryMap, MapKey, DescriptorSize, DescriptorVersion);
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
      ASSERT(MemoryMap != NULL);
      SplitTable (MemoryMapSize, MemoryMap, *DescriptorSize);
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
      ((mMemoryProtectionAttribute & EFI_MEMORY_ATTRIBUTES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA) != 0)) {
    DEBUG ((DEBUG_VERBOSE, "SMM SetMemoryAttributesTableSectionAlignment - Clear\n"));
    mMemoryProtectionAttribute &= ~((UINT64)EFI_MEMORY_ATTRIBUTES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA);
  }
}

/**
  Swap two code sections in image record.

  @param[in]  FirstImageRecordCodeSection    first code section in image record
  @param[in]  SecondImageRecordCodeSection   second code section in image record
**/
STATIC
VOID
SwapImageRecordCodeSection (
  IN IMAGE_PROPERTIES_RECORD_CODE_SECTION      *FirstImageRecordCodeSection,
  IN IMAGE_PROPERTIES_RECORD_CODE_SECTION      *SecondImageRecordCodeSection
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION      TempImageRecordCodeSection;

  TempImageRecordCodeSection.CodeSegmentBase = FirstImageRecordCodeSection->CodeSegmentBase;
  TempImageRecordCodeSection.CodeSegmentSize = FirstImageRecordCodeSection->CodeSegmentSize;

  FirstImageRecordCodeSection->CodeSegmentBase = SecondImageRecordCodeSection->CodeSegmentBase;
  FirstImageRecordCodeSection->CodeSegmentSize = SecondImageRecordCodeSection->CodeSegmentSize;

  SecondImageRecordCodeSection->CodeSegmentBase = TempImageRecordCodeSection.CodeSegmentBase;
  SecondImageRecordCodeSection->CodeSegmentSize = TempImageRecordCodeSection.CodeSegmentSize;
}

/**
  Sort code section in image record, based upon CodeSegmentBase from low to high.

  @param[in]  ImageRecord    image record to be sorted
**/
STATIC
VOID
SortImageRecordCodeSection (
  IN IMAGE_PROPERTIES_RECORD              *ImageRecord
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION      *ImageRecordCodeSection;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION      *NextImageRecordCodeSection;
  LIST_ENTRY                                *ImageRecordCodeSectionLink;
  LIST_ENTRY                                *NextImageRecordCodeSectionLink;
  LIST_ENTRY                                *ImageRecordCodeSectionEndLink;
  LIST_ENTRY                                *ImageRecordCodeSectionList;

  ImageRecordCodeSectionList = &ImageRecord->CodeSegmentList;

  ImageRecordCodeSectionLink = ImageRecordCodeSectionList->ForwardLink;
  NextImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink;
  ImageRecordCodeSectionEndLink = ImageRecordCodeSectionList;
  while (ImageRecordCodeSectionLink != ImageRecordCodeSectionEndLink) {
    ImageRecordCodeSection = CR (
                               ImageRecordCodeSectionLink,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                               Link,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                               );
    while (NextImageRecordCodeSectionLink != ImageRecordCodeSectionEndLink) {
      NextImageRecordCodeSection = CR (
                                     NextImageRecordCodeSectionLink,
                                     IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                                     Link,
                                     IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                                     );
      if (ImageRecordCodeSection->CodeSegmentBase > NextImageRecordCodeSection->CodeSegmentBase) {
        SwapImageRecordCodeSection (ImageRecordCodeSection, NextImageRecordCodeSection);
      }
      NextImageRecordCodeSectionLink = NextImageRecordCodeSectionLink->ForwardLink;
    }

    ImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink;
    NextImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink;
  }
}

/**
  Check if code section in image record is valid.

  @param[in]  ImageRecord    image record to be checked

  @retval TRUE  image record is valid
  @retval FALSE image record is invalid
**/
STATIC
BOOLEAN
IsImageRecordCodeSectionValid (
  IN IMAGE_PROPERTIES_RECORD              *ImageRecord
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION      *ImageRecordCodeSection;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION      *LastImageRecordCodeSection;
  LIST_ENTRY                                *ImageRecordCodeSectionLink;
  LIST_ENTRY                                *ImageRecordCodeSectionEndLink;
  LIST_ENTRY                                *ImageRecordCodeSectionList;

  DEBUG ((DEBUG_VERBOSE, "SMM ImageCode SegmentCount - 0x%x\n", ImageRecord->CodeSegmentCount));

  ImageRecordCodeSectionList = &ImageRecord->CodeSegmentList;

  ImageRecordCodeSectionLink = ImageRecordCodeSectionList->ForwardLink;
  ImageRecordCodeSectionEndLink = ImageRecordCodeSectionList;
  LastImageRecordCodeSection = NULL;
  while (ImageRecordCodeSectionLink != ImageRecordCodeSectionEndLink) {
    ImageRecordCodeSection = CR (
                               ImageRecordCodeSectionLink,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                               Link,
                               IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                               );
    if (ImageRecordCodeSection->CodeSegmentSize == 0) {
      return FALSE;
    }
    if (ImageRecordCodeSection->CodeSegmentBase < ImageRecord->ImageBase) {
      return FALSE;
    }
    if (ImageRecordCodeSection->CodeSegmentBase >= MAX_ADDRESS - ImageRecordCodeSection->CodeSegmentSize) {
      return FALSE;
    }
    if ((ImageRecordCodeSection->CodeSegmentBase + ImageRecordCodeSection->CodeSegmentSize) > (ImageRecord->ImageBase + ImageRecord->ImageSize)) {
      return FALSE;
    }
    if (LastImageRecordCodeSection != NULL) {
      if ((LastImageRecordCodeSection->CodeSegmentBase + LastImageRecordCodeSection->CodeSegmentSize) > ImageRecordCodeSection->CodeSegmentBase) {
        return FALSE;
      }
    }

    LastImageRecordCodeSection = ImageRecordCodeSection;
    ImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink;
  }

  return TRUE;
}

/**
  Swap two image records.

  @param[in]  FirstImageRecord   first image record.
  @param[in]  SecondImageRecord  second image record.
**/
STATIC
VOID
SwapImageRecord (
  IN IMAGE_PROPERTIES_RECORD      *FirstImageRecord,
  IN IMAGE_PROPERTIES_RECORD      *SecondImageRecord
  )
{
  IMAGE_PROPERTIES_RECORD      TempImageRecord;

  TempImageRecord.ImageBase = FirstImageRecord->ImageBase;
  TempImageRecord.ImageSize = FirstImageRecord->ImageSize;
  TempImageRecord.CodeSegmentCount = FirstImageRecord->CodeSegmentCount;

  FirstImageRecord->ImageBase = SecondImageRecord->ImageBase;
  FirstImageRecord->ImageSize = SecondImageRecord->ImageSize;
  FirstImageRecord->CodeSegmentCount = SecondImageRecord->CodeSegmentCount;

  SecondImageRecord->ImageBase = TempImageRecord.ImageBase;
  SecondImageRecord->ImageSize = TempImageRecord.ImageSize;
  SecondImageRecord->CodeSegmentCount = TempImageRecord.CodeSegmentCount;

  SwapListEntries (&FirstImageRecord->CodeSegmentList, &SecondImageRecord->CodeSegmentList);
}

/**
  Sort image record based upon the ImageBase from low to high.
**/
STATIC
VOID
SortImageRecord (
  VOID
  )
{
  IMAGE_PROPERTIES_RECORD      *ImageRecord;
  IMAGE_PROPERTIES_RECORD      *NextImageRecord;
  LIST_ENTRY                   *ImageRecordLink;
  LIST_ENTRY                   *NextImageRecordLink;
  LIST_ENTRY                   *ImageRecordEndLink;
  LIST_ENTRY                   *ImageRecordList;

  ImageRecordList = &mImagePropertiesPrivateData.ImageRecordList;

  ImageRecordLink = ImageRecordList->ForwardLink;
  NextImageRecordLink = ImageRecordLink->ForwardLink;
  ImageRecordEndLink = ImageRecordList;
  while (ImageRecordLink != ImageRecordEndLink) {
    ImageRecord = CR (
                    ImageRecordLink,
                    IMAGE_PROPERTIES_RECORD,
                    Link,
                    IMAGE_PROPERTIES_RECORD_SIGNATURE
                    );
    while (NextImageRecordLink != ImageRecordEndLink) {
      NextImageRecord = CR (
                          NextImageRecordLink,
                          IMAGE_PROPERTIES_RECORD,
                          Link,
                          IMAGE_PROPERTIES_RECORD_SIGNATURE
                          );
      if (ImageRecord->ImageBase > NextImageRecord->ImageBase) {
        SwapImageRecord (ImageRecord, NextImageRecord);
      }
      NextImageRecordLink = NextImageRecordLink->ForwardLink;
    }

    ImageRecordLink = ImageRecordLink->ForwardLink;
    NextImageRecordLink = ImageRecordLink->ForwardLink;
  }
}

/**
  Dump image record.
**/
STATIC
VOID
DumpImageRecord (
  VOID
  )
{
  IMAGE_PROPERTIES_RECORD      *ImageRecord;
  LIST_ENTRY                   *ImageRecordLink;
  LIST_ENTRY                   *ImageRecordList;
  UINTN                        Index;

  ImageRecordList = &mImagePropertiesPrivateData.ImageRecordList;

  for (ImageRecordLink = ImageRecordList->ForwardLink, Index= 0;
       ImageRecordLink != ImageRecordList;
       ImageRecordLink = ImageRecordLink->ForwardLink, Index++) {
    ImageRecord = CR (
                    ImageRecordLink,
                    IMAGE_PROPERTIES_RECORD,
                    Link,
                    IMAGE_PROPERTIES_RECORD_SIGNATURE
                    );
    DEBUG ((DEBUG_VERBOSE, "SMM  Image[%d]: 0x%016lx - 0x%016lx\n", Index, ImageRecord->ImageBase, ImageRecord->ImageSize));
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
  VOID                                 *ImageAddress;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  UINT32                               PeCoffHeaderOffset;
  UINT32                               SectionAlignment;
  EFI_IMAGE_SECTION_HEADER             *Section;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  UINT8                                *Name;
  UINTN                                Index;
  IMAGE_PROPERTIES_RECORD              *ImageRecord;
  CHAR8                                *PdbPointer;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION *ImageRecordCodeSection;
  UINT16                               Magic;

  DEBUG ((DEBUG_VERBOSE, "SMM InsertImageRecord - 0x%x\n", DriverEntry));
  DEBUG ((DEBUG_VERBOSE, "SMM InsertImageRecord - 0x%016lx - 0x%08x\n", DriverEntry->ImageBuffer, DriverEntry->NumberOfPage));

  ImageRecord = AllocatePool (sizeof(*ImageRecord));
  if (ImageRecord == NULL) {
    return ;
  }
  ImageRecord->Signature = IMAGE_PROPERTIES_RECORD_SIGNATURE;

  DEBUG ((DEBUG_VERBOSE, "SMM ImageRecordCount - 0x%x\n", mImagePropertiesPrivateData.ImageRecordCount));

  //
  // Step 1: record whole region
  //
  ImageRecord->ImageBase = DriverEntry->ImageBuffer;
  ImageRecord->ImageSize = EfiPagesToSize(DriverEntry->NumberOfPage);

  ImageAddress = (VOID *)(UINTN)DriverEntry->ImageBuffer;

  PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageAddress);
  if (PdbPointer != NULL) {
    DEBUG ((DEBUG_VERBOSE, "SMM   Image - %a\n", PdbPointer));
  }

  //
  // Check PE/COFF image
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER *) (UINTN) ImageAddress;
  PeCoffHeaderOffset = 0;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    PeCoffHeaderOffset = DosHdr->e_lfanew;
  }

  Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINT8 *) (UINTN) ImageAddress + PeCoffHeaderOffset);
  if (Hdr.Pe32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    DEBUG ((DEBUG_VERBOSE, "SMM Hdr.Pe32->Signature invalid - 0x%x\n", Hdr.Pe32->Signature));
    goto Finish;
  }

  //
  // Get SectionAlignment
  //
  if (Hdr.Pe32->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64 && Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // NOTE: Some versions of Linux ELILO for Itanium have an incorrect magic value
    //       in the PE/COFF Header. If the MachineType is Itanium(IA64) and the
    //       Magic value in the OptionalHeader is EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC
    //       then override the magic value to EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC
    //
    Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
  } else {
    //
    // Get the magic value from the PE/COFF Optional Header
    //
    Magic = Hdr.Pe32->OptionalHeader.Magic;
  }
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    SectionAlignment  = Hdr.Pe32->OptionalHeader.SectionAlignment;
  } else {
    SectionAlignment  = Hdr.Pe32Plus->OptionalHeader.SectionAlignment;
  }

  SetMemoryAttributesTableSectionAlignment (SectionAlignment);
  if ((SectionAlignment & (RUNTIME_PAGE_ALLOCATION_GRANULARITY - 1)) != 0) {
    DEBUG ((DEBUG_WARN, "SMM !!!!!!!!  InsertImageRecord - Section Alignment(0x%x) is not %dK  !!!!!!!!\n",
      SectionAlignment, RUNTIME_PAGE_ALLOCATION_GRANULARITY >> 10));
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageAddress);
    if (PdbPointer != NULL) {
      DEBUG ((DEBUG_WARN, "SMM !!!!!!!!  Image - %a  !!!!!!!!\n", PdbPointer));
    }
    goto Finish;
  }

  Section = (EFI_IMAGE_SECTION_HEADER *) (
               (UINT8 *) (UINTN) ImageAddress +
               PeCoffHeaderOffset +
               sizeof(UINT32) +
               sizeof(EFI_IMAGE_FILE_HEADER) +
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
      ImageRecordCodeSection = AllocatePool (sizeof(*ImageRecordCodeSection));
      if (ImageRecordCodeSection == NULL) {
        return ;
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
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageAddress);
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

  SortImageRecord ();

Finish:
  return ;
}

/**
  Find image record according to image base and size.

  @param[in]  ImageBase    Base of PE image
  @param[in]  ImageSize    Size of PE image

  @return image record
**/
STATIC
IMAGE_PROPERTIES_RECORD *
FindImageRecord (
  IN EFI_PHYSICAL_ADDRESS  ImageBase,
  IN UINT64                ImageSize
  )
{
  IMAGE_PROPERTIES_RECORD    *ImageRecord;
  LIST_ENTRY                 *ImageRecordLink;
  LIST_ENTRY                 *ImageRecordList;

  ImageRecordList = &mImagePropertiesPrivateData.ImageRecordList;

  for (ImageRecordLink = ImageRecordList->ForwardLink;
       ImageRecordLink != ImageRecordList;
       ImageRecordLink = ImageRecordLink->ForwardLink) {
    ImageRecord = CR (
                    ImageRecordLink,
                    IMAGE_PROPERTIES_RECORD,
                    Link,
                    IMAGE_PROPERTIES_RECORD_SIGNATURE
                    );

    if ((ImageBase == ImageRecord->ImageBase) &&
        (ImageSize == ImageRecord->ImageSize)) {
      return ImageRecord;
    }
  }

  return NULL;
}

/**
  Remove Image record.

  @param[in]  DriverEntry    Driver information
**/
VOID
SmmRemoveImageRecord (
  IN EFI_SMM_DRIVER_ENTRY  *DriverEntry
  )
{
  IMAGE_PROPERTIES_RECORD              *ImageRecord;
  LIST_ENTRY                           *CodeSegmentListHead;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION *ImageRecordCodeSection;

  DEBUG ((DEBUG_VERBOSE, "SMM RemoveImageRecord - 0x%x\n", DriverEntry));
  DEBUG ((DEBUG_VERBOSE, "SMM RemoveImageRecord - 0x%016lx - 0x%016lx\n", DriverEntry->ImageBuffer, DriverEntry->NumberOfPage));

  ImageRecord = FindImageRecord (DriverEntry->ImageBuffer, EfiPagesToSize(DriverEntry->NumberOfPage));
  if (ImageRecord == NULL) {
    DEBUG ((DEBUG_ERROR, "SMM !!!!!!!! ImageRecord not found !!!!!!!!\n"));
    return ;
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

/**
  Publish MemoryAttributesTable to SMM configuration table.
**/
VOID
PublishMemoryAttributesTable (
  VOID
  )
{
  UINTN                                MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR                *MemoryMap;
  UINTN                                MapKey;
  UINTN                                DescriptorSize;
  UINT32                               DescriptorVersion;
  UINTN                                Index;
  EFI_STATUS                           Status;
  UINTN                                RuntimeEntryCount;
  EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE *MemoryAttributesTable;
  EFI_MEMORY_DESCRIPTOR                *MemoryAttributesEntry;
  UINTN                                MemoryAttributesTableSize;

  MemoryMapSize = 0;
  MemoryMap = NULL;
  Status = SmmCoreGetMemoryMapMemoryAttributesTable (
             &MemoryMapSize,
             MemoryMap,
             &MapKey,
             &DescriptorSize,
             &DescriptorVersion
             );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  do {
    DEBUG ((DEBUG_INFO, "MemoryMapSize - 0x%x\n", MemoryMapSize));
    MemoryMap = AllocatePool (MemoryMapSize);
    ASSERT (MemoryMap != NULL);
    DEBUG ((DEBUG_INFO, "MemoryMap - 0x%x\n", MemoryMap));

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
  RuntimeEntryCount = MemoryMapSize/DescriptorSize;
  MemoryAttributesTableSize = sizeof(EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE) + DescriptorSize * RuntimeEntryCount;
  MemoryAttributesTable = AllocatePool (sizeof(EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE) + DescriptorSize * RuntimeEntryCount);
  ASSERT (MemoryAttributesTable != NULL);
  MemoryAttributesTable->Version         = EDKII_PI_SMM_MEMORY_ATTRIBUTES_TABLE_VERSION;
  MemoryAttributesTable->NumberOfEntries = (UINT32)RuntimeEntryCount;
  MemoryAttributesTable->DescriptorSize  = (UINT32)DescriptorSize;
  MemoryAttributesTable->Reserved        = 0;
  DEBUG ((DEBUG_INFO, "MemoryAttributesTable:\n"));
  DEBUG ((DEBUG_INFO, "  Version              - 0x%08x\n", MemoryAttributesTable->Version));
  DEBUG ((DEBUG_INFO, "  NumberOfEntries      - 0x%08x\n", MemoryAttributesTable->NumberOfEntries));
  DEBUG ((DEBUG_INFO, "  DescriptorSize       - 0x%08x\n", MemoryAttributesTable->DescriptorSize));
  MemoryAttributesEntry = (EFI_MEMORY_DESCRIPTOR *)(MemoryAttributesTable + 1);
  for (Index = 0; Index < MemoryMapSize/DescriptorSize; Index++) {
    CopyMem (MemoryAttributesEntry, MemoryMap, DescriptorSize);
    DEBUG ((DEBUG_INFO, "Entry (0x%x)\n", MemoryAttributesEntry));
    DEBUG ((DEBUG_INFO, "  Type              - 0x%x\n", MemoryAttributesEntry->Type));
    DEBUG ((DEBUG_INFO, "  PhysicalStart     - 0x%016lx\n", MemoryAttributesEntry->PhysicalStart));
    DEBUG ((DEBUG_INFO, "  VirtualStart      - 0x%016lx\n", MemoryAttributesEntry->VirtualStart));
    DEBUG ((DEBUG_INFO, "  NumberOfPages     - 0x%016lx\n", MemoryAttributesEntry->NumberOfPages));
    DEBUG ((DEBUG_INFO, "  Attribute         - 0x%016lx\n", MemoryAttributesEntry->Attribute));
    MemoryAttributesEntry = NEXT_MEMORY_DESCRIPTOR(MemoryAttributesEntry, DescriptorSize);

    MemoryMap = NEXT_MEMORY_DESCRIPTOR(MemoryMap, DescriptorSize);
  }

  Status = gSmst->SmmInstallConfigurationTable (gSmst, &gEdkiiPiSmmMemoryAttributesTableGuid, MemoryAttributesTable, MemoryAttributesTableSize);
  ASSERT_EFI_ERROR (Status);
}

/**
  This function returns if image is inside SMRAM.

  @param[in] LoadedImage LoadedImage protocol instance for an image.

  @retval TRUE  the image is inside SMRAM.
  @retval FALSE the image is outside SMRAM.
**/
BOOLEAN
IsImageInsideSmram (
  IN EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage
  )
{
  UINTN  Index;

  for (Index = 0; Index < mFullSmramRangeCount; Index++) {
    if ((mFullSmramRanges[Index].PhysicalStart <= (UINTN)LoadedImage->ImageBase)&&
        (mFullSmramRanges[Index].PhysicalStart + mFullSmramRanges[Index].PhysicalSize >= (UINTN)LoadedImage->ImageBase + LoadedImage->ImageSize)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  This function installs all SMM image record information.
**/
VOID
SmmInstallImageRecord (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINTN                       NoHandles;
  EFI_HANDLE                  *HandleBuffer;
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage;
  UINTN                       Index;
  EFI_SMM_DRIVER_ENTRY        DriverEntry;

  Status = SmmLocateHandleBuffer (
             ByProtocol,
             &gEfiLoadedImageProtocolGuid,
             NULL,
             &NoHandles,
             &HandleBuffer
             );
  if (EFI_ERROR (Status)) {
    return ;
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
      VOID *PdbPointer;
      PdbPointer = PeCoffLoaderGetPdbPointer (LoadedImage->ImageBase);
      if (PdbPointer != NULL) {
        DEBUG ((DEBUG_VERBOSE, "(%a) ", PdbPointer));
      }
    }
    DEBUG ((DEBUG_VERBOSE, "\n"));
    ZeroMem (&DriverEntry, sizeof(DriverEntry));
    DriverEntry.ImageBuffer  = (UINTN)LoadedImage->ImageBase;
    DriverEntry.NumberOfPage = EFI_SIZE_TO_PAGES((UINTN)LoadedImage->ImageSize);
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

  DEBUG ((DEBUG_INFO, "SMM MemoryProtectionAttribute - 0x%016lx\n", mMemoryProtectionAttribute));
  if ((mMemoryProtectionAttribute & EFI_MEMORY_ATTRIBUTES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA) == 0) {
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_VERBOSE, "SMM Total Image Count - 0x%x\n", mImagePropertiesPrivateData.ImageRecordCount));
  DEBUG ((DEBUG_VERBOSE, "SMM Dump ImageRecord:\n"));
  DumpImageRecord ();

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
  EFI_STATUS                        Status;
  VOID                              *Registration;

  Status = gSmst->SmmRegisterProtocolNotify (
                    &gEfiSmmEndOfDxeProtocolGuid,
                    SmmInstallMemoryAttributesTable,
                    &Registration
                    );
  ASSERT_EFI_ERROR (Status);

  return ;
}
