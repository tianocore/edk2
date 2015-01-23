/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2014-2015, ARM Ltd. All rights reserved.<BR>
    This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ArmPlatform.h"

DefinitionBlock("DsdtTable.aml", "DSDT", 1, "ARMLTD", "ARM-JUNO", EFI_ACPI_ARM_OEM_REVISION) {
  Scope(_SB) {
    //
    // A57x2-A53x4 Processor declaration
    //
    Device(CPU0) { // A53-0: Cluster 1, Cpu 0
      Name(_HID, "ACPI0007")
      Name(_UID, 0)
    }
    Device(CPU1) { // A53-1: Cluster 1, Cpu 1
      Name(_HID, "ACPI0007")
      Name(_UID, 1)
    }
    Device(CPU2) { // A53-2: Cluster 1, Cpu 2
      Name(_HID, "ACPI0007")
      Name(_UID, 2)
    }
    Device(CPU3) { // A53-3: Cluster 1, Cpu 3
      Name(_HID, "ACPI0007")
      Name(_UID, 3)
    }
    Device(CPU4) { // A57-0: Cluster 0, Cpu 0
      Name(_HID, "ACPI0007")
      Name(_UID, 4)
    }
    Device(CPU5) { // A57-1: Cluster 0, Cpu 1
      Name(_HID, "ACPI0007")
      Name(_UID, 5)
    }

    //
    // Keyboard and Mouse
    //
    Device(KMI0) {
      Name(_HID, "ARMH0501")
      Name(_CID, "PL050_KBD")
      Name(_CRS, ResourceTemplate() {
              Memory32Fixed(ReadWrite, 0x1C060008, 0x4)
              Memory32Fixed(ReadWrite, 0x1C060000, 0x4)
              Memory32Fixed(ReadOnly, 0x1C060004, 0x4)
              Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 197 }
      })
    }

    //
    // LAN9118 Ethernet
    //
    Device(ETH0) {
      Name(_HID, "ARMH9118")
      Name(_UID, Zero)
      Name(_CRS, ResourceTemplate() {
              Memory32Fixed(ReadWrite, 0x1A000000, 0x1000)
              Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 192 }
      })
    }

    // UART PL011
    Device(COM0) {
      Name(_HID, "ARMH0011")
      Name(_CID, "PL011")
      Name(_UID, Zero)
      Name(_CRS, ResourceTemplate() {
        Memory32Fixed(ReadWrite, 0x7FF80000, 0x1000)
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { 115 }
      })
    }

    //
    // USB Host Controller
    //
    Device(USB0){
        Name(_HID, "ARMH0D20")
        Name(_CID, "PNP0D20")
        Name(_UID, 2)

        Method(_CRS, 0x0, Serialized){
            Name(RBUF, ResourceTemplate(){
                Memory32Fixed(ReadWrite, 0x7FFC0000, 0x000000B0)
                Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) {149}  // INT ID=149 GIC IRQ ID=117 for Juno SoC USB EHCI Controller
            })
            Return(RBUF)
        }

        //
        // Root Hub
        //
        Device(RHUB){
            Name(_ADR, 0x00000000)  // Address of Root Hub should be 0 as per ACPI 5.0 spec

            //
            // Ports connected to Root Hub
            //
            Device(HUB1){
                Name(_ADR, 0x00000001)
                Name(_UPC, Package(){
                    0x00,       // Port is NOT connectable
                    0xFF,       // Don't care
                    0x00000000, // Reserved 0 must be zero
                    0x00000000  // Reserved 1 must be zero
                })

                Device(PRT1){
                    Name(_ADR, 0x00000001)
                    Name(_UPC, Package(){
                        0xFF,        // Port is connectable
                        0x00,        // Port connector is A
                        0x00000000,
                        0x00000000
                    })
                    Name(_PLD, Package(){
                        Buffer(0x10){
                            0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x31, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                        }
                    })
                } // USB0_RHUB_HUB1_PRT1
                Device(PRT2){
                    Name(_ADR, 0x00000002)
                    Name(_UPC, Package(){
                        0xFF,        // Port is connectable
                        0x00,        // Port connector is A
                        0x00000000,
                        0x00000000
                    })
                    Name(_PLD, Package(){
                        Buffer(0x10){
                            0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x31, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                        }
                    })
                } // USB0_RHUB_HUB1_PRT2

                Device(PRT3){
                    Name(_ADR, 0x00000003)
                    Name(_UPC, Package(){
                        0xFF,        // Port is connectable
                        0x00,        // Port connector is A
                        0x00000000,
                        0x00000000
                    })
                    Name(_PLD, Package(){
                        Buffer(0x10){
                            0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x31, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                        }
                    })
                } // USB0_RHUB_HUB1_PRT3

                Device(PRT4){
                    Name(_ADR, 0x00000004)
                    Name(_UPC, Package(){
                        0xFF,        // Port is connectable
                        0x00,        // Port connector is A
                        0x00000000,
                        0x00000000
                    })
                    Name(_PLD, Package(){
                        Buffer(0x10){
                            0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x31, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                        }
                    })
                } // USB0_RHUB_HUB1_PRT4
            } // USB0_RHUB_HUB1
        } // USB0_RHUB
    } // USB0
  } // Scope(_SB)
}
