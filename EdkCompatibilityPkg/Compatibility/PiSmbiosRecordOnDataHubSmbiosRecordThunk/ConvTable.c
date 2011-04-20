/** @file
  The conversion table that guides the generation of the Smbios struture list.
  
Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Thunk.h"

///
/// The minimal length includes last two zero bytes.
///
SMBIOS_TYPE_INFO_TABLE_ENTRY  mTypeInfoTable[] = {
  //
  // Type 0: Bios Information
  //
  {
    0,
    0x1a,
    TRUE,
    FALSE
  },  // size includes wo extension bytes
  //
  // Type 1: System Information
  //
  {
    1,
    0x1d,
    TRUE,
    FALSE
  },
  //
  // Type 2: Base Board Information
  //
  {
    2,
    0x12,
    FALSE,
    FALSE
  },
  //
  // Type 3: System Enclosure or Chassis
  //
  {
    3,
    0x17, // 0x13 covers till OEM-defined, not right
    TRUE,
    FALSE
  },
  //
  // Type 4: Processor
  //
  {
    4,
    0x2C,
    TRUE,
    FALSE
  },
  //
  // Type 5: Memory Controller
  //
  {
    5,
    0x12,
    FALSE,
    FALSE
  },  
  //
  // Type 6: Memory
  //
  {
    6,
    0x0E,
    FALSE,
    FALSE
  },
  //
  // Type 7: Cache
  //
  {
    7,
    0x15,
    TRUE,
    FALSE
  },
  //
  // Type 8: Port Connector Information
  //
  {
    8,
    0x0B,
    FALSE,
    FALSE
  },
  //
  // Type 9: System Slots
  //
  {
    9,
    0x0f,
    TRUE,
    FALSE
  },
  //
  // Type 10: On Board Device Information
  //
  {
    10,
    0x8,
    FALSE,
    FALSE
  },
  //
  // Type 11: OEM Strings
  //
  {
    11, 
    0x7, 
    FALSE, 
    FALSE
  },
  //
  // Type 12: System Configuration Options
  //
  {
    12,
    0x7,
    FALSE,
    FALSE
  },
  //
  // Type 13: BIOS Language Information
  //
  {
    13,
    0x18,
    FALSE,
    FALSE
  },
  //
  // Type 15: System Event Log
  //
  {
    15,
    0x19,
    FALSE,
    FALSE
  },  
  //
  // Type 16: Physical Memory Array
  //
  {
    16,
    0x11,
    TRUE,
    FALSE
  },
  //
  // Type 17: Memory Device
  //
  {
    17,
    0x1d,
    TRUE,
    FALSE
  },
  //
  // Type 18: 32 bit Memory Error Information
  //
  {
    18,
    0x19,
    FALSE,
    FALSE
  },  
  //
  // Type 19: Memory Array Mapped Address
  //
  {
    19,
    0x11,
    TRUE,
    FALSE
  },
  //
  // Type 20: Memory Device Mapped Address
  //
  {
    20,
    0x15,
    TRUE,
    FALSE
  },
  //
  // Type 21: Pointing Device
  //
  {
    21,
    0x9,
    FALSE,
    FALSE
  },
  //
  // Type 22: Portable Battery
  //
  {
    22,
    0x1c,
    FALSE,
    FALSE
  },  
  //
  // Type 23: System Reset
  //
  {
    23,
    0x0f,
    FALSE,
    FALSE
  },
  //
  // Type 24: Hardware Security
  //
  {
    24,
    0x07,
    FALSE,
    FALSE
  },
  //
  // Type 25: System Power Controls
  //
  {
    25,
    0x0b,
    FALSE,
    FALSE
  },
  //
  // Type 26: Voltage Probe
  //
  {
    26,
    0x18,
    FALSE,
    FALSE
  },
  //
  // Type 27: Cooling Device
  //
  {
    27,
    0x10,
    FALSE,
    FALSE
  },
  //
  // Type 28: Temperature Probe
  //
  {
    28,
    0x18,
    FALSE,
    FALSE
  },
  //
  // Type 29: Electrical Current Probe
  //
  {
    29,
    0x18,
    FALSE,
    FALSE
  },
  //
  // Type 30: Out-of-Band Remote Access
  //
  {
    30,
    0x08,
    FALSE,
    FALSE
  },         
  //
  // Type 31: BIS Entry Point
  //
  {
    31,
    0x1c,
    FALSE,
    FALSE
  },   
  //
  // Type 32: System Boot Information
  //
  {
    32,
    0x16,
    TRUE,
    FALSE
  },
  //
  // Type 33: 64 bit Memory Error Information
  //
  {
    33,
    0x21,
    FALSE,
    FALSE
  },  
  //
  // Type 34: Management Device
  //
  {
    34,
    0x0d,
    FALSE,
    FALSE
  },
  //
  // Type 35: Management Device Component
  //
  {
    35,
    0x0d,
    FALSE,
    FALSE
  },
  //
  // Type 36: Management Device Threshold
  //
  {
    36,
    0x12,
    FALSE,
    FALSE
  },        
  //
  // Type 37: Memory Channel
  //
  {
    37,
    0x0c,
    FALSE,
    FALSE
  },
  //
  // Type 38: IPMI device info
  //
  {
    38,
    0x12,
    TRUE,
    FALSE
  },
  //
  // Type 39: Power supply
  //
  {
    39,
    0x18,
    FALSE,
    FALSE
  },  
  //
  // Type 0x80-0xFF: OEM type
  //
  {
    0x80,
    0x6,
    FALSE,
    FALSE
  },
  //
  // Type 127: End of Table
  //
  {
    127,
    0x6,
    FALSE,
    FALSE
  },
  //
  // Terminator
  //
  {
    0,
    0
  }
};

SMBIOS_CONVERSION_TABLE_ENTRY mConversionTable[] = {

  {
    //
    // Processor Sub Class -- Record Type 1: Frequency
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorCoreFrequencyRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x16,
    SmbiosFldBase10ToWordWithMega
  },

  {
    //
    // Processor SubClass -- Record Type 2: Bus Frequency
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorFsbFrequencyRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x12,
    SmbiosFldBase10ToWordWithMega
  },

  {
    //
    // Processor SubClass -- Record Type 3: Version
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorVersionRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x10,
    SmbiosFldString
  },

  {
    //
    // Processor SubClass -- Record Type 4: Manufacturor
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorManufacturerRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x7,
    SmbiosFldString
  },

  {
    //
    // Processor SubClass -- Record Type 5: Serial Number
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorSerialNumberRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x20,
    SmbiosFldString
  },

  {
    //
    // Processor SubClass -- Record Type 6: ID
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorIdRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x08,
    SmbiosFldProcessorType6
  },

  {
    //
    // Processor SubClass -- Record Type 7: Type
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorTypeRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x05,
    SmbiosFldTruncateToByte
  },

  {
    //
    // Processor SubClass -- Record Type 8: Family
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorFamilyRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x06,
    SmbiosFldTruncateToByte
  },

  {
    //
    // Processor SubClass -- Record Type 9: Voltage
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorVoltageRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x11,
    SmbiosFldProcessorType9
  },

  {
    //
    // Processor SubClass -- Record Type 14: Status
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorStatusRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x18,
    SmbiosFldTruncateToByte
  },

  {
    //
    // Processor SubClass -- Record Type 15: Socket Type
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorSocketTypeRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x19,
    SmbiosFldTruncateToByte
  },

  {
    //
    // Processor SubClass -- Record Type 16: Socket Name
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorSocketNameRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x04,
    SmbiosFldString
  },

  {
    //
    // Processor SubClass -- Record Type 17: Cache Associtation
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    CacheAssociationRecordType,
    4,
    BySubClassInstanceProducer,
    ByFunctionWithWholeDataRecord,
    0,
    SmbiosFldProcessorType17
  },

  {
    //
    // Processor Sub Class -- Record Type 18: MaxFrequency
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorMaxCoreFrequencyRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x14,
    SmbiosFldBase10ToWordWithMega
  },

  {
    //
    // Processor SubClass -- Record Type 19: Asset Tag
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorAssetTagRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x21,
    SmbiosFldString
  },

  {
    //
    // Processor Sub Class -- Record Type 25: Core Count
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorCoreCountRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x23,
    SmbiosFldTruncateToByte
  },
  
  {
    //
    // Processor Sub Class -- Record Type 26: Enabled Core Count
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorEnabledCoreCountRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x24,
    SmbiosFldTruncateToByte
  },
  
  {
    //
    // Processor Sub Class -- Record Type 27: Thread Count
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorThreadCountRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x25,
    SmbiosFldTruncateToByte
  },

  {
    //
    // Processor Sub Class -- Record Type 28: Processor Characteristics
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorCharacteristicsRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x26,
    SmbiosFldTruncateToWord
  },
  
  {
    //
    // Processor Sub Class -- Record Type 29: Family 2
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorFamily2RecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x28,
    SmbiosFldTruncateToWord
  },
  
  {
    //
    // Processor Sub Class -- Record Type 30: Part Number
    //
    EFI_PROCESSOR_SUBCLASS_GUID,
    ProcessorPartNumberRecordType,
    4,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x22,
    SmbiosFldString
  },

  {
    //
    // Cache SubClass -- Record Type 1: Size
    //
    EFI_CACHE_SUBCLASS_GUID,
    CacheSizeRecordType,
    7,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x09,
    SmbiosFldBase2ToWordWithKilo
  },

  {
    //
    // Cache SubClass -- Record Type 2: Max Size
    //
    EFI_CACHE_SUBCLASS_GUID,
    MaximumSizeCacheRecordType,
    7,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x07,
    SmbiosFldBase2ToWordWithKilo
  },

  {
    //
    // Cache SubClass -- Record Type 3: Speed
    //
    EFI_CACHE_SUBCLASS_GUID,
    CacheSpeedRecordType,
    7,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x0f,
    SmbiosFldBase10ToByteWithNano
  },

  {
    //
    // Cache SubClass -- Record Type 4: Socket
    //
    EFI_CACHE_SUBCLASS_GUID,
    CacheSocketRecordType,
    7,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x04,
    SmbiosFldString
  },

  {
    //
    // Cache SubClass -- Record Type 5: Supported SRAM type
    //
    EFI_CACHE_SUBCLASS_GUID,
    CacheSramTypeRecordType,
    7,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x0b,
    SmbiosFldCacheType5  // Asynchronous and Synchronous are reversed
  },

  {
    //
    // Cache SubClass -- Record Type 6: Installed SRAM type
    //
    EFI_CACHE_SUBCLASS_GUID,
    CacheInstalledSramTypeRecordType,
    7,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x0d,
    SmbiosFldCacheType5
  },

  {
    //
    // Cache SubClass -- Record Type 7: error correction type
    //
    EFI_CACHE_SUBCLASS_GUID,
    CacheErrorTypeRecordType,
    7,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x10,
    SmbiosFldTruncateToByte
  },

  {
    //
    // Cache SubClass -- Record Type 8: cache type
    //
    EFI_CACHE_SUBCLASS_GUID,
    CacheTypeRecordType,
    7,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x11,
    SmbiosFldTruncateToByte
  },

  {
    //
    // Cache SubClass -- Record Type 9: Associativity
    //
    EFI_CACHE_SUBCLASS_GUID,
    CacheAssociativityRecordType,
    7,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x12,
    SmbiosFldTruncateToByte
  },

  {
    //
    // Cache SubClass -- Record Type 10: Cache configuration
    //
    EFI_CACHE_SUBCLASS_GUID,
    CacheConfigRecordType,
    7,
    BySubclassInstanceSubinstanceProducer,
    ByFunctionWithOffsetSpecified,
    0x05,
    SmbiosFldCacheType10
  },

  {
    //
    // Memory SubClass -- Record Type 2: Physical Memory Array
    //
    EFI_MEMORY_SUBCLASS_GUID,
    EFI_MEMORY_ARRAY_LOCATION_RECORD_NUMBER,
    16,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMemoryType2
  },

  {
    //
    // Memory SubClass -- Record Type 3: Memory Device to SMBIOS type 6
    //
    EFI_MEMORY_SUBCLASS_GUID,
    EFI_MEMORY_ARRAY_LINK_RECORD_NUMBER,
    6,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldSMBIOSType6
  },

  {
    //
    // Memory SubClass -- Record Type 3: Memory Device to SMBIOS type 17
    //
    EFI_MEMORY_SUBCLASS_GUID,
    EFI_MEMORY_ARRAY_LINK_RECORD_NUMBER,
    17,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMemoryType3
  },

  {
    //
    // Memory SubClass -- Record Type 4: Memory Array Mapped Address
    //
    EFI_MEMORY_SUBCLASS_GUID,
    EFI_MEMORY_ARRAY_START_ADDRESS_RECORD_NUMBER,
    19,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMemoryType4
  },

  {
    //
    // Memory SubClass -- Record Type 5: Memory Device Mapped Address
    //
    EFI_MEMORY_SUBCLASS_GUID,
    EFI_MEMORY_DEVICE_START_ADDRESS_RECORD_NUMBER,
    20,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMemoryType5
  },

  {
    //
    // Memory SubClass -- Record Type 6: Memory Channel Type
    //
    EFI_MEMORY_SUBCLASS_GUID,
    EFI_MEMORY_CHANNEL_TYPE_RECORD_NUMBER,
    37,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMemoryType6
  },

  {
    //
    // Memory SubClass -- Record Type 7: Memory Channel Device
    //
    EFI_MEMORY_SUBCLASS_GUID,
    EFI_MEMORY_CHANNEL_DEVICE_RECORD_NUMBER,
    37,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMemoryType7
  },

  {
    //
    // Memory SubClass -- Record Type 8: Memory Controller information
    //
    EFI_MEMORY_SUBCLASS_GUID,
    EFI_MEMORY_CONTROLLER_INFORMATION_RECORD_NUMBER,
    5,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMemoryType8
  },

  {
    //
    // Memory SubClass -- Record Type 9: Memory 32 Bit Error Information
    //
    EFI_MEMORY_SUBCLASS_GUID,
    EFI_MEMORY_32BIT_ERROR_INFORMATION_RECORD_NUMBER,
    18,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMemoryType9
  },

  {
    //
    // Memory SubClass -- Record Type 10: Memory 64 Bit Error Information
    //
    EFI_MEMORY_SUBCLASS_GUID,
    EFI_MEMORY_64BIT_ERROR_INFORMATION_RECORD_NUMBER,
    33,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMemoryType10
  },

  {
    //
    // Misc SubClass -- Record Type 2: Bios Information (SMBIOS Type 0)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_BIOS_VENDOR_RECORD_NUMBER, // 0,
    0,                                  // smbios Type 0
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType0
  },

  {
    //
    // Misc SubClass -- Record Type 3: System Information (SMBIOS Type 1)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_SYSTEM_MANUFACTURER_RECORD_NUMBER, // 1,
    1,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType1
  },

  {
    //
    // Misc SubClass -- Record Type 4: Base Board Manufacturer (SMBIOS Type 2)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_BASE_BOARD_MANUFACTURER_RECORD_NUMBER, // 2,
    2,  // SMBIOS Type 2
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType2
  },

  {
    //
    // Misc SubClass -- Record Type 5: System Enclosure or Chassis (SMBIOS Type 3)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_CHASSIS_MANUFACTURER_RECORD_NUMBER,  // 3,
    3,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType3
  },

  {
    //
    // Misc SubClass -- Record Type 6: Port Connector (SMBIOS Type 8)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_RECORD_NUMBER,  // 8,
    8,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType8
  },

  {
    //
    // Misc SubClass -- Record Type 7: System Slots (SMBIOS Type 9)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_SYSTEM_SLOT_DESIGNATION_RECORD_NUMBER, // 9,
    9,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType9
  },

  {
    //
    // Misc SubClass -- Record Type 8: Onboard Device (SMBIOS Type 10)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_ONBOARD_DEVICE_RECORD_NUMBER,  // 10,
    10,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType10
  },

  {
    //
    // Misc Subclass -- Record Type 9: OEM strings (SMBIOS Type 11)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_OEM_STRING_RECORD_NUMBER,  // 11,
    11,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType11
  },

  {
    //
    // Misc SubClass -- Record Type 0A: System Options (SMBIOS Type 12)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_SYSTEM_OPTION_STRING_RECORD_NUMBER,  // 12,
    12,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType12
  },

  {
    //
    // Misc SubClass -- Record Type 0B: Number of Installable Languages (SMBIOS Type 13)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_RECORD_NUMBER, // 13,
    13,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType13
  },

  {
    //
    // Misc SubClass -- Record Type 0C: Installable Languages (SMBIOS Type 13)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_SYSTEM_LANGUAGE_STRING_RECORD_NUMBER, // 13,
    13,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType14
  },

  {
    //
    // Misc SubClass -- Record Type 20: System Event Log (SMBIOS Type 15)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_SYSTEM_EVENT_LOG_RECORD_NUMBER, // 15,
    15,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType15
  },  
  
  {
    //
    // Misc SubClass -- Record Type 0F: Pointing Device (SMBIOS Type 21)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_POINTING_DEVICE_TYPE_RECORD_NUMBER,  // 21,
    21,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType21
  },

  {
    //
    // Misc SubClass -- Record Type 10: Portable Battery (SMBIOS Type 22)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_PORTABLE_BATTERY_RECORD_NUMBER, // 22,
    22,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType22
  },  

  {
    //
    // Misc SubClass -- Record Type 0x11: Reset Capabilities (SMBIOS Type 23)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_RESET_CAPABILITIES_RECORD_NUMBER,  // 23,
    23,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType23
  },

  {
    //
    // Misc SubClass -- Record Type 0x12: Hardware Security (SMBIOS Type 24)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_HARDWARE_SECURITY_SETTINGS_DATA_RECORD_NUMBER,  // 24,
    24,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType24
  },

  {
    //
    // Misc SubClass -- Record Type 0x13: System Power Controls (SMBIOS Type 25)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_SCHEDULED_POWER_ON_MONTH_RECORD_NUMBER,  // 25,
    25,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType25
  },

  {
    //
    // Misc SubClass -- Record Type 0x14: System Power Controls (SMBIOS Type 26)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_VOLTAGE_PROBE_DESCRIPTION_RECORD_NUMBER,  // 26,
    26,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType26
  },

  {
    //
    // Misc SubClass -- Record Type 0x15: Cooling Device (SMBIOS Type 27)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_COOLING_DEVICE_TEMP_LINK_RECORD_NUMBER,  // 27,
    27,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType27
  },

  {
    //
    // Misc SubClass -- Record Type 0x16: Temperature Probe (SMBIOS Type 28)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_TEMPERATURE_PROBE_DESCRIPTION_RECORD_NUMBER,  // 28,
    28,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType28
  },

  {
    //
    // Misc SubClass -- Record Type 0x17: Electrical Current Probe (SMBIOS Type 29)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_ELECTRICAL_CURRENT_PROBE_DESCRIPTION_RECORD_NUMBER,  // 29,
    29,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType29
  },

  {
    //
    // Misc SubClass -- Record Type 0x18: Temperature Probe (SMBIOS Type 30)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_REMOTE_ACCESS_MANUFACTURER_DESCRIPTION_RECORD_NUMBER,  // 30,
    30,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType30
  },

  {
    //
    // Misc SubClass -- Record Type 0x1A: Boot Information (SMBIOS Type 32)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_BOOT_INFORMATION_STATUS_RECORD_NUMBER, // 32,
    32,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType32
  },

  {
    //
    // Misc SubClass -- Record Type 0x1B: Management Device (SMBIOS Type 34)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_MANAGEMENT_DEVICE_DESCRIPTION_RECORD_NUMBER, // 34,
    34,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType34
  },

  {
    //
    // Misc SubClass -- Record Type 0x1C: Management Device Component (SMBIOS Type 35)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_MANAGEMENT_DEVICE_COMPONENT_DESCRIPTION_RECORD_NUMBER, // 35,
    35,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType35
  },

  {
    //
    // Misc SubClass -- Record Type 0x21: Management Device Threshold (SMBIOS Type 36)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_MANAGEMENT_DEVICE_THRESHOLD_RECORD_NUMBER, // 36,
    36,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType36
  },

  {
    //
    // Misc SubClass -- Record Type 0x1D: Boot Information (SMBIOS Type 38)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_IPMI_INTERFACE_TYPE_RECORD_NUMBER, // 38,
    38,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType38
  },

  {
    //
    // Misc SubClass -- Record Type 0x1E: Power supply (SMBIOS Type 39)
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_SYSTEM_POWER_SUPPLY_RECORD_NUMBER, // 39,
    39,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType39
  },

  {
    //
    // Misc SubClass -- Record Type 0x80-0xFF: OEM type
    //
    EFI_MISC_SUBCLASS_GUID,
    EFI_MISC_SMBIOS_STRUCT_ENCAP_RECORD_NUMBER, // 0x80,
    0x80,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscTypeOEM
  },

  {
    //
    // End-of-Table -- Record Type 127
    //
    EFI_MISC_SUBCLASS_GUID,
    127,
    127,
    BySubclassInstanceSubinstanceProducer,
    ByFunction,
    0,
    SmbiosFldMiscType127
  },
  //
  // Table Terminator
  //
  {
    {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    0,
    0,
    (SMBIOS_STRUCTURE_LOCATING_METHOD) 0,
    (SMBIOS_FIELD_FILLING_METHOD) 0,
    0,
    0
  }
};
