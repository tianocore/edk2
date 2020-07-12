/** @file

  Copyright (c) 2016 - 2020, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Sbbr or SBBR   - Server Base Boot Requirements

  @par Reference(s):
    - Arm Server Base Boot Requirements 1.2, September 2019
**/

#include <Guid/Acpi.h>

#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/AcpiViewCommandLib.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiView.h"
#include "AcpiViewConfig.h"
#include "AcpiViewLog.h"

#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
#include "Arm/SbbrValidator.h"
#endif

STATIC UINT32             mTableCount;
STATIC UINT32             mBinTableCount;

/**
  This function dumps the ACPI table to a file.

  @param [in] Ptr       Pointer to the ACPI table data.
  @param [in] Length    The length of the ACPI table.

  @retval TRUE          Success.
  @retval FALSE         Failure.
**/
STATIC
BOOLEAN
DumpAcpiTableToFile (
  IN CONST UINT8*  Ptr,
  IN CONST UINTN   Length
  )
{
  CHAR16              FileNameBuffer[MAX_FILE_NAME_LEN];
  UINTN               TransferBytes;

  UnicodeSPrint (
    FileNameBuffer,
    sizeof (FileNameBuffer),
    L".\\%s%04d.bin",
    mSelectedAcpiTable.Name,
    mBinTableCount++
    );

  AcpiInfo (L"Dumping ACPI table to : %s ... ", FileNameBuffer);

  TransferBytes = ShellDumpBufferToFile (FileNameBuffer, Ptr, Length);
  return (Length == TransferBytes);
}

/**
  This function processes the table reporting options for the ACPI table.

  @param [in] Signature The ACPI table Signature.
  @param [in] TablePtr  Pointer to the ACPI table data.
  @param [in] Length    The length fo the ACPI table.

  @retval Returns TRUE if the ACPI table should be traced.
**/
BOOLEAN
ProcessTableReportOptions (
  IN CONST UINT32  Signature,
  IN CONST UINT8*  TablePtr,
  IN CONST UINT32  Length
  )
{
  BOOLEAN             Log;
  Log = FALSE;

  switch (mConfig.ReportType) {
    case ReportAll:
      Log = TRUE;
      break;
    case ReportSelected:
      if (Signature == mSelectedAcpiTable.Type) {
        Log = TRUE;
        mSelectedAcpiTable.Found = TRUE;
      }
      break;
    case ReportTableList:
      if (mTableCount == 0) {
        AcpiLog (ACPI_HIGHLIGHT, L"\nInstalled Table(s):");
      }
      AcpiInfo (L"\t%4d. %.*a", ++mTableCount, 4, &Signature);
      break;
    case ReportDumpBinFile:
      if (Signature == mSelectedAcpiTable.Type) {
        mSelectedAcpiTable.Found = TRUE;
        DumpAcpiTableToFile (TablePtr, Length);
      }
      break;
    case ReportMax:
      // We should never be here.
      // This case is only present to prevent compiler warning.
      break;
  } // switch

  if (Log) {
    AcpiLog (
      ACPI_HIGHLIGHT,
      L"\n --------------- %.*a Table --------------- \n",
      4,
      &Signature);
  }

  return Log;
}

/**
  This function iterates the configuration table entries in the
  system table, retrieves the RSDP pointer and starts parsing the ACPI tables.

  @param [in] SystemTable Pointer to the EFI system table.

  @retval Returns EFI_NOT_FOUND   if the RSDP pointer is not found.
          Returns EFI_UNSUPPORTED if the RSDP version is less than 2.
          Returns EFI_SUCCESS     if successful.
**/
EFI_STATUS
EFIAPI
AcpiView (
  IN EFI_SYSTEM_TABLE* SystemTable
  )
{
  EFI_STATUS               Status;
  UINTN                    Index;
  EFI_CONFIGURATION_TABLE* EfiConfigurationTable;
  UINT8*                   RsdpPtr;
  UINT32                   RsdpLength;
  UINT8                    RsdpRevision;
  PARSE_ACPI_TABLE_PROC    RsdpParserProc;
  BOOLEAN                  Trace;

  // Reset Table counts
  mTableCount = 0;
  mBinTableCount = 0;

  // Reset The error/warning counters
  mTableErrorCount = 0;
  mTableWarningCount = 0;

  // Search the table for an entry that matches the ACPI Table Guid
  EfiConfigurationTable = NULL;
  for (Index = 0; Index < SystemTable->NumberOfTableEntries; Index++) {
    if (CompareGuid (&gEfiAcpiTableGuid,
          &(SystemTable->ConfigurationTable[Index].VendorGuid))) {
      EfiConfigurationTable = &SystemTable->ConfigurationTable[Index];
      break;
    }
  }

  if (!EfiConfigurationTable) {
    AcpiFatal (L"No ACPI Table Guid in System Configuration Table.");
    return EFI_NOT_FOUND;
  }

  RsdpPtr = (UINT8 *)EfiConfigurationTable->VendorTable;

  // The RSDP revision is 1 byte starting at offset 15
  RsdpRevision = *(RsdpPtr + RSDP_REVISION_OFFSET);

  if (RsdpRevision < 2) {
    AcpiFatal (L"RSDP version less than 2 is not supported.");
    return EFI_UNSUPPORTED;
  }

#if defined(MDE_CPU_ARM) || defined(MDE_CPU_AARCH64)
  if (mConfig.MandatoryTableValidate) {
    ArmSbbrResetTableCounts();
  }
#endif

  // The RSDP length is 4 bytes starting at offset 20
  RsdpLength = *(UINT32 *)(RsdpPtr + RSDP_LENGTH_OFFSET);

  Trace = ProcessTableReportOptions(RSDP_TABLE_INFO, RsdpPtr, RsdpLength);

  Status = GetParser(RSDP_TABLE_INFO, &RsdpParserProc);
  if (EFI_ERROR(Status)) {
    AcpiFatal (L"No registered parser found for RSDP.");
    return Status;
  }

  RsdpParserProc(Trace, RsdpPtr, RsdpLength, RsdpRevision);

#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  if (mConfig.MandatoryTableValidate) {
    ArmSbbrReqsValidate ((ARM_SBBR_VERSION) mConfig.MandatoryTableSpec);
  }
#endif

  if (mConfig.ReportType == ReportSelected ||
      mConfig.ReportType == ReportDumpBinFile) {
    if (!mSelectedAcpiTable.Found) {
      AcpiFatal (L"Requested ACPI Table not found.");
      return EFI_SUCCESS;
    }
  }

  if (mConfig.ConsistencyCheck) {
    if (mConfig.ReportType == ReportSelected ||
        mConfig.ReportType == ReportAll) {
      AcpiInfo (L"Table Statistics:");
      AcpiLog (
        mTableErrorCount ? ACPI_BAD : ACPI_GOOD,
        L"\t%d Error(s)",
        mTableErrorCount);
      AcpiLog (
        mTableWarningCount ? ACPI_BAD : ACPI_GOOD,
        L"\t%d Warning(s)\n",
        mTableWarningCount);
    }
  }

  return EFI_SUCCESS;
}
