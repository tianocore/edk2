/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c)  2012  - 2014, Intel Corporation. All rights reserved   *;
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



// Use this information when determining the Possible IRQs that can be
// used in a given system.
//
// The following IRQs are always in use by legacy devices:
//              0  = System Timer
//              2  = 8259 PIC
//              8  = RTC
//              9  = SCI Interrupt (It may be used, we choose not to)
//              13 = Co-processor Error
//
// The following may be in use by legacy devices:
//              1  = If using PS/2 Keyboard
//              3  = If COMx Port Enabled and IRQ = 3
//              4  = If COMx Port Enabled and IRQ = 4
//              5  = If LPT Port Enabled and IRQ = 5
//              6  = If FDC Enabled
//              7  = If LPT Port Enabled and IRQ = 7
//              12 = If using PS/2 Mouse
//              14 = Primary IDE (If populated and in Compatibility Mode)
//              15 = Secondary IDE (If populated and in Compatibility Mode)
//
// The following will never be in use by legacy devices:
//              10 = Assign to PARC, PCRC, PERC, PGRC
//              11 = Assign to PBRC, PDRC, PFRC, PHRC

Device(LNKA)                            // PARC Routing Resource
{
  Name(_HID,EISAID("PNP0C0F"))    // PCI Interrupt Link Device

  Name(_UID,1)                    // Unique to other Link Devices

  // Disable the PCI IRQ.

  Method(_DIS,0,Serialized)
  {
    Or(PARC,0x80,PARC)
  }

  // Possible IRQ Resource Setting.

  Method (_PRS, 0, Serialized)
  {
    return (PRSA)
  }

  // Current IRQ Resource Setting.

  Method(_CRS,0,Serialized)
  {
    Name(RTLA,ResourceTemplate()
    {
      IRQ(Level,ActiveLow,Shared) {}
    })

    // Point to specific byte.

    CreateWordField(RTLA,1,IRQ0)

    // Zero out IRQ mask bits 0-15

    Store(Zero,IRQ0)

    ShiftLeft(1,And(PARC,0x0F),IRQ0)

    Return(RTLA)
  }

  // Set IRQ Resource Setting.

  Method(_SRS,1,Serialized)
  {
    // Point to the specific byte passed in

    CreateWordField(Arg0,1,IRQ0)

    // Determine the IRQ bit to set and store it

    FindSetRightBit(IRQ0,Local0)
    Decrement(Local0)
    Store(Local0,PARC)
  }

  // PCI IRQ Status.

  Method(_STA,0,Serialized)
  {
    If(And(PARC,0x80))
    {
      Return(0x0009)
    }
    Else
    {
      Return(0x000B)
    }
  }
}

Device(LNKB)                            // PBRC Routing Resource
{
  Name(_HID,EISAID("PNP0C0F"))

  Name(_UID,2)

  // Disable the PCI IRQ.

  Method(_DIS,0,Serialized)
  {
    Or(PBRC,0x80,PBRC)
  }

  // Possible IRQ Resource Setting.

  Method (_PRS, 0, Serialized)
  {
    return (PRSB)
  }

  // Current IRQ Resource Setting.

  Method(_CRS,0,Serialized)
  {
    Name(RTLB,ResourceTemplate()
    {
      IRQ(Level,ActiveLow,Shared) {}
    })

    // Point to specific byte.

    CreateWordField(RTLB,1,IRQ0)

    // Zero out IRQ mask bits 0-15

    Store(Zero,IRQ0)

    ShiftLeft(1,And(PBRC,0x0F),IRQ0)

    Return(RTLB)
  }

  // Set IRQ Resource Setting.

  Method(_SRS,1,Serialized)
  {
    // Point to the specific byte passed in.

    CreateWordField(Arg0,1,IRQ0)

    // Determine the IRQ bit to set and store it,

    FindSetRightBit(IRQ0,Local0)
    Decrement(Local0)
    Store(Local0,PBRC)
  }

  // PCI IRQ Status.

  Method(_STA,0,Serialized)
  {
    If(And(PBRC,0x80))
    {
      Return(0x0009)
    }
    Else
    {
      Return(0x000B)
    }
  }
}

Device(LNKC)                            // PCRC Routing Resource
{
  Name(_HID,EISAID("PNP0C0F"))

  Name(_UID,3)

  // Disable the PCI IRQ.

  Method(_DIS,0,Serialized)
  {
    Or(PCRC,0x80,PCRC)
  }

  // Possible IRQ Resource Setting.

  Method (_PRS, 0, Serialized)
  {
    return (PRSC)
  }

  // Current IRQ Resource Setting.

  Method(_CRS,0,Serialized)
  {
    Name(RTLC,ResourceTemplate()
    {
      IRQ(Level,ActiveLow,Shared) {}
    })

    // Point to specific byte.

    CreateWordField(RTLC,1,IRQ0)

    // Zero out IRQ mask bits 0-15

    Store(Zero,IRQ0)

    ShiftLeft(1,And(PCRC,0x0F),IRQ0)

    Return(RTLC)
  }

  // Set IRQ Resource Setting.

  Method(_SRS,1,Serialized)
  {
    // Point to the specific byte passed in.

    CreateWordField(Arg0,1,IRQ0)

    // Determine the IRQ bit to set and store it,

    FindSetRightBit(IRQ0,Local0)
    Decrement(Local0)
    Store(Local0,PCRC)
  }

  // PCI IRQ Status.

  Method(_STA,0,Serialized)
  {
    If(And(PCRC,0x80))
    {
      Return(0x0009)
    }
    Else
    {
      Return(0x000B)
    }
  }
}

Device(LNKD)                            // PDRC Routing Resource
{
  Name(_HID,EISAID("PNP0C0F"))

  Name(_UID,4)

  // Disable the PCI IRQ.

  Method(_DIS,0,Serialized)
  {
    Or(PDRC,0x80,PDRC)
  }

  // Possible IRQ Resource Setting.

  Method (_PRS, 0, Serialized)
  {
    return (PRSD)
  }

  // Current IRQ Resource Setting.

  Method(_CRS,0,Serialized)
  {
    Name(RTLD,ResourceTemplate()
    {
      IRQ(Level,ActiveLow,Shared) {}
    })

    // Point to specific byte.

    CreateWordField(RTLD,1,IRQ0)

    // Zero out IRQ mask bits 0-15

    Store(Zero,IRQ0)

    ShiftLeft(1,And(PDRC,0x0F),IRQ0)

    Return(RTLD)
  }

  // Set IRQ Resource Setting.

  Method(_SRS,1,Serialized)
  {
    // Point to the specific byte passed in.

    CreateWordField(Arg0,1,IRQ0)

    // Determine the IRQ bit to set and store it,

    FindSetRightBit(IRQ0,Local0)
    Decrement(Local0)
    Store(Local0,PDRC)
  }

  // PCI IRQ Status.

  Method(_STA,0,Serialized)
  {
    If(And(PDRC,0x80))
    {
      Return(0x0009)
    }
    Else
    {
      Return(0x000B)
    }
  }
}

Device(LNKE)                            // PERC Routing Resource
{
  Name(_HID,EISAID("PNP0C0F"))

  Name(_UID,5)

  // Disable the PCI IRQ.

  Method(_DIS,0,Serialized)
  {
    Or(PERC,0x80,PERC)
  }

  // Possible IRQ Resource Setting.

  Method (_PRS, 0, Serialized)
  {
    return (PRSE)
  }

  // Current IRQ Resource Setting.

  Method(_CRS,0,Serialized)
  {
    Name(RTLE,ResourceTemplate()
    {
      IRQ(Level,ActiveLow,Shared) {}
    })

    // Point to specific byte.

    CreateWordField(RTLE,1,IRQ0)

    // Zero out IRQ mask bits 0-15

    Store(Zero,IRQ0)

    ShiftLeft(1,And(PERC,0x0F),IRQ0)

    Return(RTLE)
  }

  // Set IRQ Resource Setting.

  Method(_SRS,1,Serialized)
  {
    // Point to the specific byte passed in

    CreateWordField(Arg0,1,IRQ0)

    // Determine the IRQ bit to set and store it

    FindSetRightBit(IRQ0,Local0)
    Decrement(Local0)
    Store(Local0,PERC)
  }

  // PCI IRQ Status.

  Method(_STA,0,Serialized)
  {
    If(And(PERC,0x80))
    {
      Return(0x0009)
    }
    Else
    {
      Return(0x000B)
    }
  }
}

Device(LNKF)                            // PFRC Routing Resource
{
  Name(_HID,EISAID("PNP0C0F"))

  Name(_UID,6)

  // Disable the PCI IRQ.

  Method(_DIS,0,Serialized)
  {
    Or(PFRC,0x80,PFRC)
  }

  // Possible IRQ Resource Setting.

  Method (_PRS, 0, Serialized)
  {
    return (PRSF)
  }

  // Current IRQ Resource Setting.

  Method(_CRS,0,Serialized)
  {
    Name(RTLF,ResourceTemplate()
    {
      IRQ(Level,ActiveLow,Shared) {}
    })

    // Point to specific byte.

    CreateWordField(RTLF,1,IRQ0)

    // Zero out IRQ mask bits 0-15

    Store(Zero,IRQ0)

    ShiftLeft(1,And(PFRC,0x0F),IRQ0)

    Return(RTLF)
  }

  // Set IRQ Resource Setting.

  Method(_SRS,1,Serialized)
  {
    // Point to the specific byte passed in.

    CreateWordField(Arg0,1,IRQ0)

    // Determine the IRQ bit to set and store it,

    FindSetRightBit(IRQ0,Local0)
    Decrement(Local0)
    Store(Local0,PFRC)
  }

  // PCI IRQ Status.

  Method(_STA,0,Serialized)
  {
    If(And(PFRC,0x80))
    {
      Return(0x0009)
    }
    Else
    {
      Return(0x000B)
    }
  }
}

Device(LNKG)                            // PGRC Routing Resource
{
  Name(_HID,EISAID("PNP0C0F"))

  Name(_UID,7)

  // Disable the PCI IRQ.

  Method(_DIS,0,Serialized)
  {
    Or(PGRC,0x80,PGRC)
  }

  // Possible IRQ Resource Setting.

  Method (_PRS, 0, Serialized)
  {
    return (PRSG)
  }

  // Current IRQ Resource Setting.

  Method(_CRS,0,Serialized)
  {
    Name(RTLG,ResourceTemplate()
    {
      IRQ(Level,ActiveLow,Shared) {}
    })

    // Point to specific byte.

    CreateWordField(RTLG,1,IRQ0)

    // Zero out IRQ mask bits 0-15

    Store(Zero,IRQ0)

    ShiftLeft(1,And(PGRC,0x0F),IRQ0)

    Return(RTLG)
  }

  // Set IRQ Resource Setting.

  Method(_SRS,1,Serialized)
  {
    // Point to the specific byte passed in.

    CreateWordField(Arg0,1,IRQ0)

    // Determine the IRQ bit to set and store it,

    FindSetRightBit(IRQ0,Local0)
    Decrement(Local0)
    Store(Local0,PGRC)
  }

  // PCI IRQ Status.

  Method(_STA,0,Serialized)
  {
    If(And(PGRC,0x80))
    {
      Return(0x0009)
    }
    Else
    {
      Return(0x000B)
    }
  }
}

Device(LNKH)                            // PHRC Routing Resource
{
  Name(_HID,EISAID("PNP0C0F"))

  Name(_UID,8)

  // Disable the PCI IRQ.

  Method(_DIS,0,Serialized)
  {
    Or(PHRC,0x80,PHRC)
  }

  // Possible IRQ Resource Setting.

  Method (_PRS, 0, Serialized)
  {
    return (PRSH)
  }

  // Current IRQ Resource Setting.

  Method(_CRS,0,Serialized)
  {
    Name(RTLH,ResourceTemplate()
    {
      IRQ(Level,ActiveLow,Shared) {}
    })

    // Point to specific byte.

    CreateWordField(RTLH,1,IRQ0)

    // Zero out IRQ mask bits 0-15

    Store(Zero,IRQ0)

    ShiftLeft(1,And(PHRC,0x0F),IRQ0)

    Return(RTLH)
  }

  // Set IRQ Resource Setting.

  Method(_SRS,1,Serialized)
  {
    // Point to the specific byte passed in.

    CreateWordField(Arg0,1,IRQ0)

    // Determine the IRQ bit to set and store it,

    FindSetRightBit(IRQ0,Local0)
    Decrement(Local0)
    Store(Local0,PHRC)
  }

  // PCI IRQ Status.

  Method(_STA,0,Serialized)
  {
    If(And(PHRC,0x80))
    {
      Return(0x0009)
    }
    Else
    {
      Return(0x000B)
    }
  }
}
