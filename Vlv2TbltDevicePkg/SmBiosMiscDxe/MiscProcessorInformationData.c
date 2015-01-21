/*++

Copyright (c) 2006  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscOnboardDeviceData.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to smbios.

--*/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"


/*
  EFI_PROCESSOR_CORE_FREQUENCY_LIST_DATA  ProcessorCoreFrequencyList;
  EFI_PROCESSOR_FSB_FREQUENCY_LIST_DATA   ProcessorFsbFrequencyList;
  EFI_PROCESSOR_SERIAL_NUMBER_DATA        ProcessorSerialNumber;
  EFI_PROCESSOR_CORE_FREQUENCY_DATA       ProcessorCoreFrequency;
  EFI_PROCESSOR_FSB_FREQUENCY_DATA        ProcessorFsbFrequency;
  EFI_PROCESSOR_MAX_CORE_FREQUENCY_DATA   ProcessorMaxCoreFrequency;
  EFI_PROCESSOR_MAX_FSB_FREQUENCY_DATA    ProcessorMaxFsbFrequency;
  EFI_PROCESSOR_VERSION_DATA              ProcessorVersion;
  EFI_PROCESSOR_MANUFACTURER_DATA         ProcessorManufacturer;
  EFI_PROCESSOR_ID_DATA                   ProcessorId;
  EFI_PROCESSOR_TYPE_DATA                 ProcessorType;
  EFI_PROCESSOR_FAMILY_DATA               ProcessorFamily;
  EFI_PROCESSOR_VOLTAGE_DATA              ProcessorVoltage;
  EFI_PROCESSOR_APIC_BASE_ADDRESS_DATA    ProcessorApicBase;
  EFI_PROCESSOR_APIC_ID_DATA              ProcessorApicId;
  EFI_PROCESSOR_APIC_VERSION_NUMBER_DATA  ProcessorApicVersionNumber;
  EFI_PROCESSOR_MICROCODE_REVISION_DATA   CpuUcodeRevisionData;
  EFI_PROCESSOR_STATUS_DATA               ProcessorStatus;
  EFI_PROCESSOR_SOCKET_TYPE_DATA          ProcessorSocketType;
  EFI_PROCESSOR_SOCKET_NAME_DATA          ProcessorSocketName;
  EFI_PROCESSOR_ASSET_TAG_DATA            ProcessorAssetTag;
  EFI_PROCESSOR_HEALTH_STATUS             ProcessorHealthStatus;
  EFI_PROCESSOR_PACKAGE_NUMBER_DATA       ProcessorPackageNumber;
} EFI_CPU_VARIABLE_RECORD;
*/


// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_CPU_DATA_RECORD, MiscProcessorInformation) = {

0,
    /*
    STRING_TOKEN (STR_MISC_SOCKET_NAME),		            // Processor Socket Name
    STRING_TOKEN (STR_MISC_PROCESSOR_MAUFACTURER),		    // Processor Manufacturer
    STRING_TOKEN (STR_MISC_PROCESSOR_VERSION),		        // Processor Version Information
	STRING_TOKEN (STR_MISC_PROCESSOR_SERIAL_NUMBER),        // Serial Number
	STRING_TOKEN (STR_MISC_ASSERT_TAG_DATA),	            // Processor Assert TAg Data
    STRING_TOKEN (STR_MISC_PART_NUMBER)	                    // Processor Part Numbe
*/
};



