/** @file
  Arm Server Base Boot Requirements ACPI table requirement validator.

  Copyright (c) 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Sbbr or SBBR   - Server Base Boot Requirements
    - Sbsa or SBSA   - Server Base System Architecture

  @par Reference(s):
    - Arm Server Base Boot Requirements 1.2, September 2019
    - Arm Server Base Boot Requirements 1.1, May 2018
    - Arm Server Base Boot Requirements 1.0, March 2016
    - Arm Server Base System Architecture 6.0
**/

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "Arm/SbbrValidator.h"

/**
  SBBR specification version strings
**/
STATIC CONST CHAR8* ArmSbbrVersions[ArmSbbrVersionMax] = {
  "1.0",     // ArmSbbrVersion_1_0
  "1.1",     // ArmSbbrVersion_1_1
  "1.2"      // ArmSbbrVersion_1_2
};

/**
  SBBR 1.0 mandatory ACPI tables
**/
STATIC CONST UINT32 ArmSbbr10Mandatory[] = {
  EFI_ACPI_6_3_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE,
  EFI_ACPI_6_3_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE
};

/**
  SBBR 1.1 mandatory ACPI tables
**/
STATIC CONST UINT32 ArmSbbr11Mandatory[] = {
  EFI_ACPI_6_3_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE,
  EFI_ACPI_6_3_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE
};

/**
  SBBR 1.2 mandatory ACPI tables
**/
STATIC CONST UINT32 ArmSbbr12Mandatory[] = {
  EFI_ACPI_6_3_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE,
  EFI_ACPI_6_3_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE
};

/**
  Mandatory ACPI tables for every SBBR specification version.
**/
STATIC CONST ACPI_SBBR_REQ ArmSbbrReqs[ArmSbbrVersionMax] = {
  { ArmSbbr10Mandatory, ARRAY_SIZE (ArmSbbr10Mandatory) },    // SBBR v1.0
  { ArmSbbr11Mandatory, ARRAY_SIZE (ArmSbbr11Mandatory) },    // SBBR v1.1
  { ArmSbbr12Mandatory, ARRAY_SIZE (ArmSbbr12Mandatory) }     // SBBR v1.2
};

/**
  Data structure to track instance counts for all ACPI tables which are
  defined as 'mandatory' in any SBBR version.
**/
STATIC ACPI_TABLE_COUNTER ArmSbbrTableCounts[] = {
  {EFI_ACPI_6_3_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE, 0},
  {EFI_ACPI_6_3_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE, 0},
  {EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE, 0},
  {EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE, 0},
  {EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE, 0},
  {EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE, 0},
  {EFI_ACPI_6_3_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE, 0},
  {EFI_ACPI_6_3_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE, 0},
  {EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE, 0}
};

/**
  Reset the platform ACPI table instance count for all SBBR-mandatory tables.
**/
VOID
EFIAPI
ArmSbbrResetTableCounts (
  VOID
  )
{
  UINT32 Table;

  for (Table = 0; Table < ARRAY_SIZE (ArmSbbrTableCounts); Table++) {
    ArmSbbrTableCounts[Table].Count = 0;
  }
}

/**
  Increment instance count for SBBR-mandatory ACPI table with the given
  signature.

  @param [in]  Signature        ACPI table signature.

  @retval TRUE      Count incremented successfully.
  @retval FALSE     Table with the input signature not found.
**/
BOOLEAN
EFIAPI
ArmSbbrIncrementTableCount (
  UINT32 Signature
  )
{
  UINT32 Table;

  for (Table = 0; Table < ARRAY_SIZE (ArmSbbrTableCounts); Table++) {
    if (Signature == ArmSbbrTableCounts[Table].Signature) {
      ArmSbbrTableCounts[Table].Count++;
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Validate that all ACPI tables required by the given SBBR specification
  version are installed on the platform.

  @param [in]  Version      SBBR spec version to validate against.

  @retval EFI_SUCCESS             All required tables are present.
  @retval EFI_INVALID_PARAMETER   Invalid SBBR version.
  @retval EFI_NOT_FOUND           One or more mandatory tables are missing.
  @retval EFI_UNSUPPORTED         Mandatory ACPI table does not have its
                                  instance count tracked.
**/
EFI_STATUS
EFIAPI
ArmSbbrReqsValidate (
  ARM_SBBR_VERSION Version
  )
{
  UINT32        Table;
  UINT32        Index;
  UINT32        MandatoryTable;
  CONST UINT8*  SignaturePtr;
  BOOLEAN       IsArmSbbrViolated;

  if (Version >= ArmSbbrVersionMax) {
    return EFI_INVALID_PARAMETER;
  }

  IsArmSbbrViolated = FALSE;

  // Go through the list of mandatory tables for the input SBBR version
  for (Table = 0; Table < ArmSbbrReqs[Version].TableCount; Table++) {
    MandatoryTable = ArmSbbrReqs[Version].Tables[Table];
    SignaturePtr = (CONST UINT8*)(UINTN)&MandatoryTable;

    // Locate the instance count for the table with the given signature
    Index = 0;
    while ((Index < ARRAY_SIZE (ArmSbbrTableCounts)) &&
           (ArmSbbrTableCounts[Index].Signature != MandatoryTable)) {
      Index++;
    }

    if (Index >= ARRAY_SIZE (ArmSbbrTableCounts)) {
      IncrementErrorCount ();
      Print (
        L"\nERROR: SBBR v%a: Mandatory %c%c%c%c table's instance count not " \
          L"found\n",
        ArmSbbrVersions[Version],
        SignaturePtr[0],
        SignaturePtr[1],
        SignaturePtr[2],
        SignaturePtr[3]
        );
      return EFI_UNSUPPORTED;
    }

    if (ArmSbbrTableCounts[Index].Count == 0) {
      IsArmSbbrViolated = TRUE;
      IncrementErrorCount ();
      Print (
        L"\nERROR: SBBR v%a: Mandatory %c%c%c%c table is missing",
        ArmSbbrVersions[Version],
        SignaturePtr[0],
        SignaturePtr[1],
        SignaturePtr[2],
        SignaturePtr[3]
        );
    }
  }

  if (!IsArmSbbrViolated) {
    Print (
      L"\nINFO: SBBR v%a: All mandatory ACPI tables are installed",
      ArmSbbrVersions[Version]
      );
  }

  Print (L"\n");

  return IsArmSbbrViolated ? EFI_NOT_FOUND : EFI_SUCCESS;
}
