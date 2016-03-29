/** @file
  The definition block in ACPI table for NVDIMM root device.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
