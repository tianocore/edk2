/** @file
  Build a table, each item is (Key, Info) pair.
  And give a interface of query a string out of a table.

  Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "../UefiShellDebug1CommandsLib.h"
#include "QueryTable.h"
#include "PrintInfo.h"

TABLE_ITEM  SystemWakeupTypeTable[] = {
  {
    0x0,
    L" Reserved"
  },
  {
    0x1,
    L" Other"
  },
  {
    0x2,
    L" Unknown"
  },
  {
    0x3,
    L" APM Timer"
  },
  {
    0x4,
    L" Modem Ring"
  },
  {
    0x5,
    L" LAN Remote"
  },
  {
    0x6,
    L" Power Switch"
  },
  {
    0x7,
    L" AC Power Restored"
  }
};

TABLE_ITEM  BaseBoardFeatureFlagsTable[] = {
  {
    0,
    L" Hosting board"
  },
  {
    1,
    L" Requires at least one daughter board or auxiliary card"
  },
  {
    2,
    L" Removable"
  },
  {
    3,
    L" Replaceable"
  },
  {
    4,
    L" Hot swappable"
  }
};

TABLE_ITEM  BaseBoardBoardTypeTable[] = {
  {
    0x01,
    L" Unknown"
  },
  {
    0x02,
    L" Other"
  },
  {
    0x03,
    L" Server Blade"
  },
  {
    0x04,
    L" Connectivity Switch"
  },
  {
    0x05,
    L" System Management Module"
  },
  {
    0x06,
    L" Processor Module"
  },
  {
    0x07,
    L" I/O Module"
  },
  {
    0x08,
    L" Memory Module"
  },
  {
    0x09,
    L" Daughter board"
  },
  {
    0x0A,
    L" Motherboard"
  },
  {
    0x0B,
    L" Processor/Memory Module"
  },
  {
    0x0C,
    L" Processor/IO Module"
  },
  {
    0x0D,
    L" Interconnect Board"
  }
};

TABLE_ITEM  SystemEnclosureTypeTable[] = {
  {
    0x01,
    L"  Other"
  },
  {
    0x02,
    L"  Unknown"
  },
  {
    0x03,
    L"  Desktop"
  },
  {
    0x04,
    L"  Low Profile Desktop"
  },
  {
    0x05,
    L"  Pizza Box"
  },
  {
    0x06,
    L"  Mini Tower"
  },
  {
    0x07,
    L"  Tower"
  },
  {
    0x08,
    L"  Portable"
  },
  {
    0x09,
    L"  LapTop"
  },
  {
    0x0A,
    L"  Notebook"
  },
  {
    0x0B,
    L"  Hand Held"
  },
  {
    0x0C,
    L"  Docking Station"
  },
  {
    0x0D,
    L"  All in One"
  },
  {
    0x0E,
    L"  Sub Notebook"
  },
  {
    0x0F,
    L"  Space-saving"
  },
  {
    0x10,
    L"  Main Server Chassis"
  },
  {
    0x11,
    L"  Expansion Chassis"
  },
  {
    0x12,
    L"  SubChassis"
  },
  {
    0x13,
    L"  Sub Notebook"
  },
  {
    0x14,
    L"  Bus Expansion Chassis"
  },
  {
    0x15,
    L"  Peripheral Chassis"
  },
  {
    0x16,
    L"  RAID Chassis"
  },
  {
    0x17,
    L"  Rack Mount Chassis"
  },
  {
    0x18,
    L"  Sealed-case PC"
  },
  {
    0x19,
    L"  Multi-system Chassis"
  },
  {
    0x1A,
    L"  CompactPCI"
  },
  {
    0x1B,
    L"  AdvancedTCA"
  },
  {
    0x1C,
    L"  Blade"
  },
  {
    0x1D,
    L"  Blade Enclosure"
  },
};

TABLE_ITEM  SystemEnclosureStatusTable[] = {
  {
    0x1,
    L" Other"
  },
  {
    0x2,
    L" Unknown"
  },
  {
    0x3,
    L" Safe"
  },
  {
    0x4,
    L" Warning"
  },
  {
    0x5,
    L" Critical"
  },
  {
    0x6,
    L" Non-recoverable"
  }
};

TABLE_ITEM  SESecurityStatusTable[] = {
  {
    0x1,
    L" Other"
  },
  {
    0x2,
    L" Unknown"
  },
  {
    0x3,
    L" None"
  },
  {
    0x4,
    L" External interface locked out"
  },
  {
    0x5,
    L" External interface enabled"
  }
};

TABLE_ITEM  ProcessorTypeTable[] = {
  {
    0x1,
    L" Other"
  },
  {
    0x2,
    L" Unknown"
  },
  {
    0x3,
    L" Central Processor"
  },
  {
    0x4,
    L" Math Processor"
  },
  {
    0x5,
    L" DSP Processor"
  },
  {
    0x6,
    L" Video Processor "
  },
};

TABLE_ITEM  ProcessorUpgradeTable[] = {
  {
    0x01,
    L"Other"
  },
  {
    0x02,
    L"Unknown"
  },
  {
    0x03,
    L"Daughter Board"
  },
  {
    0x04,
    L"ZIF Socket"
  },
  {
    0x05,
    L"Replaceable Piggy Back"
  },
  {
    0x06,
    L"None"
  },
  {
    0x07,
    L"LIF Socket"
  },
  {
    0x08,
    L"Slot 1"
  },
  {
    0x09,
    L"Slot 2"
  },
  {
    0x0A,
    L"370-pin socket"
  },
  {
    0x0B,
    L"Slot A"
  },
  {
    0x0C,
    L"Slot M"
  },
  {
    0x0D,
    L"Socket 423"
  },
  {
    0x0E,
    L"Socket A"
  },
  {
    0x0F,
    L"Socket 478"
  },
  {
    0x10,
    L"Socket 754"
  },
  {
    0x11,
    L"Socket 940"
  },
  {
    0x12,
    L"Socket 939"
  },
  {
    0x13,
    L"Socket mPGA604"
  },
  {
    0x14,
    L"Socket LGA771"
  },
  {
    0x15,
    L"Socket LGA775"
  },
  {
    0x16,
    L"Socket S1"
  },
  {
    0x17,
    L"Socket AM2"
  },
  {
    0x18,
    L"Socket F"
  },
  {
    0x19,
    L"Socket LGA1366"
  },
  {
    0x1A,
    L"Socket G34"
  },
  {
    0x1B,
    L"Socket AM3"
  },
  {
    0x1C,
    L"Socket C32"
  },
  {
    0x1D,
    L"Socket LGA1156"
  },
  {
    0x1E,
    L"Socket LGA1567"
  },
  {
    0x1F,
    L"Socket PGA988A"
  },
  {
    0x20,
    L"Socket BGA1288"
  },
  {
    0x21,
    L"Socket rPGA988B"
  },
  {
    0x22,
    L"Socket BGA1023"
  },
  {
    0x23,
    L"Socket BGA1224"
  },
  {
    0x24,
    L"Socket LGA1155"
  },
  {
    0x25,
    L"Socket LGA1356"
  },
  {
    0x26,
    L"Socket LGA2011"
  },
  {
    0x27,
    L"Socket FS1"
  },
  {
    0x28,
    L"Socket FS2"
  },
  {
    0x29,
    L"Socket FM1"
  },
  {
    0x2A,
    L"Socket FM2"
  },
  {
    0x2B,
    L"Socket LGA2011-3"
  },
  {
    0x2C,
    L"Socket LGA1356-3"
  }
};

TABLE_ITEM  ProcessorCharacteristicsTable[] = {
  {
    1,
    L" Unknown"
  },
  {
    2,
    L" 64-bit Capable"
  },
  {
    3,
    L" Multi-Core"
  },
  {
    4,
    L" Hardware Thread"
  },
  {
    5,
    L" Execute Protection"
  },
  {
    6,
    L" Enhanced Virtualization"
  },
  {
    7,
    L" Power/Performance Control"
  }
};


TABLE_ITEM  McErrorDetectMethodTable[] = {
  {
    0x01,
    L"Other"
  },
  {
    0x02,
    L"Unknown"
  },
  {
    0x03,
    L"None"
  },
  {
    0x04,
    L"8-bit Parity"
  },
  {
    0x05,
    L"32-bit ECC"
  },
  {
    0x06,
    L"64-bit ECC"
  },
  {
    0x07,
    L"128-bit ECC"
  },
  {
    0x08,
    L"CRC"
  },
};

TABLE_ITEM  McErrorCorrectCapabilityTable[] = {
  {
    0,
    L"Other"
  },
  {
    1,
    L"Unknown"
  },
  {
    2,
    L"None"
  },
  {
    3,
    L"Single Bit Error Correcting"
  },
  {
    4,
    L"Double Bit Error Correcting"
  },
  {
    5,
    L"Error Scrubbing"
  },
};

TABLE_ITEM  McInterleaveSupportTable[] = {
  {
    0x01,
    L"Other"
  },
  {
    0x02,
    L"Unknown"
  },
  {
    0x03,
    L"One Way Interleave"
  },
  {
    0x04,
    L"Two Way Interleave"
  },
  {
    0x05,
    L"Four Way Interleave"
  },
  {
    0x06,
    L"Eight Way Interleave"
  },
  {
    0x07,
    L"Sixteen Way Interleave"
  }
};

TABLE_ITEM  McMemorySpeedsTable[] = {
  {
    0,
    L" Other"
  },
  {
    1,
    L" Unknown"
  },
  {
    2,
    L" 70ns"
  },
  {
    3,
    L" 60ns"
  },
  {
    4,
    L" 50ns"
  },
};

TABLE_ITEM  MemoryModuleVoltageTable[] = {
  {
    0,
    L" 5V"
  },
  {
    1,
    L" 3.3V"
  },
  {
    2,
    L" 2.9V"
  },
};

TABLE_ITEM  MmMemoryTypeTable[] = {
  {
    0,
    L" Other"
  },
  {
    1,
    L" Unknown"
  },
  {
    2,
    L" Standard"
  },
  {
    3,
    L" Fast Page Mode"
  },
  {
    4,
    L" EDO"
  },
  {
    5,
    L" Parity"
  },
  {
    6,
    L" ECC "
  },
  {
    7,
    L" SIMM"
  },
  {
    8,
    L" DIMM"
  },
  {
    9,
    L" Burst EDO"
  },
  {
    10,
    L" SDRAM"
  }
};

TABLE_ITEM  MmErrorStatusTable[] = {
  {
    0,
    L" Uncorrectable errors received"
  },
  {
    1,
    L" Correctable errors received"
  },
  {
    2,
    L" Error Status obtained from the event log"
  }
};

TABLE_ITEM  CacheSRAMTypeTable[] = {
  {
    0,
    L" Other"
  },
  {
    1,
    L" Unknown"
  },
  {
    2,
    L" Non-Burst"
  },
  {
    3,
    L" Burst"
  },
  {
    4,
    L" Pipeline Burst"
  },
  {
    5,
    L" Synchronous"
  },
  {
    6,
    L" Asynchronous"
  },
};

TABLE_ITEM  CacheErrCorrectingTypeTable[] = {
  {
    0x01,
    L"Other"
  },
  {
    0x02,
    L"Unknown"
  },
  {
    0x03,
    L"None"
  },
  {
    0x04,
    L"Parity"
  },
  {
    0x05,
    L"Single-bit ECC"
  },
  {
    0x06,
    L"Multi-bit ECC"
  }
};

TABLE_ITEM  CacheSystemCacheTypeTable[] = {
  {
    0x01,
    L"Other"
  },
  {
    0x02,
    L"Unknown"
  },
  {
    0x03,
    L"Instruction"
  },
  {
    0x04,
    L"Data"
  },
  {
    0x05,
    L"Unified"
  }
};

TABLE_ITEM  CacheAssociativityTable[] = {
  {
    0x01,
    L"Other"
  },
  {
    0x02,
    L"Unknown"
  },
  {
    0x03,
    L"Direct Mapped"
  },
  {
    0x04,
    L"2-way Set-Associative"
  },
  {
    0x05,
    L"4-way Set-Associative"
  },
  {
    0x06,
    L"Fully Associative"
  },
  {
    0x07,
    L"8-way Set-Associative"
  },
  {
    0x08,
    L"16-way Set-Associative"
  },
  {
    0x09,
    L"12-way Set-Associative"
  },
  {
    0x0A,
    L"24-way Set-Associative"
  },
  {
    0x0B,
    L"32-way Set-Associative"
  },
  {
    0x0C,
    L"48-way Set-Associative"
  },
  {
    0x0D,
    L"64-way Set-Associative"
  },
  {
    0x0E,
    L"20-way Set-Associative"
  }
};

TABLE_ITEM  PortConnectorTypeTable[] = {
  {
    0x00,
    L"None"
  },
  {
    0x01,
    L"Centronics"
  },
  {
    0x02,
    L"Mini Centronics"
  },
  {
    0x03,
    L"Proprietary"
  },
  {
    0x04,
    L"DB-25 pin male"
  },
  {
    0x05,
    L"DB-25 pin female"
  },
  {
    0x06,
    L"DB-15 pin male"
  },
  {
    0x07,
    L"DB-15 pin female"
  },
  {
    0x08,
    L"DB-9 pin male"
  },
  {
    0x09,
    L"DB-9 pin female"
  },
  {
    0x0A,
    L"RJ-11"
  },
  {
    0x0B,
    L"RJ-45"
  },
  {
    0x0C,
    L"50 Pin MiniSCSI"
  },
  {
    0x0D,
    L"Mini-DIN"
  },
  {
    0x0E,
    L"Micro-DIN"
  },
  {
    0x0F,
    L"PS/2"
  },
  {
    0x10,
    L"Infrared"
  },
  {
    0x11,
    L"HP-HIL"
  },
  {
    0x12,
    L"Access Bus (USB)"
  },
  {
    0x13,
    L"SSA SCSI"
  },
  {
    0x14,
    L"Circular DIN-8 male"
  },
  {
    0x15,
    L"Circular DIN-8 female"
  },
  {
    0x16,
    L"On Board IDE"
  },
  {
    0x17,
    L"On Board Floppy"
  },
  {
    0x18,
    L"9 Pin Dual Inline (pin 10 cut)"
  },
  {
    0x19,
    L"25 Pin Dual Inline (pin 26 cut)"
  },
  {
    0x1A,
    L"50 Pin Dual Inline"
  },
  {
    0x1B,
    L"68 Pin Dual Inline"
  },
  {
    0x1C,
    L"On Board Sound Input from CD-ROM"
  },
  {
    0x1D,
    L"Mini-Centronics Type-14"
  },
  {
    0x1E,
    L"Mini-Centronics Type-26"
  },
  {
    0x1F,
    L"Mini-jack (headphones)"
  },
  {
    0x20,
    L"BNC"
  },
  {
    0x21,
    L"1394"
  },
  {
    0x22,
    L"SAS/SATA Plug Receptacle"
  },
  {
    0xA0,
    L"PC-98"
  },
  {
    0xA1,
    L"PC-98Hireso"
  },
  {
    0xA2,
    L"PC-H98"
  },
  {
    0xA3,
    L"PC-98Note"
  },
  {
    0xA4,
    L"PC-98Full"
  },
  {
    0xFF,
    L"Other"
  },
};

TABLE_ITEM  PortTypeTable[] = {
  {
    0x00,
    L"None"
  },
  {
    0x01,
    L"Parallel Port XT/AT Compatible"
  },
  {
    0x02,
    L"Parallel Port PS/2"
  },
  {
    0x03,
    L"Parallel Port ECP"
  },
  {
    0x04,
    L"Parallel Port EPP"
  },
  {
    0x05,
    L"Parallel Port ECP/EPP"
  },
  {
    0x06,
    L"Serial Port XT/AT Compatible"
  },
  {
    0x07,
    L"Serial Port 16450 Compatible"
  },
  {
    0x08,
    L"Serial Port 16550 Compatible"
  },
  {
    0x09,
    L"Serial Port 16550A Compatible"
  },
  {
    0x0A,
    L"SCSI Port"
  },
  {
    0x0B,
    L"MIDI Port"
  },
  {
    0x0C,
    L"Joy Stick Port"
  },
  {
    0x0D,
    L"Keyboard Port"
  },
  {
    0x0E,
    L"Mouse Port"
  },
  {
    0x0F,
    L"SSA SCSI"
  },
  {
    0x10,
    L"USB"
  },
  {
    0x11,
    L"FireWire (IEEE P1394)"
  },
  {
    0x12,
    L"PCMCIA Type II"
  },
  {
    0x13,
    L"PCMCIA Type II"
  },
  {
    0x14,
    L"PCMCIA Type III"
  },
  {
    0x15,
    L"Cardbus"
  },
  {
    0x16,
    L"Access Bus Port"
  },
  {
    0x17,
    L"SCSI II"
  },
  {
    0x18,
    L"SCSI Wide"
  },
  {
    0x19,
    L"PC-98"
  },
  {
    0x1A,
    L"PC-98-Hireso"
  },
  {
    0x1B,
    L"PC-H98"
  },
  {
    0x1C,
    L"Video Port"
  },
  {
    0x1D,
    L"Audio Port"
  },
  {
    0x1E,
    L"Modem Port"
  },
  {
    0x1F,
    L"Network Port"
  },
  {
    0x20,
    L"SATA Port"
  },
  {
    0x21,
    L"SAS Port"
  },
  {
    0xA0,
    L"8251 Compatible"
  },
  {
    0xA1,
    L"8251 FIFO Compatible"
  },
  {
    0xFF,
    L"Other "
  },
};

TABLE_ITEM  SystemSlotTypeTable[] = {
  {
    0x01,
    L"Other"
  },
  {
    0x02,
    L"Unknown"
  },
  {
    0x03,
    L"ISA"
  },
  {
    0x04,
    L"MCA"
  },
  {
    0x05,
    L"EISA"
  },
  {
    0x06,
    L"PCI"
  },
  {
    0x07,
    L"PC Card (PCMCIA)"
  },
  {
    0x08,
    L"VL-VESA"
  },
  {
    0x09,
    L"Proprietary"
  },
  {
    0x0A,
    L"Processor Card Slot"
  },
  {
    0x0B,
    L"Proprietary Memory Card Slot"
  },
  {
    0x0C,
    L"I/O Riser Card Slot"
  },
  {
    0x0D,
    L"NuBus"
  },
  {
    0x0E,
    L"PCI - 66MHz Capable"
  },
  {
    0x0F,
    L"AGP"
  },
  {
    0x10,
    L"AGP 2X"
  },
  {
    0x11,
    L"AGP 4X"
  },
  {
    0x12,
    L"PCI-X"
  },
  {
    0xA0,
    L"PC-98/C20 "
  },
  {
    0xA1,
    L"PC-98/C24 "
  },
  {
    0xA2,
    L"PC-98/E "
  },
  {
    0xA3,
    L"PC-98/Local Bus "
  },
  {
    0xA4,
    L"PC-98/Card "
  },
  {
    0xA5,
    L"PCI Express "
  },
  {
    0xA6,
    L"PCI Express X1"
  },
  {
    0xA7,
    L"PCI Express X2"
  },
  {
    0xA8,
    L"PCI Express X4"
  },
  {
    0xA9,
    L"PCI Express X8"
  },
  {
    0xAA,
    L"PCI Express X16"
  },
  {
    0xAB,
    L"PCI Express Gen 2"
  },
  {
    0xAC,
    L"PCI Express Gen 2 X1"
  },
  {
    0xAD,
    L"PCI Express Gen 2 X2"
  },
  {
    0xAE,
    L"PCI Express Gen 2 X4"
  },
  {
    0xAF,
    L"PCI Express Gen 2 X8"
  },
  {
    0xB0,
    L"PCI Express Gen 2 X16"
  },
  {
    0xB1,
    L"PCI Express Gen 3"
  },
  {
    0xB2,
    L"PCI Express Gen 3 X1"
  },
  {
    0xB3,
    L"PCI Express Gen 3 X2"
  },
  {
    0xB4,
    L"PCI Express Gen 3 X4"
  },
  {
    0xB5,
    L"PCI Express Gen 3 X8"
  },
  {
    0xB6,
    L"PCI Express Gen 3 X16"
  }
};

TABLE_ITEM  SystemSlotDataBusWidthTable[] = {
  {
    0x01,
    L" Other"
  },
  {
    0x02,
    L" Unknown"
  },
  {
    0x03,
    L" 8 bit"
  },
  {
    0x04,
    L" 16 bit"
  },
  {
    0x05,
    L" 32 bit"
  },
  {
    0x06,
    L" 64 bit"
  },
  {
    0x07,
    L" 128 bit"
  },
  {
    0x08,
    L" 1x or x1"
  },
  {
    0x09,
    L" 2x or x2"
  },
  {
    0x0A,
    L" 4x or x4"
  },
  {
    0x0B,
    L" 8x or x8"
  },
  {
    0x0C,
    L" 12x or x12"
  },
  {
    0x0D,
    L" 16x or x16"
  },
  {
    0x0E,
    L" 32x or x32"
  }
};

TABLE_ITEM  SystemSlotCurrentUsageTable[] = {
  {
    0x01,
    L" Other"
  },
  {
    0x02,
    L" Unknown"
  },
  {
    0x03,
    L" Available"
  },
  {
    0x04,
    L" In use"
  },
};

TABLE_ITEM  SystemSlotLengthTable[] = {
  {
    0x01,
    L" Other"
  },
  {
    0x02,
    L" Unknown"
  },
  {
    0x03,
    L" Short length"
  },
  {
    0x04,
    L" Long Length"
  },
};

TABLE_ITEM  SlotCharacteristics1Table[] = {
  {
    0,
    L" Characteristics Unknown"
  },
  {
    1,
    L" Provides 5.0 Volts"
  },
  {
    2,
    L" Provides 3.3 Volts"
  },
  {
    3,
    L" Slot's opening is shared with another slot, e.g. PCI/EISA shared slot."
  },

  {
    4,
    L" PC Card slot supports PC Card-16"
  },
  {
    5,
    L" PC Card slot supports CardBus"
  },
  {
    6,
    L" PC Card slot supports Zoom Video "
  },
  {
    7,
    L" PC Card slot supports Modem Ring Resume "
  }
};

TABLE_ITEM  SlotCharacteristics2Table[] = {
  {
    0,
    L" PCI slot supports Power Management Enable (PME#) signal"
  },
  {
    1,
    L" Slot supports hot-plug devices"
  },
  {
    2,
    L" PCI slot supports SMBus signal"
  }
};

TABLE_ITEM  OnboardDeviceTypesTable[] = {
  {
    0x01,
    L"  Other"
  },
  {
    0x02,
    L"  Unknown"
  },
  {
    0x03,
    L"  Video"
  },
  {
    0x04,
    L"  SCSI Controller"
  },
  {
    0x05,
    L"  Ethernet"
  },
  {
    0x06,
    L"  Token Ring"
  },
  {
    0x07,
    L"  Sound"
  },
  {
    0x08,
    L"  Pata Controller"
  },
  {
    0x09,
    L"  Sata Controller"
  },
  {
    0x0A,
    L"  Sas Controller"
  },
};

TABLE_ITEM  SELTypesTable[] = {
  {
    0x00,
    L" Reserved."
  },
  {
    0x01,
    L" Single-bit ECC memory error"
  },
  {
    0x02,
    L" Multi-bit ECC memory error"
  },
  {
    0x03,
    L" Parity memory error"
  },
  {
    0x04,
    L" Bus time-out"
  },
  {
    0x05,
    L" I/O Channel Check"
  },
  {
    0x06,
    L" Software NMI"
  },
  {
    0x07,
    L" POST Memory Resize"
  },
  {
    0x08,
    L" POST Error"
  },
  {
    0x09,
    L" PCI Parity Error"
  },
  {
    0x0A,
    L" PCI System Error"
  },
  {
    0x0B,
    L" CPU Failure"
  },
  {
    0x0C,
    L" EISA FailSafe Timer time-out"
  },
  {
    0x0D,
    L" Correctable memory log disabled"
  },
  {
    0x0E,
    L" Logging disabled for a specific Event Type"
  },
  {
    0x0F,
    L" Reserved"
  },
  {
    0x10,
    L" System Limit Exceeded"
  },
  {
    0x11,
    L" Asynchronous hardware timer expired and issued a system reset"
  },
  {
    0x12,
    L" System configuration information"
  },
  {
    0x13,
    L" Hard-disk information"
  },
  {
    0x14,
    L" System reconfigured"
  },
  {
    0x15,
    L" Uncorrectable CPU-complex error"
  },
  {
    0x16,
    L" Log Area Reset/Cleared"
  },
  {
    0x17,
    L" System boot"
  },
  {
    0x7F18,
    L" Unused by SMBIOS specification"
  },
  {
    0xFE80,
    L" System and OEM specified"
  },
  {
    0xFF,
    L" End-of-log"
  },
};

TABLE_ITEM  SELVarDataFormatTypeTable[] = {
  {
    0x00,
    L" None "
  },
  {
    0x01,
    L" Handle "
  },
  {
    0x02,
    L" Multiple-Event "
  },
  {
    0x03,
    L" Multiple-Event Handle "
  },
  {
    0x04,
    L" POST Results Bitmap "
  },
  //
  // Defined below
  //
  {
    0x05,
    L" System Management Type"
  },
  //
  // Defined below
  //
  {
    0x06,
    L" Multiple-Event System Management Type "
  },
  {
    0x7F07,
    L" Unused "
  },
  {
    0xFF80,
    L" OEM assigned "
  },
};

TABLE_ITEM  PostResultsBitmapDw1Table[] = {
  {
    0,
    L" Channel 2 Timer error "
  },
  {
    1,
    L" Master PIC (8259 #1) error "
  },
  {
    2,
    L" Slave PIC (8259 #2) error "
  },
  {
    3,
    L" CMOS Battery Failure "
  },
  {
    4,
    L" CMOS System Options Not Set "
  },
  {
    5,
    L" CMOS Checksum Error "
  },
  {
    6,
    L" CMOS Configuration Error "
  },
  {
    7,
    L" Mouse and Keyboard Swapped "
  },
  {
    8,
    L" Keyboard Locked "
  },
  {
    9,
    L" Keyboard Not Functional "
  },
  {
    10,
    L" Keyboard Controller Not Functional "
  },
  {
    11,
    L" CMOS Memory Size Different "
  },
  {
    12,
    L" Memory Decreased in Size "
  },
  {
    13,
    L" Cache Memory Error "
  },
  {
    14,
    L" Floppy Drive 0 Error "
  },
  {
    15,
    L" Floppy Drive 1 Error "
  },
  {
    16,
    L" Floppy Controller Failure "
  },
  {
    17,
    L" Number of ATA Drives Reduced Error "
  },
  {
    18,
    L" CMOS Time Not Set "
  },
  {
    19,
    L" DDC Monitor Configuration Change "
  },
  {
    20,
    L" Reserved, set to 0 "
  },
  {
    21,
    L" Reserved, set to 0 "
  },
  {
    22,
    L" Reserved, set to 0 "
  },
  {
    23,
    L" Reserved, set to 0 "
  },
  {
    24,
    L" Second DWORD has valid data "
  },
  {
    25,
    L" Reserved, set to 0 "
  },
  {
    26,
    L" Reserved, set to 0 "
  },
  {
    27,
    L" Reserved, set to 0 "
  },
  {
    28,
    L" Normally 0; available for OEM assignment "
  },
  {
    29,
    L" Normally 0; available for OEM assignment "
  },
  {
    30,
    L" Normally 0; available for OEM assignment "
  },
  {
    31,
    L" Normally 0; available for OEM assignment "
  },
};

TABLE_ITEM  PostResultsBitmapDw2Table[] = {
  {
    0,
    L" Normally 0; available for OEM assignment "
  },
  {
    1,
    L" Normally 0; available for OEM assignment "
  },
  {
    2,
    L" Normally 0; available for OEM assignment "
  },
  {
    3,
    L" Normally 0; available for OEM assignment "
  },
  {
    4,
    L" Normally 0; available for OEM assignment "
  },
  {
    5,
    L" Normally 0; available for OEM assignment "
  },
  {
    6,
    L" Normally 0; available for OEM assignment "
  },
  {
    7,
    L" PCI Memory Conflict "
  },
  {
    8,
    L" PCI I/O Conflict "
  },
  {
    9,
    L" PCI IRQ Conflict "
  },
  {
    10,
    L" PNP Memory Conflict "
  },
  {
    11,
    L" PNP 32 bit Memory Conflict "
  },
  {
    12,
    L" PNP I/O Conflict "
  },
  {
    13,
    L" PNP IRQ Conflict "
  },
  {
    14,
    L" PNP DMA Conflict "
  },
  {
    15,
    L" Bad PNP Serial ID Checksum "
  },
  {
    16,
    L" Bad PNP Resource Data Checksum "
  },
  {
    17,
    L" Static Resource Conflict "
  },
  {
    18,
    L" NVRAM Checksum Error, NVRAM Cleared "
  },
  {
    19,
    L" System Board Device Resource Conflict "
  },
  {
    20,
    L" Primary Output Device Not Found "
  },
  {
    21,
    L" Primary Input Device Not Found "
  },
  {
    22,
    L" Primary Boot Device Not Found "
  },
  {
    23,
    L" NVRAM Cleared By Jumper "
  },
  {
    24,
    L" NVRAM Data Invalid, NVRAM Cleared "
  },
  {
    25,
    L" FDC Resource Conflict "
  },
  {
    26,
    L" Primary ATA Controller Resource Conflict "
  },
  {
    27,
    L" Secondary ATA Controller Resource Conflict "
  },
  {
    28,
    L" Parallel Port Resource Conflict "
  },
  {
    29,
    L" Serial Port 1 Resource Conflict "
  },
  {
    30,
    L" Serial Port 2 Resource Conflict "
  },
  {
    31,
    L" Audio Resource Conflict "
  },
};

TABLE_ITEM  SELSysManagementTypesTable[] = {
  {
    0x01,
    L" +2.5V Out of range, #2 "
  },
  {
    0x02,
    L" +3.3V Out of range "
  },
  {
    0x03,
    L" +5V Out of range "
  },
  {
    0x04,
    L" -5V Out of range "
  },
  {
    0x05,
    L" +12V Out of range "
  },
  {
    0x06,
    L" -12V Out of range "
  },
  {
    0x0F07,
    L" Reserved for future out-of-range voltage levels "
  },
  {
    0x10,
    L" System board temperature out of range "
  },
  {
    0x11,
    L" Processor #1 temperature out of range "
  },
  {
    0x12,
    L" Processor #2 temperature out of range "
  },
  {
    0x13,
    L" Processor #3 temperature out of range "
  },
  {
    0x14,
    L" Processor #4 temperature out of range "
  },
  {
    0x1F15,
    L" Reserved for future out-of-range temperatures"
  },
  {
    0x2720,
    L" Fan n (n = 0 to 7) Out of range "
  },
  {
    0x2F28,
    L" Reserved for future assignment via this specification "
  },
  {
    0x30,
    L" Chassis secure switch activated "
  },
};

TABLE_ITEM  PMALocationTable[] = {
  {
    0x01,
    L"  Other"
  },
  {
    0x02,
    L"  Unknown"
  },
  {
    0x03,
    L"  System board or motherboard"
  },
  {
    0x04,
    L"  ISA add-on card"
  },
  {
    0x05,
    L"  EISA add-on card"
  },
  {
    0x06,
    L"  PCI add-on card"
  },
  {
    0x07,
    L"  MCA add-on card"
  },
  {
    0x08,
    L"  PCMCIA add-on card"
  },
  {
    0x09,
    L"  Proprietary add-on card"
  },
  {
    0x0A,
    L"  NuBus"
  },
  {
    0xA0,
    L"  PC-98/C20 add-on card"
  },
  {
    0xA1,
    L"  PC-98/C24 add-on card"
  },
  {
    0xA2,
    L"  PC-98/E add-on card"
  },
  {
    0xA3,
    L"  PC-98/Local bus add-on card"
  }
};

TABLE_ITEM  PMAUseTable[] = {
  {
    0x01,
    L" Other"
  },
  {
    0x02,
    L" Unknown"
  },
  {
    0x03,
    L" System memory"
  },
  {
    0x04,
    L" Video memory"
  },
  {
    0x05,
    L" Flash memory"
  },
  {
    0x06,
    L" Non-volatile RAM"
  },
  {
    0x07,
    L" Cache memory"
  }
};

TABLE_ITEM  PMAErrorCorrectionTypesTable[] = {
  {
    0x01,
    L"  Other"
  },
  {
    0x02,
    L"  Unknown"
  },
  {
    0x03,
    L"  None"
  },
  {
    0x04,
    L"  Parity"
  },
  {
    0x05,
    L"  Single-bit ECC"
  },
  {
    0x06,
    L"  Multi-bit ECC"
  },
  {
    0x07,
    L"  CRC"
  }
};

TABLE_ITEM  MemoryDeviceFormFactorTable[] = {
  {
    0x01,
    L"  Other"
  },
  {
    0x02,
    L"  Unknown"
  },
  {
    0x03,
    L"  SIMM"
  },
  {
    0x04,
    L"  SIP"
  },
  {
    0x05,
    L"  Chip"
  },
  {
    0x06,
    L"  DIP"
  },
  {
    0x07,
    L"  ZIP"
  },
  {
    0x08,
    L"  Proprietary Card"
  },
  {
    0x09,
    L"  DIMM"
  },
  {
    0x0A,
    L"  TSOP"
  },
  {
    0x0B,
    L"  Row of chips"
  },
  {
    0x0C,
    L"  RIMM"
  },
  {
    0x0D,
    L"  SODIMM"
  },
  {
    0x0E,
    L"  SRIMM"
  },
  {
    0x0F,
    L"  FB-DIMM"
  }
};

TABLE_ITEM  MemoryDeviceTypeTable[] = {
  {
    0x01,
    L"  Other"
  },
  {
    0x02,
    L"  Unknown"
  },
  {
    0x03,
    L"  DRAM"
  },
  {
    0x04,
    L"  EDRAM"
  },
  {
    0x05,
    L"  VRAM"
  },
  {
    0x06,
    L"  SRAM"
  },
  {
    0x07,
    L"  RAM"
  },
  {
    0x08,
    L"  ROM"
  },
  {
    0x09,
    L"  FLASH"
  },
  {
    0x0A,
    L"  EEPROM"
  },
  {
    0x0B,
    L"  FEPROM"
  },
  {
    0x0C,
    L"  EPROM"
  },
  {
    0x0D,
    L"  CDRAM"
  },
  {
    0x0E,
    L"  3DRAM"
  },
  {
    0x0F,
    L"  SDRAM"
  },
  {
    0x10,
    L"  SGRAM"
  },
  {
    0x11,
    L"  RDRAM"
  },
  {
    0x12,
    L"  DDR"
  },
  {
    0x13,
    L"  DDR2"
  },
  {
    0x14,
    L"  DDR2 FB-DIMM"
  },
  {
    0x18,
    L"  DDR3"
  },
  {
    0x19,
    L"  FBD2"
  }
};

TABLE_ITEM  MemoryDeviceTypeDetailTable[] = {
  {
    1,
    L" Other"
  },
  {
    2,
    L" Unknown"
  },
  {
    3,
    L" Fast-paged"
  },
  {
    4,
    L" Static column"
  },
  {
    5,
    L" Pseudo-STATIC"
  },
  {
    6,
    L" RAMBUS "
  },
  {
    7,
    L" Synchronous"
  },
  {
    8,
    L" CMOS"
  },
  {
    9,
    L" EDO"
  },
  {
    10,
    L" Window DRAM"
  },
  {
    11,
    L" Cache DRAM"
  },
  {
    12,
    L" Non-volatile"
  },
  {
    13,
    L" Registered(Buffered)"
  },
  {
    14,
    L" Unbuffered(Unregistered)"
  }
};

TABLE_ITEM  MemoryErrorTypeTable[] = {
  {
    0x01,
    L"  Other"
  },
  {
    0x02,
    L"  Unknown"
  },
  {
    0x03,
    L"  OK"
  },
  {
    0x04,
    L"  Bad read"
  },
  {
    0x05,
    L"  Parity error"
  },
  {
    0x06,
    L"  Single-bit error"
  },
  {
    0x07,
    L"  Double-bit error"
  },
  {
    0x08,
    L"  Multi-bit error"
  },
  {
    0x09,
    L"  Nibble error"
  },
  {
    0x0A,
    L"  Checksum error"
  },
  {
    0x0B,
    L"  CRC error"
  },
  {
    0x0C,
    L"  Corrected single-bit error"
  },
  {
    0x0D,
    L"  Corrected error"
  },
  {
    0x0E,
    L"  Uncorrectable error"
  },
};

TABLE_ITEM  MemoryErrorGranularityTable[] = {
  {
    0x01,
    L"  Other"
  },
  {
    0x02,
    L"  Unknown"
  },
  {
    0x03,
    L"  Device level"
  },
  {
    0x04,
    L"  Memory partition level"
  },
};

TABLE_ITEM  MemoryErrorOperationTable[] = {
  {
    0x01,
    L"  Other"
  },
  {
    0x02,
    L"  Unknown"
  },
  {
    0x03,
    L"  Read"
  },
  {
    0x04,
    L"  Write"
  },
  {
    0x05,
    L"  Partial Write"
  },
};

TABLE_ITEM  PointingDeviceTypeTable[] = {
  {
    0x01,
    L"  Other"
  },
  {
    0x02,
    L"  Unknown"
  },
  {
    0x03,
    L"  Mouse"
  },
  {
    0x04,
    L"  Track Ball"
  },
  {
    0x05,
    L"  Track Point"
  },
  {
    0x06,
    L"  Glide Point"
  },
  {
    0x07,
    L"  Touch Pad"
  },
};

TABLE_ITEM  PointingDeviceInterfaceTable[] = {
  {
    0x01,
    L" Other"
  },
  {
    0x02,
    L" Unknown"
  },
  {
    0x03,
    L" Serial"
  },
  {
    0x04,
    L" PS/2"
  },
  {
    0x05,
    L" Infrared"
  },
  {
    0x06,
    L" HP-HIL"
  },
  {
    0x07,
    L" Bus mouse"
  },
  {
    0x08,
    L" ADB(Apple Desktop Bus"
  },
  {
    0xA0,
    L" Bus mouse DB-9"
  },
  {
    0xA1,
    L" Bus mouse mirco-DIN"
  },
  {
    0xA2,
    L" USB"
  },
};

TABLE_ITEM  PBDeviceChemistryTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" Lead Acid "
  },
  {
    0x04,
    L" Nickel Cadmium "
  },
  {
    0x05,
    L" Nickel metal hydride "
  },
  {
    0x06,
    L" Lithium-ion "
  },
  {
    0x07,
    L" Zinc air "
  },
  {
    0x08,
    L" Lithium Polymer "
  },
};

TABLE_ITEM  VPLocationTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" OK "
  },
  {
    0x04,
    L" Non-critical "
  },
  {
    0x05,
    L" Critical "
  },
  {
    0x06,
    L" Non-recoverable "
  },
};

TABLE_ITEM  VPStatusTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" Processor "
  },
  {
    0x04,
    L" Disk "
  },
  {
    0x05,
    L" Peripheral Bay "
  },
  {
    0x06,
    L" System Management Module "
  },
  {
    0x07,
    L" Motherboard "
  },
  {
    0x08,
    L" Memory Module "
  },
  {
    0x09,
    L" Processor Module "
  },
  {
    0x0A,
    L" Power Unit "
  },
  {
    0x0B,
    L" Add-in Card "
  },
};

TABLE_ITEM  CoolingDeviceStatusTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" OK "
  },
  {
    0x04,
    L" Non-critical "
  },
  {
    0x05,
    L" Critical "
  },
  {
    0x06,
    L" Non-recoverable "
  },
};

TABLE_ITEM  CoolingDeviceTypeTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" Fan "
  },
  {
    0x04,
    L" Centrifugal Blower "
  },
  {
    0x05,
    L" Chip Fan "
  },
  {
    0x06,
    L" Cabinet Fan "
  },
  {
    0x07,
    L" Power Supply Fan "
  },
  {
    0x08,
    L" Heat Pipe "
  },
  {
    0x09,
    L" Integrated Refrigeration "
  },
  {
    0x10,
    L" Active Cooling "
  },
  {
    0x11,
    L" Passive Cooling "
  },
};

TABLE_ITEM  TemperatureProbeStatusTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" OK "
  },
  {
    0x04,
    L" Non-critical "
  },
  {
    0x05,
    L" Critical "
  },
  {
    0x06,
    L" Non-recoverable "
  },
};

TABLE_ITEM  TemperatureProbeLocTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" Processor "
  },
  {
    0x04,
    L" Disk "
  },
  {
    0x05,
    L" Peripheral Bay "
  },
  {
    0x06,
    L" System Management Module "
  },
  {
    0x07,
    L" Motherboard "
  },
  {
    0x08,
    L" Memory Module "
  },
  {
    0x09,
    L" Processor Module "
  },
  {
    0x0A,
    L" Power Unit "
  },
  {
    0x0B,
    L" Add-in Card "
  },
};

TABLE_ITEM  ECPStatusTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" OK "
  },
  {
    0x04,
    L" Non-critical "
  },
  {
    0x05,
    L" Critical "
  },
  {
    0x06,
    L" Non-recoverable "
  },
};

TABLE_ITEM  ECPLocTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" Processor "
  },
  {
    0x04,
    L" Disk "
  },
  {
    0x05,
    L" Peripheral Bay "
  },
  {
    0x06,
    L" System Management Module "
  },
  {
    0x07,
    L" Motherboard "
  },
  {
    0x08,
    L" Memory Module "
  },
  {
    0x09,
    L" Processor Module "
  },
  {
    0x0A,
    L" Power Unit "
  },
  {
    0x0B,
    L" Add-in Card "
  },
};

TABLE_ITEM  MDTypeTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" National Semiconductor LM75 "
  },
  {
    0x04,
    L" National Semiconductor LM78 "
  },
  {
    0x05,
    L" National Semiconductor LM79 "
  },
  {
    0x06,
    L" National Semiconductor LM80 "
  },
  {
    0x07,
    L" National Semiconductor LM81 "
  },
  {
    0x08,
    L" Analog Devices ADM9240 "
  },
  {
    0x09,
    L" Dallas Semiconductor DS1780 "
  },
  {
    0x0A,
    L" Maxim 1617 "
  },
  {
    0x0B,
    L" Genesys GL518SM "
  },
  {
    0x0C,
    L" Winbond W83781D "
  },
  {
    0x0D,
    L" Holtek HT82H791 "
  },
};

TABLE_ITEM  MDAddressTypeTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" I/O Port "
  },
  {
    0x04,
    L" Memory "
  },
  {
    0x05,
    L" SM Bus "
  },
};

TABLE_ITEM  MemoryChannelTypeTable[] = {
  {
    0x01,
    L" Other "
  },
  {
    0x02,
    L" Unknown "
  },
  {
    0x03,
    L" RamBus "
  },
  {
    0x04,
    L" SyncLink "
  },
};

TABLE_ITEM  IPMIDIBMCInterfaceTypeTable[] = {
  {
    0x00,
    L" Unknown "
  },
  {
    0x01,
    L" KCS: Keyboard Controller Style "
  },
  {
    0x02,
    L" SMIC: Server Management Interface Chip "
  },
  {
    0x03,
    L" BT: Block Transfer "
  },
  {
    0xFF04,
    L" Reserved for future assignment by this specification "
  },
};

TABLE_ITEM  StructureTypeInfoTable[] = {
  {
    0,
    L" BIOS Information"
  },
  {
    1,
    L" System Information"
  },
  {
    2,
    L" Base Board Information"
  },
  {
    3,
    L" System Enclosure"
  },
  {
    4,
    L" Processor Information"
  },
  {
    5,
    L" Memory Controller Information "
  },
  {
    6,
    L" Memory Module Information "
  },
  {
    7,
    L" Cache Information "
  },
  {
    8,
    L" Port Connector Information "
  },
  {
    9,
    L" System Slots "
  },
  {
    10,
    L" On Board Devices Information  "
  },
  {
    11,
    L" OEM Strings"
  },
  {
    12,
    L" System Configuration Options "
  },
  {
    13,
    L" BIOS Language Information  "
  },
  {
    14,
    L" Group Associations "
  },
  {
    15,
    L" System Event Log "
  },
  {
    16,
    L" Physical Memory Array "
  },
  {
    17,
    L" Memory Device "
  },
  {
    18,
    L" 32-bit Memory Error Information "
  },
  {
    19,
    L" Memory Array Mapped Address "
  },
  {
    20,
    L" Memory Device Mapped Address  "
  },
  {
    21,
    L" Built-in Pointing Device "
  },
  {
    22,
    L" Portable Battery "
  },
  {
    23,
    L" System Reset "
  },
  {
    24,
    L" Hardware Security "
  },
  {
    25,
    L" System Power Controls "
  },
  {
    26,
    L" Voltage Probe "
  },
  {
    27,
    L" Cooling Device "
  },
  {
    28,
    L" Temperature Probe "
  },
  {
    29,
    L" Electrical Current Probe "
  },
  {
    30,
    L" Out-of-Band Remote Access  "
  },
  {
    31,
    L" Boot Integrity Services (BIS) Entry Point"
  },
  {
    32,
    L" System Boot Information "
  },
  {
    33,
    L" 64-bit Memory Error Information "
  },
  {
    34,
    L" Management Device "
  },
  {
    35,
    L" Management Device Component "
  },
  {
    36,
    L" Management Device Threshold Data "
  },
  {
    37,
    L" Memory Channel "
  },
  {
    38,
    L" IPMI Device Information "
  },
  {
    39,
    L" System Power Supply"
  },
  {
    40,
    L" Additional Information"
  },
  {
    41,
    L" Onboard Devices Extended Information"
  },
  {
    42,
    L" Management Controller Host Interface"
  },
  {
    0x7E,
    L" Inactive"
  },
  {
    0x7F,
    L" End-of-Table "
  },
};


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
  )
{
  UINTN Index;
  //
  // High byte and Low byte of word
  //
  UINT8 High;
  UINT8 Low;

  for (Index = 0; Index < Number; Index++) {
    High  = (UINT8) (Table[Index].Key >> 8);
    Low   = (UINT8) (Table[Index].Key & 0x00FF);

    //
    // Check if Key is in the range
    // or if Key == Value in the table
    //
    if ((High > Low && Key >= Low && Key <= High) 
      || (Table[Index].Key == Key)) {
      StrCpyS (Info, InfoLen, Table[Index].Info);
      StrCatS (Info, InfoLen, L"\n");
      return Key;
    }
  }

  StrCpyS (Info, InfoLen, L"Undefined Value\n");
  return QUERY_TABLE_UNFOUND;
}

/**
  Given a table of bit info and a Key, return the responding info to the Key.

  @param[in] Table     Point to a table which maintains a map of 'bit' to 'message'.
  @param[in] Number    Number of table items.
  @param[in] Bits      The Key of query the bit map information.
**/
VOID
PrintBitsInfo (
  IN  TABLE_ITEM    *Table,
  IN  UINTN         Number,
  IN  UINT32        Bits
  )
{
  //
  // Get certain bit of 'Value':
  //
#define BIT(Value, bit) ((Value) & ((UINT32) 1) << (bit))
  //
  // Clear certain bit of 'Value':
  //
#define CLR_BIT(Value, bit) ((Value) -= (BIT (Value, bit)))

  UINTN   Index;
  UINT32  Value;
  BOOLEAN NoInfo;

  NoInfo  = TRUE;
  Value   = Bits;
  //
  // query the table and print information
  //
  for (Index = 0; Index < Number; Index++) {
    if (BIT (Value, Table[Index].Key) != 0) {
      Print (Table[Index].Info);
      Print (L" | ");

      NoInfo = FALSE;
      //
      // clear the bit, for reserved bits test
      //
      CLR_BIT (Value, Table[Index].Key);
    }
  }

  if (NoInfo) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_NO_INFO), gShellDebug1HiiHandle);
  }

  if (Value != 0) {
    ShellPrintHiiEx(-1,-1,NULL,
      STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_RSVD_BITS_SET),
      gShellDebug1HiiHandle,
      Value
     );
  }

  Print (L"\n");
}
//
// //////////////////////////////////////////////////////////////////
//
// Following uses QueryTable functions to simplify the coding.
// QueryTable(), PrintBitsInfo()
//
//
#define PRINT_TABLE_ITEM(Table, Key) \
  do { \
    UINTN   Num; \
    CHAR16  Info[66]; \
    Num = sizeof (Table) / sizeof (TABLE_ITEM); \
    ZeroMem (Info, sizeof (Info)); \
    QueryTable (Table, Num, Key, Info, sizeof(Info)/sizeof(Info[0])); \
    Print (Info); \
  } while (0);

#define PRINT_BITS_INFO(Table, bits) \
  do { \
    UINTN Num; \
    Num = sizeof (Table) / sizeof (TABLE_ITEM); \
    PrintBitsInfo (Table, Num, (UINT32) bits); \
  } while (0);

/**
  Display System Information (Type 1) Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplaySystemWakeupType (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SYSTEM_WAKEUP_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (SystemWakeupTypeTable, Type);
}

/**
  Display Base Board (Type 2) Feature Flags.

  @param[in] FeatureFlags   The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayBaseBoardFeatureFlags (
  IN UINT8 FeatureFlags,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_BASE_BOARD_FEATURE_FLAGS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (FeatureFlags, Option);
  PRINT_BITS_INFO (BaseBoardFeatureFlagsTable, FeatureFlags);
}

/**
  Display Base Board (Type 2) Board Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayBaseBoardBoardType(
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_BASE_BOARD_BOARD_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (BaseBoardBoardTypeTable, Type);
}

/**
  Display System Enclosure (Type 3) Enclosure Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplaySystemEnclosureType (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SYSTEM_CHASSIS_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  //
  // query table and print info
  //
  PRINT_TABLE_ITEM (SystemEnclosureTypeTable, Type);

  if (BIT (Type, 7) != 0) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_CHASSIS_LOCK_PRESENT), gShellDebug1HiiHandle);
  }
}

/**
  Display System Enclosure (Type 3) Enclosure Status.

  @param[in] Status         The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplaySystemEnclosureStatus (
  IN UINT8 Status,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SYSTEM_CHASSIS_STATUS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Status, Option);
  PRINT_TABLE_ITEM (SystemEnclosureStatusTable, Status);
}

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
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SYSTEM_CHASSIS_SECURITY), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Status, Option);
  PRINT_TABLE_ITEM (SESecurityStatusTable, Status);
}

/**
  Display Processor Information (Type 4) Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayProcessorType (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_PROC_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (ProcessorTypeTable, Type);
}

/**
  Display Processor Information (Type 4) Upgrade.

  @param[in] Upgrade        The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayProcessorUpgrade (
  IN UINT8 Upgrade,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_PROC_UPDATE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Upgrade, Option);
  PRINT_TABLE_ITEM (ProcessorUpgradeTable, Upgrade);
}

/**
  Display Processor Information (Type 4) Characteristics.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayProcessorCharacteristics (
  IN UINT16 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_PROC_CHARACTERISTICS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_BITS_INFO (ProcessorCharacteristicsTable, Type);
}

/**
  Display Memory Controller Information (Type 5) method.

  @param[in] Method         The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMcErrorDetectMethod (
  IN UINT8 Method,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_DETECTMETHOD), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Method, Option);
  PRINT_TABLE_ITEM (McErrorDetectMethodTable, Method);
}

/**
  Display Memory Controller Information (Type 5) Capability.

  @param[in] Capability     The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMcErrorCorrectCapability (
  IN UINT8 Capability,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_CORRECT_CAPABILITY), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Capability, Option);
  PRINT_BITS_INFO (McErrorCorrectCapabilityTable, Capability);
}

/**
  Display Memory Controller Information (Type 5) Support.

  @param[in] Support        The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMcInterleaveSupport (
  IN UINT8 Support,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_INTERLEAVE_SUPPORT), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Support, Option);
  PRINT_TABLE_ITEM (McInterleaveSupportTable, Support);
}

/**
  Display Memory Controller Information (Type 5) speeds.

  @param[in] Speed          The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMcMemorySpeeds (
  IN UINT16  Speed,
  IN UINT8   Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_MEMORY_SPEED), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Speed, Option);
  PRINT_BITS_INFO (McMemorySpeedsTable, Speed);
}

/**
  Display Memory Controller Information (Type 5) voltage.

  @param[in] Voltage        The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMemoryModuleVoltage (
  IN UINT8 Voltage,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_REQUIRED_VOLTAGES), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Voltage, Option);
  PRINT_BITS_INFO (MemoryModuleVoltageTable, Voltage);
}

/**
  Display Memory Module Information (Type 6) type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMmMemoryType (
  IN UINT16  Type,
  IN UINT8   Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_MODULE_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_BITS_INFO (MmMemoryTypeTable, Type);
}

/**
  Display Memory Module Information (Type 6) status.

  @param[in] Status         The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayMmErrorStatus (
  IN UINT8 Status,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_MODULE_ERROR_STATUS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Status, Option);
  PRINT_BITS_INFO (MmErrorStatusTable, Status);
}

/**
  Display Cache Information (Type 7) SRAM Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayCacheSRAMType (
  IN UINT16  Type,
  IN UINT8   Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_CACHE_SRAM_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION ((UINT8) Type, Option);
  PRINT_BITS_INFO (CacheSRAMTypeTable, (UINT8) Type);
}

/**
  Display Cache Information (Type 7) correcting Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayCacheErrCorrectingType (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_CACHE_ERROR_CORRECTING), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (CacheErrCorrectingTypeTable, Type);
}

/**
  Display Cache Information (Type 7) Type.

  @param[in] Type           The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayCacheSystemCacheType (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_CACHE_SYSTEM_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (CacheSystemCacheTypeTable, Type);
}

/**
  Display Cache Information (Type 7) Associativity.

  @param[in] Associativity  The key of the structure.
  @param[in] Option         The optional information.
**/
VOID
DisplayCacheAssociativity (
  IN UINT8 Associativity,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_CACHE_ASSOCIATIVITY), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Associativity, Option);
  PRINT_TABLE_ITEM (CacheAssociativityTable, Associativity);
}

/**
  Display Port Connector Information  (Type 8) type.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPortConnectorType (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_PORT_CONNECTOR_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (PortConnectorTypeTable, Type);
}

/**
  Display Port Connector Information  (Type 8) port type.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPortType (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_PORT_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (PortTypeTable, Type);
}

/**
  Display System Slots (Type 9) slot type.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySystemSlotType (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SYSTEM_SLOT_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (SystemSlotTypeTable, Type);
}

/**
  Display System Slots (Type 9) data bus width.

  @param[in] Width      The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySystemSlotDataBusWidth (
  IN UINT8 Width,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SYSTEM_SLOT_DATA), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Width, Option);
  PRINT_TABLE_ITEM (SystemSlotDataBusWidthTable, Width);
}

/**
  Display System Slots (Type 9) usage information.

  @param[in] Usage      The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySystemSlotCurrentUsage (
  IN UINT8 Usage,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SYSTEM_SLOT_CURRENT_USAGE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Usage, Option);
  PRINT_TABLE_ITEM (SystemSlotCurrentUsageTable, Usage);
}

/**
  Display System Slots (Type 9) slot length.

  @param[in] Length     The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySystemSlotLength (
  IN UINT8 Length,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SYSTEM_SLOT_LENGTH), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Length, Option);
  PRINT_TABLE_ITEM (SystemSlotLengthTable, Length);
}

/**
  Display System Slots (Type 9) characteristics.

  @param[in] Chara1     The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySlotCharacteristics1 (
  IN UINT8 Chara1,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SLOT_CHARACTERISTICS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Chara1, Option);
  PRINT_BITS_INFO (SlotCharacteristics1Table, Chara1);
}

/**
  Display System Slots (Type 9) characteristics.

  @param[in] Chara2     The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySlotCharacteristics2 (
  IN UINT8 Chara2,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SLOT_CHARACTERISTICS_2), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Chara2, Option);
  PRINT_BITS_INFO (SlotCharacteristics2Table, Chara2);
}

/**
  Display On Board Devices Information (Type 10) types.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayOnboardDeviceTypes (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_ONBOARD_DEVICE_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (OnboardDeviceTypesTable, Type);
}

/**
  Display System Event Log (Type 15) types.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySELTypes (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SYSTEM_EVENT_LOG_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (SELTypesTable, Type);
}

/**
  Display System Event Log (Type 15) format type.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySELVarDataFormatType (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_EVENT_LOG_VAR_DATA_FORMAT), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (SELVarDataFormatTypeTable, Type);
}

/**
  Display System Event Log (Type 15) dw1.

  @param[in] Key        The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPostResultsBitmapDw1 (
  IN UINT32  Key,
  IN UINT8   Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_POST_RESULTS_BITMAP), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);
  PRINT_BITS_INFO (PostResultsBitmapDw1Table, Key);
}

/**
  Display System Event Log (Type 15) dw2.

  @param[in] Key        The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPostResultsBitmapDw2 (
  IN UINT32  Key,
  IN UINT8   Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_POST_RESULTS_SECOND_DWORD), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);
  PRINT_BITS_INFO (PostResultsBitmapDw2Table, Key);
}

/**
  Display System Event Log (Type 15) type.

  @param[in] SMType     The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplaySELSysManagementTypes (
  IN UINT32  SMType,
  IN UINT8   Option
  )
{
  UINT8       Temp;

  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SYSTEM_MANAGEMENT_TYPES), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (SMType, Option);

  //
  // Deal with wide range Value
  //
  if (SMType >= 0x80000000) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_OEM_ASSIGNED), gShellDebug1HiiHandle);
  } else if (SMType >= 0x00020000) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_RSVD_FOR_FUTURE_ASSIGN), gShellDebug1HiiHandle);
  } else if (SMType >= 0x00010000) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_SYSTEM_MANAGEMENT_PROBE), gShellDebug1HiiHandle);
  } else if (SMType >= 0x31) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_RSVD_FOR_FUTURE_ASSIGN), gShellDebug1HiiHandle);
  } else {
    //
    // Deal with One byte data
    //
    Temp = (UINT8) (SMType & 0x3F);
    PRINT_TABLE_ITEM (SELSysManagementTypesTable, Temp);
  }
}

/**
  Display Physical Memory Array (Type 16) Location.

  @param[in] Location   The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPMALocation (
  IN UINT8 Location,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_PHYS_MEM_ARRAY_LOCATION), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Location, Option);
  PRINT_TABLE_ITEM (PMALocationTable, Location);
}

/**
  Display Physical Memory Array (Type 16) Use.

  @param[in] Use        The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPMAUse (
  IN UINT8 Use,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_PHYS_MEM_ARRAY_USE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Use, Option);
  PRINT_TABLE_ITEM (PMAUseTable, Use);
}

/**
  Display Physical Memory Array (Type 16) Types.

  @param[in] Type       The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPMAErrorCorrectionTypes (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_PHYS_MEM_ARRAY_ERROR), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (PMAErrorCorrectionTypesTable, Type);
}

/**
  Display Memory Device (Type 17) form factor.

  @param[in] FormFactor The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayMemoryDeviceFormFactor (
  IN UINT8 FormFactor,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_DEVICE_FORM_FACTOR), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (FormFactor, Option);
  PRINT_TABLE_ITEM (MemoryDeviceFormFactorTable, FormFactor);
}

/**
  Display Memory Device (Type 17) type.

  @param[in] Type     The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMemoryDeviceType (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_DEVICE_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (MemoryDeviceTypeTable, Type);
}

/**
  Display Memory Device (Type 17) details.

  @param[in] Para     The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMemoryDeviceTypeDetail (
  IN UINT16  Para,
  IN UINT8   Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_DEVICE_TYPE_DETAIL), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Para, Option);
  PRINT_BITS_INFO (MemoryDeviceTypeDetailTable, Para);
}

/**
  Display 32-bit Memory Error Information (Type 18) type.

  @param[in] ErrorType  The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayMemoryErrorType (
  IN UINT8 ErrorType,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_ERROR_INFO), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (ErrorType, Option);
  PRINT_TABLE_ITEM (MemoryErrorTypeTable, ErrorType);
}

/**
  Display 32-bit Memory Error Information (Type 18) error granularity.

  @param[in] Granularity  The key of the structure.
  @param[in] Option       The optional information.
**/
VOID
DisplayMemoryErrorGranularity (
  IN UINT8 Granularity,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_ERROR_GRANULARITY), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Granularity, Option);
  PRINT_TABLE_ITEM (MemoryErrorGranularityTable, Granularity);
}

/**
  Display 32-bit Memory Error Information (Type 18) error information.

  @param[in] Operation  The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayMemoryErrorOperation (
  IN UINT8 Operation,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_ERROR_OP), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Operation, Option);
  PRINT_TABLE_ITEM (MemoryErrorOperationTable, Operation);
}

/**
  Display Built-in Pointing Device (Type 21) type information.

  @param[in] Type     The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayPointingDeviceType (
  IN UINT8 Type,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_POINTING_DEVICE_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (PointingDeviceTypeTable, Type);
}

/**
  Display Built-in Pointing Device (Type 21) information.

  @param[in] Interface  The key of the structure.
  @param[in] Option     The optional information.
**/
VOID
DisplayPointingDeviceInterface (
  IN UINT8   Interface,
  IN UINT8   Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_POINTING_DEVICE_INTERFACE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Interface, Option);
  PRINT_TABLE_ITEM (PointingDeviceInterfaceTable, Interface);
}

/**
  Display Portable Battery  (Type 22) information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayPBDeviceChemistry (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_PORTABLE_BATT_DEV_CHEM), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);
  PRINT_TABLE_ITEM (PBDeviceChemistryTable, Key);
}

/**
  Display Voltage Probe (Type 26) location information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayVPLocation (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  UINT8       Loc;

  Loc = (UINT8) ((Key & 0xE0) >> 5);
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_VOLTAGE_PROBE_LOC), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Loc, Option);
  PRINT_TABLE_ITEM (VPLocationTable, Loc);
}

/**
  Display Voltage Probe (Type 26) status ype information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayVPStatus (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  UINT8       Status;

  Status = (UINT8) (Key & 0x1F);
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_VOLTAGE_PROBE_STATUS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Status, Option);
  PRINT_TABLE_ITEM (VPStatusTable, Status);
}

/**
  Display Cooling (Type 27) status information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayCoolingDeviceStatus (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  UINT8       Status;

  Status = (UINT8) ((Key & 0xE0) >> 5);
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_COOLING_DEV_STATUS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Status, Option);
  PRINT_TABLE_ITEM (CoolingDeviceStatusTable, Status);
}

/**
  Display Cooling (Type 27) type information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayCoolingDeviceType (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  UINT8       Type;

  Type = (UINT8) (Key & 0x1F);
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_COOLING_DEV_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Type, Option);
  PRINT_TABLE_ITEM (CoolingDeviceTypeTable, Type);
}

/**
  Display Temperature Probe (Type 28) status information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayTemperatureProbeStatus (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  UINT8       Status;

  Status = (UINT8) ((Key & 0xE0) >> 5);
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_TEMP_PROBE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Status, Option);
  PRINT_TABLE_ITEM (TemperatureProbeStatusTable, Status);
}

/**
  Display Temperature Probe  (Type 28) location information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayTemperatureProbeLoc (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  UINT8       Loc;

  Loc = (UINT8) (Key & 0x1F);
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_VOLTAGE_PROBE_LOC), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Loc, Option);
  PRINT_TABLE_ITEM (TemperatureProbeLocTable, Loc);
}

/**
  Display Electrical Current Probe (Type 29)  status information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayECPStatus (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  UINT8       Status;

  Status = (UINT8) ((Key & 0xE0) >> 5);
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_ELEC_PROBE_STATUS), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Status, Option);
  PRINT_TABLE_ITEM (ECPStatusTable, Status);
}

/**
  Display Type 29 information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayECPLoc (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  UINT8       Loc;

  Loc = (UINT8) (Key & 0x1F);
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_ELEC_PROBE_LOC), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Loc, Option);
  PRINT_TABLE_ITEM (ECPLocTable, Loc);
}

/**
  Display Management Device (Type 34) Type.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMDType (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MANAGEMENT_DEV_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);
  PRINT_TABLE_ITEM (MDTypeTable, Key);
}

/**
  Display Management Device (Type 34) Address Type.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMDAddressType (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MANAGEMENT_DEV_ADDR_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);
  PRINT_TABLE_ITEM (MDAddressTypeTable, Key);
}

/**
  Display Memory Channel (Type 37) information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayMemoryChannelType (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_MEM_CHANNEL_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);
  PRINT_TABLE_ITEM (MemoryChannelTypeTable, Key);
}

/**
  Display IPMI Device Information (Type 38) information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayIPMIDIBMCInterfaceType (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_BMC_INTERFACE_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);
  PRINT_TABLE_ITEM (IPMIDIBMCInterfaceTypeTable, Key);
}

/**
  Display the structure type information.

  @param[in] Key      The key of the structure.
  @param[in] Option   The optional information.
**/
VOID
DisplayStructureTypeInfo (
  IN UINT8 Key,
  IN UINT8 Option
  )
{
  //
  // display
  //
  ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_QUERYTABLE_STRUCT_TYPE), gShellDebug1HiiHandle);
  PRINT_INFO_OPTION (Key, Option);
  PRINT_TABLE_ITEM (StructureTypeInfoTable, Key);
}
