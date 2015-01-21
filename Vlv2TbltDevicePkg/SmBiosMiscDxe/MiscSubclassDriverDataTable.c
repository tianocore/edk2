/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscSubclassDriverDataTable.c

Abstract:

  Create the mMiscSubclassDataTable structure, and it is used to report
  any generate data to the DataHub.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// External definitions referenced by Data Table entries.
//
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_BIOS_VENDOR_DATA, MiscBiosVendor, MiscBiosVendor);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_SYSTEM_MANUFACTURER_DATA, MiscSystemManufacturer, MiscSystemManufacturer);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_BASE_BOARD_MANUFACTURER_DATA, MiscBaseBoardManufacturer, MiscBaseBoardManufacturer);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_CHASSIS_MANUFACTURER_DATA, MiscChassisManufacturer, MiscChassisManufacturer);
MISC_SMBIOS_TABLE_EXTERNS(EFI_CACHE_VARIABLE_RECORD, MiscProcessorCache, MiscProcessorCache); //type 7
MISC_SMBIOS_TABLE_EXTERNS(EFI_CPU_DATA_RECORD, MiscProcessorInformation, MiscProcessorInformation); //type 4
MISC_SMBIOS_TABLE_EXTERNS(EFI_MEMORY_ARRAY_LOCATION_DATA, MiscPhysicalMemoryArray,MiscPhysicalMemoryArray);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MEMORY_ARRAY_LINK_DATA, MiscMemoryDevice, MiscMemoryDevice);

MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortIde1, MiscPortInternalConnectorDesignator);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortIde2, MiscPortInternalConnectorDesignator);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA, MiscPortAtxPower, MiscPortInternalConnectorDesignator);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx16Slot1, MiscSystemSlotDesignation);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx16Slot2, MiscSystemSlotDesignation);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx4, MiscSystemSlotDesignation);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx1Slot1, MiscSystemSlotDesignation);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx1Slot2, MiscSystemSlotDesignation);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx1Slot3, MiscSystemSlotDesignation);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCI1, MiscSystemSlotDesignation);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCI2, MiscSystemSlotDesignation);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCI3, MiscSystemSlotDesignation);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_ONBOARD_DEVICE_DATA, MiscOnboardDeviceVideo, MiscOnboardDevice);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_ONBOARD_DEVICE_DATA, MiscOnboardDeviceNetwork, MiscOnboardDevice);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_ONBOARD_DEVICE_DATA, MiscOnboardDeviceAudio, MiscOnboardDevice);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA, NumberOfInstallableLanguages, NumberOfInstallableLanguages);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_SYSTEM_LANGUAGE_STRING_DATA, SystemLanguageString, SystemLanguageString);
MISC_SMBIOS_TABLE_EXTERNS(EFI_MISC_BOOT_INFORMATION_STATUS_DATA, BootInformationStatus, BootInformationStatus);

//
// Data Table.
//
EFI_MISC_SMBIOS_DATA_TABLE  mMiscSubclassDataTable[] = {
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscBiosVendor, MiscBiosVendor),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscSystemManufacturer, MiscSystemManufacturer),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscBaseBoardManufacturer, MiscBaseBoardManufacturer),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscChassisManufacturer, MiscChassisManufacturer),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscProcessorCache, MiscProcessorCache),  //type 7
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscProcessorInformation, MiscProcessorInformation),  //type 4
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscPhysicalMemoryArray, MiscPhysicalMemoryArray),    //Type 16
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscMemoryDevice, MiscMemoryDevice), //Type 17
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscPortIde1, MiscPortInternalConnectorDesignator),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscPortIde2, MiscPortInternalConnectorDesignator),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscPortAtxPower, MiscPortInternalConnectorDesignator),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscSystemSlotPCIEx16Slot1, MiscSystemSlotDesignation),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscSystemSlotPCIEx16Slot2, MiscSystemSlotDesignation),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscSystemSlotPCIEx4, MiscSystemSlotDesignation),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscSystemSlotPCIEx1Slot1, MiscSystemSlotDesignation),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscSystemSlotPCIEx1Slot2, MiscSystemSlotDesignation),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscSystemSlotPCIEx1Slot3, MiscSystemSlotDesignation),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscSystemSlotPCI1, MiscSystemSlotDesignation),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscSystemSlotPCI2, MiscSystemSlotDesignation),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscSystemSlotPCI3, MiscSystemSlotDesignation),
#if defined( ALWAYS_DISABLE_ONBOARD_VIDEO ) && \
    ( ALWAYS_DISABLE_ONBOARD_VIDEO != 0 )
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscOnboardDeviceNetwork, MiscOnboardDevice),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscOnboardDeviceAudio, MiscOnboardDevice),
#else
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscOnboardDeviceVideo, MiscOnboardDevice),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(MiscOnboardDeviceAudio, MiscOnboardDevice),
#endif
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(SystemLanguageString, SystemLanguageString),
  MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(BootInformationStatus, BootInformationStatus),
};

//
// Number of Data Table entries.
//
UINTN mMiscSubclassDataTableEntries =
        (sizeof mMiscSubclassDataTable) / sizeof(EFI_MISC_SMBIOS_DATA_TABLE);
