/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c) 2012  - 2016, Intel Corporation. All rights reserved    *;
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/
//
// LPIO1 DMA#1 (Synopsis GP DMA)
//
Device (GDM1)
{
  Name (_HID, "INTL9C60")
  Name (_DDN, "Intel(R) DMA Controller #1 - INTL9C60")
  Name (_UID, 1)

  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00004000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {42}  // DMA #1 IRQ
  })
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(D10A, B0BA)
    Store(D10L, B0LN)
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

    If (LOr(LEqual(D10A, 0), LEqual(L10D, 1)))
    {
      Return (0x0)
    }
    Return (0xF)
  }
}

//
// LPIO1 DMA#2 (Synopsis GP DMA)
//
Device (GDM2)
{
  Name (_HID, "INTL9C60")
  Name (_DDN, "Intel(R) DMA Controller #2 - INTL9C60")
  Name (_UID, 2)

  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00004000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {43}  // DMA #2 IRQ
  })
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(D20A, B0BA)
    Store(D20L, B0LN)
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

    If (LOr(LEqual(D20A, 0), LEqual(L20D, 1)))
    {
      Return (0x0)
    }
    Return (0xF)
  }
}

//
// LPIO1 PWM #1
//
Device(PWM1)
{
  Name (_ADR, 0)
  Name (_HID, "80860F09")
  Name (_CID, "80860F09")
  Name (_DDN, "Intel(R) PWM Controller #1 - 80860F08")
  Name (_UID, 1)

  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
  })
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(P10A, B0BA)
    Store(P10L, B0LN)
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

    If (LOr(LEqual(P10A, 0), LEqual(L11D, 1)))
    {
      Return (0x0)
    }
    Return (0xF)
  }
}

//
// LPIO1 PWM #2
//
Device(PWM2)
{
  Name (_ADR, 0)
  Name (_HID, "80860F09")
  Name (_CID, "80860F09")
  Name (_DDN, "Intel(R) PWM Controller #2 - 80860F09")
  Name (_UID, 2)

  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
  })
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(P20A, B0BA)
    Store(P20L, B0LN)
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

    If (LOr(LEqual(P20A, 0), LEqual(L12D, 1)))
    {
      Return (0x0)
    }
    Return (0xF)
  }
}

//
// LPIO1 HS-UART #1
//
Device(URT1)
{
  Name (_ADR, 0)
  Name (_HID, "80860F0A")
  Name (_CID, "80860F0A")
  Name (_DDN, "Intel(R) HS-UART Controller #1 - 80860F0A")
  Name (_UID, 1)
  Name(_DEP, Package(0x1)
  {
    PEPD
  })
  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {39}  // HS-UART #1 IRQ

    FixedDMA(0x2, 0x2, Width32Bit, )
    FixedDMA(0x3, 0x3, Width32Bit, )
  })
  Method (_HRV, 0x0, NotSerialized)
  {
    Return (SOCS)
  }
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(U10A, B0BA)
    Store(U10L, B0LN)
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

    If (LOr(LEqual(U10A, 0), LEqual(L13D, 1)))
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

  OperationRegion (KEYS, SystemMemory, U11A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
    PSAT,   32
  }
}//  Device (URT1)

//
// LPIO1 HS-UART #2
//
Device(URT2)
{
  Name (_ADR, 0)
  Name (_HID, "80860F0A")
  Name (_CID, "80860F0A")
  Name (_DDN, "Intel(R) HS-UART Controller #2 - 80860F0C")
  Name (_UID, 2)

  Name(_DEP, Package(0x1)
  {
    PEPD
  })

  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {40}  // HS-UART #2 IRQ

    FixedDMA(0x4, 0x4, Width32Bit, )
    FixedDMA(0x5, 0x5, Width32Bit, )
  })

  Method (_HRV, 0x0, NotSerialized)
  {
    Return (SOCS)
  }

  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(U20A, B0BA)
    Store(U20L, B0LN)
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

    If (LOr(LEqual(U20A, 0), LEqual(L14D, 1)))
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

  OperationRegion (KEYS, SystemMemory, U21A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
    PSAT,   32
  }
}//  Device (URT2)

//
// LPIO1 SPI
//
Device(SPI1)
{
  Name (_ADR, 0)
  Name (_HID, "80860F0E")
  Name (_CID, "80860F0E")
  Name (_UID, "0")  // Static bus number assignment
  Name(_DEP, Package(0x1)
  {
    PEPD
  })
  Name (_DDN, "Intel(R) SPI Controller - 80860F0E")

  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {41}  // SPI IRQ

    FixedDMA(0x0, 0x0, Width32Bit, )
    FixedDMA(0x1, 0x1, Width32Bit, )
  })

  Method (_HRV, 0x0, NotSerialized)
  {
    Return (SOCS)
  }

  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(SP0A, B0BA)
    Store(SP0L, B0LN)
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

    If (LOr(LEqual(SP0A, 0), LEqual(L15D, 1)))
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

  OperationRegion (KEYS, SystemMemory, SP1A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
           PSAT,   32
  }
}//  Device (SPI1)

//
// LPIO2 I2C #1
//
Device(I2C1)
{
  Name (_ADR, 0)
  Name (_HID, "80860F41")
  Name (_CID, "80860F41")
  Name(_DEP, Package(0x1)
  {
    PEPD
  })
  Name (_DDN, "Intel(R) I2C Controller #1 - 80860F41")
  Name (_UID, 1)

  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {32}  // I2C #1 IRQ

    FixedDMA(0x10, 0x0, Width32Bit, )
    FixedDMA(0x11, 0x1, Width32Bit, )
  })

  Method (SSCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x200, 0x200, 0x06 })
    Return (PKG)
  }
  Method (FMCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x55, 0x99, 0x06 })
    Return (PKG)
  }
  Method (FPCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x1b, 0x3a, 0x06 })
    Return (PKG)
  }

  Method (_HRV, 0x0, NotSerialized)
  {
    Return (SOCS)
  }
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(I10A, B0BA)
    Store(I10L, B0LN)
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

    If (LOr(LEqual(I10A, 0), LEqual(L21D, 1)))
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
  OperationRegion (KEYS, SystemMemory, I11A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
           PSAT,   32
  }

}

//
// LPIO2 I2C #2
//
Device(I2C2)
{
  Name (_ADR, 0)
  Name (_HID, "80860F41")
  Name (_CID, "80860F41")
  Name(_DEP, Package(0x1)
  {
    PEPD
  })
  Name (_DDN, "Intel(R) I2C Controller #2 - 80860F42")
  Name (_UID, 2)

  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {33}  // I2C #2 IRQ

    FixedDMA(0x12, 0x2, Width32Bit, )
    FixedDMA(0x13, 0x3, Width32Bit, )
  })

  Method (SSCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x200, 0x200, 0x06 })
    Return (PKG)
  }
  Method (FMCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x55, 0x99, 0x06 })
    Return (PKG)
  }
  Method (FPCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x1b, 0x3a, 0x06 })
    Return (PKG)
  }

  Method (_HRV, 0x0, NotSerialized)
  {
    Return (SOCS)
  }
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(I20A, B0BA)
    Store(I20L, B0LN)
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

    If (LOr(LEqual(I20A, 0), LEqual(L22D, 1)))
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
  OperationRegion (KEYS, SystemMemory, I21A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
           PSAT,   32
  }


  //
  // Realtek Audio Codec
  //
  Device (RTEK)   //Audio Codec driver I2C
  {
    Name (_ADR, 0)
    Name (_HID, "10EC5640")
    Name (_CID, "10EC5640")
    Name (_DDN, "RTEK Codec Controller " )
    Name (_UID, 1)


    Method(_CRS, 0x0, Serialized)
    {
      Name(SBUF,ResourceTemplate ()
      {
        I2CSerialBus(0x1C,          //SlaveAddress: bus address
                     ,                         //SlaveMode: default to ControllerInitiated
                     400000,                   //ConnectionSpeed: in Hz
                     ,                         //Addressing Mode: default to 7 bit
                     "\\_SB.I2C2",             //ResourceSource: I2C bus controller name
                     ,                         //ResourceSourceIndex: defaults to 0
                     ,                         //ResourceUsage: Defaults to ResourceConsumer
                     ,                         //Descriptor Name: creates name for offset of resource descriptor
                    )  //VendorData
        GpioInt(Edge, ActiveHigh, ExclusiveAndWake, PullNone, 0,"\\_SB.GPO2") {4} //  AUD_INT
      })
      Return (SBUF)
    }

    Method (_STA, 0x0, NotSerialized)
    {

      If (LEqual(LPEE, 2)) { // LPE enable/disable
        If (LEqual(LPAD, 1))
        {
          Return(0xF)
        }
      }
      Return(0)
    }

    Method (_DIS, 0x0, NotSerialized)
    {

    }
  } // Device (RTEK)
} //  Device (I2C2)

//
// LPIO2 I2C #3
//
Device(I2C3)
{
  Name (_ADR, 0)
  Name (_HID, "80860F41")
  Name (_CID, "80860F41")
  Name (_DDN, "Intel(R) I2C Controller #3 - 80860F43")
  Name (_UID, 3)
  Name(_DEP, Package(0x1)
  {
    PEPD
  })
  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {34}  // I2C #3 IRQ

    FixedDMA(0x14, 0x4, Width32Bit, )
    FixedDMA(0x15, 0x5, Width32Bit, )
  })

  Method (SSCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x200, 0x200, 0x06 })
    Return (PKG)
  }
  Method (FMCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x55, 0x99, 0x06 })
    Return (PKG)
  }
  Method (FPCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x1b, 0x3a, 0x06 })
    Return (PKG)
  }

  Method (_HRV, 0x0, NotSerialized)
  {
    Return (SOCS)
  }
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(I30A, B0BA)
    Store(I30L, B0LN)
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

    If (LOr(LEqual(I30A, 0), LEqual(L23D, 1)))
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
  OperationRegion (KEYS, SystemMemory, I31A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
    PSAT,   32
  }


}

//
// LPIO2 I2C #4
//
Device(I2C4)
{
  Name (_ADR, 0)
  Name (_HID, "80860F41")
  Name (_CID, "80860F41")
  Name (_DDN, "Intel(R) I2C Controller #4 - 80860F44")
  Name (_UID, 4)
  Name(_DEP, Package(0x1)
  {
    PEPD
  })
  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {35}  // I2C #4 IRQ

    FixedDMA(0x16, 0x6, Width32Bit, )
    FixedDMA(0x17, 0x7, Width32Bit, )
  })

  Method (SSCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x200, 0x200, 0x06 })
    Return (PKG)
  }
  Method (FMCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x55, 0x99, 0x06 })
    Return (PKG)
  }
  Method (FPCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x1b, 0x3a, 0x06 })
    Return (PKG)
  }


  Method (_HRV, 0x0, NotSerialized)
  {
    Return (SOCS)
  }
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(I40A, B0BA)
    Store(I40L, B0LN)
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

    If (LOr(LEqual(I40A, 0), LEqual(L24D, 1)))
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
  OperationRegion (KEYS, SystemMemory, I41A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
    PSAT,   32
  }

  PowerResource (CLK0, 0x00, 0x0000)
  {
    Method (_STA, 0, NotSerialized)   // _STA: Status
    {
      Return (CKC0)
    }

    Method (_ON, 0, NotSerialized)   // _ON_: Power On
    {
      Store (One, CKC0)
      Store (One, CKF0)
      Sleep (0x20)
    }

    Method (_OFF, 0, NotSerialized)   // _OFF: Power Off
    {
      Store (0x02, CKC0)
    }
  }
  PowerResource (CLK1, 0x00, 0x0000)
  {
    Method (_STA, 0, NotSerialized)   // _STA: Status
    {
      Return (CKC1)
    }

    Method (_ON, 0, NotSerialized)   // _ON_: Power On
    {
      Store (One, CKC1)
      Store (One, CKF1)
      Sleep (0x20)
    }

    Method (_OFF, 0, NotSerialized)   // _OFF: Power Off
    {
      Store (0x02, CKC1)
    }
  }
}

//
// LPIO2 I2C #5
//
Device(I2C5)
{
  Name (_ADR, 0)
  Name (_HID, "80860F41")
  Name (_CID, "80860F41")
  Name (_DDN, "Intel(R) I2C Controller #5 - 80860F45")
  Name (_UID, 5)
  Name(_DEP, Package(0x1)
  {
    PEPD
  })
  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {36}  // I2C #5 IRQ

    FixedDMA(0x18, 0x0, Width32Bit, )
    FixedDMA(0x19, 0x1, Width32Bit, )
  })

  Method (SSCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x200, 0x200, 0x06 })
    Return (PKG)
  }
  Method (FMCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x55, 0x99, 0x06 })
    Return (PKG)
  }
  Method (FPCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x1b, 0x3a, 0x06 })
    Return (PKG)
  }

  Method (_HRV, 0x0, NotSerialized)
  {
    Return (SOCS)
  }
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(I50A, B0BA)
    Store(I50L, B0LN)
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

    If (LOr(LEqual(I50A, 0), LEqual(L25D, 1)))
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
  OperationRegion (KEYS, SystemMemory, I51A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
    PSAT,   32
  }
}

//
// LPIO2 I2C #6
//
Device(I2C6)
{
  Name (_ADR, 0)
  Name (_HID, "80860F41")
  Name (_CID, "80860F41")
  Name (_DDN, "Intel(R) I2C Controller #6 - 80860F46")
  Name (_UID, 6)
  Name(_DEP, Package(0x1)
  {
    PEPD
  })
  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {37}  // I2C #6 IRQ

    FixedDMA(0x1A, 0x02, Width32Bit, )
    FixedDMA(0x1B, 0x03, Width32Bit, )
  })

  Method (SSCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x200, 0x200, 0x06 })
    Return (PKG)
  }
  Method (FMCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x55, 0x99, 0x06 })
    Return (PKG)
  }
  Method (FPCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x1b, 0x3a, 0x06 })
    Return (PKG)
  }

  Method (_HRV, 0x0, NotSerialized)
  {
    Return (SOCS)
  }
  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(I60A, B0BA)
    Store(I60L, B0LN)
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

    If (LOr(LEqual(I60A, 0), LEqual(L26D, 1)))
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
  OperationRegion (KEYS, SystemMemory, I61A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
           PSAT,   32
  }
}

//
// LPIO2 I2C #7
//
Device(I2C7)
{
  Name (_ADR, 0)
  Name (_HID, "80860F41")
  Name (_CID, "80860F41")
  //Name (_CLS, Package (3) {0x0C, 0x80, 0x00})
  Name (_DDN, "Intel(R) I2C Controller #7 - 80860F47")
  Name (_UID, 7)
  Name(_DEP, Package(0x1)
  {
    PEPD
  })
  Name (RBUF, ResourceTemplate ()
  {
    Memory32Fixed (ReadWrite, 0x00000000, 0x00001000, BAR0)
    Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {38}  // I2C #7 IRQ

    FixedDMA(0x1C, 0x4, Width32Bit, )
    FixedDMA(0x1D, 0x5, Width32Bit, )
  })

  Method (SSCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x200, 0x200, 0x06 })
    Return (PKG)
  }
  Method (FMCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x55, 0x99, 0x06 })
    Return (PKG)
  }
  Method (FPCN, 0x0, NotSerialized)
  {
    Name (PKG, Package(3) { 0x1b, 0x3a, 0x06 })
    Return (PKG)
  }

  Method (_HRV, 0x0, NotSerialized)
  {
    Return (SOCS)
  }

  Method (_CRS, 0x0, NotSerialized)
  {
    CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
    CreateDwordField(^RBUF, ^BAR0._LEN, B0LN)
    Store(I70A, B0BA)
    Store(I70L, B0LN)
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

    If (LOr(LEqual(I70A, 0), LEqual(L27D, 1)))
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

  OperationRegion (KEYS, SystemMemory, I71A, 0x100)
  Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
  {
    Offset (0x84),
    PSAT,   32
  }

}

