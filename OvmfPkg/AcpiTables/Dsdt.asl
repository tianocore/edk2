/** @file
  Contains root level name space objects for the platform
  
  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/ 

DefinitionBlock ("Dsdt.aml", "DSDT", 1, "INTEL ", "OVMF    ", 3) {
  //
  // System Sleep States
  //
  Name (\_S0, Package () {5, 0, 0, 0})
  Name (\_S4, Package () {1, 0, 0, 0})
  Name (\_S5, Package () {0, 0, 0, 0})

  //
  //  System Bus
  //
  Scope (\_SB) {
    //
    // PCI Root Bridge
    //
    Device (PCI0) {
      Name (_HID, EISAID ("PNP0A03"))
      Name (_ADR, 0x00000000)
      Name (_BBN, 0x00)
      Name (_UID, 0x00)

      //
      // BUS, I/O, and MMIO resources
      //
      Name (_CRS, ResourceTemplate () {
        WORDBusNumber (          // Bus number resource (0); the bridge produces bus numbers for its subsequent buses
          ResourceProducer,      // bit 0 of general flags is 1
          MinFixed,              // Range is fixed
          MaxFixed,              // Range is fixed
          PosDecode,             // PosDecode
          0x0000,                // Granularity
          0x0000,                // Min
          0x00FF,                // Max
          0x0000,                // Translation
          0x0100                 // Range Length = Max-Min+1
          )

        IO (Decode16, 0xCF8, 0xCF8, 0x01, 0x08)       //Consumed resource (0xCF8-0xCFF)

        WORDIO (                 // Consumed-and-produced resource (all I/O below CF8)
          ResourceProducer,      // bit 0 of general flags is 0
          MinFixed,              // Range is fixed
          MaxFixed,              // Range is fixed
          PosDecode,             
          EntireRange,
          0x0000,                // Granularity
          0x0000,                // Min
          0x0CF7,                // Max
          0x0000,                // Translation
          0x0CF8                 // Range Length
          )

        WORDIO (                 // Consumed-and-produced resource (all I/O above CFF)
          ResourceProducer,      // bit 0 of general flags is 0
          MinFixed,              // Range is fixed
          MaxFixed,              // Range is fixed
          PosDecode,             
          EntireRange,
          0x0000,                // Granularity
          0x0D00,                // Min
          0xFFFF,                // Max
          0x0000,                // Translation
          0xF300                 // Range Length
          )

        DWORDMEMORY (            // Descriptor for legacy VGA video RAM
          ResourceProducer,      // bit 0 of general flags is 0
          PosDecode,
          MinFixed,              // Range is fixed
          MaxFixed,              // Range is Fixed
          Cacheable,
          ReadWrite,
          0x00000000,            // Granularity
          0x000A0000,            // Min
          0x000BFFFF,            // Max
          0x00000000,            // Translation
          0x00020000             // Range Length
          )

        DWORDMEMORY (            // Descriptor for linear frame buffer video RAM
          ResourceProducer,      // bit 0 of general flags is 0
          PosDecode,
          MinFixed,              // Range is fixed
          MaxFixed,              // Range is Fixed
          Cacheable,
          ReadWrite,
          0x00000000,            // Granularity
          0xF8000000,            // Min
          0xFFFBFFFF,            // Max
          0x00000000,            // Translation
          0x07FC0000             // Range Length
          )
      })

      //
      // PCI Interrupt Routing Table - PIC Mode Only
      //
      Method (_PRT, 0, NotSerialized) {
        Return (
          Package () {
            //
            // Bus 0, Device 1
            //
            Package () {0x0001FFFF, 0x00, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x0001FFFF, 0x01, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x0001FFFF, 0x02, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x0001FFFF, 0x03, \_SB.PCI0.LPC.LNKD, 0x00},
            //
            // Bus 0, Device 3
            //
            Package () {0x0003FFFF, 0x00, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x0003FFFF, 0x01, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x0003FFFF, 0x02, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x0003FFFF, 0x03, \_SB.PCI0.LPC.LNKD, 0x00},
          }
        )
      }

      //
      // PCI to ISA Bridge (Bus 0, Device 1, Function 0)
      //
      Device (LPC) {
        Name (_ADR, 0x00010000)

        //
        // PCI Interrupt Routing Configuration Registers
        //
        OperationRegion (PRR0, PCI_Config, 0x60, 0x04)
        Field (PRR0, ANYACC, NOLOCK, PRESERVE) {
          PIRA, 8,
          PIRB, 8,
          PIRC, 8,
          PIRD, 8
        }

        //
        // _STA method for LNKA, LNKB, LNKC, LNKD
        //
        Method (PSTA, 1, NotSerialized) {
          If (And (Arg0, 0x80)) {
            Return (0x9)
          } Else {
            Return (0xB)
          }
        }

        //
        // _DIS method for LNKA, LNKB, LNKC, LNKD
        //
        Method (PDIS, 1, NotSerialized) {
          Or (Arg0, 0x80, Arg0)
        }

        //
        // _CRS method for LNKA, LNKB, LNKC, LNKD
        //
        Method (PCRS, 1, NotSerialized) {
          Name (BUF0, ResourceTemplate () {IRQ (Level, ActiveLow, Shared){0}})
          //
          // Define references to buffer elements
          //
          CreateWordField (BUF0, 0x01, IRQW)  // IRQ low
          //
          // Write current settings into IRQ descriptor
          //
          If (And (Arg0, 0x80)) {
            Store (Zero, Local0)
          } Else {
            Store (One, Local0)
          }
          //
          // Shift 1 by value in register 70
          //
          ShiftLeft (Local0, And (Arg0, 0x0F), IRQW)   // Save in buffer
          Return (BUF0)                                // Return Buf0 
        }

        //
        // _PRS resource for LNKA, LNKB, LNKC, LNKD
        //
        Name (PPRS, ResourceTemplate () {
          IRQ (Level, ActiveLow, Shared) {3, 4, 5, 7, 9, 10, 11, 12, 14, 15}
        })

        //
        // _SRS method for LNKA, LNKB, LNKC, LNKD
        //
        Method (PSRS, 2, NotSerialized) {
          CreateWordField (Arg1, 0x01, IRQW)      // IRQ low
          FindSetRightBit (IRQW, Local0)          // Set IRQ
          If (LNotEqual (IRQW, Zero)) {
            And (Local0, 0x7F, Local0)
            Decrement (Local0)
          } Else {
            Or (Local0, 0x80, Local0)
          }
          Store (Local0, Arg0)
        }

        //
        // PCI IRQ Link A
        //
        Device (LNKA) {
          Name (_HID, EISAID("PNP0C0F"))
          Name (_UID, 1)

          Method (_STA, 0, NotSerialized) { Return (PSTA (PIRA)) }
          Method (_DIS, 0, NotSerialized) { PDIS (PIRA)  }
          Method (_CRS, 0, NotSerialized) { Return (PCRS (PIRA)) }
          Method (_PRS, 0, NotSerialized) { Return (PPRS) }
          Method (_SRS, 1, NotSerialized) { PSRS (PIRA, Arg0) } 
        }

        //
        // PCI IRQ Link B
        //
        Device (LNKB) {
          Name (_HID, EISAID("PNP0C0F"))
          Name (_UID, 2)

          Method (_STA, 0, NotSerialized) { Return (PSTA (PIRB)) }
          Method (_DIS, 0, NotSerialized) { PDIS (PIRB) }
          Method (_CRS, 0, NotSerialized) { Return (PCRS (PIRB)) }
          Method (_PRS, 0, NotSerialized) { Return (PPRS) }
          Method (_SRS, 1, NotSerialized) { PSRS (PIRB, Arg0) } 
        }

        //
        // PCI IRQ Link C
        //
        Device (LNKC) {
          Name (_HID, EISAID("PNP0C0F"))
          Name (_UID, 3)

          Method (_STA, 0, NotSerialized) { Return (PSTA (PIRC)) }
          Method (_DIS, 0, NotSerialized) { PDIS (PIRC) }
          Method (_CRS, 0, NotSerialized) { Return (PCRS (PIRC)) }
          Method (_PRS, 0, NotSerialized) { Return (PPRS) }
          Method (_SRS, 1, NotSerialized) { PSRS (PIRC, Arg0) } 
        }

        //
        // PCI IRQ Link D
        //
        Device (LNKD) {
          Name (_HID, EISAID("PNP0C0F"))
          Name (_UID, 1)

          Method (_STA, 0, NotSerialized) { Return (PSTA (PIRD)) }
          Method (_DIS, 0, NotSerialized) { PDIS (PIRD) }
          Method (_CRS, 0, NotSerialized) { Return (PCRS (PIRD)) }
          Method (_PRS, 0, NotSerialized) { Return (PPRS) }
          Method (_SRS, 1, NotSerialized) { PSRS (PIRD, Arg0) } 
        }
        
        //
        // Programmable Interrupt Controller (PIC)
        //
        Device(PIC) {
          Name (_HID, EISAID ("PNP0000"))
          Name (_CRS, ResourceTemplate () {
            IO (Decode16, 0x020, 0x020, 0x00, 0x02)
            IO (Decode16, 0x0A0, 0x0A0, 0x00, 0x02)
            IO (Decode16, 0x4D0, 0x4D0, 0x00, 0x02)
            IRQNoFlags () {2}
          })
        }

        //
        // ISA DMA 
        //
        Device (DMAC) {
          Name (_HID, EISAID ("PNP0200")) 
          Name (_CRS, ResourceTemplate () {
            IO (Decode16, 0x00, 0x00, 0, 0x10)
            IO (Decode16, 0x81, 0x81, 0, 0x03)
            IO (Decode16, 0x87, 0x87, 0, 0x01)
            IO (Decode16, 0x89, 0x89, 0, 0x03)
            IO (Decode16, 0x8f, 0x8f, 0, 0x01)
            IO (Decode16, 0xc0, 0xc0, 0, 0x20)
            DMA (Compatibility, NotBusMaster, Transfer8) {4}
          })
        }

        //
        // 8254 Timer
        //
        Device(TMR) {
          Name(_HID,EISAID("PNP0100"))
          Name(_CRS, ResourceTemplate () {
            IO (Decode16, 0x40, 0x40, 0x00, 0x04)
            IRQNoFlags () {0}
          })
        }

        //
        // Real Time Clock
        //
        Device (RTC) {
          Name (_HID, EISAID ("PNP0B00"))
          Name (_CRS, ResourceTemplate () {
            IO (Decode16, 0x70, 0x70, 0x00, 0x02)
            IRQNoFlags () {8}
          })
        }

        //
        // PCAT Speaker
        //
        Device(SPKR) {
          Name (_HID, EISAID("PNP0800"))
          Name (_CRS, ResourceTemplate () {
            IO (Decode16, 0x61, 0x61, 0x01, 0x01)
          })
        }

        //
        // Floating Point Coprocessor
        //
        Device(FPU) {
          Name (_HID, EISAID("PNP0C04"))
          Name (_CRS, ResourceTemplate () {
            IO (Decode16, 0xF0, 0xF0, 0x00, 0x10)
            IRQNoFlags () {13}
          })
        }

        //
        // Generic motherboard devices and pieces that don't fit anywhere else
        //
        Device(XTRA) {
          Name (_HID, EISAID ("PNP0C02"))
          Name (_UID, 0x01)
          Name (_CRS, ResourceTemplate () {
            IO (Decode16, 0x010, 0x010, 0x00, 0x10)
            IO (Decode16, 0x022, 0x022, 0x00, 0x1E)
            IO (Decode16, 0x044, 0x044, 0x00, 0x1C)
            IO (Decode16, 0x062, 0x062, 0x00, 0x02)
            IO (Decode16, 0x065, 0x065, 0x00, 0x0B)
            IO (Decode16, 0x072, 0x072, 0x00, 0x0E)
            IO (Decode16, 0x080, 0x080, 0x00, 0x01)
            IO (Decode16, 0x084, 0x084, 0x00, 0x03)
            IO (Decode16, 0x088, 0x088, 0x00, 0x01)
            IO (Decode16, 0x08c, 0x08c, 0x00, 0x03)
            IO (Decode16, 0x090, 0x090, 0x00, 0x10)
            IO (Decode16, 0x0A2, 0x0A2, 0x00, 0x1E)
            IO (Decode16, 0x0E0, 0x0E0, 0x00, 0x10)
            IO (Decode16, 0x1E0, 0x1E0, 0x00, 0x10)
            IO (Decode16, 0x160, 0x160, 0x00, 0x10)
            IO (Decode16, 0x278, 0x278, 0x00, 0x08)
            IO (Decode16, 0x370, 0x370, 0x00, 0x02)
            IO (Decode16, 0x378, 0x378, 0x00, 0x08)
            IO (Decode16, 0x400, 0x400, 0x00, 0x40)       // PMBLK1
            IO (Decode16, 0x440, 0x440, 0x00, 0x10)
            IO (Decode16, 0x678, 0x678, 0x00, 0x08)
            IO (Decode16, 0x778, 0x778, 0x00, 0x08)
            Memory32Fixed (ReadOnly, 0xFEC00000, 0x1000)  // IO APIC
            Memory32Fixed (ReadOnly, 0xFEE00000, 0x1000)
          })
        }

        //
        // PS/2 Keyboard and PC/AT Enhanced Keyboard 101/102
        //
        Device (PS2K) {  
          Name (_HID, EISAID ("PNP0303"))
          Name (_CID, EISAID ("PNP030B"))
          Name(_CRS,ResourceTemplate() {
            IO (Decode16, 0x60, 0x60, 0x00, 0x01)
            IO (Decode16, 0x64, 0x64, 0x00, 0x01)
            IRQNoFlags () {1}
          })
        }

        //
        // PS/2 Mouse and Microsoft Mouse
        //
        Device (PS2M) {  // PS/2 stype mouse port
          Name (_HID, EISAID ("PNP0F03"))
          Name (_CID, EISAID ("PNP0F13"))
          Name (_CRS, ResourceTemplate() {
            IRQNoFlags () {12}
          })
        }

        //
        // UART Serial Port - COM1
        //
        Device (UAR1) {
          Name (_HID, EISAID ("PNP0501"))
          Name (_DDN, "COM1")
          Name (_UID, 0x01)
          Name(_CRS,ResourceTemplate() {
            IO (Decode16, 0x3F8, 0x3F8, 0x01, 0x08)
            IRQ (Edge, ActiveHigh, Exclusive, ) {4}
          })
        }

        //
        // UART Serial Port - COM2
        //
        Device (UAR2) {
          Name (_HID, EISAID ("PNP0501"))
          Name (_DDN, "COM2")
          Name (_UID, 0x02)
          Name(_CRS,ResourceTemplate() {
            IO (Decode16, 0x2F8, 0x2F8, 0x01, 0x08)
            IRQ (Edge, ActiveHigh, Exclusive, ) {3}
          })
        }

        //
        // Floppy Disk Controller
        //
        Device (FDC) {
          Name (_HID, EISAID ("PNP0700"))
          Name (_CRS,ResourceTemplate() {
            IO (Decode16, 0x3F0, 0x3F0, 0x01, 0x06)
            IO (Decode16, 0x3F7, 0x3F7, 0x01, 0x01)
            IRQNoFlags () {6}
            DMA (Compatibility, NotBusMaster, Transfer8) {2}
          })
        }
      }
    }
  }
}

