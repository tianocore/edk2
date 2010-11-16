/** @file
  Module to clarify the element info of the smbios structure.

  Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMBIOS_PRINT_INFO_H
#define _SMBIOS_PRINT_INFO_H

#include "LibSmbios.h"

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

VOID
SmbiosPrintEPSInfo (
  IN  SMBIOS_STRUCTURE_TABLE  *pSmbiosTable,
  IN  UINT8                   Option
  );

EFI_STATUS
SmbiosPrintStructure (
  IN  SMBIOS_STRUCTURE_POINTER  *pStruct,
  IN  UINT8                     Option
  );

//
// BIOS Information (Type 0)
//
VOID
DisplayBiosCharacteristics (
  UINT64  chara,
  UINT8   Option
  );
VOID
DisplayBiosCharacteristicsExt1 (
  UINT8 byte1,
  UINT8 Option
  );
VOID
DisplayBiosCharacteristicsExt2 (
  UINT8 byte2,
  UINT8 Option
  );

//
// Processor Information (Type 4)
//
VOID
DisplayProcessorFamily (
  UINT8 Family,
  UINT8 Option
  );

VOID
DisplayProcessorFamily2 (
  UINT16 Family2,
  UINT8  Option
  );

VOID
DisplayProcessorVoltage (
  UINT8 Voltage,
  UINT8 Option
  );
VOID
DisplayProcessorStatus (
  UINT8 Status,
  UINT8 Option
  );

//
// Memory Controller Information (Type 5)
//
VOID
DisplayMaxMemoryModuleSize (
  UINT8 Size,
  UINT8 SlotNum,
  UINT8 Option
  );
VOID
DisplayMemoryModuleConfigHandles (
  UINT16  *pHandles,
  UINT8   SlotNum,
  UINT8   Option
  );

//
// Memory Module Information (Type 6)
//
VOID
DisplayMmBankConnections (
  UINT8 BankConnections,
  UINT8 Option
  );
VOID
DisplayMmMemorySize (
  UINT8 Size,
  UINT8 Option
  );

//
// System Slots (Type 9)
//
VOID
DisplaySystemSlotId (
  UINT16  SlotId,
  UINT8   SlotType,
  UINT8   Option
  );

//
// Physical Memory Array (Type 16)
// Memory Device (Type 17)
// Memory Array Mapped Address (Type 19)
// Memory Device Mapped Address (Type 20)
// Portable Battery (Type 22)
//
VOID
DisplaySBDSManufactureDate (
  UINT16  Date,
  UINT8   Option
  );

//
// System Reset  (Type 23)
//
VOID
DisplaySystemResetCapabilities (
  UINT8 Reset,
  UINT8 Option
  );

//
// Hardware Security (Type 24)
//
VOID
DisplayHardwareSecuritySettings (
  UINT8 Settings,
  UINT8 Option
  );

//
// Out-of-Band Remote Access (Type 30)
//
VOID
DisplayOBRAConnections (
  UINT8   Connections,
  UINT8   Option
  );

//
// System Boot Information (Type 32)
//
VOID
DisplaySystemBootStatus (
  UINT8 Parameter,
  UINT8 Option
  );

//
// System Power Supply (Type 39)
//
VOID
DisplaySPSCharacteristics (
  UINT16  Characteristics,
  UINT8   Option
  );

#endif
