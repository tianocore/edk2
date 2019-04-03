/** @file
  Contains root level name space objects for the platform

  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock ("Dsdt.aml", "DSDT", 1, "INTEL ", "OVMF    ", 4) {
  //
  // System Sleep States
  //
  // We build S3 and S4 with GetSuspendStates() in
  // "OvmfPkg/AcpiPlatformDxe/Qemu.c".
  //
  Name (\_S0, Package () {5, 0, 0, 0}) // Working
  Name (\_S5, Package () {0, 0, 0, 0}) // Soft Off

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
      Name (CRES, ResourceTemplate () {
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

        DWORDMEMORY (            // Descriptor for 32-bit MMIO
          ResourceProducer,      // bit 0 of general flags is 0
          PosDecode,
          MinFixed,              // Range is fixed
          MaxFixed,              // Range is Fixed
          NonCacheable,
          ReadWrite,
          0x00000000,            // Granularity
          0xF8000000,            // Min
          0xFFFBFFFF,            // Max
          0x00000000,            // Translation
          0x07FC0000,            // Range Length
          ,                      // ResourceSourceIndex
          ,                      // ResourceSource
          PW32                   // DescriptorName
          )
      })

      Name (CR64, ResourceTemplate () {
        QWordMemory (            // Descriptor for 64-bit MMIO
            ResourceProducer,    // bit 0 of general flags is 0
            PosDecode,
            MinFixed,            // Range is fixed
            MaxFixed,            // Range is Fixed
            Cacheable,
            ReadWrite,
            0x00000000,          // Granularity
            0x8000000000,        // Min
            0xFFFFFFFFFF,        // Max
            0x00000000,          // Translation
            0x8000000000,        // Range Length
            ,                    // ResourceSourceIndex
            ,                    // ResourceSource
            PW64                 // DescriptorName
            )
      })

      Method (_CRS, 0, Serialized) {
        //
        // see the FIRMWARE_DATA structure in "OvmfPkg/AcpiPlatformDxe/Qemu.c"
        //
        External (FWDT, OpRegionObj)
        Field(FWDT, QWordAcc, NoLock, Preserve) {
          P0S, 64,               // PciWindow32.Base
          P0E, 64,               // PciWindow32.End
          P0L, 64,               // PciWindow32.Length
          P1S, 64,               // PciWindow64.Base
          P1E, 64,               // PciWindow64.End
          P1L, 64                // PciWindow64.Length
        }
        Field(FWDT, DWordAcc, NoLock, Preserve) {
          P0SL, 32,              // PciWindow32.Base,   low  32 bits
          P0SH, 32,              // PciWindow32.Base,   high 32 bits
          P0EL, 32,              // PciWindow32.End,    low  32 bits
          P0EH, 32,              // PciWindow32.End,    high 32 bits
          P0LL, 32,              // PciWindow32.Length, low  32 bits
          P0LH, 32,              // PciWindow32.Length, high 32 bits
          P1SL, 32,              // PciWindow64.Base,   low  32 bits
          P1SH, 32,              // PciWindow64.Base,   high 32 bits
          P1EL, 32,              // PciWindow64.End,    low  32 bits
          P1EH, 32,              // PciWindow64.End,    high 32 bits
          P1LL, 32,              // PciWindow64.Length, low  32 bits
          P1LH, 32               // PciWindow64.Length, high 32 bits
        }

        //
        // fixup 32-bit PCI IO window
        //
        CreateDWordField (CRES, \_SB.PCI0.PW32._MIN, PS32)
        CreateDWordField (CRES, \_SB.PCI0.PW32._MAX, PE32)
        CreateDWordField (CRES, \_SB.PCI0.PW32._LEN, PL32)
        Store (P0SL, PS32)
        Store (P0EL, PE32)
        Store (P0LL, PL32)

        If (LAnd (LEqual (P1SL, 0x00), LEqual (P1SH, 0x00))) {
          Return (CRES)
        } Else {
          //
          // fixup 64-bit PCI IO window
          //
          CreateQWordField (CR64, \_SB.PCI0.PW64._MIN, PS64)
          CreateQWordField (CR64, \_SB.PCI0.PW64._MAX, PE64)
          CreateQWordField (CR64, \_SB.PCI0.PW64._LEN, PL64)
          Store (P1S, PS64)
          Store (P1E, PE64)
          Store (P1L, PL64)

          //
          // add window and return result
          //
          ConcatenateResTemplate (CRES, CR64, Local0)
          Return (Local0)
        }
      }

      //
      // PCI Interrupt Routing Table - PIC Mode Only
      //
      Method (_PRT, 0, NotSerialized) {
        Return (
          Package () {
            //
            // Bus 0; Devices 0 to 15
            //
            Package () {0x0000FFFF, 0x00, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x0000FFFF, 0x01, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x0000FFFF, 0x02, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x0000FFFF, 0x03, \_SB.PCI0.LPC.LNKC, 0x00},

            //
            // Bus 0, Device 1, Pin 0 (INTA) is special; it corresponds to the
            // internally generated SCI (System Control Interrupt), which is
            // always routed to GSI 9. By setting the third (= Source) field to
            // zero, we could use the fourth (= Source Index) field to hardwire
            // the pin to GSI 9 directly.
            //
            // That way however, in accordance with the ACPI spec's description
            // of SCI, the interrupt would be treated as "active low,
            // shareable, level", and that doesn't match qemu.
            //
            // In QemuInstallAcpiMadtTable() [OvmfPkg/AcpiPlatformDxe/Qemu.c]
            // we install an Interrupt Override Structure for the identity
            // mapped IRQ#9 / GSI 9 (the corresponding bit being set in
            // Pcd8259LegacyModeEdgeLevel), which describes the correct
            // polarity (active high). As a consequence, some OS'en (eg. Linux)
            // override the default (active low) polarity originating from the
            // _PRT; others (eg. FreeBSD) don't. Therefore we need a separate
            // link device just to specify a polarity that matches the MADT.
            //
            Package () {0x0001FFFF, 0x00, \_SB.PCI0.LPC.LNKS, 0x00},

            Package () {0x0001FFFF, 0x01, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x0001FFFF, 0x02, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x0001FFFF, 0x03, \_SB.PCI0.LPC.LNKD, 0x00},

            Package () {0x0002FFFF, 0x00, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x0002FFFF, 0x01, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x0002FFFF, 0x02, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x0002FFFF, 0x03, \_SB.PCI0.LPC.LNKA, 0x00},

            Package () {0x0003FFFF, 0x00, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x0003FFFF, 0x01, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x0003FFFF, 0x02, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x0003FFFF, 0x03, \_SB.PCI0.LPC.LNKB, 0x00},

            Package () {0x0004FFFF, 0x00, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x0004FFFF, 0x01, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x0004FFFF, 0x02, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x0004FFFF, 0x03, \_SB.PCI0.LPC.LNKC, 0x00},

            Package () {0x0005FFFF, 0x00, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x0005FFFF, 0x01, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x0005FFFF, 0x02, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x0005FFFF, 0x03, \_SB.PCI0.LPC.LNKD, 0x00},

            Package () {0x0006FFFF, 0x00, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x0006FFFF, 0x01, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x0006FFFF, 0x02, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x0006FFFF, 0x03, \_SB.PCI0.LPC.LNKA, 0x00},

            Package () {0x0007FFFF, 0x00, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x0007FFFF, 0x01, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x0007FFFF, 0x02, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x0007FFFF, 0x03, \_SB.PCI0.LPC.LNKB, 0x00},

            Package () {0x0008FFFF, 0x00, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x0008FFFF, 0x01, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x0008FFFF, 0x02, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x0008FFFF, 0x03, \_SB.PCI0.LPC.LNKC, 0x00},

            Package () {0x0009FFFF, 0x00, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x0009FFFF, 0x01, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x0009FFFF, 0x02, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x0009FFFF, 0x03, \_SB.PCI0.LPC.LNKD, 0x00},

            Package () {0x000AFFFF, 0x00, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x000AFFFF, 0x01, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x000AFFFF, 0x02, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x000AFFFF, 0x03, \_SB.PCI0.LPC.LNKA, 0x00},

            Package () {0x000BFFFF, 0x00, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x000BFFFF, 0x01, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x000BFFFF, 0x02, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x000BFFFF, 0x03, \_SB.PCI0.LPC.LNKB, 0x00},

            Package () {0x000CFFFF, 0x00, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x000CFFFF, 0x01, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x000CFFFF, 0x02, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x000CFFFF, 0x03, \_SB.PCI0.LPC.LNKC, 0x00},

            Package () {0x000DFFFF, 0x00, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x000DFFFF, 0x01, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x000DFFFF, 0x02, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x000DFFFF, 0x03, \_SB.PCI0.LPC.LNKD, 0x00},

            Package () {0x000EFFFF, 0x00, \_SB.PCI0.LPC.LNKB, 0x00},
            Package () {0x000EFFFF, 0x01, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x000EFFFF, 0x02, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x000EFFFF, 0x03, \_SB.PCI0.LPC.LNKA, 0x00},

            Package () {0x000FFFFF, 0x00, \_SB.PCI0.LPC.LNKC, 0x00},
            Package () {0x000FFFFF, 0x01, \_SB.PCI0.LPC.LNKD, 0x00},
            Package () {0x000FFFFF, 0x02, \_SB.PCI0.LPC.LNKA, 0x00},
            Package () {0x000FFFFF, 0x03, \_SB.PCI0.LPC.LNKB, 0x00}
          }
        )
      }

      //
      // PCI to ISA Bridge (Bus 0, Device 1, Function 0)
      // "Low Pin Count"
      //
      Device (LPC) {
        Name (_ADR, 0x00010000)

        //
        // The SCI cannot be rerouted or disabled with PIRQRC[A:D]; we only
        // need this link device in order to specify the polarity.
        //
        Device (LNKS) {
          Name (_HID, EISAID("PNP0C0F"))
          Name (_UID, 0)

          Name (_STA, 0xB) // 0x1: device present
                           // 0x2: enabled and decoding resources
                           // 0x8: functioning properly

          Method (_SRS, 1, NotSerialized) { /* no-op */ }
          Method (_DIS, 0, NotSerialized) { /* no-op */ }

          Name (_PRS, ResourceTemplate () {
            Interrupt (ResourceConsumer, Level, ActiveHigh, Shared) { 9 }
            //
            // list of IRQs occupied thus far: 9
            //
          })
          Method (_CRS, 0, NotSerialized) { Return (_PRS) }
        }

        //
        // PCI Interrupt Routing Configuration Registers, PIRQRC[A:D]
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
        // Arg0[in]: value of PIRA / PIRB / PIRC / PIRD
        //
        Method (PSTA, 1, NotSerialized) {
          If (And (Arg0, 0x80)) { // disable-bit set?
            Return (0x9)          // "device present" | "functioning properly"
          } Else {
            Return (0xB)          // same | "enabled and decoding resources"
          }
        }

        //
        // _CRS method for LNKA, LNKB, LNKC, LNKD
        // Arg0[in]: value of PIRA / PIRB / PIRC / PIRD
        //
        Method (PCRS, 1, Serialized) {
          //
          // create temporary buffer with an Extended Interrupt Descriptor
          // whose single vector defaults to zero
          //
          Name (BUF0, ResourceTemplate () {
              Interrupt (ResourceConsumer, Level, ActiveHigh, Shared){0}
            }
          )

          //
          // define reference to first interrupt vector in buffer
          //
          CreateDWordField (BUF0, 0x05, IRQW)

          //
          // If the disable-bit is clear, overwrite the default zero vector
          // with the value in Arg0 (ie. PIRQRC[A:D]). Reserved bits are read
          // as 0.
          //
          If (LNot (And (Arg0, 0x80))) {
            Store (Arg0, IRQW)
          }
          Return (BUF0)
        }

        //
        // _PRS resource for LNKA, LNKB, LNKC, LNKD
        //
        Name (PPRS, ResourceTemplate () {
          Interrupt (ResourceConsumer, Level, ActiveHigh, Shared) {5, 10, 11}
          //
          // list of IRQs occupied thus far: 9, 5, 10, 11
          //
        })

        //
        // PCI IRQ Link A
        //
        Device (LNKA) {
          Name (_HID, EISAID("PNP0C0F"))
          Name (_UID, 1)

          Method (_STA, 0, NotSerialized) { Return (PSTA (PIRA)) }
          Method (_DIS, 0, NotSerialized) {
            Or (PIRA, 0x80, PIRA) // set disable-bit
          }
          Method (_CRS, 0, NotSerialized) { Return (PCRS (PIRA)) }
          Method (_PRS, 0, NotSerialized) { Return (PPRS) }
          Method (_SRS, 1, NotSerialized) {
            CreateDWordField (Arg0, 0x05, IRQW)
            Store (IRQW, PIRA)
          }
        }

        //
        // PCI IRQ Link B
        //
        Device (LNKB) {
          Name (_HID, EISAID("PNP0C0F"))
          Name (_UID, 2)

          Method (_STA, 0, NotSerialized) { Return (PSTA (PIRB)) }
          Method (_DIS, 0, NotSerialized) {
            Or (PIRB, 0x80, PIRB) // set disable-bit
          }
          Method (_CRS, 0, NotSerialized) { Return (PCRS (PIRB)) }
          Method (_PRS, 0, NotSerialized) { Return (PPRS) }
          Method (_SRS, 1, NotSerialized) {
            CreateDWordField (Arg0, 0x05, IRQW)
            Store (IRQW, PIRB)
          }
        }

        //
        // PCI IRQ Link C
        //
        Device (LNKC) {
          Name (_HID, EISAID("PNP0C0F"))
          Name (_UID, 3)

          Method (_STA, 0, NotSerialized) { Return (PSTA (PIRC)) }
          Method (_DIS, 0, NotSerialized) {
            Or (PIRC, 0x80, PIRC) // set disable-bit
          }
          Method (_CRS, 0, NotSerialized) { Return (PCRS (PIRC)) }
          Method (_PRS, 0, NotSerialized) { Return (PPRS) }
          Method (_SRS, 1, NotSerialized) {
            CreateDWordField (Arg0, 0x05, IRQW)
            Store (IRQW, PIRC)
          }
        }

        //
        // PCI IRQ Link D
        //
        Device (LNKD) {
          Name (_HID, EISAID("PNP0C0F"))
          Name (_UID, 4)

          Method (_STA, 0, NotSerialized) { Return (PSTA (PIRD)) }
          Method (_DIS, 0, NotSerialized) {
            Or (PIRD, 0x80, PIRD) // set disable-bit
          }
          Method (_CRS, 0, NotSerialized) { Return (PCRS (PIRD)) }
          Method (_PRS, 0, NotSerialized) { Return (PPRS) }
          Method (_SRS, 1, NotSerialized) {
            CreateDWordField (Arg0, 0x05, IRQW)
            Store (IRQW, PIRD)
          }
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
            //
            // list of IRQs occupied thus far: 9, 5, 10, 11, 2
            //
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
            //
            // list of IRQs occupied thus far: 9, 5, 10, 11, 2, 0
            //
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
            //
            // list of IRQs occupied thus far: 9, 5, 10, 11, 2, 0, 8
            //
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
            //
            // list of IRQs occupied thus far: 9, 5, 10, 11, 2, 0, 8, 13
            //
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
            IO (Decode16, FixedPcdGet16 (PcdDebugIoPort), FixedPcdGet16 (PcdDebugIoPort), 0x00, 0x01)
            IO (Decode16, 0x440, 0x440, 0x00, 0x10)
            IO (Decode16, 0x678, 0x678, 0x00, 0x08)
            IO (Decode16, 0x778, 0x778, 0x00, 0x08)
            IO (Decode16, 0xafe0, 0xafe0, 0x00, 0x04)      // QEMU GPE0 BLK
            IO (Decode16, 0xb000, 0xb000, 0x00, 0x40)      // PMBLK1
            Memory32Fixed (ReadOnly, 0xFEC00000, 0x1000)   // IO APIC
            Memory32Fixed (ReadOnly, 0xFEE00000, 0x100000) // LAPIC
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
            //
            // list of IRQs occupied thus far: 9, 5, 10, 11, 2, 0, 8, 13, 1
            //
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
            //
            // list of IRQs occupied thus far:
            // 9, 5, 10, 11, 2, 0, 8, 13, 1, 12
            //
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
            //
            // list of IRQs occupied thus far:
            // 9, 5, 10, 11, 2, 0, 8, 13, 1, 12, 4
            //
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
            //
            // list of IRQs occupied thus far:
            // 9, 5, 10, 11, 2, 0, 8, 13, 1, 12, 4, 3
            //
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
            //
            // list of IRQs occupied thus far:
            // 9, 5, 10, 11, 2, 0, 8, 13, 1, 12, 4, 3, 6
            //
            DMA (Compatibility, NotBusMaster, Transfer8) {2}
          })
        }

        //
        // parallel port -- no DMA for now
        //
        Device (PAR1) {
          Name (_HID, EISAID ("PNP0400"))
          Name (_DDN, "LPT1")
          Name (_UID, 0x01)
          Name(_CRS, ResourceTemplate() {
            IO (Decode16, 0x0378, 0x0378, 0x00, 0x08)
            IRQNoFlags () {7}
            //
            // list of IRQs occupied thus far:
            // 9, 5, 10, 11, 2, 0, 8, 13, 1, 12, 4, 3, 6, 7
            // in order:
            // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
            //
          })
        }
      }
    }
  }
}
