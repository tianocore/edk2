/** @file
  Main file for 'acpiview' Shell command function.

  Copyright (c) 2016 - 2019, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/ShellLibHiiGuid.h>
#include <IndustryStandard/Acpi.h>
#include <Library/HiiLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Uefi.h>
#include "AcpiParser.h"
#include "AcpiTableParser.h"
#include "AcpiView.h"
#include "UefiShellAcpiViewCommandLib.h"

CONST CHAR16 gShellAcpiViewFileName[] = L"ShellCommand";

/**
  A list of available table parsers.
*/
STATIC
CONST
ACPI_TABLE_PARSER ParserList[] = {
  {EFI_ACPI_6_2_BOOT_GRAPHICS_RESOURCE_TABLE_SIGNATURE, ParseAcpiBgrt},
  {EFI_ACPI_6_2_DEBUG_PORT_2_TABLE_SIGNATURE, ParseAcpiDbg2},
  {EFI_ACPI_6_2_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
   ParseAcpiDsdt},
  {EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE, ParseAcpiFadt},
  {EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE, ParseAcpiGtdt},
  {EFI_ACPI_6_2_IO_REMAPPING_TABLE_SIGNATURE, ParseAcpiIort},
  {EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE, ParseAcpiMadt},
  {EFI_ACPI_6_2_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
   ParseAcpiMcfg},
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
