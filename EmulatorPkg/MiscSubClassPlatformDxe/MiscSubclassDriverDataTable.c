/*++

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MiscSubclassDriverDataTable.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubClassDriver.h"


//
// External definitions referenced by Data Table entries.
//
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_BASE_BOARD_MANUFACTURER_DATA, MiscBaseBoardManufacturer, MiscBaseBoardManufacturer);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_BIOS_VENDOR_DATA, MiscBiosVendor,MiscBiosVendor );
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_BOOT_INFORMATION_STATUS_DATA, BootInformationStatus, BootInformationStatus);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_CHASSIS_MANUFACTURER_DATA, MiscChassisManufacturer, MiscChassisManufacturer);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA,NumberOfInstallableLanguages, NumberOfInstallableLanguages);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_OEM_STRING_DATA,OemString, OemString);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortInternalConnectorDesignator, MiscPortInternalConnectorDesignator);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortKeyboard, MiscPortInternalConnectorDesignator);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortMouse, MiscPortInternalConnectorDesignator);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortCom1, MiscPortInternalConnectorDesignator);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortCom2, MiscPortInternalConnectorDesignator);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortExtensionPower, MiscPortInternalConnectorDesignator);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortFloppy, MiscPortInternalConnectorDesignator);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_RESET_CAPABILITIES, MiscResetCapabilities, MiscResetCapabilities);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA,SystemLanguageString, SystemLanguageString);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_SYSTEM_MANUFACTURER_DATA, MiscSystemManufacturer, MiscSystemManufacturer);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_SYSTEM_OPTION_STRING_DATA, SystemOptionString, SystemOptionString);
MISC_SMBIOS_TABLE_EXTERNS ( EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotDesignation, MiscSystemSlotDesignation);


//
// Data Table.
//
EFI_MISC_SMBIOS_DATA_TABLE  mMiscSubclassDataTable[] = {
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscBaseBoardManufacturer, MiscBaseBoardManufacturer),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscBiosVendor,MiscBiosVendor ),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( BootInformationStatus, BootInformationStatus),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscChassisManufacturer, MiscChassisManufacturer),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(NumberOfInstallableLanguages, NumberOfInstallableLanguages),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(OemString, OemString),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscPortInternalConnectorDesignator, MiscPortInternalConnectorDesignator),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscPortKeyboard, MiscPortInternalConnectorDesignator),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscPortMouse, MiscPortInternalConnectorDesignator),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscPortCom1, MiscPortInternalConnectorDesignator),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscPortCom2, MiscPortInternalConnectorDesignator),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscPortExtensionPower, MiscPortInternalConnectorDesignator),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscPortFloppy, MiscPortInternalConnectorDesignator),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscResetCapabilities, MiscResetCapabilities),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(SystemLanguageString, SystemLanguageString),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscSystemManufacturer, MiscSystemManufacturer),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( SystemOptionString, SystemOptionString),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION( MiscSystemSlotDesignation, MiscSystemSlotDesignation),
  };

//
// Number of Data Table entries.
//
UINTN mMiscSubclassDataTableEntries = (sizeof mMiscSubclassDataTable) / sizeof (EFI_MISC_SMBIOS_DATA_TABLE);

/* eof - MiscSubclassDriverDataTable.c */
