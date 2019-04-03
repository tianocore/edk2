/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved    *;
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/


// LPC Bridge - Device 31, Function 0
// Define the needed LPC registers used by ASL.

scope(\_SB)
{
  OperationRegion(ILBR, SystemMemory, \IBAS, 0x8C)
  Field(ILBR, AnyAcc, NoLock, Preserve)
  {
    Offset(0x08), // 0x08
    PARC,   8,
    PBRC,   8,
    PCRC,   8,
    PDRC,   8,
    PERC,   8,
    PFRC,   8,
    PGRC,   8,
    PHRC,   8,
    Offset(0x88), // 0x88
    ,       3,
    UI3E,   1,
    UI4E,   1
  }

  Include ("98_LINK.ASL")
}

OperationRegion(LPC0, PCI_Config, 0x00, 0xC0)
Field(LPC0, AnyAcc, NoLock, Preserve)
{
  Offset(0x08), // 0x08
  SRID,   8,  // Revision ID
  Offset(0x080), // 0x80
  C1EN,   1, // COM1 Enable
  ,      31
}


Include ("LPC_DEV.ASL")





