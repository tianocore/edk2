/** @file
  SSDT Serial Template

  Copyright (c) 2019 - 2024, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Arm Server Base Boot Requirements (SBBR), s4.2.1.8 "SPCR".

  @par Glossary:
    - {template} - Data fixed up using AML Fixup APIs.
    - {codegen}  - Data generated using AML Codegen APIs.
**/

DefinitionBlock ("SsdtSerialPortTemplate.aml", "SSDT", 2, "ARMLTD", "SERIAL", 1) {
  Scope (_SB) {
    // UART PL011
    Device (COM0) {                                       // {template}
      Name (_UID, 0x0)                                    // {template}
      Name (_HID, "HID0000")                              // {template}
      Name (_CID, "CID0000")                              // {template}

      Method(_STA) {
        Return(0xF)
      }

      Name (_CRS, ResourceTemplate() {
        QWordMemory (
          ,                   // ResourceUsage
          ,                   // Decode
          ,                   // IsMinFixed
          ,                   // IsMaxFixed
          ,                   // Cacheable
          ReadWrite,          // ReadAndWrite
          0x0,                // AddressGranularity
          0xA0000000,         // AddressMinimum           // {template}
          0xAFFFFFFF,         // AddressMaximum           // {template}
          0,                  // AddressTranslation
          0x10000000,         // RangeLength              // {template}
          ,                   // ResourceSourceIndex
          ,                   // ResourceSource
          ,                   // DescriptorName
          ,                   // MemoryRangeType
                              // TranslationType
        ) // QWordMemory

        // The Interrupt information is generated using AmlCodegen.
        //
        // Interrupt (                                    // {codegen}
        //  ResourceConsumer, // ResourceUsage
        //  Level,            // EdgeLevel
        //  ActiveHigh,       // ActiveLevel
        //  Exclusive,        // Shared
        //  ,                 // ResourceSourceIndex
        //  ,                 // ResourceSource
        //                    // DescriptorName
        //  ) {
        //    <IRQ>           // <spi>
        // } // Interrupt

      }) // Name
    } // Device
  } // Scope (_SB)
}
