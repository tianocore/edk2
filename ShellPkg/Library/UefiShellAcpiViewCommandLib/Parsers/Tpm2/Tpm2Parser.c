/** @file
  TPM2 table parser

  Copyright (c) 2024, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - TCG ACPI Specification - Version 1.4, Revision 15, April 3, 2024.
      (https://trustedcomputinggroup.org/resource/tcg-acpi-specification/)
**/

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Tpm2Acpi.h>
#include <Library/UefiLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"

#define TPM2_ACPI_TABLE_LOG_AREA_SIZE  (sizeof(UINT32) + sizeof(UINT64))

// Log area parameter offset is different on ACPI table revision 4 and 5, due to different SMSP max size.
#define TPM2_ACPI_TABLE_SIZE_WITH_LOG_AREA_REVISION_4  (sizeof(EFI_TPM2_ACPI_TABLE) + EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_4 + TPM2_ACPI_TABLE_LOG_AREA_SIZE)
#define TPM2_ACPI_TABLE_SIZE_WITH_LOG_AREA_REVISION_5  (sizeof(EFI_TPM2_ACPI_TABLE) + EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_5 + TPM2_ACPI_TABLE_LOG_AREA_SIZE)

// Local variables
STATIC ACPI_DESCRIPTION_HEADER_INFO  AcpiHdrInfo;
STATIC UINT32                        *StartMethod;

/**
  An ACPI_PARSER array describing the ACPI TPM 2.0 Table.
**/
STATIC CONST ACPI_PARSER  Tpm2Parser[] = {
  PARSE_ACPI_HEADER (&AcpiHdrInfo),
  { L"Platform Class",             2,  36, L"0x%x",  NULL, NULL,                  NULL, NULL },
  { L"Reserved",                   2,  38, L"0x%x",  NULL, NULL,                  NULL, NULL },
  { L"Address of CRB Control Area",8,  40, L"0x%lx", NULL, NULL,                  NULL, NULL },
  { L"Start Method",               4,  48, L"0x%x",  NULL, (VOID **)&StartMethod, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the ACPI TPM 2.0 Table Log Area entries.
**/
STATIC CONST ACPI_PARSER  Tpm2LogArea[] = {
  { L"Log Area Minimum Length", 4, 0, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Log Area Start Address",  8, 4, L"0x%lx", NULL, NULL, NULL, NULL }
};

/**
  An ACPI_PARSER array describing the Start Method Specific Parameters for Arm SMC Start Method table.
**/
STATIC CONST ACPI_PARSER  Tpm2StartMethodArmSmc[] = {
  { L"Interrupt",           4, 0, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Flags",               1, 4, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Operation Flags",     1, 5, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Attributes",          1, 6, L"0x%x", NULL, NULL, NULL, NULL },
  { L"Reserved",            1, 7, L"0x%x", NULL, NULL, NULL, NULL },
  { L"SMC/HVC Function ID", 4, 8, L"0x%x", NULL, NULL, NULL, NULL },
};

/**
  An ACPI_PARSER array describing the Start Method Specific Parameters for Arm FF-A table.
**/
STATIC CONST ACPI_PARSER  Tpm2StartMethodArmFFA[] = {
  { L"Flags",        1, 0, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Attributes",   1, 1, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Partition ID", 2, 2, L"0x%x",  NULL, NULL, NULL, NULL },
  { L"Reserved",     8, 4, L"0x%lx", NULL, NULL, NULL, NULL },
};

/**
  This function parses the ACPI TPM2 table.
  When trace is enabled this function parses the TPM2 table and
  traces the ACPI table fields.

  This function also performs validation of the ACPI table fields.

  @param [in] Trace              If TRUE, trace the ACPI fields.
  @param [in] Ptr                Pointer to the start of the buffer.
  @param [in] AcpiTableLength    Length of the ACPI table.
  @param [in] AcpiTableRevision  Revision of the ACPI table.
**/
VOID
EFIAPI
ParseAcpiTpm2 (
  IN BOOLEAN  Trace,
  IN UINT8    *Ptr,
  IN UINT32   AcpiTableLength,
  IN UINT8    AcpiTableRevision
  )
{
  UINT32   Offset;
  BOOLEAN  LogAreaPresent;

  if (!Trace) {
    return;
  }

  Offset = ParseAcpi (
             TRUE,
             0,
             "TPM2",
             Ptr,
             AcpiTableLength,
             PARSER_PARAMS (Tpm2Parser)
             );

  // Log area parameters are optional. Presence is determined by table length.
  LogAreaPresent = (*AcpiHdrInfo.Revision == EFI_TPM2_ACPI_TABLE_REVISION_4 && AcpiTableLength == TPM2_ACPI_TABLE_SIZE_WITH_LOG_AREA_REVISION_4) ||
                   (*AcpiHdrInfo.Revision == EFI_TPM2_ACPI_TABLE_REVISION_5 && AcpiTableLength == TPM2_ACPI_TABLE_SIZE_WITH_LOG_AREA_REVISION_5);

  switch (*StartMethod) {
    case EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE_WITH_SMC:
      ParseAcpi (
        TRUE,
        0,
        "Start Method Specific Parameters for Arm SMC",
        Ptr + Offset,
        AcpiTableLength - Offset,
        PARSER_PARAMS (Tpm2StartMethodArmSmc)
        );
      break;

    case EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE_WITH_FFA:
      ParseAcpi (
        TRUE,
        0,
        "Start Method Specific Parameters for Arm FF-A",
        Ptr + Offset,
        AcpiTableLength - Offset,
        PARSER_PARAMS (Tpm2StartMethodArmFFA)
        );
      break;

    default:
      Print (
        L"WARNING: Start Method %u not supported\n",
        *StartMethod
        );
      break;
  }

  if (LogAreaPresent) {
    Offset += (*AcpiHdrInfo.Revision == EFI_TPM2_ACPI_TABLE_REVISION_4) ?
              EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_4 :
              EFI_TPM2_ACPI_TABLE_START_METHOD_SPECIFIC_PARAMETERS_MAX_SIZE_REVISION_5;

    ParseAcpi (
      TRUE,
      0,
      "TPM2 Log Area",
      Ptr + Offset,
      AcpiTableLength - Offset,
      PARSER_PARAMS (Tpm2LogArea)
      );
  }
}
