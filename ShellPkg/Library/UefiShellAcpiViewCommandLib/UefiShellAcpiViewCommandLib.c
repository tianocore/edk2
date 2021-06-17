/** @file
  Main file for 'acpiview' Shell command function.

  Copyright (c) 2016 - 2020, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/ShellLibHiiGuid.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/ArmErrorSourceTable.h>

#include <Library/BaseMemoryLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/AcpiViewCommandLib.h>
#include <Uefi.h>

#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiView.h"
#include "AcpiViewConfig.h"

CONST CHAR16 gShellAcpiViewFileName[] = L"ShellCommand";
EFI_HII_HANDLE gShellAcpiViewHiiHandle = NULL;

/**
  An array of acpiview command line parameters.
**/
STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-q", TypeFlag},
  {L"-d", TypeFlag},
  {L"-h", TypeFlag},
  {L"-l", TypeFlag},
  {L"-s", TypeValue},
  {L"-r", TypeValue},
  {NULL, TypeMax}
};

/**
  A list of available table parsers.
*/
STATIC
CONST
ACPI_TABLE_PARSER ParserList[] = {
  {EFI_ACPI_6_3_ARM_ERROR_SOURCE_TABLE_SIGNATURE, ParseAcpiAest},
  {EFI_ACPI_6_2_BOOT_GRAPHICS_RESOURCE_TABLE_SIGNATURE, ParseAcpiBgrt},
  {EFI_ACPI_6_2_DEBUG_PORT_2_TABLE_SIGNATURE, ParseAcpiDbg2},
  {EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
   ParseAcpiDsdt},
  {EFI_ACPI_6_3_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE, ParseAcpiFacs},
  {EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE, ParseAcpiFadt},
  {EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE, ParseAcpiGtdt},
  {EFI_ACPI_6_3_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_SIGNATURE, ParseAcpiHmat},
  {EFI_ACPI_6_2_IO_REMAPPING_TABLE_SIGNATURE, ParseAcpiIort},
  {EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE, ParseAcpiMadt},
  {EFI_ACPI_6_2_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
   ParseAcpiMcfg},
  {EFI_ACPI_6_2_PLATFORM_COMMUNICATIONS_CHANNEL_TABLE_SIGNATURE,
   ParseAcpiPcct},
  {EFI_ACPI_6_2_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE,
   ParseAcpiPptt},
  {RSDP_TABLE_INFO, ParseAcpiRsdp},
  {EFI_ACPI_6_2_SYSTEM_LOCALITY_INFORMATION_TABLE_SIGNATURE, ParseAcpiSlit},
  {EFI_ACPI_6_2_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE, ParseAcpiSpcr},
  {EFI_ACPI_6_2_SYSTEM_RESOURCE_AFFINITY_TABLE_SIGNATURE, ParseAcpiSrat},
  {EFI_ACPI_6_2_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE, ParseAcpiSsdt},
  {EFI_ACPI_6_2_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE, ParseAcpiXsdt}
};

/**
  This function registers all the available table parsers.

  @retval EFI_SUCCESS           The parser is registered.
  @retval EFI_ALREADY_STARTED   The parser for the ACPI Table
                                was already registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  No space to register the
                                parser.
**/
EFI_STATUS
RegisterAllParsers (
  )
{
  EFI_STATUS Status;
  UINTN Count;

  Status = EFI_SUCCESS;
  Count = sizeof (ParserList) / sizeof (ParserList[0]);

  while (Count-- != 0) {
    Status = RegisterParser (
               ParserList[Count].Signature,
               ParserList[Count].Parser
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  return Status;
}

/**
  Dump a buffer to a file. Print error message if a file cannot be created.

  @param[in] FileName   The filename that shall be created to contain the buffer.
  @param[in] Buffer     Pointer to buffer that shall be dumped.
  @param[in] BufferSize The size of buffer to be dumped in bytes.

  @return The number of bytes that were written
**/
UINTN
EFIAPI
ShellDumpBufferToFile (
  IN CONST CHAR16* FileNameBuffer,
  IN CONST VOID*   Buffer,
  IN CONST UINTN   BufferSize
  )
{
  EFI_STATUS          Status;
  SHELL_FILE_HANDLE   DumpFileHandle;
  UINTN               TransferBytes;

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
    return 0;
  }

  TransferBytes = BufferSize;
  Status = ShellWriteFile (
             DumpFileHandle,
             &TransferBytes,
             (VOID *) Buffer
             );

  if (EFI_ERROR (Status)) {
    Print (L"ERROR: Failed to write binary file.\n");
    TransferBytes = 0;
  } else {
    Print (L"DONE.\n");
  }

  ShellCloseFile (&DumpFileHandle);
  return TransferBytes;
}

/**
  Return the file name of the help text file if not using HII.

  @return The string pointer to the file name.
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameAcpiView (
  VOID
  )
{
  return gShellAcpiViewFileName;
}

/**
  Function for 'acpiview' command.

  @param[in] ImageHandle  Handle to the Image (NULL if internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if internal).

  @retval SHELL_INVALID_PARAMETER The command line invocation could not be parsed
  @retval SHELL_NOT_FOUND         The command failed
  @retval SHELL_SUCCESS           The command was successful
**/
SHELL_STATUS
EFIAPI
ShellCommandRunAcpiView (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable
  )
{
  EFI_STATUS         Status;
  SHELL_STATUS       ShellStatus;
  LIST_ENTRY*        Package;
  CHAR16*            ProblemParam;
  SHELL_FILE_HANDLE  TmpDumpFileHandle;
  CONST CHAR16*      MandatoryTableSpecStr;
  CONST CHAR16*      SelectedTableName;

  // Set configuration defaults
  AcpiConfigSetDefaults ();

  ShellStatus = SHELL_SUCCESS;
  Package = NULL;
  TmpDumpFileHandle = NULL;

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
    } else {
      Print (L"acpiview: Error processing input parameter(s)\n");
    }
    ShellStatus = SHELL_INVALID_PARAMETER;
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
    } else if (ShellCommandLineGetFlag (Package, L"-s") &&
               ShellCommandLineGetValue (Package, L"-s") == NULL) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_NO_VALUE),
        gShellAcpiViewHiiHandle,
        L"acpiview",
        L"-s"
        );
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag (Package, L"-r") &&
               ShellCommandLineGetValue (Package, L"-r") == NULL) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_NO_VALUE),
        gShellAcpiViewHiiHandle,
        L"acpiview",
        L"-r"
        );
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if ((ShellCommandLineGetFlag (Package, L"-s") &&
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
    } else if (ShellCommandLineGetFlag (Package, L"-d") &&
               !ShellCommandLineGetFlag (Package, L"-s")) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_GEN_MISSING_OPTION),
          gShellAcpiViewHiiHandle,
          L"acpiview",
          L"-s",
          L"-d"
          );
        ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      // Turn on colour highlighting if requested
      SetColourHighlighting (ShellCommandLineGetFlag (Package, L"-h"));

      // Surpress consistency checking if requested
      SetConsistencyChecking (!ShellCommandLineGetFlag (Package, L"-q"));

      // Evaluate the parameters for mandatory ACPI table presence checks
      SetMandatoryTableValidate (ShellCommandLineGetFlag (Package, L"-r"));
      MandatoryTableSpecStr = ShellCommandLineGetValue (Package, L"-r");

      if (MandatoryTableSpecStr != NULL) {
        SetMandatoryTableSpec (ShellHexStrToUintn (MandatoryTableSpecStr));
      }

      if (ShellCommandLineGetFlag (Package, L"-l")) {
        SetReportOption (ReportTableList);
      } else {
        SelectedTableName = ShellCommandLineGetValue (Package, L"-s");
        if (SelectedTableName != NULL) {
          SelectAcpiTable (SelectedTableName);
          SetReportOption (ReportSelected);

          if (ShellCommandLineGetFlag (Package, L"-d"))  {
            // Create a temporary file to check if the media is writable.
            CHAR16 FileNameBuffer[MAX_FILE_NAME_LEN];
            SetReportOption (ReportDumpBinFile);

            UnicodeSPrint (
              FileNameBuffer,
              sizeof (FileNameBuffer),
              L".\\%s0000.tmp",
              SelectedTableName
              );

            Status = ShellOpenFileByName (
                       FileNameBuffer,
                       &TmpDumpFileHandle,
                       EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE |
                       EFI_FILE_MODE_CREATE,
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
          } // -d
        } // -s
      }

      // Parse ACPI Table information
      Status = AcpiView (SystemTable);
      if (EFI_ERROR (Status)) {
        ShellStatus = SHELL_NOT_FOUND;
      }
    }
  }

Done:
  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }
  return ShellStatus;
}

/**
  Constructor for the Shell AcpiView Command library.

  Install the handlers for acpiview UEFI Shell command.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           The Shell command handlers were installed
                                successfully.
  @retval EFI_DEVICE_ERROR      Hii package failed to install.
**/
EFI_STATUS
EFIAPI
UefiShellAcpiViewCommandLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;
  gShellAcpiViewHiiHandle = NULL;

  // Check Shell Profile Debug1 bit of the profiles mask
  if ((PcdGet8 (PcdShellProfileMask) & BIT1) == 0) {
    return EFI_SUCCESS;
  }

  Status = RegisterAllParsers ();
  if (EFI_ERROR (Status)) {
    Print (L"acpiview: Error failed to register parser.\n");
    return Status;
  }

  gShellAcpiViewHiiHandle = HiiAddPackages (
                              &gShellAcpiViewHiiGuid,
                              gImageHandle,
                              UefiShellAcpiViewCommandLibStrings,
                              NULL
                              );
  if (gShellAcpiViewHiiHandle == NULL) {
    return EFI_DEVICE_ERROR;
  }
  // Install our Shell command handler
  ShellCommandRegisterCommandName (
    L"acpiview",
    ShellCommandRunAcpiView,
    ShellCommandGetManFileNameAcpiView,
    0,
    L"acpiview",
    TRUE,
    gShellAcpiViewHiiHandle,
    STRING_TOKEN (STR_GET_HELP_ACPIVIEW)
    );

  return EFI_SUCCESS;
}

/**
  Destructor for the library. free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
UefiShellAcpiViewCommandLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellAcpiViewHiiHandle != NULL) {
    HiiRemovePackages (gShellAcpiViewHiiHandle);
  }
  return EFI_SUCCESS;
}
