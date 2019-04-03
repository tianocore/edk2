/** @file
This file describes the contents of the ACPI Multiple APIC Description
Table (MADT).  Some additional ACPI values are defined in Acpi10.h and
Acpi20.h.
To make changes to the MADT, it is necessary to update the count for the
APIC structure being updated, and to modify table found in Madt.c.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _MADT_H
#define _MADT_H


//
// Statements that include other files
//

#include <IndustryStandard/Acpi.h>
#include <Library/PcdLib.h>

//
// MADT Definitions
//

#define EFI_ACPI_OEM_MADT_REVISION 0x00000001

//
// Local APIC address
//

#define EFI_ACPI_LOCAL_APIC_ADDRESS 0xFEE00000

//
// Multiple APIC Flags are defined in AcpiX.0.h
//
#define EFI_ACPI_1_0_MULTIPLE_APIC_FLAGS (EFI_ACPI_1_0_PCAT_COMPAT)
#define EFI_ACPI_2_0_MULTIPLE_APIC_FLAGS (EFI_ACPI_2_0_PCAT_COMPAT)

//
// Define the number of each table type.
// This is where the table layout is modified.
//

#define EFI_ACPI_PROCESSOR_LOCAL_APIC_COUNT           2
#define EFI_ACPI_IO_APIC_COUNT                        1
#define EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT      2
#define EFI_ACPI_NON_MASKABLE_INTERRUPT_SOURCE_COUNT  0
#define EFI_ACPI_LOCAL_APIC_NMI_COUNT                 2
#define EFI_ACPI_LOCAL_APIC_ADDRESS_OVERRIDE_COUNT    0
#define EFI_ACPI_IO_SAPIC_COUNT                       0
#define EFI_ACPI_PROCESSOR_LOCAL_SAPIC_COUNT          0
#define EFI_ACPI_PLATFORM_INTERRUPT_SOURCES_COUNT     0

#define EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT_MAX  16

//
// MADT structure
//

//
// Ensure proper structure formats
//
#pragma pack (1)

//
// ACPI 1.0 Table structure
//
typedef struct {
  EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER   Header;

#if EFI_ACPI_PROCESSOR_LOCAL_APIC_COUNT > 0
  EFI_ACPI_1_0_PROCESSOR_LOCAL_APIC_STRUCTURE           LocalApic[EFI_ACPI_PROCESSOR_LOCAL_APIC_COUNT];
#endif

#if EFI_ACPI_IO_APIC_COUNT > 0
  EFI_ACPI_1_0_IO_APIC_STRUCTURE                        IoApic[EFI_ACPI_IO_APIC_COUNT];
#endif

#if EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT > 0
  EFI_ACPI_1_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE      Iso[EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT];
#endif

#if EFI_ACPI_NON_MASKABLE_INTERRUPT_SOURCE_COUNT > 0
  EFI_ACPI_1_0_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE  NmiSource[EFI_ACPI_NON_MASKABLE_INTERRUPT_SOURCE_COUNT];
#endif

#if EFI_ACPI_LOCAL_APIC_NMI_COUNT > 0
  EFI_ACPI_1_0_LOCAL_APIC_NMI_STRUCTURE                 LocalApicNmi[EFI_ACPI_LOCAL_APIC_NMI_COUNT];
#endif

#if EFI_ACPI_LOCAL_APIC_ADDRESS_OVERRIDE_COUNT > 0
  EFI_ACPI_1_0_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE    LocalApicOverride[EFI_ACPI_LOCAL_APIC_OVERRIDE_COUNT];
#endif

} EFI_ACPI_1_0_MULTIPLE_APIC_DESCRIPTION_TABLE;

//
// ACPI 2.0 Table structure
//
typedef struct {
  EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER   Header;

#if EFI_ACPI_PROCESSOR_LOCAL_APIC_COUNT > 0
  EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE           LocalApic[EFI_ACPI_PROCESSOR_LOCAL_APIC_COUNT];
#endif

#if EFI_ACPI_IO_APIC_COUNT > 0
  EFI_ACPI_2_0_IO_APIC_STRUCTURE                        IoApic[EFI_ACPI_IO_APIC_COUNT];
#endif

#if EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT > 0
  EFI_ACPI_2_0_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE      Iso[EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT];
#endif

#if EFI_ACPI_NON_MASKABLE_INTERRUPT_SOURCE_COUNT > 0
  EFI_ACPI_2_0_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE  NmiSource[EFI_ACPI_NON_MASKABLE_INTERRUPT_SOURCE_COUNT];
#endif

#if EFI_ACPI_LOCAL_APIC_NMI_COUNT > 0
  EFI_ACPI_2_0_LOCAL_APIC_NMI_STRUCTURE                 LocalApicNmi[EFI_ACPI_LOCAL_APIC_NMI_COUNT];
#endif

#if EFI_ACPI_LOCAL_APIC_ADDRESS_OVERRIDE_COUNT > 0
  EFI_ACPI_2_0_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE    LocalApicOverride[EFI_ACPI_LOCAL_APIC_ADDRESS_OVERRIDE_COUNT];
#endif

#if EFI_ACPI_IO_SAPIC_COUNT > 0
  EFI_ACPI_2_0_IO_SAPIC_STRUCTURE                       IoSapic[EFI_ACPI_IO_SAPIC_COUNT];
#endif

#if EFI_ACPI_PROCESSOR_LOCAL_SAPIC_COUNT > 0
  EFI_ACPI_2_0_PROCESSOR_LOCAL_SAPIC_STRUCTURE          LocalSapic[EFI_ACPI_PROCESSOR_LOCAL_SAPIC_COUNT];
#endif

#if EFI_ACPI_PLATFORM_INTERRUPT_SOURCES_COUNT > 0
  EFI_ACPI_2_0_PLATFORM_INTERRUPT_SOURCES_STRUCTURE     PlatformInterruptSources[EFI_ACPI_PLATFORM_INTERRUPT_SOURCES_COUNT];
#endif

} EFI_ACPI_2_0_MULTIPLE_APIC_DESCRIPTION_TABLE;

#define _PcdIntSettingTblEnable(x)       PcdGet8 (PcdInterruptOverrideSettingTable##x##Enable)
#define PcdIntSettingTblEnable(x)        _PcdIntSettingTblEnable(x)

#define _PcdIntSettingTblSourceIrq(x)    PcdGet8 (PcdInterruptOverrideSettingTable##x##Enable)
#define PcdIntSettingTblSourceIrq(x)     _PcdIntSettingTblSourceIrq(x)

#define _PcdIntSettingTblPolarity(x)     PcdGet8 (PcdInterruptOverrideSettingTable##x##Polarity)
#define PcdIntSettingTblPolarity(x)      _PcdIntSettingTblPolarity(x)

#define _PcdIntSettingTableTrigerMode(x) PcdGet8 (PcdInterruptOverrideSettingTable##x##TrigerMode)
#define PcdIntSettingTableTrigerMode(x)  _PcdIntSettingTableTrigerMode(x)

#define _PcdIntSettingTableGlobalIrq(x)  PcdGet32 (PcdInterruptOverrideSettingTable##x##GlobalIrq)
#define PcdIntSettingTableGlobalIrq(x)   _PcdIntSettingTableGlobalIrq(x)

typedef struct {
  UINT8     Enable;
  UINT8     SourceIrq;
  UINT8     Polarity;
  UINT8     TrigerMode;
  UINT32    GlobalIrq;
} INTERRUPT_OVERRIDE_SETTING;


typedef struct {
  UINT32    IoApicAddress;
  UINT32    GlobalInterruptBase;
  UINT8     IoApicId;
  UINT8     NmiEnable;
  UINT8     NmiSource;
  UINT8     Polarity;
  UINT8     TrigerMode;
} IO_APIC_SETTING;

typedef struct {
  UINT8     NmiEnabelApicIdMask;
  UINT8     AddressOverrideEnable;
  UINT8     Polarity;
  UINT8     TrigerMode;
  UINT8     LocalApicLint;
  UINT8     Reserve[3];
  UINT32    LocalApicAddress;
  UINT64    LocalApicAddressOverride;
} LOCAL_APIC_SETTING;

typedef struct _MADT_CONFIG_DATA {
  INTERRUPT_OVERRIDE_SETTING MadtInterruptSetting[EFI_ACPI_INTERRUPT_SOURCE_OVERRIDE_COUNT_MAX];
  IO_APIC_SETTING        MadtIoApicSetting;
  LOCAL_APIC_SETTING       MadtLocalApicSetting;
}MADT_CONFIG_DATA;

#pragma pack ()

EFI_STATUS
EFIAPI
MadtTableInitialize (
  OUT   EFI_ACPI_COMMON_HEADER  **MadtTable,
  OUT   UINTN                   *Size
  );


#endif
