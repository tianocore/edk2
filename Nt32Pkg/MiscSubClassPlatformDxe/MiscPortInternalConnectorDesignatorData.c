/**@file

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MiscPortInternalConnectorDesignatorData.c
  
Abstract: 

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortInternalConnectorDesignator) = {
  STRING_TOKEN(STR_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR),  // PortInternalConnectorDesignator
  STRING_TOKEN(STR_MISC_PORT_EXTERNAL_CONNECTOR_DESIGNATOR),  // PortExternalConnectorDesignator
  EfiPortConnectorTypeOther,  // PortInternalConnectorType
  EfiPortConnectorTypeOther,  // PortExternalConnectorType
  EfiPortTypeNone,            // PortType
  0                           // PortPath
};

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortKeyboard) = {
  STRING_TOKEN (STR_MISC_PORT_INTERNAL_KEYBOARD),   // PortInternalConnectorDesignator
  STRING_TOKEN (STR_MISC_PORT_EXTERNAL_KEYBOARD),   // PortExternalConnectorDesignator
  EfiPortConnectorTypeNone, // PortInternalConnectorType
  EfiPortConnectorTypePS2,  // PortExternalConnectorType
  EfiPortTypeKeyboard,      // PortType
  // mPs2KbyboardDevicePath                          // PortPath
  //
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortMouse) = {
  STRING_TOKEN (STR_MISC_PORT_INTERNAL_MOUSE),      // PortInternalConnectorDesignator
  STRING_TOKEN (STR_MISC_PORT_EXTERNAL_MOUSE),      // PortExternalConnectorDesignator
  EfiPortConnectorTypeNone, // PortInternalConnectorType
  EfiPortConnectorTypePS2,  // PortExternalConnectorType
  EfiPortTypeMouse,         // PortType
  // mPs2MouseDevicePath                // PortPath
  //
  0
};


MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortCom1) = {
  STRING_TOKEN(STR_MISC_PORT_INTERNAL_COM1),
  STRING_TOKEN(STR_MISC_PORT_EXTERNAL_COM1),
  EfiPortConnectorTypeNone,
  EfiPortConnectorTypeDB9Female,
  EfiPortTypeSerial16550ACompatible,
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortCom2) = {
  STRING_TOKEN(STR_MISC_PORT_INTERNAL_COM2),
  STRING_TOKEN(STR_MISC_PORT_EXTERNAL_COM2),
  EfiPortConnectorTypeNone,
  EfiPortConnectorTypeDB9Female,
  EfiPortTypeSerial16550ACompatible,
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortExtensionPower) = {
  STRING_TOKEN(STR_MISC_PORT_INTERNAL_EXTENSION_POWER),
  STRING_TOKEN(STR_MISC_PORT_EXTERNAL_EXTENSION_POWER),
  EfiPortConnectorTypeOther,
  EfiPortConnectorTypeNone,
  EfiPortTypeOther,
  0
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortFloppy) = {
  STRING_TOKEN(STR_MISC_PORT_INTERNAL_FLOPPY),
  STRING_TOKEN(STR_MISC_PORT_EXTERNAL_FLOPPY),
  EfiPortConnectorTypeOnboardFloppy,
  EfiPortConnectorTypeNone,
  EfiPortTypeOther,
  0
};

/* eof - MiscPortInternalConnectorDesignatorData.c */
