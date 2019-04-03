/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved   *;
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
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
