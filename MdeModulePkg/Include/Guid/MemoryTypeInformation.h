/** @file
  This file defines:
  * Memory Type Information GUID for Guided HOB and Variable.
  * Memory Type Information Variable Name.
  * Memory Type Information GUID HOB data structure.
  * Memory Statistics data structure and GUID that is published as a config table.
  * Memory Bin Location Variable name.

  The memory type information HOB and variable can be used to store information
  for each memory type in Variable or HOB.

  The Memory Type Information GUID can also be optionally used as the Owner
  field of a Resource Descriptor HOB to provide the preferred memory range
  for the memory types described in the Memory Type Information GUID HOB.

  Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#define EFI_MEMORY_TYPE_INFORMATION_GUID \
  { 0x4c19049f,0x4137,0x4dd3, { 0x9c,0x10,0x8b,0x97,0xa8,0x3f,0xfd,0xfa } }

#define EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME             L"MemoryTypeInformation"
#define EFI_MEMORY_TYPE_INFORMATION_BINS_RANGE_VARIABLE_NAME  L"MemoryTypeInformationBinsRange"

extern EFI_GUID  gEfiMemoryTypeInformationGuid;

#define MEMORY_TYPE_INFORMATION_STATISTICS_GUID \
  { 0x1a684878, 0xf466, 0x4d88, { 0x81, 0xc2, 0x80, 0x7e, 0x7d, 0xe6, 0x67, 0x9f } }

extern EFI_GUID  gMemoryTypeInformationStatisticsGuid;

typedef struct {
  UINT32    Type;           ///< EFI memory type defined in UEFI specification.
  UINT32    NumberOfPages;  ///< The pages of this type memory.
} EFI_MEMORY_TYPE_INFORMATION;

//
// Entry in an array that keeps track of memory type statistics per memory bin
//
typedef struct {
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  EFI_PHYSICAL_ADDRESS    MaximumAddress;
  UINT32                  CurrentNumberOfPagesInBin;
  UINT32                  CurrentNumberOfPagesOutOfBin;
  UINT32                  BinNumberOfPages;
  UINTN                   InformationIndex;
  BOOLEAN                 Special;
  BOOLEAN                 Runtime;
  BOOLEAN                 DefaultBin;
  EFI_MEMORY_TYPE         Type;
} EFI_MEMORY_TYPE_STATISTICS;

typedef struct {
  UINT32                        Version;
  EFI_MEMORY_TYPE_STATISTICS    Statistics[EfiMaxMemoryType + 1];
} EFI_MEMORY_TYPE_STATISTICS_HEADER;

#define CURRENT_MEMORY_TYPE_STATISTICS_VERSION  1

typedef struct {
  EFI_PHYSICAL_ADDRESS    BaseAddress;
  UINTN                   Length;
} MEMORY_BINS_RANGE;
