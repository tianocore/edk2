/** @file
  Build a table, each item is (key, info) pair.
  and give a interface of query a string out of a table.

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMBIOS_QUERY_TABLE_H_
#define _SMBIOS_QUERY_TABLE_H_

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

  @param[in] Table     The begin address of table.
  @param[in] Number    The number of table items.
  @param[in] Key       The query Key.
  @param[in, out] Info Input as empty buffer; output as data buffer.
  @param[in] InfoLen   The max number of characters for Info.

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

/**
  Display the structure type information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayStructureTypeInfo (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display System Information (Type 1) Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplaySystemWakeupType (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display Base Board (Type 2) Feature Flags.

  @param[in] FeatureFlags   The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayBaseBoardFeatureFlags (
  IN UINT8 FeatureFlags,
  IN UINT8 Option
  );

/**
  Display Base Board (Type 2) Board Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayBaseBoardBoardType(
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display System Enclosure (Type 3) Enclosure Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplaySystemEnclosureType (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display System Enclosure (Type 3) Enclosure Status.

  @param[in] Status         The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplaySystemEnclosureStatus (
  IN UINT8 Status,
  IN UINT8 Option
  );

/**
  Display System Enclosure (Type 3) Security Status.

  @param[in] Status         The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplaySESecurityStatus (
  IN UINT8 Status,
  IN UINT8 Option
  )
;

/**
  Display Processor Information (Type 4) Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayProcessorType (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display Processor Information (Type 4) Upgrade.

  @param[in] Upgrade        The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayProcessorUpgrade (
  IN UINT8 Upgrade,
  IN UINT8 Option
  );

/**
  Display Processor Information (Type 4) Characteristics.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayProcessorCharacteristics (
  IN UINT16 Type,
  IN UINT8 Option
  );

/**
  Display Memory Controller Information (Type 5) method.

  @param[in] Method         The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMcErrorDetectMethod (
  IN UINT8 Method,
  IN UINT8 Option
  );

/**
  Display Memory Controller Information (Type 5) Capability.

  @param[in] Capability     The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMcErrorCorrectCapability (
  IN UINT8 Capability,
  IN UINT8 Option
  );

/**
  Display Memory Controller Information (Type 5) Support.

  @param[in] Support        The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMcInterleaveSupport (
  IN UINT8 Support,
  IN UINT8 Option
  );

/**
  Display Memory Controller Information (Type 5) speeds.

  @param[in] Speed          The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMcMemorySpeeds (
  IN UINT16  Speed,
  IN UINT8   Option
  );

/**
  Display Memory Controller Information (Type 5) voltage.

  @param[in] Voltage        The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMemoryModuleVoltage (
  IN UINT8 Voltage,
  IN UINT8 Option
  );

/**
  Display Memory Module Information (Type 6) type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMmMemoryType (
  IN UINT16  Type,
  IN UINT8   Option
  );

/**
  Display Memory Module Information (Type 6) status.

  @param[in] Status         The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMmErrorStatus (
  IN UINT8 Status,
  IN UINT8 Option
  );

/**
  Display Cache Information (Type 7) SRAM Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayCacheSRAMType (
  IN UINT16  Type,
  IN UINT8   Option
  );

/**
  Display Cache Information (Type 7) correcting Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayCacheErrCorrectingType (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display Cache Information (Type 7) Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayCacheSystemCacheType (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display Cache Information (Type 7) Associativity.

  @param[in] Associativity  The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayCacheAssociativity (
  IN UINT8 Associativity,
  IN UINT8 Option
  );

/**
  Display Port Connector Information  (Type 8) type.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPortConnectorType (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display Port Connector Information  (Type 8) port type.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPortType (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display System Slots (Type 9) slot type.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySystemSlotType (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display System Slots (Type 9) data bus width.

  @param[in] Width      The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySystemSlotDataBusWidth (
  IN UINT8 Width,
  IN UINT8 Option
  );

/**
  Display System Slots (Type 9) usage information.

  @param[in] Usage      The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySystemSlotCurrentUsage (
  IN UINT8 Usage,
  IN UINT8 Option
  );

/**
  Display System Slots (Type 9) slot length.

  @param[in] Length     The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySystemSlotLength (
  IN UINT8 Length,
  IN UINT8 Option
  );

/**
  Display System Slots (Type 9) characteristics.

  @param[in] Chara1     The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySlotCharacteristics1 (
  IN UINT8 Chara1,
  IN UINT8 Option
  );

/**
  Display System Slots (Type 9) characteristics.

  @param[in] Chara2     The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySlotCharacteristics2 (
  IN UINT8 Chara2,
  IN UINT8 Option
  );

/**
  Display On Board Devices Information (Type 10) types.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayOnboardDeviceTypes (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display System Event Log (Type 15) types.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySELTypes (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display System Event Log (Type 15) format type.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySELVarDataFormatType (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display System Event Log (Type 15) dw1.

  @param[in] Key        The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPostResultsBitmapDw1 (
  IN UINT32  Key,
  IN UINT8   Option
  );

/**
  Display System Event Log (Type 15) dw2.

  @param[in] Key        The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPostResultsBitmapDw2 (
  IN UINT32  Key,
  IN UINT8   Option
  );

/**
  Display System Event Log (Type 15) type.

  @param[in] SMType     The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySELSysManagementTypes (
  IN UINT32  SMType,
  IN UINT8   Option
  );

/**
  Display Physical Memory Array (Type 16) Location.

  @param[in] Location   The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPMALocation (
  IN UINT8 Location,
  IN UINT8 Option
  );

/**
  Display Physical Memory Array (Type 16) Use.

  @param[in] Use        The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPMAUse (
  IN UINT8 Use,
  IN UINT8 Option
  );

/**
  Display Physical Memory Array (Type 16) Types.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPMAErrorCorrectionTypes (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display Memory Device (Type 17) form factor.

  @param[in] FormFactor The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayMemoryDeviceFormFactor (
  IN UINT8 FormFactor,
  IN UINT8 Option
  );

/**
  Display Memory Device (Type 17) type.

  @param[in] Type     The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMemoryDeviceType (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display Memory Device (Type 17) details.

  @param[in] Para     The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMemoryDeviceTypeDetail (
  IN UINT16  Para,
  IN UINT8   Option
  );

/**
  Display Memory Device (Type 17) memory technology.

  @param[in] Para     The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMemoryDeviceMemoryTechnology (
  IN UINT8  Para,
  IN UINT8  Option
  );

/**
  Display Memory Device (Type 17) memory operating mode capability.

  @param[in] Para     The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMemoryDeviceMemoryOperatingModeCapability (
  IN UINT16  Para,
  IN UINT8   Option
  );

/**
  Display 32-bit Memory Error Information (Type 18) type.

  @param[in] ErrorType  The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayMemoryErrorType (
  IN UINT8 ErrorType,
  IN UINT8 Option
  );

/**
  Display 32-bit Memory Error Information (Type 18) error granularity.

  @param[in] Granularity  The key of the structure.
  @param[in] Option       The optional information.
**/
VOID
DisplayMemoryErrorGranularity (
  IN UINT8 Granularity,
  IN UINT8 Option
  );

/**
  Display 32-bit Memory Error Information (Type 18) error information.

  @param[in] Operation  The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayMemoryErrorOperation (
  IN UINT8 Operation,
  IN UINT8 Option
  );

/**
  Display Built-in Pointing Device (Type 21) type information.

  @param[in] Type     The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayPointingDeviceType (
  IN UINT8 Type,
  IN UINT8 Option
  );

/**
  Display Built-in Pointing Device (Type 21) information.

  @param[in] Interface  The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPointingDeviceInterface (
  IN UINT8   Interface,
  IN UINT8   Option
  );

/**
  Display Portable Battery  (Type 22) information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayPBDeviceChemistry (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Voltage Probe (Type 26) location information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayVPLocation (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Voltage Probe (Type 26) status ype information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayVPStatus (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Cooling (Type 27) status information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayCoolingDeviceStatus (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Cooling (Type 27) type information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayCoolingDeviceType (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Temperature Probe (Type 28) status information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayTemperatureProbeStatus (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Temperature Probe  (Type 28) location information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayTemperatureProbeLoc (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Electrical Current Probe (Type 29)  status information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayECPStatus (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Electrical Current Probe (Type 29) location information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayECPLoc (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Management Device (Type 34) Type.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMDType (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Management Device (Type 34) Address Type.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMDAddressType (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Memory Channel (Type 37) information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMemoryChannelType (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display IPMI Device Information (Type 38) information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayIPMIDIBMCInterfaceType (
  IN UINT8 Key,
  IN UINT8 Option
  );

/**
  Display Management Controller Host Interface (Type 42) information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMCHostInterfaceType (
  IN UINT8 Key,
  IN UINT8 Option
  );

#endif
