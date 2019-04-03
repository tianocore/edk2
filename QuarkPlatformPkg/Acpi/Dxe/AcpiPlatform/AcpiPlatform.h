/** @file
This is an implementation of the ACPI platform driver.  Requirements for
this driver are defined in the Tiano ACPI External Product Specification,
revision 0.3.6.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _ACPI_PLATFORM_H_
#define _ACPI_PLATFORM_H_

//
// Statements that include other header files
//

#include <PiDxe.h>
#include <IntelQNCDxe.h>
#include <Platform.h>
#include <PlatformBoards.h>
#include <Ioh.h>
#include <QNCCommonDefinitions.h>

#include <Protocol/GlobalNvsArea.h>
#include <Protocol/MpService.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/FirmwareVolume2.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QNCAccessLib.h>
#include <Library/PlatformHelperLib.h>

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/HighPrecisionEventTimerTable.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>

#include "Madt.h"
#include "AcpiPciUpdate.h"

#pragma pack(1)
typedef struct {
  UINT8   StartByte;
  UINT32  NameStr;
  UINT8   OpCode;
  UINT16  Size;                // Hardcode to 16bit width because the table we use is fixed size
  UINT8   NumEntries;
} EFI_ACPI_NAME_COMMAND;

typedef struct {
  UINT8   PackageOp;
  UINT8   PkgLeadByte;
  UINT8   NumEntries;
  UINT8   DwordPrefix0;
  UINT32  CoreFreq;
  UINT8   DwordPrefix1;
  UINT32  Power;
  UINT8   DwordPrefix2;
  UINT32  TransLatency;
  UINT8   DwordPrefix3;
  UINT32  BMLatency;
  UINT8   DwordPrefix4;
  UINT32  Control;
  UINT8   DwordPrefix5;
  UINT32  Status;
} EFI_PSS_PACKAGE;
#pragma pack()


#define AML_NAME_OP               0x08
#define AML_METHOD_OP             0x14
#define AML_OPREGION_OP           0x80
#define AML_PACKAGE_OP            0x12    // Package operator.

//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_OEM_ID           "INTEL "
#define EFI_ACPI_OEM_TABLE_ID     0x2020204F4E414954ULL  // "TIANO   "
#define EFI_ACPI_OEM_REVISION     0x00000002
#define EFI_ACPI_CREATOR_ID       0x5446534D          // "MSFT"
#define EFI_ACPI_CREATOR_REVISION 0x01000013

#define ACPI_COMPATIBLE_1_0       0
#define ACPI_COMPATIBLE_2_0       1
#define ACPI_COMPATIBLE_3_0       2




//
// Private Driver Data
//

//
// Define Union of IO APIC & Local APIC structure;
//

typedef union {
  EFI_ACPI_2_0_PROCESSOR_LOCAL_APIC_STRUCTURE     AcpiLocalApic;
  EFI_ACPI_2_0_IO_APIC_STRUCTURE                  AcpiIoApic;
  struct {
    UINT8                                         Type;
    UINT8                                         Length;
  } AcpiApicCommon;
} ACPI_APIC_STRUCTURE_PTR;

#endif
