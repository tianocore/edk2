/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  INTELISPDev2.ASL

Abstract:

  ISP Exist as B0D2F0 Software ACPI Reference Code for the Baytrail Family.

--*/
////Device ISP0
Device(ISP0)
{
  Name(_ADR, 0x0F38)
  //Name (_HID, "80860F38")
  //Name (_CID, "80860F38")
  Name(_DDN, "VLV2 ISP - 80860F38")
  Name(_UID, 0x01)

  Method (_STA, 0, NotSerialized)
  {
    If(LEqual(ISPD,1))   //Dev2 need report ISP0 as GFX0 child
    {
      Return (0xF)
    }
    Else
    {
      Return (0x0)
    }
  }
  Name(SBUF,ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00400000, ISP0)
  })
  Method(_CRS, 0x0, NotSerialized)
  {
    Return (SBUF)
  }
  Method (_SRS, 0x1, NotSerialized)
  {
  }
  Method (_DIS, 0x0, NotSerialized)
  {
  }
  Method(_DSM, 0x4, NotSerialized)
  {
    If (LEqual (Arg0, 0x01))
    {
      ///Switch ISP to D3H
      Return (0x01)
    }
    Elseif (LEqual (Arg0, 0x02))
    {
      //Switch ISP to D0
      Return (0x02)
    }
    Else
    {
      //Do nothing
      Return (0x0F)
    }
  }
} ///End ISP0

