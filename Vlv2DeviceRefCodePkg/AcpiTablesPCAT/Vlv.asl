/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



Module Name:

  VLV.ASL

Abstract:

  Baytrail PCI configuration space definition.

--*/
Scope (\_SB.PCI0)
{

  Device(GFX0)   // Mobile I.G.D
  {
    Name(_ADR, 0x00020000)

    Method(GDEP, 0)
    {
      If(LEqual(OSYS,2013))
      {
        Name(_DEP, Package(0x1)
        {
          PEPD
        })
      }
    }

    include("INTELGFX.ASL")
    include("INTELISPDev2.ASL")
  } // end "Mobile I.G.D"
}//end scope
