/** @file
  Processor resource HOB

  If BSP does not known how many cores are online or the platform cannot
  wake up AP via broadcast, this HOB can be used to store the processor
  resource data that may come from ACPI or FDT, etc.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PROCESSOR_RESOURCE_HOB_H_
#define PROCESSOR_RESOURCE_HOB_H_

#define PROCESSOR_RESOURCE_HOB_GUID \
  { \
    0xb855c7fe, 0xa758, 0x701f, { 0xa7, 0x30, 0x87, 0xf3, 0x9c, 0x03, 0x46, 0x7e } \
  }

typedef struct {
  UINT32    NumberOfProcessor;
  UINTN     ApicId[];
} PROCESSOR_RESOURCE_DATA;

extern EFI_GUID  gProcessorResourceHobGuid;

#endif
