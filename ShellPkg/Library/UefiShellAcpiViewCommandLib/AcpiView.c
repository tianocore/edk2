/** @file

  Copyright (c) 2016 - 2021, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Sbbr or SBBR   - Server Base Boot Requirements

  @par Reference(s):
    - Arm Server Base Boot Requirements 1.2, September 2019
**/

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

#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
#include "Arm/SbbrValidator.h"
#endif

STATIC UINT32  mTableCount;

/**
  This function finds a filename not already used by adding a number in between
  The BaseFileName and the extension.

  Make sure the buffer FileName is big enough before calling the function. A
  size of MAX_FILE_NAME_LEN is recommended.

  @param [in]      BaseFileName      Start of the desired file name.
  @param [in]      Extension         Extension of the desired file name
                                     (without '.').
  @param [in, out] FileName          Preallocated buffer for the returned file
                                     name.
  @param [in]      FileNameBufferLen Size of FileName buffer..
**/
EFI_STATUS
GetNewFileName (
  IN     CONST CHAR16* BaseFileName,
  IN     CONST CHAR16* Extension,
  IN OUT       CHAR16* FileName,
  IN           UINT32  FileNameBufferLen
  )
{
  UINT16            Index;
  EFI_STATUS        Status;
  SHELL_FILE_HANDLE tmpFileHandle;
  for (Index = 0; Index <= 99; Index++) {
    UnicodeSPrint(
      FileName,
      FileNameBufferLen,
      L"%s%02d.%s",
      BaseFileName,
      Index,
      Extension
      );
    Status = ShellOpenFileByName (
               FileName,
               &tmpFileHandle,
               EFI_FILE_MODE_READ,
               0
               );
    if (Status == EFI_NOT_FOUND) {
      return EFI_SUCCESS;
    }
    ShellCloseFile (&tmpFileHandle);
  }
  return EFI_OUT_OF_RESOURCES;
}

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
  CHAR16               FileNameBuffer[MAX_FILE_NAME_LEN];
  UINTN                TransferBytes;
  EFI_STATUS           Status;
  SELECTED_ACPI_TABLE* SelectedTable;

  GetSelectedAcpiTable (&SelectedTable);

  Status = GetNewFileName (
             SelectedTable->Name,
             L"bin",
             FileNameBuffer,
             sizeof (FileNameBuffer)
             );
  if (EFI_ERROR (Status)) {
    Print (
      L"Error: Could not open bin file for %s table:\n"
      L"Could not get a file name.",
      SelectedTable->Name
      );
    return FALSE;
  }

  Print (L"Dumping ACPI table to : %s ... ", FileNameBuffer);

  TransferBytes = ShellDumpBufferToFile (FileNameBuffer, Ptr, Length);
  return (Length == TransferBytes);
}

/**
  This function processes the table reporting options for the ACPI table.

  @param [in] Signature The ACPI table Signature.
  @param [in] TablePtr  Pointer to the ACPI table data.
  @param [in] Length    The length fo the ACPI table.

  @retval Returns the ParseFlags for the ACPI table.
**/
UINT8
ProcessTableReportOptions (
  IN CONST UINT32  Signature,
  IN CONST UINT8*  TablePtr,
  IN CONST UINT32  Length
  )
{
  UINTN                OriginalAttribute;
  UINT8                *SignaturePtr;
  UINT8                ParseFlags;
  BOOLEAN              HighLight;
  SELECTED_ACPI_TABLE  *SelectedTable;

  //
  // set local variables to suppress incorrect compiler/analyzer warnings
  //
  OriginalAttribute = 0;
  SignaturePtr = (UINT8*)(UINTN)&Signature;
  ParseFlags = 0;
  HighLight = GetColourHighlighting ();
  GetSelectedAcpiTable (&SelectedTable);

  switch (GetReportOption ()) {
    case ReportAll:
      ParseFlags |= PARSE_FLAGS_TRACE;
      break;
    case ReportSelected:
      if (Signature == SelectedTable->Type) {
        ParseFlags |= PARSE_FLAGS_TRACE;
        SelectedTable->Found = TRUE;
      }
      break;
    case ReportTableList:
      if (mTableCount == 0) {
        if (HighLight) {
          OriginalAttribute = gST->ConOut->Mode->Attribute;
          gST->ConOut->SetAttribute (
                         gST->ConOut,
                         EFI_TEXT_ATTR(EFI_CYAN,
                           ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4))
                         );
        }
        Print (L"\nInstalled Table(s):\n");
        if (HighLight) {
          gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
        }
      }
      Print (
        L"\t%4d. %c%c%c%c\n",
        ++mTableCount,
        SignaturePtr[0],
        SignaturePtr[1],
        SignaturePtr[2],
        SignaturePtr[3]
        );
      break;
    case ReportDumpBinFile:
      if (Signature == SelectedTable->Type) {
        SelectedTable->Found = TRUE;
        DumpAcpiTableToFile (TablePtr, Length);
      }
      break;
    case ReportMax:
      // We should never be here.
      // This case is only present to prevent compiler warning.
      break;
  } // switch

  if (IS_TRACE_FLAG_SET (ParseFlags)) {
    if (HighLight) {
      OriginalAttribute = gST->ConOut->Mode->Attribute;
      gST->ConOut->SetAttribute (
                     gST->ConOut,
                     EFI_TEXT_ATTR(EFI_LIGHTBLUE,
                       ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4))
                     );
    }
    Print (
      L"\n\n --------------- %c%c%c%c Table --------------- \n\n",
      SignaturePtr[0],
      SignaturePtr[1],
      SignaturePtr[2],
      SignaturePtr[3]
      );
    if (HighLight) {
      gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
    }
  }

  return ParseFlags;
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
  BOOLEAN                  FoundAcpiTable;
  UINTN                    OriginalAttribute;
  UINTN                    PrintAttribute;
  EREPORT_OPTION           ReportOption;
  UINT8*                   RsdpPtr;
  UINT32                   RsdpLength;
  UINT8                    RsdpRevision;
  PARSE_ACPI_TABLE_PROC    RsdpParserProc;
  UINT8                    ParseFlags;
  SELECTED_ACPI_TABLE      *SelectedTable;

  //
  // set local variables to suppress incorrect compiler/analyzer warnings
  //
  EfiConfigurationTable = NULL;
  OriginalAttribute = 0;

  // Reset Table counts
  mTableCount = 0;

  // Reset The error/warning counters
  ResetErrorCount ();
  ResetWarningCount ();

  // Retrieve the user selection of ACPI table to process
  GetSelectedAcpiTable (&SelectedTable);

  // Search the table for an entry that matches the ACPI Table Guid
  FoundAcpiTable = FALSE;
  for (Index = 0; Index < SystemTable->NumberOfTableEntries; Index++) {
    if (CompareGuid (&gEfiAcpiTableGuid,
          &(SystemTable->ConfigurationTable[Index].VendorGuid))) {
      EfiConfigurationTable = &SystemTable->ConfigurationTable[Index];
      FoundAcpiTable = TRUE;
      break;
    }
  }

  if (FoundAcpiTable) {
    RsdpPtr = (UINT8*)EfiConfigurationTable->VendorTable;

    // The RSDP revision is 1 byte starting at offset 15
    RsdpRevision = *(RsdpPtr + RSDP_REVISION_OFFSET);

    if (RsdpRevision < 2) {
      Print (
        L"ERROR: RSDP version less than 2 is not supported.\n"
        );
      return EFI_UNSUPPORTED;
    }

#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
    if (GetMandatoryTableValidate ()) {
      ArmSbbrResetTableCounts ();
    }
#endif

    // The RSDP length is 4 bytes starting at offset 20
    RsdpLength = *(UINT32*)(RsdpPtr + RSDP_LENGTH_OFFSET);

    ParseFlags = ProcessTableReportOptions (
                   RSDP_TABLE_INFO,
                   RsdpPtr,
                   RsdpLength
                   );

    Status = GetParser (RSDP_TABLE_INFO, &RsdpParserProc);
    if (EFI_ERROR (Status)) {
      Print (
        L"ERROR: No registered parser found for RSDP.\n"
        );
      return Status;
    }

    RsdpParserProc (
      ParseFlags,
      RsdpPtr,
      RsdpLength,
      RsdpRevision
      );

  } else {
    IncrementErrorCount ();
    Print (
      L"ERROR: Failed to find ACPI Table Guid in System Configuration Table.\n"
      );
    return EFI_NOT_FOUND;
  }

#if defined(MDE_CPU_ARM) || defined (MDE_CPU_AARCH64)
  if (GetMandatoryTableValidate ()) {
    ArmSbbrReqsValidate ((ARM_SBBR_VERSION)GetMandatoryTableSpec ());
  }
#endif

  ReportOption = GetReportOption ();
  if (ReportTableList != ReportOption) {
    if (((ReportSelected == ReportOption)  ||
         (ReportDumpBinFile == ReportOption)) &&
        (!SelectedTable->Found)) {
      Print (L"\nRequested ACPI Table not found.\n");
    } else if (GetConsistencyChecking () &&
               (ReportDumpBinFile != ReportOption)) {
      OriginalAttribute = gST->ConOut->Mode->Attribute;

      Print (L"\nTable Statistics:\n");

      if (GetColourHighlighting ()) {
        PrintAttribute = (GetErrorCount () > 0) ?
                            EFI_TEXT_ATTR (
                              EFI_RED,
                              ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)
                              ) :
                            OriginalAttribute;
        gST->ConOut->SetAttribute (gST->ConOut, PrintAttribute);
      }
      Print (L"\t%d Error(s)\n", GetErrorCount ());

      if (GetColourHighlighting ()) {
        PrintAttribute = (GetWarningCount () > 0) ?
                            EFI_TEXT_ATTR (
                              EFI_RED,
                              ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)
                              ) :
                            OriginalAttribute;

        gST->ConOut->SetAttribute (gST->ConOut, PrintAttribute);
      }
      Print (L"\t%d Warning(s)\n", GetWarningCount ());

      if (GetColourHighlighting ()) {
        gST->ConOut->SetAttribute (gST->ConOut, OriginalAttribute);
      }
    }
  }
  return EFI_SUCCESS;
}
