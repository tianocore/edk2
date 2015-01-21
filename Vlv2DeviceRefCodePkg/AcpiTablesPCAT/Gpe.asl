/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved    *;
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


// General Purpose Events.  This Scope handles the Run-time and
// Wake-time SCIs.  The specific method called will be determined by
// the _Lxx value, where xx equals the bit location in the General
// Purpose Event register(s).

Scope(\_GPE)
{
  //
  // Software GPE caused the event.
  //
  Method(_L02)
  {
    // Clear GPE status bit.
    Store(0,GPEC)
    //
    // Handle DTS Thermal Events.
    //
    External(DTSE, IntObj)
    If(CondRefOf(DTSE))
    {
      If(LGreaterEqual(DTSE, 0x01))
      {
        Notify(\_TZ.TZ01,0x80)
      }
    }
  }

  //
  // PUNIT SCI event.
  //
  Method(_L04)
  {
    // Clear the PUNIT Status Bit.
    Store(1, PSCI)
  }


  //
  // IGD OpRegion SCI event (see IGD OpRegion/Software SCI BIOS SPEC).
  //
  Method(_L05)
  {
    If(LAnd(\_SB.PCI0.GFX0.GSSE, LNot(GSMI)))   // Graphics software SCI event?
    {
      \_SB.PCI0.GFX0.GSCI()     // Handle the SWSCI
    }
  }

  //
  // This PME event (PCH's GPE #13) is received when any PCH internal device with PCI Power Management capabilities
  // on bus 0 asserts the equivalent of the PME# signal.
  //
  Method(_L0D, 0)
  {
    If(LAnd(\_SB.PCI0.EHC1.PMEE, \_SB.PCI0.EHC1.PMES))
    {
      If(LNotEqual(OSEL, 1))
      {
        Store(1, \_SB.PCI0.EHC1.PMES) //Clear PME status
        Store(0, \_SB.PCI0.EHC1.PMEE) //Disable PME
      }
      Notify(\_SB.PCI0.EHC1, 0x02)
    }
    If(LAnd(\_SB.PCI0.XHC1.PMEE, \_SB.PCI0.XHC1.PMES))
    {
      If(LNotEqual(OSEL, 1))
      {
        Store(1, \_SB.PCI0.XHC1.PMES) //Clear PME status
        Store(0, \_SB.PCI0.XHC1.PMEE) //Disable PME
      }
      Notify(\_SB.PCI0.XHC1, 0x02)
    }
    If(LAnd(\_SB.PCI0.HDEF.PMEE, \_SB.PCI0.HDEF.PMES))
    {
      If(LNotEqual(OSEL, 1))
      {
        Store(1, \_SB.PCI0.HDEF.PMES) //Clear PME status
        Store(0, \_SB.PCI0.HDEF.PMEE) //Disable PME
      }
      Notify(\_SB.PCI0.HDEF, 0x02)
    }
  }
}
