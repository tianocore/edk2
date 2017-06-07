/** @file
*
*  Copyright (c) 2016 - 2017, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <IndustryStandard/Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiView.h"
#include "AcpiTableParser.h"

/** This function processes the ACPI tables.
  This function calls ProcessTableReportOptions() to list the ACPI
  tables, perform binary dump of the tables and determine if the
  ACPI fields should be traced.

  This function also invokes the parser for the ACPI tables.

  This function also performs a RAW dump of the ACPI table including
  the unknown/unparsed ACPI tables and validates the checksum.

  @params [in] Ptr                Pointer to the start of the ACPI
                                  table data buffer.
**/
VOID
ProcessAcpiTable (
  IN UINT8* Ptr
  )
{
  BOOLEAN       Trace;
  CONST UINT32* AcpiTableSignature;
  CONST UINT32* AcpiTableLength;
  CONST UINT8*  AcpiTableRevision;

  ParseAcpiHeader (
    Ptr,
    &AcpiTableSignature,
    &AcpiTableLength,
    &AcpiTableRevision
    );

  Trace = ProcessTableReportOptions (
            *AcpiTableSignature,
            Ptr,
            *AcpiTableLength
            );

  if (Trace) {
    DumpRaw (Ptr, *AcpiTableLength);
    VerifyChecksum (TRUE, Ptr, *AcpiTableLength);
  }

  switch (*AcpiTableSignature) {
    case EFI_ACPI_6_1_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
      ParseAcpiXsdt (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE:
      ParseAcpiFadt (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
      ParseAcpiDsdt (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE:
      ParseAcpiGtdt (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE:
      ParseAcpiSpcr (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE:
      ParseAcpiMadt (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_DEBUG_PORT_2_TABLE_SIGNATURE:
      ParseAcpiDbg2 (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE:
      ParseAcpiMcfg (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
      ParseAcpiSsdt (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_SYSTEM_RESOURCE_AFFINITY_TABLE_SIGNATURE:
      ParseAcpiSrat (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_SYSTEM_LOCALITY_INFORMATION_TABLE_SIGNATURE:
      ParseAcpiSlit (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_IO_REMAPPING_TABLE_SIGNATURE:
      ParseAcpiIort (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    case EFI_ACPI_6_1_BOOT_GRAPHICS_RESOURCE_TABLE_SIGNATURE:
      ParseAcpiBgrt (
        Trace,
        Ptr,
        *AcpiTableLength,
        *AcpiTableRevision
        );
      break;
    default:
      if (Trace) {
        DumpAcpiHeader (Ptr);
      }
    } // switch
}

