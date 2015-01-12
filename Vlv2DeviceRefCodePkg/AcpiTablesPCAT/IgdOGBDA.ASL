/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  IgdOGBDA.ASL

Abstract:

  IGD OpRegion/Software SCI Reference Code for the Baytrail Family.
  This file contains Get BIOS Data Area funciton support for
  the Integrated Graphics Device (IGD) OpRegion/Software SCI mechanism.

--*/


Method (GBDA, 0, Serialized)
{

  // Supported calls: Sub-function 0

  If (LEqual(GESF, 0))
  {
    //<TODO> Update implementation specific supported calls.  Reference
    // code is set to Intel's validated implementation.

    Store(0x0000279, PARM)

    Store(Zero, GESF)               // Clear the exit parameter
    Return(SUCC)                    // Success
  }

  // Requested callbacks: Sub-function 1

  If (LEqual(GESF, 1))
  {

    //<TODO> Update implementation specific system BIOS requested call
    // back functions.  Call back functions are where the driver calls the
    // system BIOS at function indicated event.

    Store(0x00000240, PARM)

    Store(Zero, GESF)               // Clear the exit parameter
    Return(SUCC)                    // Success
  }

  // Get Boot display Preferences: Sub-function 4

  If (LEqual(GESF, 4))
  {

    //<TODO> Update the implementation specific Get Boot Display
    // Preferences function.

    And(PARM, 0xEFFF0000, PARM)     // PARM[30:16] = Boot device ports
    And(PARM, ShiftLeft(DeRefOf(Index(DBTB, IBTT)), 16), PARM)
    Or(IBTT, PARM, PARM)            // PARM[7:0] = Boot device type

    Store(Zero, GESF)               // Clear the exit parameter
    Return(SUCC)                    // Success
  }

  // Panel details: Sub-function 5

  If (LEqual(GESF, 5))
  {

    //<TODO> Update the implementation specific Get Panel Details
    // function.

    Store(IPSC, PARM)               // Report the scaling setting
    Or(PARM, ShiftLeft(IPAT, 8), PARM)
    Add(PARM, 0x100, PARM)          // Adjust panel type, 0 = VBT default
    Or(PARM, ShiftLeft(LIDS, 16), PARM) // Report the lid state
    Add(PARM, 0x10000, PARM)        // Adjust the lid state, 0 = Unknown
    Or(PARM, ShiftLeft(IBLC, 18), PARM) // Report the BLC setting
    Or(PARM, ShiftLeft(IBIA, 20), PARM) // Report the BIA setting
    Store(Zero, GESF)
    Return(SUCC)
  }

  // TV-standard/Video-connector: Sub-function 6

  If (LEqual(GESF, 6))
  {

    //<TODO> Update the implementation specific Get
    // TV-standard/Video-connectorPanel function.

    Store(ITVF, PARM)
    Or(PARM, ShiftLeft(ITVM, 4), PARM)
    Store(Zero, GESF)
    Return(SUCC)
  }

  // Internal graphics: Sub-function 7

  If (LEqual(GESF, 7))
  {
    Store(GIVD, PARM)               // PARM[0]      - VGA mode(1=VGA)
    Xor(PARM, 1, PARM)              // Invert the VGA mode polarity
    Or(PARM, ShiftLeft(GMFN, 1), PARM) // PARM[1]   - # IGD PCI functions-1
    // PARM[3:2]    - Reserved
    // PARM[4]      - IGD D3 support(0=cold)
    // PARM[10:5]   - Reserved
    Or(PARM, ShiftLeft(3, 11), PARM) // PARM[12:11] - DVMT mode(11b = 5.0)

    //
    // Report DVMT 5.0 Total Graphics memory size.
    //
    Or(PARM, ShiftLeft(IDMS, 17), PARM)   // Bits 20:17 are for Gfx total memory size

    // If the "Set Internal Graphics" call is supported, the modified
    // settings flag must be programmed per the specification.  This means
    // that the flag must be set to indicate that system BIOS requests
    // these settings.  Once "Set Internal Graphics" is called, the
    //  modified settings flag must be cleared on all subsequent calls to
    // this function.

    // Report the graphics frequency based on DISPLAY_CLOCK_FREQUENCY_ENCODING [MMADR+0x20C8]

    Or(ShiftLeft(Derefof(Index(CDCT, \_SB.PCI0.GFX0.MCHK.DCFE)), 21),PARM, PARM)

    Store(1, GESF)                  // Set the modified settings flag
    Return(SUCC)
  }

  // Spread spectrum clocks: Sub-function 10

  If (LEqual(GESF, 10))
  {

    Store(0, PARM)                  // Assume SSC is disabled

    If(ISSC)
    {
      Or(PARM, 3, PARM)       // If SSC enabled, return SSC1+Enabled
    }

    Store(0, GESF)                  // Set the modified settings flag
    Return(SUCC)                    // Success
  }


  // A call to a reserved "Get BIOS data" function was received.

  Store(Zero, GESF)                     // Clear the exit parameter
  Return(CRIT)                          // Reserved, "Critical failure"
}
