/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved    *;
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


Scope(\)
{
  //
  // Define VLV ABASE I/O as an ACPI operating region. The base address
  // can be found in Device 31, Registers 40-43h.
  //
  OperationRegion(PMIO, SystemIo, \PMBS, 0x46)
  Field(PMIO, ByteAcc, NoLock, Preserve)
  {
    ,      8,
    PWBS,  1,       // Power Button Status
    Offset(0x20),
    ,      13,
    PMEB,  1,     // PME_B0_STS
    Offset(0x42),     // General Purpose Control
    ,      1,
    GPEC,  1
  }
  Field(PMIO, ByteAcc, NoLock, WriteAsZeros)
  {
    Offset(0x20),     // GPE0 Status
    ,      4,
    PSCI,  1,       // PUNIT SCI Status
    SCIS,  1        // GUNIT SCI Status
  }



  //
  // Define a Memory Region that will allow access to the PMC
  // Register Block.  Note that in the Intel Reference Solution, the PMC
  // will get fixed up dynamically during POST.
  //
  OperationRegion(PMCR, SystemMemory, \PFDR, 0x04)// PMC Function Disable Register
  Field(PMCR,DWordAcc,Lock,Preserve)
  {
    Offset(0x00),   //  Function Disable Register
    L10D,  1,         //  (0) LPIO1 DMA Disable
    L11D,  1,         //  (1) LPIO1 PWM #1 Disable
    L12D,  1,         //  (2) LPIO1 PWM #2 Disable
    L13D,  1,         //  (3) LPIO1 HS-UART #1 Disable
    L14D,  1,         //  (4) LPIO1 HS-UART #2 Disable
    L15D,  1,         //  (5) LPIO1 SPI Disable
    ,          2,     //  (6:7) Reserved
    SD1D,  1,         //  (8) SCC SDIO #1 Disable
    SD2D,  1,         //  (9) SCC SDIO #2 Disable
    SD3D,  1,         //  (10) SCC SDIO #3 Disable
    HSID,  1,         //  (11)
    HDAD,  1,         //  (12) Azalia Disable
    LPED,  1,         //  (13) LPE Disable
    OTGD,  1,         //  (14) USB OTG Disable
    ,          1,     //  (15) USH Disable
    ,          1,     //  (16)
    ,          1,     //  (17)
    ,          1,     //  (18) USB Disable
    ,          1,     //  (19) SEC Disable
    RP1D,  1,         //  (20) Root Port 0 Disable
    RP2D,  1,         //  (21) Root Port 1 Disable
    RP3D,  1,         //  (22) Root Port 2 Disable
    RP4D,  1,         //  (23) Root Port 3 Disable
    L20D,  1,         //  (24) LPIO2 DMA Disable
    L21D,  1,         //  (25) LPIO2 I2C #1 Disable
    L22D,  1,         //  (26) LPIO2 I2C #2 Disable
    L23D,  1,         //  (27) LPIO2 I2C #3 Disable
    L24D,  1,         //  (28) LPIO2 I2C #4 Disable
    L25D,  1,         //  (29) LPIO2 I2C #5 Disable
    L26D,  1,         //  (30) LPIO2 I2C #6 Disable
    L27D,  1          //  (31) LPIO2 I2C #7 Disable
  }


  OperationRegion(CLKC, SystemMemory, \PCLK, 0x18)// PMC CLK CTL Registers
  Field(CLKC,DWordAcc,Lock,Preserve)
  {
    Offset(0x00),   //  PLT_CLK_CTL_0
    CKC0, 2,
    CKF0, 1,
    ,     29,
    Offset(0x04),   //  PLT_CLK_CTL_1
    CKC1, 2,
    CKF1, 1,
    ,     29,
    Offset(0x08),   //  PLT_CLK_CTL_2
    CKC2,  2,
    CKF2, 1,
    ,     29,
    Offset(0x0C),   //  PLT_CLK_CTL_3
    CKC3,  2,
    CKF3, 1,
    ,     29,
    Offset(0x10),   //  PLT_CLK_CTL_4
    CKC4,  2,
    CKF4, 1,
    ,     29,
    Offset(0x14),   //  PLT_CLK_CTL_5
    CKC5,  2,
    CKF5, 1,
    ,     29,
  }
} //end Scope(\)

scope (\_SB)
{
  Device(LPEA)
  {
    Name (_ADR, 0)
    Name (_HID, "80860F28")
    Name (_CID, "80860F28")
    //Name (_CLS, Package (3) {0x04, 0x01, 0x00})
    Name (_DDN, "Intel(R) Low Power Audio Controller - 80860F28")
    Name (_SUB, "80867270")
    Name (_UID, 1)
    Name (_DEP, Package() {\_SB.I2C2.RTEK})
    Name(_PR0,Package() {PLPE})

    Method (_STA, 0x0, NotSerialized)
    {
      If (LAnd(LAnd(LEqual(LPEE, 2), LEqual(LPED, 0)), LEqual(OSEL, 0)))
      {
        Return (0xF)
      }
      Return (0x0)
    }

    Method (_DIS, 0x0, NotSerialized)
    {
      //Add a dummy disable function
    }

    Name (RBUF, ResourceTemplate ()
      {
        Memory32Fixed (ReadWrite, 0xFE400000, 0x00200000, BAR0)  // MMIO 1 - LPE MMIO
        Memory32Fixed (ReadWrite, 0xFE830000, 0x00001000, BAR1)  // MMIO 2 - Shadowed PCI Config Space
        Memory32Fixed (ReadWrite, 0x55AA55AA, 0x00100000, BAR2)  // LPE Memory Bar Allocate during post
        Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {24}
        Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {25}
        Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {26}
        Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {27}
        Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {28}
        Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {29}
        GpioInt(Edge, ActiveBoth, ExclusiveAndWake, PullNone, 0,"\\_SB.GPO2") {28} //  Audio jack interrupt
      }
    )

    Method (_CRS, 0x0, NotSerialized)
    {
      CreateDwordField(^RBUF, ^BAR0._BAS, B0BA)
      Store(LPE0, B0BA)
      CreateDwordField(^RBUF, ^BAR1._BAS, B1BA)
      Store(LPE1, B1BA)
      CreateDwordField(^RBUF, ^BAR2._BAS, B2BA)
      Store(LPE2, B2BA)
      Return (RBUF)
    }

    OperationRegion (KEYS, SystemMemory, LPE1, 0x100)
    Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
    {
      Offset (0x84),
      PSAT,   32
    }

    PowerResource(PLPE, 0, 0)   // Power Resource for LPEA
    {
      Method (_STA)
      {
        Return (1)      // Power Resource is always available.
      }

      Method (_ON)
      {
        And(PSAT, 0xfffffffC, PSAT)
        OR(PSAT, 0X00000000, PSAT)
      }

      Method (_OFF)
      {
        OR(PSAT, 0x00000003, PSAT)
        OR(PSAT, 0X00000000, PSAT)
      }
    } // End PLPE
  } // End "Low Power Engine Audio"

  Device(LPA2)
  {
    Name (_ADR, 0)
    Name (_HID, "LPE0F28")  // _HID: Hardware ID
    Name (_CID, "LPE0F28")  // _CID: Compatible ID
    Name (_DDN, "Intel(R) SST Audio - LPE0F28")  // _DDN: DOS Device Name
    Name (_SUB, "80867270")
    Name (_UID, 1)
    Name (_DEP, Package() {\_SB.I2C2.RTEK})
    Name(_PR0,Package() {PLPE})

    Method (_STA, 0x0, NotSerialized)
    {
      If (LAnd(LAnd(LEqual(LPEE, 2), LEqual(LPED, 0)), LEqual(OSEL, 1)))
      {
        Return (0xF)
      }
      Return (0x0)
    }

    Method (_DIS, 0x0, NotSerialized)
    {
      //Add a dummy disable function
    }

    Name (RBUF, ResourceTemplate ()
      {
        Memory32Fixed (ReadWrite, 0x55AA55AA, 0x00100000, BAR2)  // LPE Memory Bar Allocate during post
        Memory32Fixed (ReadWrite, 0x55AA55AA, 0x00000100, SHIM)
        Memory32Fixed (ReadWrite, 0x55AA55AA, 0x00001000, MBOX)
        Memory32Fixed (ReadWrite, 0x55AA55AA, 0x00014000, IRAM)
        Memory32Fixed (ReadWrite, 0x55AA55AA, 0x00028000, DRAM)
        Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {29}
        Memory32Fixed (ReadWrite, 0xFE830000, 0x00001000, BAR1)  // MMIO 2 - Shadowed PCI Config Space
      }
    )

    Method (_CRS, 0x0, NotSerialized)
    {
      CreateDwordField(^RBUF, ^SHIM._BAS, SHBA)
      Add(LPE0, 0x140000, SHBA)
      CreateDwordField(^RBUF, ^MBOX._BAS, MBBA)
      Add(LPE0, 0x144000, MBBA)
      CreateDwordField(^RBUF, ^IRAM._BAS, IRBA)
      Add(LPE0, 0xC0000, IRBA)
      CreateDwordField(^RBUF, ^DRAM._BAS, DRBA)
      Add(LPE0, 0x100000, DRBA)
      CreateDwordField(^RBUF, ^BAR1._BAS, B1BA)
      Store(LPE1, B1BA)
      CreateDwordField(^RBUF, ^BAR2._BAS, B2BA)
      Store(LPE2, B2BA)
      Return (RBUF)
    }

    OperationRegion (KEYS, SystemMemory, LPE1, 0x100)
    Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
    {
      Offset (0x84),
      PSAT,   32
    }

    PowerResource(PLPE, 0, 0)   // Power Resource for LPEA
    {
      Method (_STA)
      {
        Return (1)      // Power Resource is always available.
      }

      Method (_ON)
      {
        And(PSAT, 0xfffffffC, PSAT)
        OR(PSAT, 0X00000000, PSAT)
      }

      Method (_OFF)
      {
        OR(PSAT, 0x00000003, PSAT)
        OR(PSAT, 0X00000000, PSAT)
      }
    } // End PLPE

    Device (ADMA)
    {
      Name (_ADR, Zero)  // _ADR: Address
      Name (_HID, "DMA0F28")  // _HID: Hardware ID
      Name (_CID, "DMA0F28")  // _CID: Compatible ID
      Name (_DDN, "Intel(R) Audio  DMA0 - DMA0F28")  // _DDN: DOS Device Name
      Name (_UID, One)  // _UID: Unique ID
      Name (RBUF, ResourceTemplate ()
      {
        Memory32Fixed (ReadWrite, 0x55AA55AA, 0x00001000, DMA0)  // LPE BASE + LPE DMA0 offset
        Memory32Fixed (ReadWrite, 0x55AA55AA, 0x00001000, SHIM)  // LPE BASE + LPE SHIM offset
        Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive, , , ) {24}
      })

      Method (_CRS, 0, NotSerialized)   // _CRS: Current Resource Settings
      {
        CreateDwordField(^RBUF, ^DMA0._BAS, D0BA)
        Add(LPE0, 0x98000, D0BA)
        CreateDwordField(^RBUF, ^SHIM._BAS, SHBA)
        Add(LPE0, 0x140000, SHBA)
        Return (RBUF)
      }
    }
  } // End "Low Power Engine Audio" for Android
}

scope (\_SB.PCI0)
{

  //
  // Serial ATA Host Controller - Device 19, Function 0
  //

  Device(SATA)
  {
    Name(_ADR,0x00130000)
    //
    // SATA Methods pulled in via SSDT.
    //

    OperationRegion(SATR, PCI_Config, 0x74,0x4)
    Field(SATR,WordAcc,NoLock,Preserve)
    {
      Offset(0x00), // 0x74, PMCR
      ,   8,
      PMEE,   1,    //PME_EN
      ,   6,
      PMES,   1     //PME_STS
    }

    Method (_STA, 0x0, NotSerialized)
    {
      Return(0xf)
    }

    Method(_DSW, 3)
    {
    } // End _DSW
  }

  //
  // For eMMC 4.41 PCI mode in order to present non-removable device under Windows environment
  //
  Device(EM41)
  {
    Name(_ADR,0x00100000)
    OperationRegion(SDIO, PCI_Config, 0x84,0x4)
    Field(SDIO,WordAcc,NoLock,Preserve)
    {
      Offset(0x00), // 0x84, PMCR
      ,   8,
      PMEE,   1,    //PME_EN
      ,   6,
      PMES,   1     //PME_STS
    }

    Method (_STA, 0x0, NotSerialized)
    {
      If (LAnd(LEqual(PCIM, 1), LEqual(SD1D, 0)))
      {
        Return(0xF)
      }
      Else
      {
        Return(0x0)
      }
    }

    Method(_DSW, 3)
    {
    } // End _DSW

    Device (CARD)
    {
      Name (_ADR, 0x00000008)
      Method(_RMV, 0x0, NotSerialized)
      {
        Return (0)
      } // End _DSW
    }
  }

  //
  // For eMMC 4.5 PCI mode in order to present non-removable device under Windows environment
  //
  Device(EM45)
  {
    Name(_ADR,0x00170000)
    OperationRegion(SDIO, PCI_Config, 0x84,0x4)
    Field(SDIO,WordAcc,NoLock,Preserve)
    {
      Offset(0x00), // 0x84, PMCR
      ,   8,
      PMEE,   1,    //PME_EN
      ,   6,
      PMES,   1     //PME_STS
    }

    Method (_STA, 0x0, NotSerialized)
    {
      If (LAnd(LEqual(PCIM, 1), LEqual(HSID, 0)))
      {
        Return(0xF)
      }
      Else
      {
        Return(0x0)
      }
    }

    Method(_DSW, 3)
    {
    } // End _DSW

    Device (CARD)
    {
      Name (_ADR, 0x00000008)
      Method(_RMV, 0x0, NotSerialized)
      {
        Return (0)
      } // End _DSW
    }
  }
  //
  // For SD Host Controller (Bus 0x00 : Dev 0x12 : Func 0x00) PCI mode in order to present non-removable device under Windows environment
  //
  Device(SD12)
  {
    Name(_ADR,0x00120000)

    Method (_STA, 0x0, NotSerialized)
    {
      //
      // PCIM>> 0:ACPI mode           1:PCI mode
      //
      If (LEqual(PCIM, 0)) {
        Return (0x0)
      }

      //
      // If device is disabled.
      //
      If (LEqual(SD3D, 1))
      {
        Return (0x0)
      }

      Return (0xF)
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

  // xHCI Controller - Device 20, Function 0
  include("PchXhci.asl")

  //
  // High Definition Audio Controller - Device 27, Function 0
  //
  Device(HDEF)
  {
    Name(_ADR, 0x001B0000)
    include("PchAudio.asl")

    Method (_STA, 0x0, NotSerialized)
    {
      If (LEqual(HDAD, 0))
      {
        Return(0xf)
      }
      Return(0x0)
    }

    Method(_DSW, 3)
    {
    } // End _DSW
  } // end "High Definition Audio Controller"



  //
  // PCIE Root Port #1
  //
  Device(RP01)
  {
    Name(_ADR, 0x001C0000)
    include("PchPcie.asl")
    Name(_PRW, Package() {9, 4})

    Method(_PRT,0)
    {
      If(PICM) { Return(AR04) }// APIC mode
      Return (PR04) // PIC Mode
    } // end _PRT
  } // end "PCIE Root Port #1"

  //
  // PCIE Root Port #2
  //
  Device(RP02)
  {
    Name(_ADR, 0x001C0001)
    include("PchPcie.asl")
    Name(_PRW, Package() {9, 4})

    Method(_PRT,0)
    {
      If(PICM) { Return(AR05) }// APIC mode
      Return (PR05) // PIC Mode
    } // end _PRT

  } // end "PCIE Root Port #2"

  //
  // PCIE Root Port #3
  //
  Device(RP03)
  {
    Name(_ADR, 0x001C0002)
    include("PchPcie.asl")
    Name(_PRW, Package() {9, 4})
    Method(_PRT,0)
    {
      If(PICM) { Return(AR06) }// APIC mode
      Return (PR06) // PIC Mode
    } // end _PRT

  } // end "PCIE Root Port #3"

  //
  // PCIE Root Port #4
  //
  Device(RP04)
  {
    Name(_ADR, 0x001C0003)
    include("PchPcie.asl")
    Name(_PRW, Package() {9, 4})
    Method(_PRT,0)
    {
      If(PICM) { Return(AR07) }// APIC mode
      Return (PR07) // PIC Mode
    } // end _PRT

  } // end "PCIE Root Port #4"


  Scope(\_SB)
  {
    //
    // Dummy power resource for USB D3 cold support
    //
    PowerResource(USBC, 0, 0)
    {
      Method(_STA) { Return (0xF) }
      Method(_ON) {}
      Method(_OFF) {}
    }
  }
  //
  // EHCI Controller - Device 29, Function 0
  //
  Device(EHC1)
  {
    Name(_ADR, 0x001D0000)
    Name(_DEP, Package(0x1)
    {
      PEPD
    })
    include("PchEhci.asl")
    Name(_PRW, Package() {0x0D, 4})

    OperationRegion(USBR, PCI_Config, 0x54,0x4)
    Field(USBR,WordAcc,NoLock,Preserve)
    {
      Offset(0x00), // 0x54, PMCR
      ,   8,
      PMEE,   1,    //PME_EN
      ,   6,
      PMES,   1     //PME_STS
    }

    Method (_STA, 0x0, NotSerialized)
    {
      If(LEqual(XHCI, 0))      //XHCI is not present. It means EHCI is there
      {
        Return (0xF)
      } Else
      {
        Return (0x0)
      }
    }

    Method (_RMV, 0, NotSerialized)
    {
      Return (0x0)
    }
    //
    // Create a dummy PR3 method to indicate to the PCI driver
    // that the device is capable of D3 cold
    //
    Method(_PR3, 0x0, NotSerialized)
    {
      return (Package() {\_SB.USBC})
    }

  } // end "EHCI Controller"

  //
  // SMBus Controller - Device 31, Function 3
  //
  Device(SBUS)
  {
    Name(_ADR,0x001F0003)
    Include("PchSmb.asl")
  }

  Device(SEC0)
  {
    Name (_ADR, 0x001a0000)                     // Device 0x1a, Function 0
    Name(_DEP, Package(0x1)
    {
      PEPD
    })


    OperationRegion (PMEB, PCI_Config, 0x84, 0x04)  //PMECTRLSTATUS
    Field (PMEB, WordAcc, NoLock, Preserve)
    {
      ,   8,
      PMEE,   1,    //bit8 PMEENABLE
      ,   6,
      PMES,   1     //bit15 PMESTATUS
    }

    // Arg0 -- integer that contains the device wake capability control (0-disable 1- enable)
    // Arg1 -- integer that contains target system state (0-4)
    // Arg2 -- integer that contains the target device state
    Method (_DSW, 3, NotSerialized)   // _DSW: Device Sleep Wake
    {
    }

    Method (_CRS, 0, NotSerialized)
    {
      Name (RBUF, ResourceTemplate ()
      {
        Memory32Fixed (ReadWrite, 0x1e000000, 0x2000000)
      })

      If (LEqual(PAVP, 2))
      {
        Return (RBUF)
      }
      Return (ResourceTemplate() {})
    }

    Method (_STA)
    {
      If (LNotEqual(PAVP, 0))
      {
        Return (0xF)
      }
      Return (0x0)
    }
  }   // Device(SEC0)

} // End scope (\_SB.PCI0)
