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

Device(FWHD) // Firmware Hub Device
{
  Name(_HID,EISAID("INT0800"))

  Name(_CRS,ResourceTemplate()
  {
    Memory32Fixed(ReadOnly,0xFF000000,0x1000000)
  })
}

Device(IPIC) // 8259 PIC
{
  Name(_HID,EISAID("PNP0000"))

  Name(_CRS,ResourceTemplate()
  {
    IO(Decode16,0x20,0x20,0x01,0x02)
    IO(Decode16,0x24,0x24,0x01,0x02)
    IO(Decode16,0x28,0x28,0x01,0x02)
    IO(Decode16,0x2C,0x2C,0x01,0x02)
    IO(Decode16,0x30,0x30,0x01,0x02)
    IO(Decode16,0x34,0x34,0x01,0x02)
    IO(Decode16,0x38,0x38,0x01,0x02)
    IO(Decode16,0x3C,0x3C,0x01,0x02)
    IO(Decode16,0xA0,0xA0,0x01,0x02)
    IO(Decode16,0xA4,0xA4,0x01,0x02)
    IO(Decode16,0xA8,0xA8,0x01,0x02)
    IO(Decode16,0xAC,0xAC,0x01,0x02)
    IO(Decode16,0xB0,0xB0,0x01,0x02)
    IO(Decode16,0xB4,0xB4,0x01,0x02)
    IO(Decode16,0xB8,0xB8,0x01,0x02)
    IO(Decode16,0xBC,0xBC,0x01,0x02)
    IO(Decode16,0x4D0,0x4D0,0x01,0x02)
    IRQNoFlags() {2}
  })
}

Device(LDRC) // LPC Device Resource Consumption
{
  Name(_HID,EISAID("PNP0C02"))

  Name(_UID,2)

  Name(_CRS,ResourceTemplate()
  {
    IO(Decode16,0x4E,0x4E,0x1,0x02)         // LPC Slot Access.
    IO(Decode16,0x61,0x61,0x1,0x1)          // NMI Status.
    IO(Decode16,0x63,0x63,0x1,0x1)          // Processor I/F.
    IO(Decode16,0x65,0x65,0x1,0x1)          // Processor I/F.
    IO(Decode16,0x67,0x67,0x1,0x1)          // Processor I/F.
    IO(Decode16,0x70,0x70,0x1,0x1)          // NMI Enable.
    IO(Decode16,0x80,0x80,0x1,0x10)         // Postcode.
    IO(Decode16,0x92,0x92,0x1,0x1)          // Processor I/F.
    IO(Decode16,0xB2,0xB2,0x01,0x02)        // Software SMI.
    IO(Decode16,0x680,0x680,0x1,0x20)       // 32 Byte I/O.
    IO(Decode16,0x400,0x400,0x1,0x80)       // ACPI Base.
    IO(Decode16,0x500,0x500,0x1,0xFF)       // GPIO Base.
  })
}

Device(TIMR) // 8254 Timer
{
  Name(_HID,EISAID("PNP0100"))

  Name(_CRS,ResourceTemplate()
  {
    IO(Decode16,0x40,0x40,0x01,0x04)
    IO(Decode16,0x50,0x50,0x10,0x04)
    IRQNoFlags() {0}
  })
}

Device(IUR3) // Internal UART
{
  Name(_HID, EISAID("PNP0501"))

  Name(_UID,1)

  // Status Method for internal UART

  Method(_STA,0,Serialized)
  {
    // Only report resources to the OS if internal UART is
    // not set to Disabled in BIOS Setup.

    If(LEqual(USEL,0))
    {
      If(LEqual(PU1E,1))
      {
        Store(1,UI3E) // Enable IRQ3 for UART
        Store(1,UI4E) // Enable IRQ4 for UART
        Store(1,C1EN) // Enable UART
        Return(0x000F)
      }
    }

    Return(0x0000)
  }

  // Disable Method for internal UART

  Method(_DIS,0,Serialized)
  {
    Store(0,UI3E)
    Store(0,UI4E)
    Store(0,C1EN)
  }

  // Current Resource Setting Method for internal UART

  Method(_CRS,0,Serialized)
  {
    // Create the Buffer that stores the Resources to
    // be returned.

    Name(BUF0,ResourceTemplate()
    {
      IO(Decode16,0x03F8,0x03F8,0x01,0x08)
      IRQNoFlags() {3}
    })

    Name(BUF1,ResourceTemplate()
    {
      IO(Decode16,0x03F8,0x03F8,0x01,0x08)
      IRQNoFlags() {4}
    })

    If (LLessEqual(SRID, 0x04))
    {
      Return(BUF0)
    } Else
    {
      Return(BUF1)
    }
  }
}
