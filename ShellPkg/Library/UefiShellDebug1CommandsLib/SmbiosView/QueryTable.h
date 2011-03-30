/** @file
  Build a table, each item is (key, info) pair.
  and give a interface of query a string out of a table.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMBIOS_QUERY_TABLE_H
#define _SMBIOS_QUERY_TABLE_H

#define QUERY_TABLE_UNFOUND 0xFF

typedef struct TABLE_ITEM {
  UINT16  Key;
  CHAR16  *Info;
} TABLE_ITEM;

//
// Print info by option
//
#define PRINT_INFO_OPTION(Value, Option) \
  do { \
    if (Option == SHOW_NONE) { \
      return ; \
    } \
    if (Option < SHOW_DETAIL) { \
      Print (L"0x%x\n", Value); \
      return ; \
    } \
  } while (0);

/**
  Given a table and a Key, return the responding info.

  Notes:
    Table[Index].Key is change from UINT8 to UINT16,
    in order to deal with "0xaa - 0xbb".

    For example:
      DisplaySELVariableDataFormatTypes(UINT8 Type, UINT8 Option)
    has a item:
      "0x07-0x7F,   Unused"
    Now define Key = 0x7F07, that is to say: High = 0x7F, Low = 0x07.
    Then all the Key Value between Low and High gets the same string
    L"Unused".

  @param[in] Table    The begin address of table.
  @param[in] Number   The number of table items.
  @param[in] Key      The query Key.
  @param[in,out] Info Input as empty buffer; output as data buffer.
  @param[in] InfoLen  The max number of characters for Info.

  @return the found Key and Info is valid.
  @retval QUERY_TABLE_UNFOUND and Info should be NULL.
**/
UINT8
QueryTable (
  IN  TABLE_ITEM    *Table,
  IN  UINTN         Number,
  IN  UINT8         Key,
  IN  OUT CHAR16    *Info,
  IN  UINTN         InfoLen
  );

VOID
PrintBitsInfo (
  IN  TABLE_ITEM    *Table,
  IN  UINTN         Number,
  IN  UINT32        Bits
  );

//
// Display the element detail information
//
VOID
DisplayStructureTypeInfo (
  UINT8 Key,
  UINT8 Option
  );

//
// System Information (Type 1)
//
VOID
DisplaySystemWakeupType (
  UINT8 Type,
  UINT8 Option
  );

//
// System Enclosure (Type 3)
//
VOID
DisplaySystemEnclosureType (
  UINT8 Type,
  UINT8 Option
  );
VOID
DisplaySystemEnclosureStatus (
  UINT8 Status,
  UINT8 Option
  );
VOID
DisplaySESecurityStatus (
  UINT8 Status,
  UINT8 Option
  );

//
// Processor Information (Type 4)
//
VOID
DisplayProcessorType (
  UINT8 Type,
  UINT8 Option
  );
VOID
DisplayProcessorUpgrade (
  UINT8 Upgrade,
  UINT8 Option
  );

//
// Memory Controller Information (Type 5)
//
VOID
DisplayMcErrorDetectMethod (
  UINT8 Method,
  UINT8 Option
  );
VOID
DisplayMcErrorCorrectCapability (
  UINT8 Capability,
  UINT8 Option
  );
VOID
DisplayMcInterleaveSupport (
  UINT8 Support,
  UINT8 Option
  );
VOID
DisplayMcMemorySpeeds (
  UINT16  Speed,
  UINT8   Option
  );
VOID
DisplayMemoryModuleVoltage (
  UINT8 Voltage,
  UINT8 Option
  );

//
// Memory Module Information (Type 6)
//
VOID
DisplayMmMemoryType (
  UINT16  Type,
  UINT8   Option
  );
VOID
DisplayMmErrorStatus (
  UINT8 Status,
  UINT8 Option
  );

//
// Cache Information (Type 7)
//
VOID
DisplayCacheSRAMType (
  UINT16  Type,
  UINT8   Option
  );
VOID
DisplayCacheErrCorrectingType (
  UINT8 Type,
  UINT8 Option
  );
VOID
DisplayCacheSystemCacheType (
  UINT8 Type,
  UINT8 Option
  );
VOID
DisplayCacheAssociativity (
  UINT8 Associativity,
  UINT8 Option
  );

//
// Port Connector Information  (Type 8)
//
VOID
DisplayPortConnectorType (
  UINT8 Type,
  UINT8 Option
  );
VOID
DisplayPortType (
  UINT8 Type,
  UINT8 Option
  );

//
// System Slots (Type 9)
//
VOID
DisplaySystemSlotType (
  UINT8 Type,
  UINT8 Option
  );
VOID
DisplaySystemSlotDataBusWidth (
  UINT8 Width,
  UINT8 Option
  );
VOID
DisplaySystemSlotCurrentUsage (
  UINT8 Usage,
  UINT8 Option
  );
VOID
DisplaySystemSlotLength (
  UINT8 Length,
  UINT8 Option
  );
VOID
DisplaySlotCharacteristics1 (
  UINT8 Chara1,
  UINT8 Option
  );
VOID
DisplaySlotCharacteristics2 (
  UINT8 Chara2,
  UINT8 Option
  );

//
// On Board Devices Information (Type 10)
//
VOID
DisplayOnboardDeviceTypes (
  UINT8 Type,
  UINT8 Option
  );

//
// System Event Log (Type 15)
//
VOID
DisplaySELTypes (
  UINT8 Type,
  UINT8 Option
  );
VOID
DisplaySELVarDataFormatType (
  UINT8 Type,
  UINT8 Option
  );
VOID
DisplayPostResultsBitmapDw1 (
  UINT32  Key,
  UINT8   Option
  );
VOID
DisplayPostResultsBitmapDw2 (
  UINT32  Key,
  UINT8   Option
  );
VOID
DisplaySELSysManagementTypes (
  UINT32  SMType,
  UINT8   Option
  );

//
// Physical Memory Array (Type 16)
//
VOID
DisplayPMALocation (
  UINT8 Location,
  UINT8 Option
  );
VOID
DisplayPMAUse (
  UINT8 Use,
  UINT8 Option
  );
VOID
DisplayPMAErrorCorrectionTypes (
  UINT8 Type,
  UINT8 Option
  );

//
// Memory Device (Type 17)
//
VOID
DisplayMemoryDeviceFormFactor (
  UINT8 FormFactor,
  UINT8 Option
  );
VOID
DisplayMemoryDeviceType (
  UINT8 Type,
  UINT8 Option
  );
VOID
DisplayMemoryDeviceTypeDetail (
  UINT16  Parameter,
  UINT8   Option
  );

//
// 32-bit Memory Error Information (Type 18)
//
VOID
DisplayMemoryErrorType (
  UINT8 ErrorType,
  UINT8 Option
  );
VOID
DisplayMemoryErrorGranularity (
  UINT8 Granularity,
  UINT8 Option
  );
VOID
DisplayMemoryErrorOperation (
  UINT8 Operation,
  UINT8 Option
  );

//
// Memory Array Mapped Address (Type 19)
// Memory Device Mapped Address  (Type 20)
//
// Built-in Pointing Device  (Type 21)
//
VOID
DisplayPointingDeviceType (
  UINT8 Type,
  UINT8 Option
  );
VOID
DisplayPointingDeviceInterface (
  UINT8   Interface,
  UINT8   Option
  );

//
// Portable Battery  (Type 22)
//
VOID
DisplayPBDeviceChemistry (
  UINT8 Key,
  UINT8 Option
  );

//
// Voltage Probe (Type 26)
//
VOID
DisplayVPLocation (
  UINT8 Key,
  UINT8 Option
  );
VOID
DisplayVPStatus (
  UINT8 Key,
  UINT8 Option
  );

//
// Voltage Probe (Type 27)
//
VOID
DisplayCoolingDeviceStatus (
  UINT8 Key,
  UINT8 Option
  );
VOID
DisplayCoolingDeviceType (
  UINT8 Key,
  UINT8 Option
  );

//
// Temperature Probe  (Type 28)
//
VOID
DisplayTemperatureProbeStatus (
  UINT8 Key,
  UINT8 Option
  );
VOID
DisplayTemperatureProbeLoc (
  UINT8 Key,
  UINT8 Option
  );

//
// Electrical Current Probe (Type 29)
//
VOID
DisplayECPStatus (
  UINT8 Key,
  UINT8 Option
  );
VOID
DisplayECPLoc (
  UINT8 Key,
  UINT8 Option
  );

//
// Management Device  (Type 34)
//
VOID
DisplayMDType (
  UINT8 Key,
  UINT8 Option
  );
VOID
DisplayMDAddressType (
  UINT8 Key,
  UINT8 Option
  );

//
// Memory Channel  (Type 37)
//
VOID
DisplayMemoryChannelType (
  UINT8 Key,
  UINT8 Option
  );

//
// IPMI Device Information  (Type 38)
//
VOID
DisplayIPMIDIBMCInterfaceType (
  UINT8 Key,
  UINT8 Option
  );

#endif
