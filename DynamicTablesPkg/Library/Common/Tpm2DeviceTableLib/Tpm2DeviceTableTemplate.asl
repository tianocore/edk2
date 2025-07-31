/** @file
  TPM2 Device Table Template

  Copyright (c) 2025, ARM Ltd. All rights reserved.<BR>
  All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s)
    - TCG ACPI Specification
    - TCG PC Client Platform Firmware Profile Specification

  @par Glossary:
    - {template} - Data fixed up using AML Fixup APIs.
**/

DefinitionBlock("Tpm2DeviceTableTemplate.aml", "SSDT", 2, "ARMLTD", "TPM2CRB", 1) {
  Scope(_SB) {
    Device (TPM0) {                                     // {template}
      Name (_HID, "MSFT0101")
      Name (_UID, 0)                                    // {template}
      Name (_CRS, ResourceTemplate () {
          QWordMemory (
            ResourceProducer,
            PosDecode,
            MinFixed,
            MaxFixed,
            NonCacheable,
            ReadWrite,
            0x0000000000000000, // Granularity
            0x00000000FFDFB000, // Range Minimum        // {template}
            0x00000000FFDFFFFF, // Range Maximum        // {template}
            0x0000000000000000, // Translation Offset
            0x0000000000005000, // Length               // {template}
            ,
            ,
            ,
            AddressRangeReserved,
            TypeStatic
            ) // QWordMemory
      }) // Name
    } // Device
  } // Scope(_SB)
}
