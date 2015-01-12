/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



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
