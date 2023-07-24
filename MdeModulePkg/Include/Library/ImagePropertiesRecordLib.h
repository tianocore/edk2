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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Find image properties record according to image base and size in the
  input ImageRecordList.

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
  );

/**
  Debug dumps the input list of IMAGE_PROPERTIES_RECORD structs.

  @param[in]  ImageRecordList   Head of the IMAGE_PROPERTIES_RECORD list
**/
VOID
EFIAPI
DumpImageRecords (
  IN LIST_ENTRY  *ImageRecordList
  );

/**
  Creates an IMAGE_PROPERTIES_RECORD from a loaded PE image. The PE/COFF header will be found
  and parsed to determine the number of code segments and their base addresses and sizes.

  @param[in]      ImageBase               Base of the PE image
  @param[in]      ImageSize               Size of the PE image
  @param[in]      RequiredAlignment       If non-NULL, the alignment specified in the PE/COFF header
                                          will be compared against this value.
  @param[out]     ImageRecord             On out, a populated image properties record

  @retval     EFI_INVALID_PARAMETER   This function ImageBase or ImageRecord was NULL, or the
                                      image located at ImageBase was not a valid PE/COFF image
  @retval     EFI_OUT_OF_RESOURCES    Failure to Allocate()
  @retval     EFI_ABORTED             The input Alignment was non-NULL and did not match the
                                      alignment specified in the PE/COFF header
  @retval     EFI_SUCCESS             The image properties record was successfully created
**/
EFI_STATUS
EFIAPI
CreateImagePropertiesRecord (
  IN  CONST   VOID                     *ImageBase,
  IN  CONST   UINT64                   ImageSize,
  IN  CONST   UINT32                   *Alignment OPTIONAL,
  OUT         IMAGE_PROPERTIES_RECORD  *ImageRecord
  );

/**
  Deleted an image properties record. The function will also call
  RemoveEntryList() on each code segment and the input ImageRecord before
  freeing each pool.

  @param[in]      ImageRecord             The IMAGE_PROPERTIES_RECORD to delete
**/
VOID
EFIAPI
DeleteImagePropertiesRecord (
  IN  IMAGE_PROPERTIES_RECORD  *ImageRecord
  );

#endif
