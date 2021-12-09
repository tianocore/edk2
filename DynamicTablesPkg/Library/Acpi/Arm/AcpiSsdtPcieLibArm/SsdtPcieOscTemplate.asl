/** @file
  SSDT Pci Osc (Operating System Capabilities)

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - PCI Firmware Specification - Revision 3.3
  - ACPI 6.4 specification:
   - s6.2.13 "_PRT (PCI Routing Table)"
   - s6.1.1 "_ADR (Address)"
  - linux kernel code
**/

DefinitionBlock ("SsdtPciOsc.aml", "SSDT", 2, "ARMLTD", "PCI-OSC", 1) {

  // This table is just a template and is never installed as a table.
  // Pci devices are dynamically created at runtime as:
  // ASL:
  // Device (PCIx) {
  //   ...
  // }
  // and the _OSC method available below is appended to the PCIx device as:
  // ASL:
  // Device (PCIx) {
  //   ...
  //   Method (_OSC, 4 {
  //    ...
  //   })
  // }
  Method (_OSC, 4) {
    //
    // OS Control Handoff
    //
    Name (SUPP, Zero) // PCI _OSC Support Field value
    Name (CTRL, Zero) // PCI _OSC Control Field value

    // Create DWord-addressable fields from the Capabilities Buffer
    CreateDWordField (Arg3, 0, CDW1)
    CreateDWordField (Arg3, 4, CDW2)
    CreateDWordField (Arg3, 8, CDW3)

    // Check for proper UUID
    If (LEqual (Arg0,ToUUID ("33DB4D5B-1FF7-401C-9657-7441C03DD766"))) {

      // Save Capabilities DWord2 & 3
      Store (CDW2, SUPP)
      Store (CDW3, CTRL)

      // Only allow native hot plug control if OS supports:
      // * ASPM
      // * Clock PM
      // * MSI/MSI-X
      If (LNotEqual (And (SUPP, 0x16), 0x16)) {
        And (CTRL, 0x1E, CTRL) // Mask bit 0 (and undefined bits)
      }

      // Always allow native PME, AER (no dependencies)

      // Never allow SHPC (no SHPC controller in this system)
      And (CTRL, 0x1D, CTRL)

      If (LNotEqual (Arg1, One)) {  // Unknown revision
        Or (CDW1, 0x08, CDW1)
      }

      If (LNotEqual (CDW3, CTRL)) {  // Capabilities bits were masked
        Or (CDW1, 0x10, CDW1)
      }

      // Update DWORD3 in the buffer
      Store (CTRL,CDW3)
      Return (Arg3)
    } Else {
      Or (CDW1, 4, CDW1) // Unrecognized UUID
      Return (Arg3)
    } // If
  } // _OSC
}
