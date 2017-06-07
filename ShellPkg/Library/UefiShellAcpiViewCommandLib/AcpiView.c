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

#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include "UefiShellAcpiViewCommandLib.h"
#include "AcpiParser.h"
#include "AcpiView.h"
#include "AcpiTableParser.h"


EFI_HII_HANDLE gShellAcpiViewHiiHandle = NULL;

// Local variables
STATIC UINT32             gSelectedAcpiTable;
STATIC CONST CHAR16*      gSelectedAcpiTableName;
STATIC BOOLEAN            gSelectedAcpiTableFound;
STATIC EREPORT_OPTION     gReportType;
STATIC UINT32             gTableCount;
STATIC UINT32             gBinTableCount;
STATIC UINT32             gTableErrorCount;
STATIC UINT32             gTableWarningCount;
STATIC BOOLEAN            gColourHighlighting;

/** An array of acpiview command line parameters.
**/
STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"/?", TypeFlag},
  {L"-l", TypeFlag},
  {L"-t", TypeValue},
  {L"-b", TypeFlag},
  {L"-c", TypeValue},
  {NULL, TypeMax}
};


/** This function increments the ACPI table error counter.

**/
VOID
IncrementErrorCount (
  )
{
  gTableErrorCount++;
}

/** This function returns the ACPI table error count.

  @retval Returns the count of errors detected in the ACPI tables.
**/
UINT32
GetErrorCount (
  )
{
  return gTableErrorCount;
}

/** This function increments the ACPI table warning counter.

**/
VOID
IncrementWarningCount (
  )
{
  gTableWarningCount++;
}

/** This function returns the ACPI table warning count.

  @retval Returns the count of warning detected in the ACPI tables.
**/
UINT32
GetWarningCount (
  )
{
  return gTableWarningCount;
}


/** This function returns the colour highlighting status.

  @retval TRUE if colour highlighting is enabled.
**/
BOOLEAN
GetColourHighlighting (
  )
{
  return gColourHighlighting;
}

/** This function returns the report options.

  @retval Returns the report option.

**/
STATIC
EREPORT_OPTION
GetReportOption (
  )
{
  return gReportType;
}

/** This function returns the selected ACPI table.

  @retval Returns the selected ACPI table.

**/
STATIC
UINT32
GetSelectedAcpiTable (
  )
{
  return gSelectedAcpiTable;
}


/** This function dumps the ACPI table to a file.
  @param [in] Ptr       Pointer to the ACPI table data.
  @param [in] Length    The length of the ACPI table.

  @retval TRUE          Success.

**/
STATIC
BOOLEAN
DumpAcpiTableToFile (
  IN CONST UINT8*  Ptr,
  IN CONST UINTN   Length
  )
{
  EFI_STATUS         Status;
  CHAR16             FileNameBuffer[MAX_FILE_NAME_LEN];
  SHELL_FILE_HANDLE  DumpFileHandle = NULL;
  UINTN              TransferBytes = Length;

  UnicodeSPrint (
    FileNameBuffer,
    sizeof (FileNameBuffer),
    L".\\%s%04d.bin",
    gSelectedAcpiTableName,
    gBinTableCount++
    );

  Status = ShellOpenFileByName (
             FileNameBuffer,
             &DumpFileHandle,
             EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
             0
             );
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_GEN_READONLY_MEDIA),
      gShellAcpiViewHiiHandle,
      L"acpiview"
      );
    return FALSE;
  }

  Print (L"Dumping ACPI table to : %s ... ", FileNameBuffer);

  Status = ShellWriteFile (
             DumpFileHandle,
             &TransferBytes,
             (VOID*)Ptr
             );
  if (EFI_ERROR (Status)) {
    Print (L"ERROR: Failed to dump table to binary file.\n");
  }

  ShellCloseFile (&DumpFileHandle);
  Print (L"DONE.\n");

  return (Length == TransferBytes);
}

/** This function processes the table reporting options for the ACPI table.

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
  UINTN   OriginalAttribute;
  UINT8*  SignaturePtr = (UINT8*)(UINTN)&Signature;
  BOOLEAN Log = FALSE;
  BOOLEAN HighLight = GetColourHighlighting ();
  switch (GetReportOption ()) {
    case EREPORT_ALL:
      Log = TRUE;
      break;
    case EREPORT_SELECTED:
      if (Signature == GetSelectedAcpiTable ()) {
        Log = TRUE;
        gSelectedAcpiTableFound = TRUE;
      }
      break;
    case EREPORT_TABLE_LIST:
      if (0 == gTableCount) {
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
        ++gTableCount,
        SignaturePtr[0],
        SignaturePtr[1],
        SignaturePtr[2],
        SignaturePtr[3]
        );
      break;
    case EREPORT_DUMP_BIN_FILE:
      if (Signature == GetSelectedAcpiTable ()) {
        gSelectedAcpiTableFound = TRUE;
        DumpAcpiTableToFile (TablePtr, Length);
      }
    break;
    case EREPORT_MAX:
      // We should never be here.
      // This case is only present to prevent compiler warning.
      break;
  } // switch

  if (Log) {
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

  return Log;
}

/** This function converts a string to ACPI table signature.

  @param [in] Str   Pointer to the string to be converted to the
                    ACPI table signature.

  @retval The ACPI table signature.
**/
STATIC
UINT32
ConvertStrToAcpiSignature (
  IN  CONST CHAR16* Str
  )
{
  UINT8 Index;
  CHAR8 Ptr[4];

  Index = 0;
  // Convert to Upper case and convert to ASCII
  while ((Index < 4) && (0 != Str[Index])) {
    if (Str[Index] >= L'a' && Str[Index] <= L'z') {
      Ptr[Index] = (CHAR8)(Str[Index] - (L'a' - L'A'));
    } else {
      Ptr[Index] = (CHAR8)Str[Index];
    }
    Index++;
  }
  return (*(UINT32*)Ptr);
}


/** This function iterates the configuration table entries in the
    system table and to retrieve the RSDP pointer and start parsing
    the ACPI tables.

  @param [in] SystemTable Pointer to the EFI system table.

  @retval Returns EFI_NOT_FOUND if the RSDP pointer is not found.
          Returns EFI_SUCCESS if successful.
**/
STATIC
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
    Status = ParseRsdp ((UINT8*)EfiConfigurationTable->VendorTable);
  } else {
    IncrementErrorCount ();
    Print (
      L"ERROR: Failed to find ACPI Table Guid in System Configuration Table.\n"
      );
    Status = EFI_NOT_FOUND;
  }

  ReportOption = GetReportOption ();
  if (EREPORT_TABLE_LIST != ReportOption) {
    if (((EREPORT_SELECTED == ReportOption)  ||
         (EREPORT_DUMP_BIN_FILE == ReportOption)) &&
        (!gSelectedAcpiTableFound)) {
      Print (L"\nRequested ACPI Table not found.\n");
    } else if (EREPORT_DUMP_BIN_FILE != ReportOption) {
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
  return Status;
}


/**
  Function for 'acpiview' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunAcpiView (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable
  )
{
  EFI_STATUS         Status;
  SHELL_STATUS       ShellStatus = SHELL_SUCCESS;
  LIST_ENTRY*        Package = NULL;
  CHAR16*            ProblemParam;
  CONST CHAR16*      Temp;
  CHAR8              ColourOption[8];
  SHELL_FILE_HANDLE  TmpDumpFileHandle = NULL;

  // Set Defaults
  gReportType = EREPORT_ALL;
  gTableCount = 0;
  gBinTableCount = 0;
  gSelectedAcpiTable = 0;
  gSelectedAcpiTableName = NULL;
  gTableErrorCount = 0;
  gTableWarningCount = 0;
  gSelectedAcpiTableFound = FALSE;

  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_PROBLEM),
        gShellAcpiViewHiiHandle,
        L"acpiview",
        ProblemParam
        );
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 1) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_TOO_MANY),
        gShellAcpiViewHiiHandle,
        L"acpiview"
        );
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag (Package, L"-?")) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GET_HELP_ACPIVIEW),
        gShellAcpiViewHiiHandle,
        L"acpiview"
        );
    } else if (ShellCommandLineGetFlag (Package, L"-t") &&
               ShellCommandLineGetValue (Package, L"-t") == NULL) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_NO_VALUE),
        gShellAcpiViewHiiHandle,
        L"acpiview",
        L"-t"
        );
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if ((ShellCommandLineGetFlag (Package, L"-t") &&
                ShellCommandLineGetFlag (Package, L"-l"))) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_TOO_MANY),
        gShellAcpiViewHiiHandle,
        L"acpiview"
        );
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag (Package, L"-c") &&
               ShellCommandLineGetValue (Package, L"-c") == NULL) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_GEN_NO_VALUE),
          gShellAcpiViewHiiHandle,
          L"acpiview",
          L"-c"
          );
        ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag (Package, L"-b") &&
               !ShellCommandLineGetFlag (Package, L"-t")) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_GEN_MISSING_OPTION),
          gShellAcpiViewHiiHandle,
          L"acpiview",
          L"-t",
          L"-b"
          );
        ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      // Check if the colour option is set
      Temp = ShellCommandLineGetValue (Package, L"-c");
      if (NULL != Temp) {
        UnicodeStrToAsciiStrS (Temp, ColourOption, sizeof (ColourOption));
        if ((0 == AsciiStriCmp (ColourOption, "ON")) ||
            (0 == AsciiStriCmp (ColourOption, "TRUE"))) {
          gColourHighlighting = TRUE;
        } else if ((0 == AsciiStriCmp (ColourOption, "OFF")) ||
                   (0 == AsciiStriCmp (ColourOption, "FALSE"))) {
          gColourHighlighting = FALSE;
        }
      }

      if (ShellCommandLineGetFlag (Package, L"-l")) {
        gReportType = EREPORT_TABLE_LIST;
      } else {
        gSelectedAcpiTableName = ShellCommandLineGetValue (Package, L"-t");
        if (gSelectedAcpiTableName != NULL) {
          gSelectedAcpiTable = (UINT32)ConvertStrToAcpiSignature (gSelectedAcpiTableName);
          gReportType = EREPORT_SELECTED;

          if (ShellCommandLineGetFlag (Package, L"-b"))  {
            // Create a temporary file to check if the media is writable.
            CHAR16 FileNameBuffer[MAX_FILE_NAME_LEN];
            gReportType = EREPORT_DUMP_BIN_FILE;

            UnicodeSPrint (
              FileNameBuffer,
              sizeof (FileNameBuffer),
              L".\\%s%04d.tmp",
              gSelectedAcpiTableName,
              gBinTableCount
              );

            Status = ShellOpenFileByName (
                       FileNameBuffer,
                       &TmpDumpFileHandle,
                       EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                       0
                       );

            if (EFI_ERROR (Status)) {
              ShellStatus = SHELL_INVALID_PARAMETER;
              TmpDumpFileHandle = NULL;
              ShellPrintHiiEx (
                -1,
                -1,
                NULL,
                STRING_TOKEN (STR_GEN_READONLY_MEDIA),
                gShellAcpiViewHiiHandle,
                L"acpiview"
                );
              goto Done;
            }
            // Delete Temporary file.
            ShellDeleteFile (&TmpDumpFileHandle);
          } // -b
        } // -t
      }

      // Parse ACPI Table information
      Status = AcpiView (SystemTable);
      if (EFI_ERROR (Status)) {
        ShellStatus = SHELL_NOT_FOUND;
        goto Done;
      }
    }
  }

Done:
  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }
  return ShellStatus;
}
