/** @file
  Differentiated System Description Table Fields (SSDT)

  Copyright (c) 2014-2015, ARM Ltd. All rights reserved.<BR>
    This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ArmPlatform.h"

/*
  See Reference [1] 6.2.12
  "There are two ways that _PRT can be used. ...
  In the second model, the PCI interrupts are hardwired to specific interrupt
  inputs on the interrupt controller and are not configurable. In this case,
  the Source field in _PRT does not reference a device, but instead contains
  the value zero, and the Source Index field contains the global system
  interrupt to which the PCI interrupt is hardwired."
*/
#define PRT_ENTRY(Address, Pin, Interrupt)                                                       \
          Package (4) {                                                                           \
            Address,    /* uses the same format as _ADR */                                        \
            Pin,        /* The PCI pin number of the device (0-INTA, 1-INTB, 2-INTC, 3-INTD). */  \
            Zero,       /* allocated from the global interrupt pool. */                           \
            Interrupt   /* global system interrupt number */                                      \
          }

/*
  See Reference [1] 6.1.1
  "High word–Device #, Low word–Function #. (for example, device 3, function 2 is
   0x00030002). To refer to all the functions on a device #, use a function number of FFFF)."
*/
#define ROOT_PRT_ENTRY(Pin, Interrupt)   PRT_ENTRY(0x0000FFFF, Pin, Interrupt)
                                                    // Device 0 for Bridge.


DefinitionBlock("SsdtPci.aml", "SSDT", 1, "ARMLTD", "ARM-JUNO", EFI_ACPI_ARM_OEM_REVISION) {
  Scope(_SB) {
	//
	// PCI Root Complex
	//
	Device(PCI0)
    {
  		Name(_HID, EISAID("PNP0A08")) // PCI Express Root Bridge
  		Name(_CID, EISAID("PNP0A03")) // Compatible PCI Root Bridge
  		Name(_SEG, Zero) // PCI Segment Group number
  		Name(_BBN, Zero) // PCI Base Bus Number

        // Root Complex 0
        Device (RP0) {
            Name(_ADR, 0xF0000000)    // Dev 0, Func 0
        }

        // PCI Routing Table
  		Name(_PRT, Package() {
        	ROOT_PRT_ENTRY(0, 136),   // INTA
        	ROOT_PRT_ENTRY(1, 137),   // INTB
        	ROOT_PRT_ENTRY(2, 138),   // INTC
        	ROOT_PRT_ENTRY(3, 139),   // INTD
      	})
        // Root complex resources
		Method (_CRS, 0, Serialized) {
  			Name (RBUF, ResourceTemplate () {
				WordBusNumber ( // Bus numbers assigned to this root
					ResourceProducer,
					MinFixed, MaxFixed, PosDecode,
					0,   // AddressGranularity
					0,   // AddressMinimum - Minimum Bus Number
					255, // AddressMaximum - Maximum Bus Number
					0,   // AddressTranslation - Set to 0
					256  // RangeLength - Number of Busses
				)

				DWordMemory ( // 32-bit BAR Windows
					ResourceProducer, PosDecode,
					MinFixed, MaxFixed,
					Cacheable, ReadWrite,
					0x00000000, 							// Granularity
					0x50000000, 							// Min Base Address
					0x57FFFFFF, 							// Max Base Address
					0x00000000, 							// Translate
					0x08000000								// Length
				)
  		
				QWordMemory ( // 64-bit BAR Windows
					ResourceProducer, PosDecode,
					MinFixed, MaxFixed,
					Cacheable, ReadWrite,
					0x00000000, 							// Granularity
					0x4000000000, 							// Min Base Address
					0x40FFFFFFFF, 							// Max Base Address
					0x00000000, 							// Translate
					0x100000000								// Length
				)
			}) // Name(RBUF)
			
			Return (RBUF)
		} // Method(_CRS)

		//
		// OS Control Handoff
		//
  		Name(SUPP, Zero) // PCI _OSC Support Field value
  		Name(CTRL, Zero) // PCI _OSC Control Field value

  		/*
     	  See [1] 6.2.10, [2] 4.5
   		*/
		Method(_OSC,4) {
			// Check for proper UUID
			If(LEqual(Arg0,ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766"))) {
				// Create DWord-adressable fields from the Capabilities Buffer
				CreateDWordField(Arg3,0,CDW1)
				CreateDWordField(Arg3,4,CDW2)
				CreateDWordField(Arg3,8,CDW3)

				// Save Capabilities DWord2 & 3
				Store(CDW2,SUPP)
				Store(CDW3,CTRL)

				// Only allow native hot plug control if OS supports:
				// * ASPM
				// * Clock PM
				// * MSI/MSI-X
				If(LNotEqual(And(SUPP, 0x16), 0x16)) {
					And(CTRL,0x1E,CTRL) // Mask bit 0 (and undefined bits)
				}

				// Always allow native PME, AER (no dependencies)

				// Never allow SHPC (no SHPC controller in this system)
				And(CTRL,0x1D,CTRL)

#if 0
				If(LNot(And(CDW1,1))) {		// Query flag clear?
					// Disable GPEs for features granted native control.
					If(And(CTRL,0x01)) {	// Hot plug control granted?
						Store(0,HPCE)		// clear the hot plug SCI enable bit
						Store(1,HPCS)		// clear the hot plug SCI status bit
					}
					If(And(CTRL,0x04)) {	// PME control granted?
						Store(0,PMCE)		// clear the PME SCI enable bit
						Store(1,PMCS)		// clear the PME SCI status bit
					}
					If(And(CTRL,0x10)) {	// OS restoring PCIe cap structure?
						// Set status to not restore PCIe cap structure
						// upon resume from S3
						Store(1,S3CR)
					}
				}
#endif

				If(LNotEqual(Arg1,One)) {	// Unknown revision
					Or(CDW1,0x08,CDW1)
				}

				If(LNotEqual(CDW3,CTRL)) {	// Capabilities bits were masked
					Or(CDW1,0x10,CDW1)
				}
				// Update DWORD3 in the buffer
				Store(CTRL,CDW3)
				Return(Arg3)
			} Else {
				Or(CDW1,4,CDW1) // Unrecognized UUID
				Return(Arg3)
			}
		} // End _OSC
    } // PCI0
  }
}
