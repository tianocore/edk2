/** @file
  Header file for SbbrValidator.c

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

#ifndef SBBR_VALIDATOR_H_
#define SBBR_VALIDATOR_H_

#include <IndustryStandard/Acpi.h>

/**
  Arm SBBR specification versions.
**/
typedef enum {
  ArmSbbrVersion_1_0 = 0,
  ArmSbbrVersion_1_1 = 1,
  ArmSbbrVersion_1_2 = 2,
  ArmSbbrVersionMax  = 3
} ARM_SBBR_VERSION;

/**
  The ACPI table instance counter.
**/
typedef struct AcpiTableCounter {
  CONST UINT32    Signature;      /// ACPI table signature
  UINT32          Count;          /// Instance count
} ACPI_TABLE_COUNTER;

/**
  ACPI table SBBR requirements.
**/
typedef struct AcpiSbbrReq {
  CONST UINT32    *Tables;       /// List of required tables
  CONST UINT32    TableCount;    /// Number of elements in Tables
} ACPI_SBBR_REQ;

/**
  Reset the platform ACPI table instance count for all SBBR-mandatory tables.
**/
VOID
EFIAPI
ArmSbbrResetTableCounts (
  VOID
  );

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
  UINT32  Signature
  );

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
  ARM_SBBR_VERSION  Version
  );

#endif // SBBR_VALIDATOR_H_
