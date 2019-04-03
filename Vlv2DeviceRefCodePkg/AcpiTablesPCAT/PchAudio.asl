/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c)  2011  - 2014, Intel Corporation. All rights reserved   *;
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/
//
// High Definition Audio - Device 27, Function 0
//
OperationRegion(HDAR, PCI_Config, 0x4C,0x10)
Field(HDAR,WordAcc,NoLock,Preserve)
{
  Offset(0x00), // 0x4C, Dock Control Register
  DCKA,    1,   // Dock Attach
  ,    7,
  Offset(0x01), // 04Dh, Dock Status Register
  DCKM,    1,   // Dock Mated
  ,    6,
  DCKS,    1,   // Docking Supported
  Offset(0x08), // 0x54, Power Management Control and Status Register
  ,    8,
  PMEE,    1,   // PME_EN
  ,    6,
  PMES,    1    // PME Status
}



