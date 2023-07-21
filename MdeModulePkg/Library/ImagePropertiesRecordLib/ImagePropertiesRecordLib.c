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
  Converts a number of EFI_PAGEs to a size in bytes.

  NOTE: Do not use EFI_PAGES_TO_SIZE because it handles UINTN only.

  @param  Pages     The number of EFI_PAGES.

  @return  The number of bytes associated with the number of EFI_PAGEs specified
           by Pages.
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

  @param  Size      A size in bytes.

  @return  The number of EFI_PAGESs associated with the number of bytes specified
           by Size.

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

  return;
}

/**
  Return the first image record, whose [ImageBase, ImageSize] covered by [Buffer, Length].

  @param Buffer           Start Address
  @param Length           Address length
  @param ImageRecordList  Image record list

  @return first image record covered by [buffer, length]
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
  Set the memory map to new entries, according to one old entry,
  based upon PE code section and data section in image record

  @param  ImageRecord            An image record whose [ImageBase, ImageSize] covered
                                 by old memory map entry.
  @param  NewRecord              A pointer to several new memory map entries.
                                 The caller gurantee the buffer size be 1 +
                                 (SplitRecordCount * DescriptorSize) calculated
                                 below.
  @param  OldRecord              A pointer to one old memory map entry.
  @param  DescriptorSize         Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
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
  Return the max number of new splitted entries, according to one old entry,
  based upon PE code section and data section.

  @param  OldRecord              A pointer to one old memory map entry.
  @param  ImageRecordList        A list of IMAGE_PROPERTIES_RECORD entries used when searching
                                 for an image record contained by the memory range described in
                                 the existing EFI memory map descriptor OldRecord

  @retval  0 no entry need to be splitted.
  @return  the max number of new splitted entries
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

    SplitRecordCount += (2 * ImageRecord->CodeSegmentCount + 1);
    PhysicalStart     = ImageRecord->ImageBase + ImageRecord->ImageSize;
  } while ((ImageRecord != NULL) && (PhysicalStart < PhysicalEnd));

  if (SplitRecordCount != 0) {
    SplitRecordCount--;
  }

  return SplitRecordCount;
}

/**
  Split the memory map to new entries, according to one old entry,
  based upon PE code section and data section.

  @param        OldRecord             A pointer to one old memory map entry.
  @param        NewRecord             A pointer to several new memory map entries.
                                      The caller gurantee the buffer size be 1 +
                                      (SplitRecordCount * DescriptorSize) calculated
                                      below.
  @param        MaxSplitRecordCount   The max number of splitted entries
  @param        DescriptorSize        Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
  @param        ImageRecordList       A list of IMAGE_PROPERTIES_RECORD entries used when searching
                                      for an image record contained by the memory range described in
                                      the existing EFI memory map descriptor OldRecord

  @retval  0 no entry is splitted.
  @return  the real number of splitted record.
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
  BOOLEAN                  IsLastRecordData;

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
      // No more image covered by this range, stop
      //
      if ((PhysicalEnd > PhysicalStart) && (ImageRecord != NULL)) {
        //
        // If this is still address in this record, need record.
        //
        NewRecord        = PREVIOUS_MEMORY_DESCRIPTOR (NewRecord, DescriptorSize);
        IsLastRecordData = FALSE;
        if ((NewRecord->Attribute & EFI_MEMORY_XP) != 0) {
          IsLastRecordData = TRUE;
        }

        if (IsLastRecordData) {
          //
          // Last record is DATA, just merge it.
          //
          NewRecord->NumberOfPages = EfiSizeToPages (PhysicalEnd - NewRecord->PhysicalStart);
        } else {
          //
          // Last record is CODE, create a new DATA entry.
          //
          NewRecord                = NEXT_MEMORY_DESCRIPTOR (NewRecord, DescriptorSize);
          NewRecord->Type          = TempRecord.Type;
          NewRecord->PhysicalStart = TempRecord.PhysicalStart;
          NewRecord->VirtualStart  = 0;
          NewRecord->NumberOfPages = TempRecord.NumberOfPages;
          NewRecord->Attribute     = TempRecord.Attribute | EFI_MEMORY_XP;
          TotalNewRecordCount++;
        }
      }

      break;
    }

    ImageRecord = NewImageRecord;

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
   +---------------+ ----
   | Record RtData |     |
   +---------------+     |
   | Record RtCode |     |-> PE/COFF1
   +---------------+     |
   | Record RtData |     |
   +---------------+ ----
   | Record RtData |     |
   +---------------+     |
   | Record RtCode |     |-> PE/COFF2
   +---------------+     |
   | Record RtData |     |
   +---------------+ ----
   | Record Y      |
   +---------------+

  @param  MemoryMapSize                   A pointer to the size, in bytes, of the
                                          MemoryMap buffer. On input, this is the size of
                                          old MemoryMap before split. The actual buffer
                                          size of MemoryMap is MemoryMapSize +
                                          (AdditionalRecordCount * DescriptorSize) calculated
                                          below. On output, it is the size of new MemoryMap
                                          after split.
  @param  MemoryMap                       A pointer to the buffer in which firmware places
                                          the current memory map.
  @param  DescriptorSize                  Size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
  @param  ImageRecordList                 A list of IMAGE_PROPERTIES_RECORD entries used when searching
                                          for an image record contained by the memory range described in
                                          EFI memory map descriptors.
  @param  NumberOfAdditionalDescriptors   The number of unused descriptors at the end of the input MemoryMap.
**/
VOID
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
  UINTN  MaxSplitRecordCount;
  UINTN  RealSplitRecordCount;
  UINTN  TotalSplitRecordCount;

  TotalSplitRecordCount = 0;
  //
  // Let old record point to end of valid MemoryMap buffer.
  //
  IndexOld = ((*MemoryMapSize) / DescriptorSize) - 1;
  //
  // Let new record point to end of full MemoryMap buffer.
  //
  IndexNew = ((*MemoryMapSize) / DescriptorSize) - 1 + NumberOfAdditionalDescriptors;
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
    //
    // Adjust IndexNew according to real split.
    //
    CopyMem (
      ((UINT8 *)MemoryMap + (IndexNew + MaxSplitRecordCount - RealSplitRecordCount) * DescriptorSize),
      ((UINT8 *)MemoryMap + IndexNew * DescriptorSize),
      RealSplitRecordCount * DescriptorSize
      );
    IndexNew               = IndexNew + MaxSplitRecordCount - RealSplitRecordCount;
    TotalSplitRecordCount += RealSplitRecordCount;
    IndexNew--;
  }

  //
  // Move all records to the beginning.
  //
  CopyMem (
    MemoryMap,
    (UINT8 *)MemoryMap + (NumberOfAdditionalDescriptors - TotalSplitRecordCount) * DescriptorSize,
    (*MemoryMapSize) + TotalSplitRecordCount * DescriptorSize
    );

  *MemoryMapSize = (*MemoryMapSize) + DescriptorSize * TotalSplitRecordCount;

  //
  // Sort from low to high (Just in case)
  //
  SortMemoryMap (MemoryMap, *MemoryMapSize, DescriptorSize);

  return;
}

/**
  Swap two code sections in image record.

  @param  FirstImageRecordCodeSection    first code section in image record
  @param  SecondImageRecordCodeSection   second code section in image record
**/
VOID
EFIAPI
SwapImageRecordCodeSection (
  IN IMAGE_PROPERTIES_RECORD_CODE_SECTION  *FirstImageRecordCodeSection,
  IN IMAGE_PROPERTIES_RECORD_CODE_SECTION  *SecondImageRecordCodeSection
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  TempImageRecordCodeSection;

  TempImageRecordCodeSection.CodeSegmentBase = FirstImageRecordCodeSection->CodeSegmentBase;
  TempImageRecordCodeSection.CodeSegmentSize = FirstImageRecordCodeSection->CodeSegmentSize;

  FirstImageRecordCodeSection->CodeSegmentBase = SecondImageRecordCodeSection->CodeSegmentBase;
  FirstImageRecordCodeSection->CodeSegmentSize = SecondImageRecordCodeSection->CodeSegmentSize;

  SecondImageRecordCodeSection->CodeSegmentBase = TempImageRecordCodeSection.CodeSegmentBase;
  SecondImageRecordCodeSection->CodeSegmentSize = TempImageRecordCodeSection.CodeSegmentSize;
}

/**
  Sort code section in image record, based upon CodeSegmentBase from low to high.

  @param  ImageRecord    image record to be sorted
**/
VOID
EFIAPI
SortImageRecordCodeSection (
  IN IMAGE_PROPERTIES_RECORD  *ImageRecord
  )
{
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *NextImageRecordCodeSection;
  LIST_ENTRY                            *ImageRecordCodeSectionLink;
  LIST_ENTRY                            *NextImageRecordCodeSectionLink;
  LIST_ENTRY                            *ImageRecordCodeSectionEndLink;
  LIST_ENTRY                            *ImageRecordCodeSectionList;

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
        SwapImageRecordCodeSection (ImageRecordCodeSection, NextImageRecordCodeSection);
      }

      NextImageRecordCodeSectionLink = NextImageRecordCodeSectionLink->ForwardLink;
    }

    ImageRecordCodeSectionLink     = ImageRecordCodeSectionLink->ForwardLink;
    NextImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink;
  }
}

/**
  Check if code section in image record is valid.

  @param  ImageRecord    image record to be checked

  @retval TRUE  image record is valid
  @retval FALSE image record is invalid
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

  @param  FirstImageRecord   first image record.
  @param  SecondImageRecord  second image record.
**/
VOID
EFIAPI
SwapImageRecord (
  IN IMAGE_PROPERTIES_RECORD  *FirstImageRecord,
  IN IMAGE_PROPERTIES_RECORD  *SecondImageRecord
  )
{
  IMAGE_PROPERTIES_RECORD  TempImageRecord;

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
}

/**
  Sort image record based upon the ImageBase from low to high.

  @param ImageRecordList    Image record list to be sorted
**/
VOID
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
        SwapImageRecord (ImageRecord, NextImageRecord);
      }

      NextImageRecordLink = NextImageRecordLink->ForwardLink;
    }

    ImageRecordLink     = ImageRecordLink->ForwardLink;
    NextImageRecordLink = ImageRecordLink->ForwardLink;
  }
}

/**
  Find image record according to image base and size.

  @param  ImageBase           Base of PE image
  @param  ImageSize           Size of PE image
  @param  ImageRecordList     Image record list to be searched

  @return image record
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
