/** @file

  Provides definitions and functionality for manipulating IMAGE_PROPERTIES_RECORD.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef IMAGE_PROPERTIES_RECORD_SUPPORT_LIB_H_
#define IMAGE_PROPERTIES_RECORD_SUPPORT_LIB_H_

#define IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE  SIGNATURE_32 ('I','P','R','C')

typedef struct {
  UINT32                  Signature;
  LIST_ENTRY              Link;
  EFI_PHYSICAL_ADDRESS    CodeSegmentBase;
  UINT64                  CodeSegmentSize;
} IMAGE_PROPERTIES_RECORD_CODE_SECTION;

#define IMAGE_PROPERTIES_RECORD_SIGNATURE  SIGNATURE_32 ('I','P','R','D')

typedef struct {
  UINT32                  Signature;
  LIST_ENTRY              Link;
  EFI_PHYSICAL_ADDRESS    ImageBase;
  UINT64                  ImageSize;
  UINTN                   CodeSegmentCount;
  LIST_ENTRY              CodeSegmentList;
} IMAGE_PROPERTIES_RECORD;

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
  );

/**
  Sort code section in image record, based upon CodeSegmentBase from low to high.

  @param  ImageRecord    image record to be sorted
**/
VOID
EFIAPI
SortImageRecordCodeSection (
  IN IMAGE_PROPERTIES_RECORD  *ImageRecord
  );

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
  );

/**
  Sort image record based upon the ImageBase from low to high.

  @param ImageRecordList    Image record list to be sorted
**/
VOID
EFIAPI
SortImageRecord (
  IN LIST_ENTRY  *ImageRecordList
  );

/**
  Swap two image records.

  @param[in]  FirstImageRecord   The first image record.
  @param[in]  SecondImageRecord  The second image record.
**/
VOID
EFIAPI
SwapImageRecord (
  IN IMAGE_PROPERTIES_RECORD  *FirstImageRecord,
  IN IMAGE_PROPERTIES_RECORD  *SecondImageRecord
  );

/**
  Swap two code sections in a single IMAGE_PROPERTIES_RECORD.

  @param[in]  FirstImageRecordCodeSection    The first code section
  @param[in]  SecondImageRecordCodeSection   The second code section
**/
VOID
EFIAPI
SwapImageRecordCodeSection (
  IN IMAGE_PROPERTIES_RECORD_CODE_SECTION  *FirstImageRecordCodeSection,
  IN IMAGE_PROPERTIES_RECORD_CODE_SECTION  *SecondImageRecordCodeSection
  );

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
  );

#endif
