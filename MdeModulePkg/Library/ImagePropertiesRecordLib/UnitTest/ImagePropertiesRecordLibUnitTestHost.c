/** @file
  Unit tests the SplitTable() ImagePropertiesRecordLib Logic

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>
#include <Library/ImagePropertiesRecordLib.h>

#define UNIT_TEST_APP_NAME     "Image Properties Record Lib Unit Test"
#define UNIT_TEST_APP_VERSION  "1.0"

#define NEXT_MEMORY_DESCRIPTOR(MemoryDescriptor, Size) \
  ((EFI_MEMORY_DESCRIPTOR *)((UINT8 *)(MemoryDescriptor) + (Size)))

// The starting memory map will contain 6 entries
#define NUMBER_OF_MEMORY_MAP_DESCRIPTORS  6

// Each memory map descriptor will be the sizeof(EFI_MEMORY_DESCRIPTOR) instead of a nonstandard size
// to catch pointer math issues
#define DESCRIPTOR_SIZE  sizeof(EFI_MEMORY_DESCRIPTOR)

// Each memory map descriptor will describe 12 pages
#define BASE_DESCRIPTOR_NUMBER_OF_PAGES  0x0C

// The size, in bytes, of each memory map descriptor range
#define BASE_DESCRIPTOR_ENTRY_SIZE  (EFI_PAGES_TO_SIZE(BASE_DESCRIPTOR_NUMBER_OF_PAGES))

// MACRO to get the starting address of a descriptor's described range based on the index of that descriptor
#define BASE_DESCRIPTOR_START_ADDRESS(DescriptorNumber)  (DescriptorNumber * BASE_DESCRIPTOR_ENTRY_SIZE)

// Virtual start must be zero
#define BASE_DESCRIPTOR_VIRTUAL_START  0x0

// Size of the default memory map
#define BASE_MEMORY_MAP_SIZE  (NUMBER_OF_MEMORY_MAP_DESCRIPTORS * DESCRIPTOR_SIZE)

// Number of images in each test case
#define NUMBER_OF_IMAGES_TO_SPLIT  3

// Maximum number of descriptors required for each image (None->Data->Code->Data->Code->Data->None)
#define MAX_DESCRIPTORS_PER_IMAGE  7

// Number of unused additional descriptors in the starting memory map buffer which is used by the
// SplitTable() logic
#define NUMBER_OF_ADDITIONAL_DESCRIPTORS  (NUMBER_OF_IMAGES_TO_SPLIT * MAX_DESCRIPTORS_PER_IMAGE)

// Size of the memory map with enough space for the starting descriptors and the split descriptors
#define SPLIT_MEMORY_MAP_SIZE  (BASE_MEMORY_MAP_SIZE + (NUMBER_OF_ADDITIONAL_DESCRIPTORS * DESCRIPTOR_SIZE))

typedef enum {
  SectionTypeCode,
  SectionTypeData,
  SectionTypeNotFound
} SECTION_TYPE;

typedef struct {
  EFI_MEMORY_DESCRIPTOR    *MemoryMap;
  LIST_ENTRY               ImageList;
} IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT;

EFI_MEMORY_DESCRIPTOR  BaseMemoryMap[] = {
  {
    EfiConventionalMemory,             // Type
    BASE_DESCRIPTOR_START_ADDRESS (0), // PhysicalStart
    BASE_DESCRIPTOR_VIRTUAL_START,     // VirtualStart
    BASE_DESCRIPTOR_NUMBER_OF_PAGES,   // Number of Pages
    0                                  // Attribute
  },
  {
    EfiConventionalMemory,             // Type
    BASE_DESCRIPTOR_START_ADDRESS (1), // PhysicalStart
    BASE_DESCRIPTOR_VIRTUAL_START,     // VirtualStart
    BASE_DESCRIPTOR_NUMBER_OF_PAGES,   // Number of Pages
    0                                  // Attribute
  },
  {
    EfiConventionalMemory,             // Type
    BASE_DESCRIPTOR_START_ADDRESS (2), // PhysicalStart
    BASE_DESCRIPTOR_VIRTUAL_START,     // VirtualStart
    BASE_DESCRIPTOR_NUMBER_OF_PAGES,   // Number of Pages
    0                                  // Attribute
  },
  {
    EfiConventionalMemory,             // Type
    BASE_DESCRIPTOR_START_ADDRESS (3), // PhysicalStart
    BASE_DESCRIPTOR_VIRTUAL_START,     // VirtualStart
    BASE_DESCRIPTOR_NUMBER_OF_PAGES,   // Number of Pages
    0                                  // Attribute
  },
  {
    EfiConventionalMemory,             // Type
    BASE_DESCRIPTOR_START_ADDRESS (4), // PhysicalStart
    BASE_DESCRIPTOR_VIRTUAL_START,     // VirtualStart
    BASE_DESCRIPTOR_NUMBER_OF_PAGES,   // Number of Pages
    0                                  // Attribute
  },
  {
    EfiConventionalMemory,             // Type
    BASE_DESCRIPTOR_START_ADDRESS (5), // PhysicalStart
    BASE_DESCRIPTOR_VIRTUAL_START,     // VirtualStart
    BASE_DESCRIPTOR_NUMBER_OF_PAGES,   // Number of Pages
    0                                  // Attribute
  }
};

/**
  Returns a bitmap where one bit is set for each section in the image list. For example, if
  there are 3 images and each image 3 sections the returned bitmap will be 111111111.

  @param[in]  ImageRecordList   A list of IMAGE_PROPERTIES_RECORD entries

  @retval A bitmap such that the most significant bit is the number of sections
          in all images and every bit between 0 -> MSB is set

**/
STATIC
UINT64
GetImageSectionBitmap (
  IN LIST_ENTRY  *ImageRecordList
  )
{
  IMAGE_PROPERTIES_RECORD               *ImageRecord;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;
  LIST_ENTRY                            *ImageRecordLink;
  LIST_ENTRY                            *ImageRecordCodeSectionLink;
  EFI_PHYSICAL_ADDRESS                  SectionBase;
  UINT64                                ReturnBitmap;
  UINT64                                Shift;

  if (ImageRecordList == NULL) {
    return 0;
  }

  ReturnBitmap = 0;
  Shift        = 0;

  // Walk through each image record
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

    SectionBase = ImageRecord->ImageBase;

    // Walk through each code entry
    for (ImageRecordCodeSectionLink = ImageRecord->CodeSegmentList.ForwardLink;
         ImageRecordCodeSectionLink != &ImageRecord->CodeSegmentList;
         ImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink)
    {
      ImageRecordCodeSection = CR (
                                 ImageRecordCodeSectionLink,
                                 IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                                 Link,
                                 IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                                 );

      // Check for data region before the code section base
      if (SectionBase < ImageRecordCodeSection->CodeSegmentBase) {
        ReturnBitmap |= LShiftU64 (1, Shift++);
      }

      // Code section
      ReturnBitmap |= LShiftU64 (1, Shift++);
      SectionBase   = ImageRecordCodeSection->CodeSegmentBase + ImageRecordCodeSection->CodeSegmentSize;
    }

    // Check for data region after the previous code section
    if (SectionBase < (ImageRecord->ImageBase + ImageRecord->ImageSize)) {
      ReturnBitmap |= LShiftU64 (1, Shift++);
    }
  }

  return ReturnBitmap;
}

/**
  Searches the input image list for a section which exactly matches the memory range Buffer -> Buffer + Length.

  @param[in] Buffer           Start Address to check
  @param[in] Length           Length to check
  @param[out] Type             The type of the section which corresponds with the memory
                              range Buffer -> Buffer + Length (Code or Data) or SectionTypeNotFound
                              if no image section matches the memory range
  @param[in] ImageRecordList  A list of IMAGE_PROPERTIES_RECORD entries to check against
                              the memory range Buffer -> Buffer + Length

  @retval A bitmap with a single bit set (1 << Shift) where Shift corresponds with the number of sections inspected
          in the image list before arriving at the section matching the memory range Buffer -> Buffer + Length
**/
STATIC
UINT64
MatchDescriptorToImageSection (
  IN  EFI_PHYSICAL_ADDRESS  Buffer,
  IN  UINT64                Length,
  OUT SECTION_TYPE          *Type,
  IN  LIST_ENTRY            *ImageRecordList
  )
{
  IMAGE_PROPERTIES_RECORD               *ImageRecord;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *ImageRecordCodeSection;
  LIST_ENTRY                            *ImageRecordLink;
  LIST_ENTRY                            *ImageRecordCodeSectionLink;
  EFI_PHYSICAL_ADDRESS                  SectionBase;
  UINT8                                 Shift;

  Shift = 0;

  if (ImageRecordList == NULL) {
    return 1;
  }

  // Walk through each image record
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

    SectionBase = ImageRecord->ImageBase;

    // Walk through each code entry
    for (ImageRecordCodeSectionLink = ImageRecord->CodeSegmentList.ForwardLink;
         ImageRecordCodeSectionLink != &ImageRecord->CodeSegmentList;
         ImageRecordCodeSectionLink = ImageRecordCodeSectionLink->ForwardLink)
    {
      ImageRecordCodeSection = CR (
                                 ImageRecordCodeSectionLink,
                                 IMAGE_PROPERTIES_RECORD_CODE_SECTION,
                                 Link,
                                 IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE
                                 );

      if (SectionBase < ImageRecordCodeSection->CodeSegmentBase) {
        // Check the data region before the code section base
        if ((Buffer == SectionBase) &&
            (Length == ImageRecordCodeSection->CodeSegmentBase - SectionBase))
        {
          *Type = SectionTypeData;
          return LShiftU64 (1, Shift);
        }

        Shift++;
      }

      // Check the code region
      if ((Buffer == ImageRecordCodeSection->CodeSegmentBase) &&
          (Length == ImageRecordCodeSection->CodeSegmentSize))
      {
        *Type = SectionTypeCode;
        return LShiftU64 (1, Shift);
      }

      Shift++;
      SectionBase = ImageRecordCodeSection->CodeSegmentBase + ImageRecordCodeSection->CodeSegmentSize;
    }

    // Check the data region after the code section
    if (SectionBase < (ImageRecord->ImageBase + ImageRecord->ImageSize)) {
      if ((Buffer == SectionBase) &&
          (Length == (ImageRecord->ImageBase + ImageRecord->ImageSize) - SectionBase))
      {
        *Type = SectionTypeData;
        return LShiftU64 (1, Shift);
      }

      Shift++;
    }
  }

  // No image sections match
  *Type = SectionTypeNotFound;
  return 0;
}

/**
  Walks through the input memory map and checks that every memory descriptor with an attribute matches
  an image in ImageRecordList.

  @param[in]        MemoryMapSize                 The size, in bytes, of the memory map
  @param[in]        MemoryMap                     A pointer to the buffer containing the memory map
  @param[in]        ImageRecordList               A list of IMAGE_PROPERTIES_RECORD entries

  @retval TRUE if all memory descriptors with attributes match an image section and have the correct attributes

**/
STATIC
BOOLEAN
IsMemoryMapValid (
  IN UINTN                  MemoryMapSize,
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN LIST_ENTRY             *ImageRecordList
  )
{
  UINT64        ImageSectionsBitmap;
  UINT64        ReturnSectionBitmask;
  UINT64        NumberOfDescriptors;
  UINT8         Index;
  SECTION_TYPE  Type;

  Index               = 0;
  NumberOfDescriptors = MemoryMapSize / DESCRIPTOR_SIZE;

  UT_ASSERT_EQUAL (MemoryMapSize % DESCRIPTOR_SIZE, 0);
  UT_ASSERT_NOT_NULL (MemoryMap);
  UT_ASSERT_NOT_NULL (ImageRecordList);

  // The returned bitmap will have one bit is set for each section in the image list.
  // If there are 3 images and 3 sections each image, the resulting bitmap will
  // be 0000000000000000000000000000000000000000000000000000000111111111. Flipping that bitmap
  // results in 1111111111111111111111111111111111111111111111111111111000000000. The return value
  // of each iteration through MatchDescriptorToImageSection() is one set bit corrosponding to the number
  // of sections before finding the section which matched the descriptor memory range which we
  // OR with ImageSectionsBitmap. If, at the end of the loop, every bit in ImageSectionsBitmap is set,
  // we must have matched every image in ImageRecordList with a descriptor in the memory map which has
  // nonzero attributes.
  ImageSectionsBitmap = ~GetImageSectionBitmap (ImageRecordList);

  // For each descriptor in the memory map
  for ( ; Index < NumberOfDescriptors; Index++) {
    if (MemoryMap[Index].Attribute != 0) {
      ReturnSectionBitmask = MatchDescriptorToImageSection (
                               MemoryMap[Index].PhysicalStart,
                               EFI_PAGES_TO_SIZE (MemoryMap[Index].NumberOfPages),
                               &Type,
                               ImageRecordList
                               );

      // Make sure the attributes of the descriptor match the returned section type.
      // DATA sections should have execution protection and CODE sections should have
      // write protection.
      if ((Type == SectionTypeNotFound) ||
          ((Type == SectionTypeData) && (MemoryMap[Index].Attribute == EFI_MEMORY_RP)) ||
          ((Type == SectionTypeCode) && (MemoryMap[Index].Attribute == EFI_MEMORY_XP)))
      {
        return FALSE;
      }

      // If the bit associated with image found has already been set, then there must be a duplicate
      // in the memory map meaning it is invalid.
      UT_ASSERT_EQUAL (ImageSectionsBitmap & ReturnSectionBitmask, 0);

      ImageSectionsBitmap |= ReturnSectionBitmask;
    }
  }

  // If every bit in ImageSectionsBitmap is set, the return value will be TRUE
  return !(~ImageSectionsBitmap);
}

/**
  Separate the image sections in the memory map and run a check to ensure the output is valid.

  @param[in]  Context   Context containing the memory map and image record pointers

  @retval TRUE if the memory map is split correctly
**/
STATIC
BOOLEAN
SeparateAndCheck (
  IN IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT  *Context
  )
{
  UINTN  MemoryMapSize;

  MemoryMapSize = BASE_MEMORY_MAP_SIZE;

  // Separate the memory map so each image section has its own descriptor
  SplitTable (
    &MemoryMapSize,
    Context->MemoryMap,
    DESCRIPTOR_SIZE,
    &Context->ImageList,
    NUMBER_OF_ADDITIONAL_DESCRIPTORS
    );

  // Ensure the updated memory map is valid
  return IsMemoryMapValid (MemoryMapSize, Context->MemoryMap, &Context->ImageList);
}

/**
  Test the case where the image range contains multiple code sections and does not perfectly align with
  the existing memory descriptor.

  @param[in]  Context   Context containing the memory map and image record pointers

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
MaxOutAdditionalDescriptors (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  IMAGE_PROPERTIES_RECORD                    *Image1;
  IMAGE_PROPERTIES_RECORD                    *Image2;
  IMAGE_PROPERTIES_RECORD                    *Image3;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage1;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage2;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage3;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *AddCodeSectionInImage1;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *AddCodeSectionInImage2;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *AddCodeSectionInImage3;
  IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT  *TestContext;

  TestContext = (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT *)Context;

  Image1 = CR (TestContext->ImageList.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);
  Image2 = CR (Image1->Link.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);
  Image3 = CR (Image2->Link.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);

  CodeSectionInImage1 = CR (Image1->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);
  CodeSectionInImage2 = CR (Image2->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);
  CodeSectionInImage3 = CR (Image3->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);

  ///////////////
  // Descriptor 1
  ///////////////
  // |         |      |      |      |      |      |             |
  // | 4K PAGE | DATA | CODE | DATA | CODE | DATA | 4K PAGE * 5 |
  // |         |      |      |      |      |      |             |

  Image1->ImageBase                    = BASE_DESCRIPTOR_START_ADDRESS (1) + EFI_PAGE_SIZE;
  Image1->ImageSize                    = BASE_DESCRIPTOR_ENTRY_SIZE - EFI_PAGE_SIZE - EFI_PAGE_SIZE;
  Image1->CodeSegmentCount             = 2;
  CodeSectionInImage1->CodeSegmentBase = Image1->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage1->CodeSegmentSize = EFI_PAGE_SIZE;
  TestContext->MemoryMap[1].Type       = EfiBootServicesCode;

  AddCodeSectionInImage1                  = AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD_CODE_SECTION));
  AddCodeSectionInImage1->Signature       = IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE;
  AddCodeSectionInImage1->CodeSegmentBase = CodeSectionInImage1->CodeSegmentBase + CodeSectionInImage1->CodeSegmentSize + EFI_PAGE_SIZE;
  AddCodeSectionInImage1->CodeSegmentSize = EFI_PAGE_SIZE;

  InsertTailList (&Image1->CodeSegmentList, &AddCodeSectionInImage1->Link);

  ///////////////
  // Descriptor 2
  ///////////////
  // |         |      |      |      |      |      |             |
  // | 4K PAGE | DATA | CODE | DATA | CODE | DATA | 4K PAGE * 5 |
  // |         |      |      |      |      |      |             |

  Image2->ImageBase                    = BASE_DESCRIPTOR_START_ADDRESS (2) + EFI_PAGE_SIZE;
  Image2->ImageSize                    = BASE_DESCRIPTOR_ENTRY_SIZE - EFI_PAGE_SIZE - EFI_PAGE_SIZE;
  Image2->CodeSegmentCount             = 2;
  CodeSectionInImage2->CodeSegmentBase = Image2->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage2->CodeSegmentSize = EFI_PAGE_SIZE;
  TestContext->MemoryMap[2].Type       = EfiLoaderCode;

  AddCodeSectionInImage2                  = AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD_CODE_SECTION));
  AddCodeSectionInImage2->Signature       = IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE;
  AddCodeSectionInImage2->CodeSegmentBase = CodeSectionInImage2->CodeSegmentBase + CodeSectionInImage2->CodeSegmentSize + EFI_PAGE_SIZE;
  AddCodeSectionInImage2->CodeSegmentSize = EFI_PAGE_SIZE;

  InsertTailList (&Image2->CodeSegmentList, &AddCodeSectionInImage2->Link);

  ///////////////
  // Descriptor 3
  ///////////////
  // |         |      |      |      |      |      |             |
  // | 4K PAGE | DATA | CODE | DATA | CODE | DATA | 4K PAGE * 5 |
  // |         |      |      |      |      |      |             |

  Image3->ImageBase                    = BASE_DESCRIPTOR_START_ADDRESS (3) + EFI_PAGE_SIZE;
  Image3->ImageSize                    = BASE_DESCRIPTOR_ENTRY_SIZE - EFI_PAGE_SIZE - EFI_PAGE_SIZE;
  Image3->CodeSegmentCount             = 2;
  CodeSectionInImage3->CodeSegmentBase = Image3->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage3->CodeSegmentSize = EFI_PAGE_SIZE;
  TestContext->MemoryMap[3].Type       = EfiRuntimeServicesCode;

  AddCodeSectionInImage3                  = AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD_CODE_SECTION));
  AddCodeSectionInImage3->Signature       = IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE;
  AddCodeSectionInImage3->CodeSegmentBase = CodeSectionInImage3->CodeSegmentBase + CodeSectionInImage3->CodeSegmentSize + EFI_PAGE_SIZE;
  AddCodeSectionInImage3->CodeSegmentSize = EFI_PAGE_SIZE;

  InsertTailList (&Image3->CodeSegmentList, &AddCodeSectionInImage3->Link);

  UT_ASSERT_TRUE (SeparateAndCheck (TestContext));

  return UNIT_TEST_PASSED;
}

/**
  Test the case where multiple image ranges lie within an existing memory descriptor.

  @param[in]  Context   Context containing the memory map and image record pointers

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
MultipleImagesInOneDescriptor (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  IMAGE_PROPERTIES_RECORD                    *Image1;
  IMAGE_PROPERTIES_RECORD                    *Image2;
  IMAGE_PROPERTIES_RECORD                    *Image3;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage1;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage2;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage3;
  IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT  *TestContext;

  TestContext = (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT *)Context;

  Image1 = CR (TestContext->ImageList.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);
  Image2 = CR (Image1->Link.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);
  Image3 = CR (Image2->Link.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);

  CodeSectionInImage1 = CR (Image1->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);
  CodeSectionInImage2 = CR (Image2->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);
  CodeSectionInImage3 = CR (Image3->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);

  ///////////////
  // Descriptor 1
  ///////////////
  // |         |      |      |      |         |      |      |      |      |      |      |         |
  // | 4K PAGE | DATA | CODE | DATA | 4K PAGE | DATA | CODE | DATA | DATA | CODE | DATA | 4K PAGE |
  // |         |      |      |      |         |      |      |      |      |      |      |         |

  Image1->ImageBase                    = BASE_DESCRIPTOR_START_ADDRESS (1) + EFI_PAGE_SIZE;
  Image1->ImageSize                    = EFI_PAGES_TO_SIZE (3);
  Image1->CodeSegmentCount             = 1;
  CodeSectionInImage1->CodeSegmentBase = Image1->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage1->CodeSegmentSize = EFI_PAGE_SIZE;
  TestContext->MemoryMap[1].Type       = EfiBootServicesCode;

  Image2->ImageBase                    = Image1->ImageBase + Image1->ImageSize + EFI_PAGE_SIZE;
  Image2->ImageSize                    = EFI_PAGES_TO_SIZE (3);
  Image2->CodeSegmentCount             = 1;
  CodeSectionInImage2->CodeSegmentBase = Image2->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage2->CodeSegmentSize = EFI_PAGE_SIZE;

  Image3->ImageBase                    = Image2->ImageBase + Image2->ImageSize;
  Image3->ImageSize                    = EFI_PAGES_TO_SIZE (3);
  Image3->CodeSegmentCount             = 1;
  CodeSectionInImage3->CodeSegmentBase = Image3->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage3->CodeSegmentSize = EFI_PAGE_SIZE;

  UT_ASSERT_TRUE (SeparateAndCheck (TestContext));

  return UNIT_TEST_PASSED;
}

/**
  Test the case where all image ranges do not fit perfectly within an existing memory descriptor.

  @param[in]  Context   Context containing the memory map and image record pointers

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
ImagesDontFitDescriptors (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  IMAGE_PROPERTIES_RECORD                    *Image1;
  IMAGE_PROPERTIES_RECORD                    *Image2;
  IMAGE_PROPERTIES_RECORD                    *Image3;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage1;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage2;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage3;
  IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT  *TestContext;

  TestContext = (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT *)Context;

  Image1 = CR (TestContext->ImageList.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);
  Image2 = CR (Image1->Link.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);
  Image3 = CR (Image2->Link.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);

  CodeSectionInImage1 = CR (Image1->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);
  CodeSectionInImage2 = CR (Image2->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);
  CodeSectionInImage3 = CR (Image3->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);

  ///////////////
  // Descriptor 1
  ///////////////
  // |         |      |          |          |
  // | 4K PAGE | DATA | CODE * 2 | DATA * 8 |
  // |         |      |          |          |

  Image1->ImageBase                    = BASE_DESCRIPTOR_START_ADDRESS (1) + EFI_PAGE_SIZE;
  Image1->ImageSize                    = BASE_DESCRIPTOR_ENTRY_SIZE - EFI_PAGE_SIZE;
  Image1->CodeSegmentCount             = 1;
  CodeSectionInImage1->CodeSegmentBase = Image1->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage1->CodeSegmentSize = EFI_PAGES_TO_SIZE (2);
  TestContext->MemoryMap[1].Type       = EfiBootServicesCode;

  ///////////////
  // Descriptor 3
  ///////////////
  // |      |          |          |         |
  // | DATA | CODE * 3 | DATA * 7 | 4K PAGE |
  // |      |          |          |         |

  Image2->ImageBase                    = BASE_DESCRIPTOR_START_ADDRESS (3);
  Image2->ImageSize                    = BASE_DESCRIPTOR_ENTRY_SIZE - EFI_PAGE_SIZE;
  Image2->CodeSegmentCount             = 1;
  CodeSectionInImage2->CodeSegmentBase = Image2->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage2->CodeSegmentSize = EFI_PAGES_TO_SIZE (3);
  TestContext->MemoryMap[3].Type       = EfiLoaderCode;

  ///////////////
  // Descriptor 4
  ///////////////
  // |         |      |          |          |         |
  // | 4K PAGE | DATA | CODE * 2 | DATA * 7 | 4K PAGE |
  // |         |      |          |          |         |

  Image3->ImageBase                    = BASE_DESCRIPTOR_START_ADDRESS (4) + EFI_PAGE_SIZE;
  Image3->ImageSize                    = BASE_DESCRIPTOR_ENTRY_SIZE - EFI_PAGE_SIZE - EFI_PAGE_SIZE;
  Image3->CodeSegmentCount             = 1;
  CodeSectionInImage3->CodeSegmentBase = Image3->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage3->CodeSegmentSize = EFI_PAGES_TO_SIZE (2);
  TestContext->MemoryMap[4].Type       = EfiRuntimeServicesCode;

  UT_ASSERT_TRUE (SeparateAndCheck (TestContext));

  return UNIT_TEST_PASSED;
}

/**
  Test the case where all image ranges fit perfectly within an existing memory descriptor.

  @param[in]  Context   Context containing the memory map and image record pointers

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
ImagesFitDescriptors (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  IMAGE_PROPERTIES_RECORD                    *Image1;
  IMAGE_PROPERTIES_RECORD                    *Image2;
  IMAGE_PROPERTIES_RECORD                    *Image3;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage1;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage2;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *CodeSectionInImage3;
  IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT  *TestContext;

  TestContext = (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT *)Context;

  Image1 = CR (TestContext->ImageList.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);
  Image2 = CR (Image1->Link.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);
  Image3 = CR (Image2->Link.ForwardLink, IMAGE_PROPERTIES_RECORD, Link, IMAGE_PROPERTIES_RECORD_SIGNATURE);

  CodeSectionInImage1 = CR (Image1->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);
  CodeSectionInImage2 = CR (Image2->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);
  CodeSectionInImage3 = CR (Image3->CodeSegmentList.ForwardLink, IMAGE_PROPERTIES_RECORD_CODE_SECTION, Link, IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE);

  ///////////////
  // Descriptor 1
  ///////////////
  // |      |          |          |
  // | DATA | CODE * 3 | DATA * 8 |
  // |      |          |          |

  Image1->ImageBase                    = BASE_DESCRIPTOR_START_ADDRESS (1);
  Image1->ImageSize                    = BASE_DESCRIPTOR_ENTRY_SIZE;
  Image1->CodeSegmentCount             = 1;
  CodeSectionInImage1->CodeSegmentBase = Image1->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage1->CodeSegmentSize = EFI_PAGES_TO_SIZE (3);
  TestContext->MemoryMap[1].Type       = EfiBootServicesCode;

  ///////////////
  // Descriptor 2
  ///////////////
  // |      |          |          |
  // | DATA | CODE * 4 | DATA * 7 |
  // |      |          |          |

  Image2->ImageBase                    = BASE_DESCRIPTOR_START_ADDRESS (2);
  Image2->ImageSize                    = BASE_DESCRIPTOR_ENTRY_SIZE;
  Image2->CodeSegmentCount             = 1;
  CodeSectionInImage2->CodeSegmentBase = Image2->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage2->CodeSegmentSize = EFI_PAGES_TO_SIZE (4);
  TestContext->MemoryMap[2].Type       = EfiLoaderCode;

  ///////////////
  // Descriptor 3
  ///////////////
  // |      |          |          |
  // | DATA | CODE * 3 | DATA * 8 |
  // |      |          |          |

  Image3->ImageBase                    = BASE_DESCRIPTOR_START_ADDRESS (3);
  Image3->ImageSize                    = BASE_DESCRIPTOR_ENTRY_SIZE;
  Image3->CodeSegmentCount             = 1;
  CodeSectionInImage3->CodeSegmentBase = Image3->ImageBase + EFI_PAGE_SIZE;
  CodeSectionInImage3->CodeSegmentSize = EFI_PAGES_TO_SIZE (3);
  TestContext->MemoryMap[3].Type       = EfiRuntimeServicesCode;

  UT_ASSERT_TRUE (SeparateAndCheck (TestContext));

  return UNIT_TEST_PASSED;
}

/**
  Free all allocated memory.

  @param[in]  Context   Context containing the memory map and image record pointers
**/
VOID
EFIAPI
TestCleanup (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  IMAGE_PROPERTIES_RECORD                    *ImageRecord;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION       *ImageRecordCodeSection;
  LIST_ENTRY                                 *ImageRecordLink;
  LIST_ENTRY                                 *CodeSegmentListHead;
  IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT  *TestContext;

  TestContext     = (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT *)Context;
  ImageRecordLink = &TestContext->ImageList;

  while (!IsListEmpty (ImageRecordLink)) {
    ImageRecord = CR (
                    ImageRecordLink->ForwardLink,
                    IMAGE_PROPERTIES_RECORD,
                    Link,
                    IMAGE_PROPERTIES_RECORD_SIGNATURE
                    );

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
  }

  if (TestContext->MemoryMap != NULL) {
    FreePool (TestContext->MemoryMap);
  }
}

/**
  Create a generic image list with the proper signatures which will be customized for each test
  and allocate the default memory map.

  @param[out]  TestContext   Context which will be passed to the test cases
**/
STATIC
VOID
CreateBaseContextEntry (
  OUT IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT  *TestContext
  )
{
  IMAGE_PROPERTIES_RECORD               *Image1;
  IMAGE_PROPERTIES_RECORD               *Image2;
  IMAGE_PROPERTIES_RECORD               *Image3;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *CodeSectionInImage1;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *CodeSectionInImage2;
  IMAGE_PROPERTIES_RECORD_CODE_SECTION  *CodeSectionInImage3;

  InitializeListHead (&TestContext->ImageList);

  Image1              = AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD));
  CodeSectionInImage1 = AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD_CODE_SECTION));

  Image1->Signature              = IMAGE_PROPERTIES_RECORD_SIGNATURE;
  CodeSectionInImage1->Signature = IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE;
  InitializeListHead (&Image1->CodeSegmentList);

  InsertTailList (&TestContext->ImageList, &Image1->Link);
  InsertTailList (&Image1->CodeSegmentList, &CodeSectionInImage1->Link);

  Image2              = AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD));
  CodeSectionInImage2 = AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD_CODE_SECTION));

  Image2->Signature              = IMAGE_PROPERTIES_RECORD_SIGNATURE;
  CodeSectionInImage2->Signature = IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE;
  InitializeListHead (&Image2->CodeSegmentList);

  InsertTailList (&TestContext->ImageList, &Image2->Link);
  InsertTailList (&Image2->CodeSegmentList, &CodeSectionInImage2->Link);

  Image3              = AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD));
  CodeSectionInImage3 = AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD_CODE_SECTION));

  Image3->Signature              = IMAGE_PROPERTIES_RECORD_SIGNATURE;
  CodeSectionInImage3->Signature = IMAGE_PROPERTIES_RECORD_CODE_SECTION_SIGNATURE;
  InitializeListHead (&Image3->CodeSegmentList);

  InsertTailList (&TestContext->ImageList, &Image3->Link);
  InsertTailList (&Image3->CodeSegmentList, &CodeSectionInImage3->Link);

  TestContext->MemoryMap = AllocateZeroPool (SPLIT_MEMORY_MAP_SIZE);
  CopyMem (TestContext->MemoryMap, &BaseMemoryMap, BASE_MEMORY_MAP_SIZE);

  return;
}

/**
  Initialze the unit test framework, suite, and unit tests.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                                 Status;
  UNIT_TEST_FRAMEWORK_HANDLE                 Framework;
  UNIT_TEST_SUITE_HANDLE                     ImagePropertiesRecordTests;
  IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT  *Context1;
  IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT  *Context2;
  IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT  *Context3;
  IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT  *Context4;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  Framework = NULL;

  Context1 = (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT *)AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT));
  Context2 = (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT *)AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT));
  Context3 = (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT *)AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT));
  Context4 = (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT *)AllocateZeroPool (sizeof (IMAGE_PROPERTIES_RECORD_HOST_TEST_CONTEXT));

  CreateBaseContextEntry (Context1);
  CreateBaseContextEntry (Context2);
  CreateBaseContextEntry (Context3);
  CreateBaseContextEntry (Context4);

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&ImagePropertiesRecordTests, Framework, "Image Properties Record Tests", "ImagePropertiesRecordLib.SplitTable", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for the Image Properties Record Tests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // --------------Suite-----------Description--------------Name----------Function--------Pre---Post-------------------Context-----------
  //
  AddTestCase (ImagePropertiesRecordTests, "All images fit perfectly into existing descriptors", "ImagesFitDescriptors", ImagesFitDescriptors, NULL, TestCleanup, Context1);
  AddTestCase (ImagePropertiesRecordTests, "All images don't fit perfectly into existing descriptors", "ImagesDontFitDescriptors", ImagesDontFitDescriptors, NULL, TestCleanup, Context2);
  AddTestCase (ImagePropertiesRecordTests, "All Images are contined In single descriptor", "MultipleImagesInOneDescriptor", MultipleImagesInOneDescriptor, NULL, TestCleanup, Context3);
  AddTestCase (ImagePropertiesRecordTests, "Multiple code sections each image", "MaxOutAdditionalDescriptors", MaxOutAdditionalDescriptors, NULL, TestCleanup, Context4);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

///
/// Avoid ECC error for function name that starts with lower case letter
///
#define ImagePropertiesRecordLibUnitTestMain  main

/**
  Standard POSIX C entry point for host based unit test execution.

  @param[in] Argc  Number of arguments
  @param[in] Argv  Array of pointers to arguments

  @retval 0      Success
  @retval other  Error
**/
INT32
ImagePropertiesRecordLibUnitTestMain (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  return UnitTestingEntry ();
}
