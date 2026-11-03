/** @file
  SSDT CMN Template

  Copyright (c) 2020 - 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Arm CoreLink CMN-600 Coherent Mesh Network Technical Reference Manual r3p0
  - Generic ACPI for Arm Components 1.3 Platform Design Document

  @par Glossary:
    - {template} - Data fixed up using AML Fixup APIs.
    - {codegen}  - Data generated using AML Codegen APIs.
**/

DefinitionBlock ("SsdtCmn.aml", "SSDT", 2, "ARMLTD", "CMN", 1) {
  Scope (_SB) {
    // CMN device object.
    Device (CMN0) {                        // {template}
      Name (_HID, "ARMHC600")              // {template}
      Name (_UID, 0x0)                     // {template}

      Name (_CRS, ResourceTemplate () {
        // Descriptor for the CMN configuration region.
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
