/** @file
  The definition block in ACPI table for NVDIMM root device.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock (
  "RamDisk.aml",
  "SSDT",
  2,
  "INTEL ",
  "RamDisk ",
  0x1000
  )
{
  Scope (\_SB)
  {
    Device (NVDR)
    {
      //
      // Define _HID, "ACPI0012" NVDIMM Root Device
      //
      Name (_HID, "ACPI0012")

      //
      // Readable name of this device
      //
      Name (_STR, Unicode ("NVDIMM Root Device"))

      Method (_STA, 0)
      {
        Return (0x0f)
      }
    }
  }
}
