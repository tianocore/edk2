/***************************************************************************************;
;*                                                                                     *;
;*                                                                                     *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail                         *;
;*    Family of Customer Reference Boards.                                             *;
;*                                                                                     *;
;*                                                                                     *;
;*    Copyright (c)  2011  - 2014, Intel Corporation. All rights reserved              *;
;*                                                                                     *;
;*   ThSPDX-License-Identifier: BSD-2-Clause-Patent
;*                                                                                     *;
;*                                                                                     *;
;*                                                                                     *;
;***************************************************************************************/

OperationRegion(PWKE,PCI_Config,0x62,0x04)

Field(PWKE,DWordAcc,NoLock,Preserve)
{
  , 1,
  PWUC, 8 // Port Wake Up Capability Mask
}

Method(_PSW,1)
{
  If(Arg0)
  {
    Store(Ones,PWUC)
  }
  Else
  {
    Store(0,PWUC)
  }
}

// Leaves the USB ports on in S3/S4 to allow
// the ability to Wake from USB.  Therefore, define
// the below control methods to state D2 entry during
// the given S-State.

Method(_S3D,0)
{
  Return(2)
}

Method(_S4D,0)
{
  Return(2)
}

Device(HUBN)
{
  Name(_ADR, Zero)
  Device(PR01)
  {
    Name(_ADR, One)

    //
    // There will have "Generic USB Hub" existed at Port 1 of each EHCI controller
    // in Windows "Device Manager" while RMH is enabled, so need to add _UPC
    // and _PLD to report OS that it's not user visible to pass WHQL: Single Computer
    // Display Object test in Win7
    //
    Name(_UPC, Package()
    {
      0xFF,                       // Port is connectable
      0x00,                       // Connector type - Type "A"
      0x00000000,                 // Reserved 0 - must be zero
      0x00000000
    })                // Reserved 1 - must be zero

    Name(_PLD, Package()
    {
      Buffer (0x10)
      {
        0x81, 0x00, 0x00, 0x00,     // Revision 1, Ignore color
        0x00, 0x00, 0x00, 0x00,
        0x30, 0x1C, 0x00, 0x00,     // Panel Unknown, Shape Unknown
        0x00, 0x00, 0x00, 0x00
      }
    })

    Device(PR11)
    {
      Name(_ADR, One)
      Name(_UPC, Package()
      {
        0xFF,                       // Port is connectable
        0xFF,                       // Proprietary connector
        0x00000000,                 // Reserved 0 - must be zero
        0x00000000
      })                // Reserved 1 - must be zero
      Name(_PLD, Package()
      {
        Buffer (0x10)
        {
          0x81, 0x00, 0x00, 0x00,     // Revision 1, Ignore color
          0x00, 0x00, 0x00, 0x00,
          0xE1, 0x1C, 0x00, 0x00,     // Front Panel, Vertical Upper, Horz. Left, Shape Unknown
          0x00, 0x00, 0x00, 0x00
        }
      })
    }

    Device(PR12)
    {
      Name(_ADR, 0x02)
      Name(_UPC, Package()
      {
        0xFF,                       // Port is connectable
        0xFF,                       // Proprietary connector
        0x00000000,                 // Reserved 0 - must be zero
        0x00000000
      })                // Reserved 1 - must be zero
      Name(_PLD, Package()
      {
        Buffer (0x10)
        {
          0x81, 0x00, 0x00, 0x00,     // Revision 1, Ignore color
          0x00, 0x00, 0x00, 0x00,
          0xE1, 0x1D, 0x00, 0x00,     // Front Panel, Vertical Center, Horz. Left, Shape Unknown
          0x00, 0x00, 0x00, 0x00
        }
      })
    }

    Device(PR13)
    {
      Name(_ADR, 0x03)
      Name(_UPC, Package()
      {
        0xFF,                       // Port is connectable
        0xFF,                       // Proprietary connector
        0x00000000,                 // Reserved 0 - must be zero
        0x00000000
      })                // Reserved 1 - must be zero
      Name(_PLD, Package()
      {
        Buffer (0x10)
        {
          0x81, 0x00, 0x00, 0x00,     // Revision 1, Ignore color
          0x00, 0x00, 0x00, 0x00,
          0xE1, 0x1D, 0x00, 0x00,     // Front Panel, Vertical Center, Horz. Left, Shape Unknown
          0x00, 0x00, 0x00, 0x00
        }
      })
    }

    Device(PR14)
    {
      Name(_ADR, 0x04)
      Name(_UPC, Package()
      {
        0xFF,                       // Port is connectable
        0xFF,                       // Proprietary connector
        0x00000000,                 // Reserved 0 - must be zero
        0x00000000
      })                // Reserved 1 - must be zero

      Name(_PLD, Package()
      {
        Buffer (0x10)
        {
          0x81, 0x00, 0x00, 0x00,     // Revision 1, Ignore color
          0x00, 0x00, 0x00, 0x00,
          0xE1, 0x1E, 0x00, 0x00,     // Front Panel, Vertical Lower, Horz. Left, Shape Unknown
          0x00, 0x00, 0x00, 0x00
        }
      })

      // copy USB Sideband Deferring GPE Vector (HOST_ALERT#1) to DSM method
      Include("UsbSbd.asl")
    }

    Device(PR15)
    {
      Name(_ADR, 0x05)
      Name(_UPC, Package()
      {
        0xFF,                       // Port is connectable
        0xFF,                       // Proprietary connector
        0x00000000,                 // Reserved 0 - must be zero
        0x00000000
      })                // Reserved 1 - must be zero
      Name(_PLD, Package()
      {
        Buffer (0x10)
        {
          0x81, 0x00, 0x00, 0x00,     // Revision 1, Ignore color
          0x00, 0x00, 0x00, 0x00,
          0xB1, 0x1E, 0x00, 0x00,     // Panel Unknown, Shape Unknown
          0x00, 0x00, 0x00, 0x00
        }
      })
      // copy USB Sideband Deferring GPE Vector (HOST_ALERT#2) to DSM method
      Include("UsbSbd.asl")
    }

    Device(PR16)
    {
      Name(_ADR, 0x06)
      Name(_UPC, Package()
      {
        0xFF,                       // Port is connectable
        0xFF,                       // Proprietary connector
        0x00000000,                 // Reserved 0 - must be zero
        0x00000000
      })                // Reserved 1 - must be zero
      Name(_PLD, Package()
      {
        Buffer (0x10)
        {
          0x81, 0x00, 0x00, 0x00,     // Revision 1, Ignore color
          0x00, 0x00, 0x00, 0x00,
          0xB1, 0x1E, 0x00, 0x00,     // Panel Unknown, Shape Unknown
          0x00, 0x00, 0x00, 0x00
        }
      })
      // copy USB Sideband Deferring GPE Vector (HOST_ALERT#1) to DSM method
      Include("UsbSbd.asl")
    }

    Device(PR17)
    {
      Name(_ADR, 0x07)
      Name(_UPC, Package()
      {
        0xFF,                       // Port is connectable
        0xFF,                       // Proprietary connector
        0x00000000,                 // Reserved 0 - must be zero
        0x00000000
      })                // Reserved 1 - must be zero
      Name(_PLD, Package()
      {
        Buffer (0x10)
        {
          0x81, 0x00, 0x00, 0x00,     // Revision 1, Ignore color
          0x00, 0x00, 0x00, 0x00,
          0xB1, 0x1E, 0x00, 0x00,     // Panel Unknown, Shape Unknown
          0x00, 0x00, 0x00, 0x00
        }
      })
      // copy USB Sideband Deferring GPE Vector (HOST_ALERT#2) to DSM method
      Include("UsbSbd.asl")
    }

    Device(PR18)
    {
      Name(_ADR, 0x08)
      Name(_UPC, Package()
      {
        0xFF,                       // Port is connectable
        0xFF,                       // Proprietary connector
        0x00000000,                 // Reserved 0 - must be zero
        0x00000000
      })                // Reserved 1 - must be zero
      Name(_PLD, Package()
      {
        Buffer (0x10)
        {
          0x81, 0x00, 0x00, 0x00,     // Revision 1, Ignore color
          0x00, 0x00, 0x00, 0x00,
          0xB1, 0x1E, 0x00, 0x00,     // Panel Unknown, Shape Unknown
          0x00, 0x00, 0x00, 0x00
        }
      })
    }
  } // End of PR01
} // End of HUBN
