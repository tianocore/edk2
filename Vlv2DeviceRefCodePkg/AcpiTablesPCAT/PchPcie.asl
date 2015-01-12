/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c)  2011  - 2014, Intel Corporation. All rights reserved   *;
;
; This program and the accompanying materials are licensed and made available under
; the terms and conditions of the BSD License that accompanies this distribution.
; The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/


OperationRegion(PXCS,PCI_Config,0x40,0xC0)
Field(PXCS,AnyAcc, NoLock, Preserve)
{
  Offset(0x10), // LCTL - Link Control Register
  L0SE, 1,      // 0, L0s Entry Enabled
  , 7,
  Offset(0x12), // LSTS - Link Status Register
  , 13,
  LASX, 1,      // 0, Link Active Status
  Offset(0x1A), // SLSTS[7:0] - Slot Status Register
  ABPX, 1,      // 0, Attention Button Pressed
  , 2,
  PDCX, 1,      // 3, Presence Detect Changed
  , 2,
  PDSX, 1,      // 6, Presence Detect State
  , 1,
  Offset(0x20), // RSTS - Root Status Register
  , 16,
  PSPX, 1,      // 16,        PME Status
}


Device(PXSX)
{
  Name(_ADR, 0x00000000)

  // NOTE:  Any PCIE Hot-Plug dependency for this port is
  // specific to the CRB.  Please modify the code based on
  // your platform requirements.

  Name(_PRW, Package() {9,4})
}


