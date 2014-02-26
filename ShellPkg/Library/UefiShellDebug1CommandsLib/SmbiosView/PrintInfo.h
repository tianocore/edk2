/** @file
  Module to clarify the element info of the smbios structure.

  Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMBIOS_PRINT_INFO_H_
#define _SMBIOS_PRINT_INFO_H_

#include <IndustryStandard/SmBios.h>

extern UINT8  SmbiosMajorVersion;
extern UINT8  SmbiosMinorVersion;

#define SHOW_NONE     0x00
#define SHOW_OUTLINE  0x01
#define SHOW_NORMAL   0x02
#define SHOW_DETAIL   0x03
//
// SHOW_ALL: WaitEnter() not wait input.
//
#define SHOW_ALL          0x04
#define SHOW_STATISTICS   0x05

#define AS_UINT16(pData)  (*((UINT16 *) pData))
#define AS_UINT32(pData)  (*((UINT32 *) pData))
#define AS_UINT64(pData)  (*((UINT64 *) pData))

/**
  Print the info of EPS(Entry Point Structure).

  @param[in] SmbiosTable    Pointer to the SMBIOS table entry point.
  @param[in] Option         Display option.
**/
VOID
SmbiosPrintEPSInfo (
  IN  SMBIOS_TABLE_ENTRY_POINT  *SmbiosTable,
  IN  UINT8                     Option
  );

/**
  This function print the content of the structure pointed by Struct.

  @param[in] Struct       Point to the structure to be printed.
  @param[in] Option       Print option of information detail.

  @retval EFI_SUCCESS               Successfully Printing this function.
  @retval EFI_INVALID_PARAMETER     Invalid Structure.
  @retval EFI_UNSUPPORTED           Unsupported.
**/
EFI_STATUS
SmbiosPrintStructure (
  IN  SMBIOS_STRUCTURE_POINTER  *Struct,
  IN  UINT8                     Option
  );

/**
  Display BIOS Information (Type 0) information.

  @param[in] Chara    The information bits.
  @param[in] Option   The optional information.
**/
VOID
DisplayBiosCharacteristics (
  IN UINT64  Chara,
  IN UINT8   Option
  );

/**
  Display Bios Characteristice extensions1 information.

  @param[in] Byte1    The information.
  @param[in] Option   The optional information.
**/
VOID
DisplayBiosCharacteristicsExt1 (
  IN UINT8 Byte1,
  IN UINT8 Option
  );

/**
  Display Bios Characteristice extensions2 information.

  @param[in] Byte2    The information.
  @param[in] Option   The optional information.
**/
VOID
DisplayBiosCharacteristicsExt2 (
  IN UINT8 Byte2,
  IN UINT8 Option
  );

/**
  Display Processor Information (Type 4) information.

  @param[in] Family       The family value.
  @param[in] Option       The option value.
**/
VOID
DisplayProcessorFamily (
  UINT8 Family,
  UINT8 Option
  );

/**
  Display processor family information.

  @param[in] Family2      The family value.
  @param[in] Option       The option value.
**/
VOID
DisplayProcessorFamily2 (
  IN UINT16 Family2,
  IN UINT8  Option
  );

/**
  Display processor voltage information.

  @param[in] Voltage      The Voltage.
                      Bit 7 Set to 0, indicating 'legacy' mode for processor voltage
                      Bits 6:4  Reserved, must be zero
                      Bits 3:0  Voltage Capability.
                                A Set bit indicates that the voltage is supported.
                        Bit 0 - 5V
                        Bit 1 - 3.3V
                        Bit 2 - 2.9V
                        Bit 3 - Reserved, must be zero.

                      Note:
                        Setting of multiple bits indicates the socket is configurable
                        If bit 7 is set to 1, the remaining seven bits of the field are set to
                        contain the processor's current voltage times 10.
                        For example, the field value for a processor voltage of 1.8 volts would be
                        92h = 80h + (1.8 * 10) = 80h + 18 = 80h +12h.

  @param[in] Option       The option.
**/
VOID
DisplayProcessorVoltage (
  IN UINT8 Voltage,
  IN UINT8 Option
  );

/**
  Display processor information.

  @param[in] Status   The status.
Bit 7 Reserved, must be 0
Bit 6 CPU Socket Populated
  1 - CPU Socket Populated
  0 - CPU Socket Unpopulated
Bits 5:3 Reserved, must be zero
Bits 2:0 CPU Status
  0h - Unknown
  1h - CPU Enabled
  2h - CPU Disabled by User via BIOS Setup
  3h - CPU Disabled By BIOS (POST Error)
  4h - CPU is Idle, waiting to be enabled.
  5-6h - Reserved
  7h - Other

  @param[in] Option   The option
**/
VOID
DisplayProcessorStatus (
  IN UINT8 Status,
  IN UINT8 Option
  );

/**
  Display information about Memory Controller Information (Type 5).

  @param[in] Size     Memory size.
  @param[in] SlotNum  Which slot is this about.
  @param[in] Option   Option for the level of detail output required.
**/
VOID
DisplayMaxMemoryModuleSize (
  IN UINT8 Size,
  IN UINT8 SlotNum,
  IN UINT8 Option
  );

/**
  Display information about memory configuration handles.

  @param[in] Handles  The buffer of handles to output info on.
  @param[in] SlotNum  The number of handles in the above buffer.
  @param[in] Option   Option for the level of detail output required.
**/
VOID
DisplayMemoryModuleConfigHandles (
  IN UINT16 *Handles,
  IN UINT8  SlotNum,
  IN UINT8  Option
  );

/**
  Display Memory Module Information (Type 6).

  @param[in] BankConnections
  @param[in] Option
**/
VOID
DisplayMmBankConnections (
  IN UINT8 BankConnections,
  IN UINT8 Option
  );

/**
  Display memory informcation.

  Bits 0:6  Size (n),
      where 2**n is the size in MB with three special-case values:
      7Dh Not determinable (Installed Size only)
      7Eh Module is installed, but no memory has been enabled
      7Fh Not installed
  Bit  7  Defines whether the memory module has a single- (0)
          or double-bank (1) connection.

  @param[in] Size   - The size
  @param[in] Option - The option
**/
VOID
DisplayMmMemorySize (
  IN UINT8 Size,
  IN UINT8 Option
  );

/**
  Display Cache Configuration.

  @param[in] CacheConfiguration   Cache Configuration.
Bits 15:10 Reserved, must be 0
Bits 9:8 Operational Mode
  0h - Write Through
  1h - Write Back
  2h - Varies with Memory Address
  3h - Unknown
Bit 7 Enabled/Disabled
  1 - Enabled
  0 - Disabled
Bits 6:5 Location
  0h - Internal
  1h - External
  2h - Reserved
  3h - Unknown
Bit 4 Reserved, must be zero
Bit 3 Cache Socketed
  1 - Socketed
  0 - Unsocketed
Bits 2:0 Cache Level
  1 through 8 (For example, an L1 cache would
  use value 000b and an L3 cache would use 010b.)

  @param[in] Option   The option
**/
VOID
DisplayCacheConfiguration (
  IN UINT16 CacheConfiguration,
  IN UINT8 Option
  );

/**
  The Slot ID field of the System Slot structure provides a mechanism to
  correlate the physical attributes of the slot to its logical access method
  (which varies based on the Slot Type field).

  @param[in] SlotId   - The slot ID
  @param[in] SlotType - The slot type
  @param[in] Option   - The Option
**/
VOID
DisplaySystemSlotId (
  IN UINT16  SlotId,
  IN UINT8   SlotType,
  IN UINT8   Option
  );

/**
  Display Portable Battery (Type 22) information.

  The date the cell pack was manufactured, in packed format:
   Bits 15:9  Year, biased by 1980, in the range 0 to 127.
   Bits 8:5 Month, in the range 1 to 12.
   Bits 4:0 Date, in the range 1 to 31.
  For example, 01 February 2000 would be identified as
  0010 1000 0100 0001b (0x2841).

  @param[in] Date     The date
  @param[in] Option   The option
**/
VOID
DisplaySBDSManufactureDate (
  IN UINT16  Date,
  IN UINT8   Option
  );

/**
  Display System Reset  (Type 23) information.

  Routine Description:
  Identifies the system-reset capabilities for the system.
   Bits 7:6 Reserved for future assignment via this specification, set to 00b.
   Bit 5  System contains a watchdog timer, either True (1) or False (0).
   Bits 4:3 Boot Option on Limit.
    Identifies the system action to be taken when the Reset Limit is reached, one of:
    00b Reserved, do not use.
    01b Operating system
    10b System utilities
    11b Do not rebootBits
   2:1  Boot Option.  Indicates the action to be taken following a watchdog reset, one of:
    00b Reserved, do not use.
    01b Operating system
    10b System utilities
    11b Do not reboot
   Bit 0  Status.
    1b The system reset is enabled by the user
    0b The system reset is not enabled by the user

  @param[in] Reset   Reset
  @param[in] Option  The option
**/
VOID
DisplaySystemResetCapabilities (
  IN UINT8 Reset,
  IN UINT8 Option
  );

/**
  Display Hardware Security (Type 24) information.

    Routine Description:
    Identifies the password and reset status for the system:

    Bits 7:6    Power-on Password Status, one of:
      00b Disabled
      01b Enabled
      10b Not Implemented
      11b Unknown
    Bits 5:4    Keyboard Password Status, one of:
      00b Disabled
      01b Enabled
      10b Not Implemented
      11b Unknown
    Bits 3:2    Administrator Password Status, one  of:
      00b Disabled
      01b Enabled
      10b Not Implemented
      11b Unknown
    Bits 1:0    Front Panel Reset Status, one of:
      00b Disabled
      01b Enabled
      10b Not Implemented
      11b Unknown

  @param[in] Settings The device settings.
  @param[in] Option   The device options.
**/
VOID
DisplayHardwareSecuritySettings (
  IN UINT8 Settings,
  IN UINT8 Option
  );

/**
  Display Out-of-Band Remote Access (Type 30) information.

  @param[in] Connections        The device characteristics.
  @param[in] Option             The device options.
**/
VOID
DisplayOBRAConnections (
  IN UINT8   Connections,
  IN UINT8   Option
  );

/**
  Display System Boot Information (Type 32) information.

  @param[in] Parameter      The parameter.
  @param[in] Option         The options.
**/
VOID
DisplaySystemBootStatus (
  IN UINT8 Parameter,
  IN UINT8 Option
  );

/**
  Display System Power Supply (Type 39) information.

  @param[in] Characteristics    The device characteristics.
  @param[in] Option             The device options.
**/
VOID
DisplaySPSCharacteristics (
  IN UINT16  Characteristics,
  IN UINT8   Option
  );

#endif
