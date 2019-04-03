/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved    *;
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/
Device (PEPD)
{
  Name (_HID, "INT3396")
  Name(_CID, 0x800dd041)
  Name (_UID, 0x1)

  // Indicates if the platform PEP has loaded
  Name(PEPP, Zero)

  // Devices score-boarded by the PEP, Rev0 format
  Name (DEVS, Package() {0})

  // Devices score-boarded by the PEP, Rev1 format
  Name(DEVX, Package()
  {
    Package () {"\\_SB.PCI0.XHC1", 0x1},
    Package () {"\\_SB.PCI0.EHC1", 0x1},
    Package () {"\\_SB.PCI0.GFX0", 0x1},
    Package () {"\\_SB.PCI0.GFX0.ISP0", 0x1},
    Package () {"\\_SB.PCI0.SEC0", 0x1},
    Package () {"\\_SB.I2C1", 0x1},
    Package () {"\\_SB.I2C2", 0x1},
    Package () {"\\_SB.I2C3", 0x1},
    Package () {"\\_SB.I2C4", 0x1},
    Package () {"\\_SB.I2C5", 0x1},
    Package () {"\\_SB.I2C6", 0x1},
    Package () {"\\_SB.I2C7", 0x1},
    Package () {"\\_SB.SDHA", 0x1},
    Package () {"\\_SB.SDHB", 0x1},
    Package () {"\\_SB.SDHC", 0x1},
    Package () {"\\_SB.SPI1", 0x1},
    Package () {"\\_SB.URT1", 0x1},
    Package () {"\\_SB.URT2", 0x1},
  })
  // Crashdump device package
  Name(CDMP, Package(2) {})
  // Device dependency for uPEP
  Name(DEVY, Package()
  {
    Package() {"\\_PR.CPU0", 0x1, Package() {Package() {0xFF, 0}}},
    Package() {"\\_PR.CPU1", 0x1, Package() {Package() {0xFF, 0}}},
    Package() {"\\_PR.CPU2", 0x1, Package() {Package() {0xFF, 0}}},
    Package() {"\\_PR.CPU3", 0x1, Package() {Package() {0xFF, 0}}},
    Package() {"\\_SB.I2C1", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.I2C2", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.I2C3", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.I2C4", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.I2C5", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.I2C6", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.I2C7", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.PCI0.GFX0", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.PCI0.SEC0", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.PCI0.XHC1", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.PCI0.GFX0.ISP0", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.LPEA", 0x1, Package() {Package() {0x0,3}, Package() {0x1,0}, Package() {0x2,3}, Package() {0x3,3}}},
    Package() {"\\_SB.SDHA", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.SDHB", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.SDHC", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.SPI1", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.URT1", 0x1, Package() {Package() {0xFF,3}}},
    Package() {"\\_SB.URT2", 0x1, Package() {Package() {0xFF,3}}}
  })
  // BCCD crashdump information
  Name(BCCD, Package()
  {
    Package()
    {
      "\\_SB.SDHA",
      Package()
      {
        Package() { Package() {0, 32, 0,  3, 0xFFFFFFFFFFFFFFFF}, Package() {0xFFFFFFFC, 0x0, 0x4}, 0}
      }
    }
  })

  Method(_STA, 0x0, NotSerialized)
  {
    Return(0xf)
  }

  Method(_DSM, 0x4, Serialized)
  {
    If(LEqual(Arg0,ToUUID("B8FEBFE0-BAF8-454b-AECD-49FB91137B21")))
    {

      // Number of fn IDs supported
      If(LEqual(Arg2, Zero))
      {
        Return(Buffer(One)
        {
          0xf
        })
      }

      // Pep presence
      If(LEqual(Arg2, One))
      {
        Store(0x1, PEPP)
        Return(0xf)
      }

      // Mitigation devices
      If(LEqual(Arg2, 0x2))
      {
        If(LEqual(Arg1, 0x0))
        {
          // Rev0
          Return(DEVS)
        }
        If(LEqual(Arg1, 0x1))
        {
          // Rev1
          Return(DEVX)
        }
      }

      // Crashdump device data
      If(LEqual(Arg2, 0x3))
      {
        Store("\\_SB.SDHA", Index(CDMP,0))
        Store(EM1A, Index(CDMP,1))
        Return(CDMP)
      }
    }
    // New UUID for built-in uPEP
    If(LEqual(Arg0,ToUUID("C4EB40A0-6CD2-11E2-BCFD-0800200C9A66")))
    {

      // Number of fn IDs supported
      If(LEqual(Arg2, Zero))
      {
        Return(Buffer(One)
        {
          0x7
        })
      }
      // LPI device dependencies
      If(LEqual(Arg2, 0x1))
      {
        Return(DEVY)
      }
      // Crashdump device data
      If(LEqual(Arg2, 0x2))
      {
        Store(EM1A, Local0)
        Add(Local0, 0x84, Local0)
        Store(Local0, Index(DerefOf(Index(DerefOf(Index(DerefOf(Index(DerefOf(Index(BCCD, Zero, )), One, )), Zero, )), Zero, )), 0x4, ))
        Return(BCCD)
      }
    }

    Return(One)
  }
}

//
// eMMC 4.41
//
Device(SDHA)
{
  Name (_ADR, 0)
  Name (_HID, "80860F14")
  Name (_CID, "PNP0D40")
  Name (_DDN, "Intel(R) eMMC Controller - 80860F14")
  Name (_UID, 1)
  Name(_DEP, Package(0x1)
  {
    PEPD
  })

  Name (RBF1, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {45}  // eMMC 4.41 IRQ
  })

  Method (_CRS, 0x0, NotSerialized)
  {
    // Update the Base address for BAR0 of eMMC 4.41
    CreateDwordField(^RBF1, ^BAR0._BAS, B0B1)
    CreateDwordField(^RBF1, ^BAR0._LEN, B0L1)
    Store(eM0A, B0B1)
    Store(eM0L, B0L1)
    Return (RBF1)
  }
  Method (_STA, 0x0, NotSerialized)
  {
    //
    // PCIM>> 0:ACPI mode            1:PCI mode
    // SD1D>> 0:eMMC 4.41 enable     1:eMMC 4.41 disable
    //
    If (LAnd(LEqual(PCIM, 0), LEqual(SD1D, 0)))
    {
      Return (0xF)
    }
    Else
    {
      Return (0x0)
    }
  }


  Method (_PS3, 0, NotSerialized)
  {
    OR(PSAT, 0x00000003, PSAT)
    OR(PSAT, 0X00000000, PSAT)
    //
    // If not B1, still keep 2 ms w/a
    //
    If(LLess(SOCS, 0x03))
    {
      Sleep(2)
    }
  }
  Method (_PS0, 0, NotSerialized)
  {
    And(PSAT, 0xfffffffC, PSAT)
    OR(PSAT, 0X00000000, PSAT)
    //
    // If not B1, still keep 2 ms w/a
    //
    If(LLess(SOCS, 0x03))
    {
      Sleep(2)
    }
  }

  OperationRegion (KEYS, SystemMemory, eM1A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
           PSAT,  32
  }

         Method (_DIS, 0x0, NotSerialized)
  {
    //Adding dummy disable methods for device EMM0
  }

  Device (EMMD)
  {
    Name (_ADR, 0x00000008) // Slot 0, Function 8
    Method (_RMV, 0, NotSerialized)
    {
      Return (0x0)
    }
  }
}


//
// eMMC 4.5
//
Device(SDHD)
{
  Name (_ADR, 0)
  Name (_HID, "80860F14")
  Name (_CID, "PNP0D40")
  Name (_DDN, "Intel(R) eMMC Controller - 80860F14")
  Name (_UID, 1)
  Name(_DEP, Package(0x1)
  {
    PEPD
  })

  Name (RBF1, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {44}  // eMMC 4.5 IRQ
  })
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBF1, ^BAR0._BAS, B0B1)
    CreateDwordField(^RBF1, ^BAR0._LEN, B0L1)
    Store(eM0A, B0B1)
    Store(eM0L, B0L1)
    Return (RBF1)
  }
  Method (_STA, 0x0, NotSerialized)
  {
    //
    // PCIM>> 0:ACPI mode           1:PCI mode
    // HSID>> 0:eMMC 4.5 enable     1:eMMC 4.5 disable
    //
    If (LAnd(LEqual(PCIM, 0), LEqual(HSID, 0)))
    {
      Return (0xF)
    }
    Else
    {
      Return (0x0)
    }
  }


  Method (_PS3, 0, NotSerialized)
  {
    OR(PSAT, 0x00000003, PSAT)
    OR(PSAT, 0X00000000, PSAT)
    //
    // If not B1, still keep 2 ms w/a
    //
    If(LLess(SOCS, 0x03))
    {
      Sleep(2)
    }
  }
  Method (_PS0, 0, NotSerialized)
  {
    And(PSAT, 0xfffffffC, PSAT)
    OR(PSAT, 0X00000000, PSAT)
    //
    // If not B1, still keep 2 ms w/a
    //
    If(LLess(SOCS, 0x03))
    {
      Sleep(2)
    }
  }

  OperationRegion (KEYS, SystemMemory, eM1A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
           PSAT,  32
  }

         Method (_DIS, 0x0, NotSerialized)
  {
    //Adding dummy disable methods for device EMM0
  }

  Device (EM45)
  {
    Name (_ADR, 0x00000008) // Slot 0, Function 8
    Method (_RMV, 0, NotSerialized)
    {
      Return (0x0)
    }
  }
}


//
// SDIO
//
Device(SDHB)
{
  Name (_ADR, 0)
  Name (_HID, "INT33BB")
  Name (_CID, "PNP0D40")
  Name (_DDN, "Intel(R) SDIO Controller - 80860F15")
  Name (_UID, 2)
  Name (_HRV, 2)
  Name(_DEP, Package(0x01)
  {
    PEPD
  })
  Name (PSTS, 0x0)

  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {46}  // SDIO IRQ
  })

  Method (_CRS, 0x0, NotSerialized)
  {

    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(SI0A, B0BA)
    Store(SI0L, B0LN)
    Return (RBUF)
  }
  Method (_STA, 0x0, NotSerialized)
  {
    If (LLessEqual(STEP, 0x04))
    {
      //A stepping
      Store(SDMD, _HRV)
    }

    //
    // PCIM>> 0:ACPI mode           1:PCI mode
    //
    If (LEqual(PCIM, 1)) {
      Return (0x0)
    }

    If (LOr(LEqual(SI0A, 0), LEqual(SD2D, 1)))
    {
      Return (0x0)
    }
    Return (0xF)
  }
  Method (_DIS, 0x0, NotSerialized)
  {
    //Adding dummy disable methods for device EMM0
  }

  Method (_PS3, 0, NotSerialized)
  {
    OR(PSAT, 0x00000003, PSAT)
    OR(PSAT, 0X00000000, PSAT)
  }
  Method (_PS0, 0, NotSerialized)
  {
    And(PSAT, 0xfffffffC, PSAT)
    OR(PSAT, 0X00000000, PSAT)

    if(LEqual(\_SB.SDHB.PSTS,0x0))
    {
      if(LEqual (\_SB.GPO2.AVBL, 1))
      {
        Store( 0x01, \_SB.GPO2.WFD3 ) // WL_WIFI_REQ_ON = 1 put the device to normal state
        Store( 0x01, \_SB.SDHB.PSTS)  // indicates that the device powered ON
      }
    }


  }
  OperationRegion (KEYS, SystemMemory, SI1A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
           PSAT,   32
  }


         Device (BRCM)
  {
    Name (_ADR, 0x01)                 //SlotNumber + Function
    Name (_DEP, Package() {\_SB.GPO2})

    Method (_RMV, 0, NotSerialized)
    {
      Return (0x0)
    }
    Name (_PRW, Package() {0, 0})
    Name (_S4W, 2)

    Method (_CRS, 0, Serialized)
    {
      Name (RBUF, ResourceTemplate ()
      {
        Interrupt (ResourceConsumer, Edge, ActiveHigh, ExclusiveAndWake, , , ) {73}
      })
      Return (RBUF)
    }

    Method (_PS3, 0, NotSerialized)
    {
      if(LEqual (\_SB.GPO2.AVBL, 1))
      {
        Store( 0x00, \_SB.GPO2.WFD3 ) // WL_WIFI_REQ_ON = 0 puts the device in reset state
        Store( 0x00, \_SB.SDHB.PSTS) //Indicates that the device is powered off
      }

    }
    Method (_PS0, 0, NotSerialized)
    {
      if(LEqual(\_SB.SDHB.PSTS,0x0))
      {
        if(LEqual (\_SB.GPO2.AVBL, 1))
        {
          Store( 0x01, \_SB.GPO2.WFD3 ) // WL_WIFI_REQ_ON = 1 put the device to normal state
          Store( 0x01, \_SB.SDHB.PSTS)     // indicates that the device powered ON
        }
      }
    }
  } // Device (BRCM)
  //
  // Secondary Broadcom WIFI function
  //
  Device(BRC2)
  {
    Name(_ADR, 0x2) // function 2
    Name(_STA, 0xf)
    //
    // The device is not removable. This must be a method.
    //
    Method(_RMV, 0x0, NotSerialized)
    {
      Return(0x0)
    }

    //
    // Describe a vendor-defined connection between this device and the
    // primary wifi device
    //

    Method(_CRS)
    {
      Name(NAM, Buffer() {"\\_SB.SDHB.BRCM"})
      Name(SPB, Buffer()
      {
        0x8E,       // SPB Descriptor
        0x18, 0x00, // Length including NAM above
        0x01,       // +0x00 SPB Descriptor Revision
        0x00,       // +0x01 Resource Source Index
        0xc0,       // +0x02 Bus type - vendor defined
        0x02,       // +0x03 Consumer + controller initiated
        0x00, 0x00, // +0x04 Type specific flags
        0x01,       // +0x06 Type specific revision
        0x00, 0x00  // +0x07 type specific data length
        // +0x09 - 0xf bytes for NULL-terminated NAM
        // Length = 0x18
      })

      Name(END, Buffer() {0x79, 0x00})
      Concatenate(SPB, NAM, Local0)
      Concatenate(Local0, END, Local1)
      Return(Local1)
    }
  }

}

//
// SD Card
//
Device(SDHC)
{
  Name (_ADR, 0)
  Name (_HID, "80860F16")
  Name (_CID, "PNP0D40")
  Name (_DDN, "Intel(R) SD Card Controller - 80860F16")
  Name (_UID, 3)
  Name(_DEP, Package(0x01)
  {
    PEPD
  })
  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {47}  // SD Card IRQ
  })
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(SD0A, B0BA)
    Store(SD0L, B0LN)
    Return (RBUF)
  }
  Method (_STA, 0x0, NotSerialized)
  {
    //
    // PCIM>> 0:ACPI mode           1:PCI mode
    //
    If (LEqual(PCIM, 1)) {
      Return (0x0)
    }

    If (LOr(LEqual(SD0A, 0), LEqual(SD3D, 1)))
    {
      Return (0x0)
    }
    Return (0xF)
  }

  Method (_PS3, 0, NotSerialized)
  {
    OR(PSAT, 0x00000003, PSAT)
    OR(PSAT, 0X00000000, PSAT)
  }
  Method (_PS0, 0, NotSerialized)
  {
    And(PSAT, 0xfffffffC, PSAT)
    OR(PSAT, 0X00000000, PSAT)
  }
  OperationRegion (KEYS, SystemMemory, SD1A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
           PSAT,   32
  }

  Device (CARD)
  {
    Name (_ADR, 0x00000008)
    Method(_RMV, 0x0, NotSerialized)
    {
      // SDRM = 0 non-removable;
      If (LEqual(SDRM, 0))
      {
        Return (0)
      }

      Return (1)
    }
  }

}

