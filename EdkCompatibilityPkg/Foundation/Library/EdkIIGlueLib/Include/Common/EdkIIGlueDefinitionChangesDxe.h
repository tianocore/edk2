/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  EdkIIGlueDefinitionChangesDxe.h
  
Abstract: 

  Data structure definition changes from EDK to EDKII

--*/

#ifndef __EDKII_GLUE_DEFINITION_CHANGES_DXE_H__
#define __EDKII_GLUE_DEFINITION_CHANGES_DXE_H__

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#include "TianoHii.h"
#else
#include "EfiInternalFormRepresentation.h"
#endif

#include "EdkIIGlueDefinitionChangesBase.h"

#if (TIANO_RELEASE_VERSION <= 0x00080005)
  #define gEfiStatusCodeRuntimeProtocolGuid         gEfiStatusCodeArchProtocolGuid
#endif

//
// typedef Edk types - EdkII types
//
typedef EFI_MEMORY_ARRAY_START_ADDRESS               EFI_MEMORY_ARRAY_START_ADDRESS_DATA;
typedef EFI_MEMORY_DEVICE_START_ADDRESS              EFI_MEMORY_DEVICE_START_ADDRESS_DATA;
typedef EFI_MISC_LAST_PCI_BUS                        EFI_MISC_LAST_PCI_BUS_DATA;
typedef EFI_MISC_BIOS_VENDOR                         EFI_MISC_BIOS_VENDOR_DATA;
typedef EFI_MISC_SYSTEM_MANUFACTURER                 EFI_MISC_SYSTEM_MANUFACTURER_DATA;
typedef EFI_MISC_BASE_BOARD_MANUFACTURER             EFI_MISC_BASE_BOARD_MANUFACTURER_DATA;
typedef EFI_MISC_CHASSIS_MANUFACTURER                EFI_MISC_CHASSIS_MANUFACTURER_DATA;
typedef EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA;
typedef EFI_MISC_SYSTEM_SLOT_DESIGNATION             EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA;
typedef EFI_MISC_ONBOARD_DEVICE                      EFI_MISC_ONBOARD_DEVICE_DATA;
typedef EFI_MISC_ONBOARD_DEVICE_TYPE_DATA            EFI_MISC_PORTING_DEVICE_TYPE_DATA;
typedef EFI_MISC_OEM_STRING                          EFI_MISC_OEM_STRING_DATA;
typedef EFI_MISC_SYSTEM_OPTION_STRING                EFI_MISC_SYSTEM_OPTION_STRING_DATA;
typedef EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES     EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA;
typedef EFI_MISC_SYSTEM_LANGUAGE_STRING              EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA;
typedef EFI_MISC_BIS_ENTRY_POINT                     EFI_MISC_BIS_ENTRY_POINT_DATA;
typedef EFI_MISC_BOOT_INFORMATION_STATUS             EFI_MISC_BOOT_INFORMATION_STATUS_DATA;
typedef EFI_MISC_SYSTEM_POWER_SUPPLY                 EFI_MISC_SYSTEM_POWER_SUPPLY_DATA ;
typedef EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION         EFI_MISC_SMBIOS_STRUCT_ENCAPSULATION_DATA;            

// -------------------
// EdkII Names - Edk Names
// -------------------
#define gEfiAcpiSupportProtocolGuid                   gEfiAcpiSupportGuid
#define gEfiLoadPeImageProtocolGuid                   gEfiLoadPeImageGuid
#if (EFI_SPECIFICATION_VERSION < 0x0002000A)
#define EFI_GLYPH_NON_SPACING                         GLYPH_NON_SPACING
#define EFI_GLYPH_WIDE                                GLYPH_NON_BREAKING
#endif
#define BOOT_OBJECT_AUTHORIZATION_PARMSET_GUID        BOOT_OBJECT_AUTHORIZATION_PARMSET_GUIDVALUE
#define EFI_EBC_PROTOCOL_GUID                         EFI_EBC_INTERPRETER_PROTOCOL_GUID
#define EFI_FILE_SYSTEM_VOLUME_LABEL_ID               EFI_FILE_SYSTEM_VOLUME_LABEL_INFO_ID_GUID
#define EFI_LOADED_IMAGE_PROTOCOL_REVISION            EFI_LOADED_IMAGE_INFORMATION_REVISION
#define EFI_LOAD_FILE_PROTOCOL_GUID                   LOAD_FILE_PROTOCOL_GUID
#define EFI_PXE_BASE_CODE_PROTOCOL_REVISION           EFI_PXE_BASE_CODE_INTERFACE_REVISION
#define EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL_REVISION  EFI_PXE_BASE_CODE_CALLBACK_INTERFACE_REVISION
#define EFI_SERIAL_IO_PROTOCOL_REVISION               SERIAL_IO_INTERFACE_REVISION
#define EFI_FILE_PROTOCOL_REVISION                    EFI_FILE_HANDLE_REVISION
#define EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID           EFI_SIMPLE_TEXT_IN_PROTOCOL_GUID


//
// typedef Edk types - EdkII types
//
typedef EFI_VOLUME_OPEN                              EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME;
typedef EFI_TEXT_OUTPUT_STRING                       EFI_TEXT_STRING;
#if (EFI_SPECIFICATION_VERSION < 0x0002000A)
typedef SCREEN_DESCRIPTOR                            EFI_SCREEN_DESCRIPTOR;
#endif
typedef EFI_SIMPLE_TEXT_IN_PROTOCOL                  EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
typedef EFI_SIMPLE_TEXT_OUT_PROTOCOL                 EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
//typedef EFI_TO_LEGACY16_INIT_TABLE                   EFI_TO_COMPATIBILITY16_INIT_TABLE;
//typedef DISPATCH_OPROM_TABLE                         EFI_DISPATCH_OPROM_TABLE;

#endif
