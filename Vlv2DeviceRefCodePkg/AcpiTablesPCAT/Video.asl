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




// Brightness Notification:
//              Generate a brightness related notification
//      to the LFP if its populated.
//
//      Arguments:
//              Arg0:   Notification value.
//
//      Return Value:
//              None
Method(BRTN,1,Serialized)
{
  If(LEqual(And(DIDX,0x0F00),0x400))
  {
    Notify(\_SB.PCI0.GFX0.DD1F,Arg0)
  }
}
