/** @file  // [CODE_FIRST] 13253
  Defines the Reserved-Memory Reporting (RMEM) GUID HOB.  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Copyright (c) Microsoft Corporation.  // [CODE_FIRST] 13253
  SPDX-License-Identifier: BSD-2-Clause-Patent  // [CODE_FIRST] 13253
**/  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#pragma once  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#include <Guid/ReservedMemoryReportingTable.h>  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#pragma pack(1)  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
/// Carries one reserved-memory range from a pre-DXE producer to the RMEM DXE  // [CODE_FIRST] 13253
/// publisher. Category values and flag bits are defined in  // [CODE_FIRST] 13253
/// ReservedMemoryReportingTable.h.  // [CODE_FIRST] 13253
///  // [CODE_FIRST] 13253
typedef struct {  // [CODE_FIRST] 13253
  UINT64    Base;  // [CODE_FIRST] 13253
  UINT64    Size;  // [CODE_FIRST] 13253
  UINT16    Category;  // [CODE_FIRST] 13253
  UINT16    Flags;  // [CODE_FIRST] 13253
  CHAR8     Label[RMEM_LABEL_MAX_LEN];  // [CODE_FIRST] 13253
} RMEM_HOB_RECORD;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#pragma pack()  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC_ASSERT (sizeof (RMEM_HOB_RECORD) == 48, "Unexpected RMEM HOB record size");  // [CODE_FIRST] 13253
STATIC_ASSERT (OFFSET_OF (RMEM_HOB_RECORD, Category) == 16, "Unexpected RMEM HOB category offset");  // [CODE_FIRST] 13253
STATIC_ASSERT (OFFSET_OF (RMEM_HOB_RECORD, Flags) == 18, "Unexpected RMEM HOB flags offset");  // [CODE_FIRST] 13253
STATIC_ASSERT (OFFSET_OF (RMEM_HOB_RECORD, Label) == 20, "Unexpected RMEM HOB label offset");  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
extern EFI_GUID  gEdkiiRmemRecordHobGuid;  // [CODE_FIRST] 13253
