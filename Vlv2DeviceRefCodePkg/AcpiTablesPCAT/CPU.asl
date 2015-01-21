/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved   *;
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



// NOTE:  The _PDC Implementation is out of the scope of this
// reference code.  Please see the latest Hyper-Threading Technology
// Reference Code for complete implementation details.

Scope(\_PR)
{
  Processor(CPU0,         // Unique name for Processor 0.
            1,                        // Unique ID for Processor 0.
            0x00,                 // CPU0 ACPI P_BLK address = ACPIBASE + 10h.
            0)                        // CPU0  P_BLK length = 6 bytes.
  {}

  Processor(CPU1,         // Unique name for Processor 1.
            2,                        // Unique ID for Processor 1.
            0x00,
            0)                    // CPU1 P_BLK length = 6 bytes.
  {}

  Processor(CPU2,         // Unique name for Processor 2.
            3,                        // Unique ID for Processor 2.
            0x00,
            0)                    // CPU2 P_BLK length = 6 bytes.
  {}

  Processor(CPU3,         // Unique name for Processor 3.
            4,                        // Unique ID for Processor 3.
            0x00,
            0)                    // CPU3 P_BLK length = 6 bytes.
  {}
}     // End _PR


