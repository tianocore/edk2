/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  INTELGFX.ASL

Abstract:

  IGD OpRegion/Software ACPI Reference Code for the Baytrail Family.

--*/

// Enable/Disable Output Switching.  In WIN2K/WINXP, _DOS = 0 will
// get called during initialization to prepare for an ACPI Display
// Switch Event.  During an ACPI Display Switch, the OS will call
// _DOS = 2 immediately after a Notify=0x80 to temporarily disable
// all Display Switching.  After ACPI Display Switching is complete,
// the OS will call _DOS = 0 to re-enable ACPI Display Switching.

Method(_DOS,1)
{
  // Store Display Switching and LCD brightness BIOS control bit
  Store(And(Arg0,7),DSEN)
}

// Enumerate the Display Environment.  This method will return
// valid addresses for all display device encoders present in the
// system.  The Miniport Driver will reject the addresses for every
// encoder that does not have an attached display device.  After
// enumeration is complete, the OS will call the _DGS methods
// during a display switch only for the addresses accepted by the
// Miniport Driver.  For hot-insertion and removal of display
// devices, a re-enumeration notification will be required so the
// address of the newly present display device will be accepted by
// the Miniport Driver.

Method(_DOD, 0, Serialized)
{
  Store(0, NDID)
  If(LNotEqual(DIDL, Zero))
  {
    Store(SDDL(DIDL),DID1)
  }
  If(LNotEqual(DDL2, Zero))
  {
    Store(SDDL(DDL2),DID2)
  }
  If(LNotEqual(DDL3, Zero))
  {
    Store(SDDL(DDL3),DID3)
  }
  If(LNotEqual(DDL4, Zero))
  {
    Store(SDDL(DDL4),DID4)
  }
  If(LNotEqual(DDL5, Zero))
  {
    Store(SDDL(DDL5),DID5)
  }

  // TODO - This level of flexibility is not needed for a true
  //      OEM design.  Simply determine the greatest number of
  //      encoders the platform will suppport then remove all
  //      return packages beyond that value.  Note that for
  //      current silicon, the maximum number of encoders
  //      possible is 5.

  If(LEqual(NDID,1))
  {
    If (LNOTEqual (ISPD, 0))
    {
      Name(TMP0,Package() {0xFFFFFFFF,0xFFFFFFFF})
      Store(Or(0x10000,DID1),Index(TMP0,0))
      //Add ISP device to GFX0
      Store(0x00020F38, Index(TMP0,1))
      Return(TMP0)
    } Else
    {
      Name(TMP1,Package() {0xFFFFFFFF})
      Store(Or(0x10000,DID1),Index(TMP1,0))
      Return(TMP1)
    }
  }

  If(LEqual(NDID,2))
  {
    If (LNOTEqual (ISPD, 0))
    {
      Name(TMP2,Package() {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF})
      Store(Or(0x10000,DID1),Index(TMP2,0))
      Store(Or(0x10000,DID2),Index(TMP2,1))
      //Add ISP device to GFX0
      Store(0x00020F38, Index(TMP2,2))
      Return(TMP2)
    } Else
    {
      Name(TMP3,Package() {0xFFFFFFFF, 0xFFFFFFFF})
      Store(Or(0x10000,DID1),Index(TMP3,0))
      Store(Or(0x10000,DID2),Index(TMP3,1))
      Return(TMP3)
    }
  }

  If(LEqual(NDID,3))
  {
    If (LNOTEqual (ISPD, 0))
    {
      Name(TMP4,Package() {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,0xFFFFFFFF})
      Store(Or(0x10000,DID1),Index(TMP4,0))
      Store(Or(0x10000,DID2),Index(TMP4,1))
      Store(Or(0x10000,DID3),Index(TMP4,2))
      //Add ISP device to GFX0
      Store(0x00020F38, Index(TMP4,3))
      Return(TMP4)
    } Else
    {
      Name(TMP5,Package() {0xFFFFFFFF, 0xFFFFFFFF,0xFFFFFFFF})
      Store(Or(0x10000,DID1),Index(TMP5,0))
      Store(Or(0x10000,DID2),Index(TMP5,1))
      Store(Or(0x10000,DID3),Index(TMP5,2))
      Return(TMP5)
    }
  }

  If(LEqual(NDID,4))
  {
    If (LNOTEqual (ISPD, 0))
    {
      Name(TMP6,Package() {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF})
      Store(Or(0x10000,DID1),Index(TMP6,0))
      Store(Or(0x10000,DID2),Index(TMP6,1))
      Store(Or(0x10000,DID3),Index(TMP6,2))
      Store(Or(0x10000,DID4),Index(TMP6,3))
      //Add ISP device to GFX0
      Store(0x00020F38, Index(TMP6,4))
      Return(TMP6)
    } Else
    {
      Name(TMP7,Package() {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF})
      Store(Or(0x10000,DID1),Index(TMP7,0))
      Store(Or(0x10000,DID2),Index(TMP7,1))
      Store(Or(0x10000,DID3),Index(TMP7,2))
      Store(Or(0x10000,DID4),Index(TMP7,3))
      Return(TMP7)
    }
  }

  If(LGreater(NDID,4))
  {
    If (LNOTEqual (ISPD, 0))
    {
      Name(TMP8,Package() {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF})
      Store(Or(0x10000,DID1),Index(TMP8,0))
      Store(Or(0x10000,DID2),Index(TMP8,1))
      Store(Or(0x10000,DID3),Index(TMP8,2))
      Store(Or(0x10000,DID4),Index(TMP8,3))
      Store(Or(0x10000,DID5),Index(TMP8,4))
      //Add ISP device to GFX0
      Store(0x00020F38, Index(TMP8,5))
      Return(TMP8)
    } Else
    {
      Name(TMP9,Package() {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF})
      Store(Or(0x10000,DID1),Index(TMP9,0))
      Store(Or(0x10000,DID2),Index(TMP9,1))
      Store(Or(0x10000,DID3),Index(TMP9,2))
      Store(Or(0x10000,DID4),Index(TMP9,3))
      Store(Or(0x10000,DID5),Index(TMP9,4))
      Return(TMP9)
    }
  }

  // If nothing else, return Unknown LFP.
  // (Prevents compiler warning.)

  //Add ISP device to GFX0
  If (LNOTEqual (ISPD, 0))
  {
    Return(Package() {0x00000400, 0x00020F38})
  } Else
  {
    Return(Package() {0x00000400})
  }
}

Device(DD01)
{

  // Return Unique ID.

  Method(_ADR,0,Serialized)
  {
    If(LEqual(And(0x0F00,DID1),0x400))
    {
      Store(0x1, EDPV)
      Store(DID1, DIDX)
      Return(1)
    }
    If(LEqual(DID1,0))
    {
      Return(1)
    }
    Else
    {
      Return(And(0xFFFF,DID1))
    }
  }

  // Return the Current Status.

  Method(_DCS,0)
  {
    Return(CDDS(DID1))
  }

  // Query Graphics State (active or inactive).

  Method(_DGS,0)
  {
    Return(NDDS(DID1))
  }

  // Device Set State.

  // _DSS Table:
  //
  //      BIT31   BIT30   Execution
  //      0       0       Don't implement.
  //      0       1       Cache change.  Nothing to Implement.
  //      1       0       Don't Implement.
  //      1       1       Display Switch Complete.  Implement.

  Method(_DSS,1)
  {
    If(LEqual(And(Arg0,0xC0000000),0xC0000000))
    {

      // State change was performed by the
      // Video Drivers.  Simply update the
      // New State.

      Store(NSTE,CSTE)
    }
  }
}

Device(DD02)
{

  // Return Unique ID.

  Method(_ADR,0,Serialized)
  {
    If(LEqual(And(0x0F00,DID2),0x400))
    {
      Store(0x2, EDPV)
      Store(DID2, DIDX)
      Return(2)
    }
    If(LEqual(DID2,0))
    {
      Return(2)
    }
    Else
    {
      Return(And(0xFFFF,DID2))
    }
  }

  // Return the Current Status.

  Method(_DCS,0)
  {
    Return(CDDS(DID2))
  }

  // Query Graphics State (active or inactive).

  Method(_DGS,0)
  {
    // Return the Next State.
    Return(NDDS(DID2))
  }

  // Device Set State. (See table above.)

  Method(_DSS,1)
  {
    If(LEqual(And(Arg0,0xC0000000),0xC0000000))
    {

      // State change was performed by the
      // Video Drivers.  Simply update the
      // New State.

      Store(NSTE,CSTE)
    }
  }
}

Device(DD03)
{

  // Return Unique ID.

  Method(_ADR,0,Serialized)
  {
    If(LEqual(And(0x0F00,DID3),0x400))
    {
      Store(0x3, EDPV)
      Store(DID3, DIDX)
      Return(3)
    }
    If(LEqual(DID3,0))
    {
      Return(3)
    }
    Else
    {
      Return(And(0xFFFF,DID3))
    }
  }

  // Return the Current Status.

  Method(_DCS,0)
  {
    If(LEqual(DID3,0))
    {
      Return(0x0B)
    }
    Else
    {
      Return(CDDS(DID3))
    }
  }

  // Query Graphics State (active or inactive).

  Method(_DGS,0)
  {
    Return(NDDS(DID3))
  }

  // Device Set State. (See table above.)

  Method(_DSS,1)
  {
    If(LEqual(And(Arg0,0xC0000000),0xC0000000))
    {
      // State change was performed by the
      // Video Drivers.  Simply update the
      // New State.

      Store(NSTE,CSTE)
    }
  }
}

Device(DD04)
{

  // Return Unique ID.

  Method(_ADR,0,Serialized)
  {
    If(LEqual(And(0x0F00,DID4),0x400))
    {
      Store(0x4, EDPV)
      Store(DID4, DIDX)
      Return(4)
    }
    If(LEqual(DID4,0))
    {
      Return(4)
    }
    Else
    {
      Return(And(0xFFFF,DID4))
    }
  }

  // Return the Current Status.

  Method(_DCS,0)
  {
    If(LEqual(DID4,0))
    {
      Return(0x0B)
    }
    Else
    {
      Return(CDDS(DID4))
    }
  }

  // Query Graphics State (active or inactive).

  Method(_DGS,0)
  {
    Return(NDDS(DID4))
  }

  // Device Set State. (See table above.)

  Method(_DSS,1)
  {
    If(LEqual(And(Arg0,0xC0000000),0xC0000000))
    {

      // State change was performed by the
      // Video Drivers.  Simply update the
      // New State.

      Store(NSTE,CSTE)
    }
  }
}


Device(DD05)
{

  // Return Unique ID.

  Method(_ADR,0,Serialized)
  {
    If(LEqual(And(0x0F00,DID5),0x400))
    {
      Store(0x5, EDPV)
      Store(DID5, DIDX)
      Return(5)
    }
    If(LEqual(DID5,0))
    {
      Return(5)
    }
    Else
    {
      Return(And(0xFFFF,DID5))
    }
  }

  // Return the Current Status.

  Method(_DCS,0)
  {
    If(LEqual(DID5,0))
    {
      Return(0x0B)
    }
    Else
    {
      Return(CDDS(DID5))
    }
  }

  // Query Graphics State (active or inactive).

  Method(_DGS,0)
  {
    Return(NDDS(DID5))
  }

  // Device Set State. (See table above.)

  Method(_DSS,1)
  {
    If(LEqual(And(Arg0,0xC0000000),0xC0000000))
    {
      // State change was performed by the
      // Video Drivers.  Simply update the
      // New State.

      Store(NSTE,CSTE)
    }
  }
}


Device(DD06)
{

  // Return Unique ID.

  Method(_ADR,0,Serialized)
  {
    If(LEqual(And(0x0F00,DID6),0x400))
    {
      Store(0x6, EDPV)
      Store(DID6, DIDX)
      Return(6)
    }
    If(LEqual(DID6,0))
    {
      Return(6)
    }
    Else
    {
      Return(And(0xFFFF,DID6))
    }
  }

  // Return the Current Status.

  Method(_DCS,0)
  {
    If(LEqual(DID6,0))
    {
      Return(0x0B)
    }
    Else
    {
      Return(CDDS(DID6))
    }
  }

  // Query Graphics State (active or inactive).

  Method(_DGS,0)
  {
    Return(NDDS(DID6))
  }

  // Device Set State. (See table above.)

  Method(_DSS,1)
  {
    If(LEqual(And(Arg0,0xC0000000),0xC0000000))
    {
      // State change was performed by the
      // Video Drivers.  Simply update the
      // New State.

      Store(NSTE,CSTE)
    }
  }
}


Device(DD07)
{

  // Return Unique ID.

  Method(_ADR,0,Serialized)
  {
    If(LEqual(And(0x0F00,DID7),0x400))
    {
      Store(0x7, EDPV)
      Store(DID7, DIDX)
      Return(7)
    }
    If(LEqual(DID7,0))
    {
      Return(7)
    }
    Else
    {
      Return(And(0xFFFF,DID7))
    }
  }

  // Return the Current Status.

  Method(_DCS,0)
  {
    If(LEqual(DID7,0))
    {
      Return(0x0B)
    }
    Else
    {
      Return(CDDS(DID7))
    }
  }

  // Query Graphics State (active or inactive).

  Method(_DGS,0)
  {
    Return(NDDS(DID7))
  }

  // Device Set State. (See table above.)

  Method(_DSS,1)
  {
    If(LEqual(And(Arg0,0xC0000000),0xC0000000))
    {
      // State change was performed by the
      // Video Drivers.  Simply update the
      // New State.

      Store(NSTE,CSTE)
    }
  }
}


Device(DD08)
{

  // Return Unique ID.

  Method(_ADR,0,Serialized)
  {
    If(LEqual(And(0x0F00,DID8),0x400))
    {
      Store(0x8, EDPV)
      Store(DID8, DIDX)
      Return(8)
    }
    If(LEqual(DID8,0))
    {
      Return(8)
    }
    Else
    {
      Return(And(0xFFFF,DID8))
    }
  }

  // Return the Current Status.

  Method(_DCS,0)
  {
    If(LEqual(DID8,0))
    {
      Return(0x0B)
    }
    Else
    {
      Return(CDDS(DID8))
    }
  }

  // Query Graphics State (active or inactive).

  Method(_DGS,0)
  {
    Return(NDDS(DID8))
  }

  // Device Set State. (See table above.)

  Method(_DSS,1)
  {
    If(LEqual(And(Arg0,0xC0000000),0xC0000000))
    {
      // State change was performed by the
      // Video Drivers.  Simply update the
      // New State.

      Store(NSTE,CSTE)
    }
  }
}

//device for eDP
Device(DD1F)
{
  // Return Unique ID.

  Method(_ADR,0,Serialized)
  {
    If(LEqual(EDPV, 0x0))
    {
      Return(0x1F)
    }
    Else
    {
      Return(And(0xFFFF,DIDX))
    }
  }

  // Return the Current Status.

  Method(_DCS,0)
  {
    If(LEqual(EDPV, 0x0))
    {
      Return(0x00)
    }
    Else
    {
      Return(CDDS(DIDX))
    }
  }

  // Query Graphics State (active or inactive).

  Method(_DGS,0)
  {
    Return(NDDS(DIDX))
  }

  // Device Set State. (See table above.)

  Method(_DSS,1)
  {
    If(LEqual(And(Arg0,0xC0000000),0xC0000000))
    {
      // State change was performed by the
      // Video Drivers.  Simply update the
      // New State.
      Store(NSTE,CSTE)
    }
  }
  // Query List of Brightness Control Levels Supported.

  Method(_BCL,0)
  {
    // List of supported brightness levels in the following sequence.

    // Level when machine has full power.
    // Level when machine is on batteries.
    // Other supported levels.
    Return(Package() {80, 50, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100})
  }

  // Set the Brightness Level.

  Method (_BCM,1)
  {
    // Set the requested level if it is between 0 and 100%.

    If(LAnd(LGreaterEqual(Arg0,0),LLessEqual(Arg0,100)))
    {
      \_SB.PCI0.GFX0.AINT(1, Arg0)
      Store(Arg0,BRTL)  // Store Brightness Level.
    }
  }

  // Brightness Query Current level.

  Method (_BQC,0)
  {
    Return(BRTL)
  }
}

Method(SDDL,1)
{
  Increment(NDID)
  Store(And(Arg0,0xF0F),Local0)
  Or(0x80000000,Local0, Local1)
  If(LEqual(DIDL,Local0))
  {
    Return(Local1)
  }
  If(LEqual(DDL2,Local0))
  {
    Return(Local1)
  }
  If(LEqual(DDL3,Local0))
  {
    Return(Local1)
  }
  If(LEqual(DDL4,Local0))
  {
    Return(Local1)
  }
  If(LEqual(DDL5,Local0))
  {
    Return(Local1)
  }
  If(LEqual(DDL6,Local0))
  {
    Return(Local1)
  }
  If(LEqual(DDL7,Local0))
  {
    Return(Local1)
  }
  If(LEqual(DDL8,Local0))
  {
    Return(Local1)
  }
  Return(0)
}

Method(CDDS,1)
{
  Store(And(Arg0,0xF0F),Local0)

  If(LEqual(0, Local0))
  {
    Return(0x1D)
  }
  If(LEqual(CADL, Local0))
  {
    Return(0x1F)
  }
  If(LEqual(CAL2, Local0))
  {
    Return(0x1F)
  }
  If(LEqual(CAL3, Local0))
  {
    Return(0x1F)
  }
  If(LEqual(CAL4, Local0))
  {
    Return(0x1F)
  }
  If(LEqual(CAL5, Local0))
  {
    Return(0x1F)
  }
  If(LEqual(CAL6, Local0))
  {
    Return(0x1F)
  }
  If(LEqual(CAL7, Local0))
  {
    Return(0x1F)
  }
  If(LEqual(CAL8, Local0))
  {
    Return(0x1F)
  }
  Return(0x1D)
}

Method(NDDS,1)
{
  Store(And(Arg0,0xF0F),Local0)

  If(LEqual(0, Local0))
  {
    Return(0)
  }
  If(LEqual(NADL, Local0))
  {
    Return(1)
  }
  If(LEqual(NDL2, Local0))
  {
    Return(1)
  }
  If(LEqual(NDL3, Local0))
  {
    Return(1)
  }
  If(LEqual(NDL4, Local0))
  {
    Return(1)
  }
  If(LEqual(NDL5, Local0))
  {
    Return(1)
  }
  If(LEqual(NDL6, Local0))
  {
    Return(1)
  }
  If(LEqual(NDL7, Local0))
  {
    Return(1)
  }
  If(LEqual(NDL8, Local0))
  {
    Return(1)
  }
  Return(0)
}

//
// Include IGD OpRegion/Software SCI interrupt handler which is use by
// the graphics drivers to request data from system BIOS.
//
include("IgdOpRn.ASL")
