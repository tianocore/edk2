/** @file  // [CODE_FIRST] 13253
  Root definition file for the Reserved-Memory Reporting (RMEM) ACPI table.  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  This file defines the RMEM wire-format structures, categories, and flags used  // [CODE_FIRST] 13253
  by RMEM producers, publishers, and consumers.  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Copyright (c) Microsoft Corporation.  // [CODE_FIRST] 13253
  SPDX-License-Identifier: BSD-2-Clause-Patent  // [CODE_FIRST] 13253
**/  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#pragma once  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#include <IndustryStandard/Acpi.h>  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#define RMEM_TABLE_SIGNATURE  SIGNATURE_32 ('R', 'M', 'E', 'M')  // [CODE_FIRST] 13253
#define RMEM_TABLE_REVISION   1  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
/// RMEM_ENTRY.Flags values.  // [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
/// When RMEM_ENTRY_FLAG_ADDRESS_HIDDEN is set, the producer still supplies the  // [CODE_FIRST] 13253
/// actual physical address for validation. The publisher writes zero to the  // [CODE_FIRST] 13253
/// serialized RMEM_ENTRY.Base field to avoid exposing that address.  // [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
#define RMEM_ENTRY_FLAG_ADDRESS_HIDDEN  BIT0  // [CODE_FIRST] 13253
#define RMEM_ENTRY_FLAG_VALID_MASK      RMEM_ENTRY_FLAG_ADDRESS_HIDDEN  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
/// Maximum serialized label size in bytes, including the null terminator.  // [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
#define RMEM_LABEL_MAX_LEN  28  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
/// Values stored in the UINT16 RMEM_ENTRY.Category field.  // [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
/// RmemCategoryUnknown is an invalid zero-value sentinel. Valid serialized  // [CODE_FIRST] 13253
/// category values must fit in UINT16 and be less than RmemCategoryMax.  // [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
typedef enum {  // [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  /// Invalid sentinel for a missing or uninitialized category.  // [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  RmemCategoryUnknown = 0,  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  /// Memory used for isolated execution, security processors, or protected  // [CODE_FIRST] 13253
  /// services.  // [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  RmemCategorySecurity = 1,  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  /// Memory regions or buffers shared between firmware execution environments  // [CODE_FIRST] 13253
  /// or components.  // [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  RmemCategorySharedMemory = 2,  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  /// Memory used for a pre-OS or persistent display framebuffer.  // [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  RmemCategoryDisplayFramebuffer = 3,  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  /// Memory reserved for graphics processing.  // [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  RmemCategoryGpuReserved = 4,  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  /// Memory reserved for AI accelerator processing.  // [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  RmemCategoryAiAcceleratorReserved = 5,  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  /// Memory used for firmware runtime data, services, or crash diagnostics.  // [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  RmemCategoryFirmwareRuntime = 6,  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  /// A valid reservation that does not fit another defined category.  // [CODE_FIRST] 13253
  ///  // [CODE_FIRST] 13253
  RmemCategoryOther = 7,  // [CODE_FIRST] 13253
  RmemCategoryMax   = 8  // [CODE_FIRST] 13253
} RMEM_CATEGORY;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#pragma pack(1)  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
/// RMEM ACPI table header. EntryCount RMEM_ENTRY structures begin at  // [CODE_FIRST] 13253
/// EntryOffset.  // [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
typedef struct {  // [CODE_FIRST] 13253
  EFI_ACPI_DESCRIPTION_HEADER    Header;  // [CODE_FIRST] 13253
  UINT16                         EntryCount;  // [CODE_FIRST] 13253
  UINT16                         EntryOffset;  // [CODE_FIRST] 13253
} RMEM_TABLE_HEADER;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
/// Describes one reserved physical-memory range.  // [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
typedef struct {  // [CODE_FIRST] 13253
  UINT64    Base;  // [CODE_FIRST] 13253
  UINT64    Size;  // [CODE_FIRST] 13253
  UINT16    Category;  // [CODE_FIRST] 13253
  UINT16    Flags;  // [CODE_FIRST] 13253
  CHAR8     Label[RMEM_LABEL_MAX_LEN];  // [CODE_FIRST] 13253
} RMEM_ENTRY;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#pragma pack()  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC_ASSERT (RmemCategoryMax <= MAX_UINT16, "RMEM categories do not fit in the wire-format field");  // [CODE_FIRST] 13253
STATIC_ASSERT (RMEM_ENTRY_FLAG_VALID_MASK <= MAX_UINT16, "RMEM flags do not fit in the wire-format field");  // [CODE_FIRST] 13253
STATIC_ASSERT (sizeof (RMEM_TABLE_HEADER) == 40, "Unexpected RMEM table header size");  // [CODE_FIRST] 13253
STATIC_ASSERT (OFFSET_OF (RMEM_TABLE_HEADER, EntryCount) == 36, "Unexpected RMEM entry count offset");  // [CODE_FIRST] 13253
STATIC_ASSERT (OFFSET_OF (RMEM_TABLE_HEADER, EntryOffset) == 38, "Unexpected RMEM entry offset field");  // [CODE_FIRST] 13253
STATIC_ASSERT (sizeof (RMEM_ENTRY) == 48, "Unexpected RMEM entry size");  // [CODE_FIRST] 13253
STATIC_ASSERT (OFFSET_OF (RMEM_ENTRY, Category) == 16, "Unexpected RMEM category offset");  // [CODE_FIRST] 13253
STATIC_ASSERT (OFFSET_OF (RMEM_ENTRY, Flags) == 18, "Unexpected RMEM flags offset");  // [CODE_FIRST] 13253
STATIC_ASSERT (OFFSET_OF (RMEM_ENTRY, Label) == 20, "Unexpected RMEM label offset");  // [CODE_FIRST] 13253
