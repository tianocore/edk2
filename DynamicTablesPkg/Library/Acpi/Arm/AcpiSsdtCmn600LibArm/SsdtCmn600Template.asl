/** @file
  SSDT CMN-600 Template

  Copyright (c) 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Arm CoreLink CMN-600 Coherent Mesh Network Technical Reference Manual r3p0
  - Generic ACPI for Arm Components 1.0 Platform Design Document

  @par Glossary:
    - {template} - Data fixed up using AML Fixup APIs.
    - {codegen}  - Data generated using AML Codegen APIs.
**/

DefinitionBlock ("SsdtCmn600.aml", "SSDT", 2, "ARMLTD", "CMN-600", 1) {
  Scope (_SB) {
    // CMN-600 device object for a X * Y mesh, where (X >= 4) || (Y >= 4).
    Device (CMN0) {                        // {template}
      Name (_HID, "ARMHC600")
      Name (_UID, 0x0)                     // {template}

      Name (_CRS, ResourceTemplate () {
        // Descriptor for 256 MB of the CFG region at offset PERIPHBASE.
        QWordMemory (
          ResourceConsumer,                // bit 0 of general flags is 0.
          PosDecode,
          MinFixed,                        // Range is fixed.
          MaxFixed,                        // Range is Fixed.
          NonCacheable,
          ReadWrite,
          0x00000000,                      // Granularity
          0xA0000000,                      // MinAddress         // {template}
          0xAFFFFFFF,                      // MaxAddress         // {template}
          0x00000000,                      // Translation
          0x10000000,                      // RangeLength        // {template}
          ,                                // ResourceSourceIndex
          ,                                // ResourceSource
          CFGR                             // DescriptorName
        ) // QWordMemory

        // Descriptor for the root node. This is a 16 KB region at offset
        // ROOTNODEBASE. In this example, ROOTNODEBASE starts at the 16 KB
        // aligned offset of PERIPHBASE.
        QWordMemory (
          ResourceConsumer,                // bit 0 of general flags is 0.
          PosDecode,
          MinFixed,                        // Range is fixed.
          MaxFixed,                        // Range is Fixed.
          NonCacheable,
          ReadWrite,
          0x00000000,                      // Granularity
          0xA0000000,                      // MinAddress         // {template}
          0xAFFFFFFF,                      // MaxAddress         // {template}
          0x00000000,                      // Translation
          0x10000000,                      // RangeLength        // {template}
          ,                                // ResourceSourceIndex
          ,                                // ResourceSource
          ROOT                             // DescriptorName
        ) // QWordMemory

        // The Interrupt information is generated using AmlCodegen.
        // Interrupt on PMU0 overflow, attached to DTC [0], with GSIV = <gsiv0>.
        //
        // Interrupt (                                            // {codegen}
        //  ResourceConsumer,                // ResourceUsage
        //  Level,                           // EdgeLevel
        //  ActiveHigh,                      // ActiveLevel
        //  Exclusive,                       // Shared
        //  ,                                // ResourceSourceIndex
        //  ,                                // ResourceSource
        //                                   // DescriptorName
        //  ) {
        //    0xA5                           // <gsiv0 >
        // } // Interrupt

      }) // Name
    } // Device
  } // _SB
} // DefinitionBlock
