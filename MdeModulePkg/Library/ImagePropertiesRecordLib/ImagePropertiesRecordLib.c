/** @file

  Provides definitions and functionality for manipulating IMAGE_PROPERTIES_RECORD.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ImagePropertiesRecordLib.h>

#define PREVIOUS_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) - (Size)))

#define NEXT_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) + (Size)))

/**
  Converts a number of pages to a size in bytes.

  NOTE: Do not use EFI_PAGES_TO_SIZE because it handles UINTN only.

  @param[in]  Pages     The number of EFI_PAGES.

  @retval  The number of bytes associated with the input number of pages.
**/
STATIC
UINT64
EfiPagesToSize (
  IN UINT64  Pages
  )
{
  return LShiftU64 (Pages, EFI_PAGE_SHIFT);
}

/**
  Converts a size, in bytes, to a number of EFI_PAGESs.

  NOTE: Do not use EFI_SIZE_TO_PAGES because it handles UINTN only.

  @param[in]  Size      A size in bytes.

  @retval  The number of pages associated with the input number of bytes.

**/
STATIC
UINT64
EfiSizeToPages (
  IN UINT64  Size
  )
{
  return RShiftU64 (Size, EFI_PAGE_SHIFT) + ((((UINTN)Size) & EFI_PAGE_MASK) ? 1 : 0);
}

/**
  Sort memory map entries based upon PhysicalStart from low to high.

  @param[in, out] MemoryMap       A pointer to the buffer in which firmware places
                                  the current memory map.
  @param[in]      MemoryMapSize   Size, in bytes, of the MemoryMap buffer.
  @param[in]      DescriptorSize  Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
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

  return;
}

/**
  Return the first image record, whose [ImageBase, ImageSize] covered by [Buffer, Length].

  @param[in] Buffer           Starting Address
  @param[in] Length           Length to check
  @param[in] ImageRecordList  A list of IMAGE_PROPERTIES_RECORD entries to check against
                              the memory range Buffer -> Buffer + Length

  @retval The first image record covered by [Buffer, Length]
**/
STATIC
IMAGE_PROPERTIES_RECORD *
GetImageRecordByAddress (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length,
  IN LIST_ENTRY            *ImageRecordList
  )
{
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  LIST_ENTRY               *ImageRecordLink;

  for (ImageRecordLink = ImageRecordList->ForwardLink;
       ImageRecordLink != ImageRecordList;
       ImageRecordLink = ImageRecordLink->ForwardLink)
  {
    ImageRecord = CR (
                    ImageRecordLink,
                    IMAGE_PROPERTIES_RECORD,
                    Link,
                    IMAGE_PROPERTIES_RECORD_SIGNATURE
                    );

    if ((Buffer <= ImageRecord->ImageBase) &&
        (Buffer + Length >= ImageRecord->ImageBase + ImageRecord->ImageSize))
    {
      return ImageRecord;
    }
  }

  return NULL;
}

/**
  Break up the input OldRecord into multiple new records based on the code
  and data sections in the input ImageRecord.

  @param[in]        ImageRecord       An IMAGE_PROPERTIES_RECORD whose ImageBase and
                                      ImageSize is covered by by OldRecord.
  @param[in, out]   NewRecord         A pointer to several new memory map entries.
                                      The caller gurantee the buffer size be 1 +
                                      (SplitRecordCount * DescriptorSize) calculated
                                      below.
  @param[in]        OldRecord         A pointer to one old memory map entry.
  @param[in]        DescriptorSize    The size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.

  @retval The number of new descriptors created.
**/
STATIC
UINTN
SetNewRecord (
  IN IMAGE_PROPERTIES_RECORD    *ImageRecord,
  IN OUT EFI_MEMORY_DESCRIPTOR  *NewRecord,
  IN EFI_MEMORY_DESCRIPTOR      *OldRecord,
  IN UINTN                      DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR                 TempRecord;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;
  LIST_ENTRY                            *ImageRecordCodeSectionLink;
  LIST_ENTRY                            *ImageRecordCodeSectionEndLink;
  LIST_ENTRY                            *ImageRecordCodeSectionList;
  UINTN                                 NewRecordCount;
  UINT64                                PhysicalEnd;
  UINT64                                ImageEnd;

  CopyMem (&TempRecord, OldRecord, sizeof (EFI_MEMORY_DESCRIPTOR));
  PhysicalEnd    = TempRecord.PhysicalStart + EfiPagesToSize (TempRecord.NumberOfPages);
  NewRecordCount = 0;

  ImageRecordCodeSectionList = &ImageRecord->CodeSegmentList;

  ImageRecordCodeSectionLink    = ImageRecordCodeSectionList->ForwardLink;
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
      NewRecord->Type          = TempRecord.Type;
      NewRecord->PhysicalStart = TempRecord.PhysicalStart;
      NewRecord->VirtualStart  = 0;
      NewRecord->NumberOfPages = EfiSizeToPages (ImageRecordCodeSection->CodeSegmentBase - NewRecord->PhysicalStart);
      NewRecord->Attribute     = TempRecord.Attribute | EFI_MEMORY_XP;
      if (NewRecord->NumberOfPages != 0) {
        NewRecord = NEXT_MEMORY_DESCRIPTOR (NewRecord, DescriptorSize);
        NewRecordCount++;
      }

      //
      // CODE
      //
      NewRecord->Type          = TempRecord.Type;
      NewRecord->PhysicalStart = ImageRecordCodeSection->CodeSegmentBase;
      NewRecord->VirtualStart  = 0;
      NewRecord->NumberOfPages = EfiSizeToPages (ImageRecordCodeSection->CodeSegmentSize);
      NewRecord->Attribute     = (TempRecord.Attribute & (~EFI_MEMORY_XP)) | EFI_MEMORY_RO;
      if (NewRecord->NumberOfPages != 0) {
        NewRecord = NEXT_MEMORY_DESCRIPTOR (NewRecord, DescriptorSize);
        NewRecordCount++;
      }

      TempRecord.PhysicalStart = ImageRecordCodeSection->CodeSegmentBase + EfiPagesToSize (EfiSizeToPages (ImageRecordCodeSection->CodeSegmentSize));
      TempRecord.NumberOfPages = EfiSizeToPages (PhysicalEnd - TempRecord.PhysicalStart);
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
    NewRecord->Type          = TempRecord.Type;
    NewRecord->PhysicalStart = TempRecord.PhysicalStart;
    NewRecord->VirtualStart  = 0;
    NewRecord->NumberOfPages = EfiSizeToPages (ImageEnd - TempRecord.PhysicalStart);
    NewRecord->Attribute     = TempRecord.Attribute | EFI_MEMORY_XP;
    NewRecordCount++;
  }

  return NewRecordCount;
}

/**
  Return the maximum number of new entries required to describe the code and data sections
  of all images covered by the input OldRecord.

  @param[in]  OldRecord         A pointer to one old memory map entry.
  @param[in]  ImageRecordList   A list of IMAGE_PROPERTIES_RECORD entries used when searching
                                for an image record contained by the memory range described by
                                OldRecord

  @retval  The maximum number of new descriptors required to describe the code and data sections
           of all images covered by OldRecord.
**/
STATIC
UINTN
GetMaxSplitRecordCount (
  IN EFI_MEMORY_DESCRIPTOR  *OldRecord,
  IN LIST_ENTRY             *ImageRecordList
  )
{
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  UINTN                    SplitRecordCount;
  UINT64                   PhysicalStart;
  UINT64                   PhysicalEnd;

  SplitRecordCount = 0;
  PhysicalStart    = OldRecord->PhysicalStart;
  PhysicalEnd      = OldRecord->PhysicalStart + EfiPagesToSize (OldRecord->NumberOfPages);

  do {
    ImageRecord = GetImageRecordByAddress (PhysicalStart, PhysicalEnd - PhysicalStart, ImageRecordList);
    if (ImageRecord == NULL) {
      break;
    }

    SplitRecordCount += (2 * ImageRecord->CodeSegmentCount + 3);
    PhysicalStart     = ImageRecord->ImageBase + ImageRecord->ImageSize;
  } while ((ImageRecord != NULL) && (PhysicalStart < PhysicalEnd));

  if (SplitRecordCount != 0) {
    SplitRecordCount--;
  }

  return SplitRecordCount;
}

/**
  Split the memory map into new entries based upon the PE code and data sections
  in ImageRecordList covered by the input OldRecord.

  @param[in]        OldRecord             A pointer to one old memory map entry.
  @param[in, out]   NewRecord             A pointer to several new memory map entries.
                                          The caller gurantee the buffer size be
                                          (SplitRecordCount * DescriptorSize).
  @param[in]        MaxSplitRecordCount   The maximum number of entries post-split.
  @param[in]        DescriptorSize        The size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
  @param[in]        ImageRecordList       A list of IMAGE_PROPERTIES_RECORD entries used when searching
                                          for an image record contained by the memory range described in
                                          the existing EFI memory map descriptor OldRecord

  @retval  The number of split entries.
**/
STATIC
UINTN
SplitRecord (
  IN EFI_MEMORY_DESCRIPTOR      *OldRecord,
  IN OUT EFI_MEMORY_DESCRIPTOR  *NewRecord,
  IN UINTN                      MaxSplitRecordCount,
  IN UINTN                      DescriptorSize,
  IN LIST_ENTRY                 *ImageRecordList
  )
{
  EFI_MEMORY_DESCRIPTOR    TempRecord;
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  IMAGE_PROPERTIES_RECORD  *NewImageRecord;
  UINT64                   PhysicalStart;
  UINT64                   PhysicalEnd;
  UINTN                    NewRecordCount;
  UINTN                    TotalNewRecordCount;

  if (MaxSplitRecordCount == 0) {
    CopyMem (NewRecord, OldRecord, DescriptorSize);
    return 0;
  }

  TotalNewRecordCount = 0;

  //
  // Override previous record
  //
  CopyMem (&TempRecord, OldRecord, sizeof (EFI_MEMORY_DESCRIPTOR));
  PhysicalStart = TempRecord.PhysicalStart;
  PhysicalEnd   = TempRecord.PhysicalStart + EfiPagesToSize (TempRecord.NumberOfPages);

  ImageRecord = NULL;
  do {
    NewImageRecord = GetImageRecordByAddress (PhysicalStart, PhysicalEnd - PhysicalStart, ImageRecordList);
    if (NewImageRecord == NULL) {
      //
      // No more images cover this range, check if we've reached the end of the old descriptor. If not,
      // add the remaining range to the new descriptor list.
      //
      if (PhysicalEnd > PhysicalStart) {
        NewRecord->Type          = TempRecord.Type;
        NewRecord->PhysicalStart = PhysicalStart;
        NewRecord->VirtualStart  = 0;
        NewRecord->NumberOfPages = EfiSizeToPages (PhysicalEnd - PhysicalStart);
        NewRecord->Attribute     = TempRecord.Attribute;
        TotalNewRecordCount++;
      }

      break;
    }

    ImageRecord = NewImageRecord;

    //
    // Update PhysicalStart to exclude the portion before the image buffer
    //
    if (TempRecord.PhysicalStart < ImageRecord->ImageBase) {
      NewRecord->Type          = TempRecord.Type;
      NewRecord->PhysicalStart = TempRecord.PhysicalStart;
      NewRecord->VirtualStart  = 0;
      NewRecord->NumberOfPages = EfiSizeToPages (ImageRecord->ImageBase - TempRecord.PhysicalStart);
      NewRecord->Attribute     = TempRecord.Attribute;
      TotalNewRecordCount++;

      PhysicalStart            = ImageRecord->ImageBase;
      TempRecord.PhysicalStart = PhysicalStart;
      TempRecord.NumberOfPages = EfiSizeToPages (PhysicalEnd - PhysicalStart);

      NewRecord = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)NewRecord + DescriptorSize);
    }

    //
    // Set new record
    //
    NewRecordCount       = SetNewRecord (ImageRecord, NewRecord, &TempRecord, DescriptorSize);
    TotalNewRecordCount += NewRecordCount;
    NewRecord            = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)NewRecord + NewRecordCount * DescriptorSize);

    //
    // Update PhysicalStart, in order to exclude the image buffer already splitted.
    //
    PhysicalStart            = ImageRecord->ImageBase + ImageRecord->ImageSize;
    TempRecord.PhysicalStart = PhysicalStart;
    TempRecord.NumberOfPages = EfiSizeToPages (PhysicalEnd - PhysicalStart);
  } while ((ImageRecord != NULL) && (PhysicalStart < PhysicalEnd));

  //
  // The logic in function SplitTable() ensures that TotalNewRecordCount will not be zero if the
  // code reaches here.
  //
  ASSERT (TotalNewRecordCount != 0);
  return TotalNewRecordCount - 1;
}

/**
  Split the original memory map and add more entries to describe PE code
  and data sections for each image in the input ImageRecordList.

  NOTE: This function assumes PE code/data section are page aligned.
  NOTE: This function assumes there are enough entries for the new memory map.

  |         |      |      |      |      |      |         |
  | 4K PAGE | DATA | CODE | DATA | CODE | DATA | 4K PAGE |
  |         |      |      |      |      |      |         |
  Assume the above memory region is the result of one split memory map descriptor. It's unlikely
  that a linker will orient an image this way, but the caller must assume the worst case scenario.
  This image layout example contains code sections oriented in a way that maximizes the number of
  descriptors which would be required to describe each section. To ensure we have enough space
  for every descriptor of the broken up memory map, the caller must assume that every image will
  have the maximum number of code sections oriented in a way which maximizes the number of data
  sections with unrelated memory regions flanking each image within a single descriptor.

  Given an image record list, the caller should use the following formula when allocating extra descriptors:
  NumberOfAdditionalDescriptors = (MemoryMapSize / DescriptorSize) +
                                    ((2 * <Most Code Segments in a Single Image> + 3) * <Number of Images>)

  @param[in, out] MemoryMapSize                   IN:   The size, in bytes, of the old memory map before the split.
                                                  OUT:  The size, in bytes, of the used descriptors of the split
                                                        memory map
  @param[in, out] MemoryMap                       IN:   A pointer to the buffer containing the current memory map.
                                                        This buffer must have enough space to accomodate the "worst case"
                                                        scenario where every image in ImageRecordList needs a new descriptor
                                                        to describe its code and data sections.
                                                  OUT:  A pointer to the updated memory map with separated image section
                                                        descriptors.
  @param[in]      DescriptorSize                  The size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
  @param[in]      ImageRecordList                 A list of IMAGE_PROPERTIES_RECORD entries used when searching
                                                  for an image record contained by the memory range described in
                                                  EFI memory map descriptors.
  @param[in]      NumberOfAdditionalDescriptors   The number of unused descriptors at the end of the input MemoryMap.
                                                  The formula in the description should be used to calculate this value.

  @retval EFI_SUCCESS                             The memory map was successfully split.
  @retval EFI_INVALID_PARAMETER                   MemoryMapSize, MemoryMap, or ImageRecordList was NULL.
**/
EFI_STATUS
EFIAPI
SplitTable (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN     UINTN                  DescriptorSize,
  IN     LIST_ENTRY             *ImageRecordList,
  IN     UINTN                  NumberOfAdditionalDescriptors
  )
{
  INTN   IndexOld;
  INTN   IndexNew;
  INTN   IndexNewStarting;
  UINTN  MaxSplitRecordCount;
  UINTN  RealSplitRecordCount;
  UINTN  TotalSkippedRecords;

  if ((MemoryMapSize == NULL) || (MemoryMap == NULL) || (ImageRecordList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TotalSkippedRecords = 0;
  //
  // Let old record point to end of valid MemoryMap buffer.
  //
  IndexOld = ((*MemoryMapSize) / DescriptorSize) - 1;
  //
  // Let new record point to end of full MemoryMap buffer.
  //
  IndexNew         = ((*MemoryMapSize) / DescriptorSize) - 1 + NumberOfAdditionalDescriptors;
  IndexNewStarting = IndexNew;
  for ( ; IndexOld >= 0; IndexOld--) {
    MaxSplitRecordCount = GetMaxSplitRecordCount ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + IndexOld * DescriptorSize), ImageRecordList);
    //
    // Split this MemoryMap record
    //
    IndexNew            -= MaxSplitRecordCount;
    RealSplitRecordCount = SplitRecord (
                             (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + IndexOld * DescriptorSize),
                             (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + IndexNew * DescriptorSize),
                             MaxSplitRecordCount,
                             DescriptorSize,
                             ImageRecordList
                             );

    // If we didn't utilize all the extra allocated descriptor slots, set the physical address of the unused slots
    // to MAX_ADDRESS so they are moved to the bottom of the list when sorting.
    for ( ; RealSplitRecordCount < MaxSplitRecordCount; RealSplitRecordCount++) {
      ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + ((IndexNew + RealSplitRecordCount + 1) * DescriptorSize)))->PhysicalStart = MAX_ADDRESS;
      TotalSkippedRecords++;
    }

    IndexNew--;
  }

  //
  // Move all records to the beginning.
  //
  CopyMem (
    MemoryMap,
    (UINT8 *)MemoryMap + ((IndexNew + 1) * DescriptorSize),
    (IndexNewStarting - IndexNew) * DescriptorSize
    );

  //
  // Sort from low to high to filter out the MAX_ADDRESS records.
  //
  SortMemoryMap (MemoryMap, (IndexNewStarting - IndexNew) * DescriptorSize, DescriptorSize);

  *MemoryMapSize = (IndexNewStarting - IndexNew - TotalSkippedRecords) * DescriptorSize;

  return EFI_SUCCESS;
}

/**
  Swap two code sections in a single IMAGE_PROPERTIES_RECORD.

  @param[in]  FirstImageRecordCodeSection    The first code section
  @param[in]  SecondImageRecordCodeSection   The second code section

  @retval EFI_SUCCESS                        The code sections were swapped successfully
  @retval EFI_INVALID_PARAMETER              FirstImageRecordCodeSection or SecondImageRecordCodeSection is NULL
**/
EFI_STATUS
EFIAPI
SwapImageRecordCodeSection (
  IN IMAGE_PROPERTIES_RECORD_CODE_SECTION  *FirstImageRecordCodeSection,
  IN IMAGE_PROPERTIES_RECORD_CODE_SECTION  *SecondImageRecordCodeSection
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  TempImageRecordCodeSection;

  if ((FirstImageRecordCodeSection == NULL) || (SecondImageRecordCodeSection == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TempImageRecordCodeSection.CodeSegmentBase = FirstImageRecordCodeSection->CodeSegmentBase;
  TempImageRecordCodeSection.CodeSegmentSize = FirstImageRecordCodeSection->CodeSegmentSize;

  FirstImageRecordCodeSection->CodeSegmentBase = SecondImageRecordCodeSection->CodeSegmentBase;
  FirstImageRecordCodeSection->CodeSegmentSize = SecondImageRecordCodeSection->CodeSegmentSize;

  SecondImageRecordCodeSection->CodeSegmentBase = TempImageRecordCodeSection.CodeSegmentBase;
  SecondImageRecordCodeSection->CodeSegmentSize = TempImageRecordCodeSection.CodeSegmentSize;

  return EFI_SUCCESS;
}

/**
  Sort the code sections in the input ImageRecord based upon CodeSegmentBase from low to high.

  @param[in]  ImageRecord         IMAGE_PROPERTIES_RECORD to be sorted

  @retval EFI_SUCCESS             The code sections in the input ImageRecord were sorted successfully
  @retval EFI_ABORTED             An error occurred while sorting the code sections in the input ImageRecord
  @retval EFI_INVALID_PARAMETER   ImageRecord is NULL
**/
EFI_STATUS
EFIAPI
SortImageRecordCodeSection (
  IN IMAGE_PROPERTIES_RECORD  *ImageRecord
  )
{
  EFI_STATUS                            Status;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *NextImageRecordCodeSection;
  LIST_ENTRY                            *ImageRecordCodeSectionLink;
  LIST_ENTRY                            *NextImageRecordCodeSectionLink;
  LIST_ENTRY                            *ImageRecordCodeSectionEndLink;
  LIST_ENTRY                            *ImageRecordCodeSectionList;

  if (ImageRecord == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ImageRecordCodeSectionList = &ImageRecord->CodeSegmentList;

  ImageRecordCodeSectionLink     = ImageRecordCodeSectionList->ForwardLink;
  NextImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink;
  ImageRecordCodeSectionEndLink  = ImageRecordCodeSectionList;
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
        Status = SwapImageRecordCodeSection (ImageRecordCodeSection, NextImageRecordCodeSection);
        if (EFI_ERROR (Status)) {
          ASSERT_EFI_ERROR (Status);
          return EFI_ABORTED;
        }
      }

      NextImageRecordCodeSectionLink = NextImageRecordCodeSectionLink->ForwardLink;
    }

    ImageRecordCodeSectionLink     = ImageRecordCodeSectionLink->ForwardLink;
    NextImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink;
  }

  return EFI_SUCCESS;
}

/**
  Check if the code sections in the input ImageRecord are valid.
  The code sections are valid if they don't overlap, are contained
  within the the ImageRecord's ImageBase and ImageSize, and are
  contained within the MAX_ADDRESS.

  @param[in]  ImageRecord    IMAGE_PROPERTIES_RECORD to be checked

  @retval TRUE  The code sections in the input ImageRecord are valid
  @retval FALSE The code sections in the input ImageRecord are invalid
**/
BOOLEAN
EFIAPI
IsImageRecordCodeSectionValid (
  IN IMAGE_PROPERTIES_RECORD  *ImageRecord
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *LastImageRecordCodeSection;
  LIST_ENTRY                            *ImageRecordCodeSectionLink;
  LIST_ENTRY                            *ImageRecordCodeSectionEndLink;
  LIST_ENTRY                            *ImageRecordCodeSectionList;

  if (ImageRecord == NULL) {
    return FALSE;
  }

  DEBUG ((DEBUG_VERBOSE, "ImageCode SegmentCount - 0x%x\n", ImageRecord->CodeSegmentCount));

  ImageRecordCodeSectionList = &ImageRecord->CodeSegmentList;

  ImageRecordCodeSectionLink    = ImageRecordCodeSectionList->ForwardLink;
  ImageRecordCodeSectionEndLink = ImageRecordCodeSectionList;
  LastImageRecordCodeSection    = NULL;
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

  @param[in]  FirstImageRecord   The first image record.
  @param[in]  SecondImageRecord  The second image record.

  @retval EFI_SUCCESS            The image records were swapped successfully
  @retval EFI_INVALID_PARAMETER  FirstImageRecord or SecondImageRecord is NULL
**/
EFI_STATUS
EFIAPI
SwapImageRecord (
  IN IMAGE_PROPERTIES_RECORD  *FirstImageRecord,
  IN IMAGE_PROPERTIES_RECORD  *SecondImageRecord
  )
{
  IMAGE_PROPERTIES_RECORD  TempImageRecord;

  if ((FirstImageRecord == NULL) || (SecondImageRecord == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TempImageRecord.ImageBase        = FirstImageRecord->ImageBase;
  TempImageRecord.ImageSize        = FirstImageRecord->ImageSize;
  TempImageRecord.CodeSegmentCount = FirstImageRecord->CodeSegmentCount;

  FirstImageRecord->ImageBase        = SecondImageRecord->ImageBase;
  FirstImageRecord->ImageSize        = SecondImageRecord->ImageSize;
  FirstImageRecord->CodeSegmentCount = SecondImageRecord->CodeSegmentCount;

  SecondImageRecord->ImageBase        = TempImageRecord.ImageBase;
  SecondImageRecord->ImageSize        = TempImageRecord.ImageSize;
  SecondImageRecord->CodeSegmentCount = TempImageRecord.CodeSegmentCount;

  SwapListEntries (&FirstImageRecord->CodeSegmentList, &SecondImageRecord->CodeSegmentList);
  return EFI_SUCCESS;
}

/**
  Sort the input ImageRecordList based upon the ImageBase from low to high.

  @param[in] ImageRecordList    Image record list to be sorted

  @retval EFI_SUCCESS           The image record list was sorted successfully
  @retval EFI_ABORTED           An error occurred while sorting the image record list
  @retval EFI_INVALID_PARAMETER ImageRecordList is NULL
**/
EFI_STATUS
EFIAPI
SortImageRecord (
  IN LIST_ENTRY  *ImageRecordList
  )
{
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  IMAGE_PROPERTIES_RECORD  *NextImageRecord;
  LIST_ENTRY               *ImageRecordLink;
  LIST_ENTRY               *NextImageRecordLink;
  LIST_ENTRY               *ImageRecordEndLink;
  EFI_STATUS               Status;

  if (ImageRecordList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ImageRecordLink     = ImageRecordList->ForwardLink;
  NextImageRecordLink = ImageRecordLink->ForwardLink;
  ImageRecordEndLink  = ImageRecordList;
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
        Status = SwapImageRecord (ImageRecord, NextImageRecord);
        if (EFI_ERROR (Status)) {
          ASSERT_EFI_ERROR (Status);
          return EFI_ABORTED;
        }
      }

      NextImageRecordLink = NextImageRecordLink->ForwardLink;
    }

    ImageRecordLink     = ImageRecordLink->ForwardLink;
    NextImageRecordLink = ImageRecordLink->ForwardLink;
  }

  return EFI_SUCCESS;
}

/**
  Find image record according to image base and size.

  @param[in]  ImageBase           Base of PE image
  @param[in]  ImageSize           Size of PE image
  @param[in]  ImageRecordList     Image record list to be searched

  @retval    NULL             No IMAGE_PROPERTIES_RECORD matches ImageBase
                              and ImageSize in the input ImageRecordList
  @retval    Other            The found IMAGE_PROPERTIES_RECORD
**/
IMAGE_PROPERTIES_RECORD *
EFIAPI
FindImageRecord (
  IN EFI_PHYSICAL_ADDRESS  ImageBase,
  IN UINT64                ImageSize,
  IN LIST_ENTRY            *ImageRecordList
  )
{
  IMAGE_PROPERTIES_RECORD  *ImageRecord;
  LIST_ENTRY               *ImageRecordLink;

  if (ImageRecordList == NULL) {
    return NULL;
  }

  for (ImageRecordLink = ImageRecordList->ForwardLink;
       ImageRecordLink != ImageRecordList;
       ImageRecordLink = ImageRecordLink->ForwardLink)
  {
    ImageRecord = CR (
                    ImageRecordLink,
                    IMAGE_PROPERTIES_RECORD,
                    Link,
                    IMAGE_PROPERTIES_RECORD_SIGNATURE
                    );

    if ((ImageBase == ImageRecord->ImageBase) &&
        (ImageSize == ImageRecord->ImageSize))
    {
      return ImageRecord;
    }
  }

  return NULL;
}
