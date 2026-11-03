/** @file
  IWB Device Table Template

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s)
    - IO Remapping Table, Platform Design Document, Revision E.g Mar 2026

  @par Glossary:
    - {template} - Data fixed up using AML Fixup APIs.
**/

DefinitionBlock("IwbDeviceTableTemplate.aml", "SSDT", 2, "ARMLTD", "IWB", 1) {
  Scope(_SB) {
    Device (IWB0) {                                       // {template}
      Name (_HID, "ARMH0003")
      Name (_UID, 0)                                           // {template}
      Name (_CRS, ResourceTemplate () {
          QWordMemory (
            ,
            ,
            MinFixed,
            MaxFixed,
            NonCacheable,
            ReadWrite,
            0x0,
            0x000000002F000000, // IWB config frame base       // {template}
            0x000000002F00FFFF, // IWB config frame last byte  // {template}
            0,                  // Translation Offset
            0x0000000000010000, // IWB config frame length
            ) // QWordMemory
      }) // Name
    } // Device
  } // Scope(_SB)
}
