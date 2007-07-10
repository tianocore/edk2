/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MiscSubclassDriverDataTable.c
  
Abstract: 

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

--*/

#include "MiscSubclassDriver.h"

//
// External definitions referenced by Data Table entries.
//
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_CHASSIS_MANUFACTURER_DATA,
  MiscChassisManufacturer
  );
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_BIOS_VENDOR_DATA,
  MiscBiosVendor
  );
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_SYSTEM_MANUFACTURER_DATA,
  MiscSystemManufacturer
  );
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_BASE_BOARD_MANUFACTURER_DATA,
  MiscBaseBoardManufacturer
  );
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA,
  MiscPortInternalConnectorDesignator
  );
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA,
  MiscPortKeyboard
  );
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA,
  MiscPortMouse
  );
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA,
  MiscPortCom1
  );
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA,
  MiscPortCom2
  );
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA,
  MiscSystemSlotDesignation
  );
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_OEM_STRING_DATA,
  OemString
  );
MISC_SUBCLASS_TABLE_EXTERNS (
  EFI_MISC_SYSTEM_OPTION_STRING_DATA,
  SystemOptionString
  );

//
// Data Table.
//
EFI_MISC_SUBCLASS_DATA_TABLE  mMiscSubclassDataTable[] = {
  MISC_SUBCLASS_TABLE_ENTRY_DATA_AND_FUNCTION(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortKeyboard, MiscPortInternalConnectorDesignator),
  MISC_SUBCLASS_TABLE_ENTRY_DATA_AND_FUNCTION(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortMouse, MiscPortInternalConnectorDesignator),
  MISC_SUBCLASS_TABLE_ENTRY_DATA_AND_FUNCTION(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortCom1, MiscPortInternalConnectorDesignator),
  MISC_SUBCLASS_TABLE_ENTRY_DATA_AND_FUNCTION(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortCom2, MiscPortInternalConnectorDesignator),
  MISC_SUBCLASS_TABLE_ENTRY_DATA_ONLY(EFI_MISC_BIOS_VENDOR, MiscBiosVendor),
  MISC_SUBCLASS_TABLE_ENTRY_DATA_ONLY(EFI_MISC_SYSTEM_MANUFACTURER, MiscSystemManufacturer),
  MISC_SUBCLASS_TABLE_ENTRY_DATA_ONLY(EFI_MISC_BASE_BOARD_MANUFACTURER, MiscBaseBoardManufacturer),
  MISC_SUBCLASS_TABLE_ENTRY_DATA_ONLY(EFI_MISC_CHASSIS_MANUFACTURER, MiscChassisManufacturer),
  MISC_SUBCLASS_TABLE_ENTRY_DATA_ONLY(EFI_MISC_SYSTEM_SLOT_DESIGNATION, MiscSystemSlotDesignation),
  MISC_SUBCLASS_TABLE_ENTRY_DATA_ONLY(EFI_MISC_OEM_STRING, OemString),
  MISC_SUBCLASS_TABLE_ENTRY_DATA_ONLY(EFI_MISC_SYSTEM_OPTION_STRING, SystemOptionString),
};

//
// Number of Data Table entries.
//
UINTN mMiscSubclassDataTableEntries = (sizeof mMiscSubclassDataTable) / sizeof (EFI_MISC_SUBCLASS_DATA_TABLE);

/* eof - MiscSubclassDriverDataTable.c */
