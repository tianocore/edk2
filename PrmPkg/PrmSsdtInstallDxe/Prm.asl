/** @file
  The definition block in ACPI table for PRM Operation Region

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

DefinitionBlock (
    "Prm.aml",
    "SSDT",
    2,
    "OEMID ",
    "PRMOPREG",
    0x1000
    )
{
    Scope (\_SB)
    {
        //
        // PRM Bridge Device
        //

        Device (PRMB)
        {
            Name (_HID, "80860222")
            Name (_CID, "80860222")
            Name (_DDN, "PRM Bridge Device")
            Name (_STA, 0xF)
            OperationRegion (OPR1, 0x80, 0, 16)
            Field (OPR1, DWordAcc, NoLock, Preserve) //Make it ByteAcc for parameter validation
            {
                Var0, 128
            }
            Method (SETV, 1, Serialized)
            {
                CopyObject (Arg0, \_SB.PRMB.Var0)
            }
        }

        //
        // PRM Test Device
        //

        Device (PRMT)
        {
            Name (_HID, "80860223")
            Name (_CID, "80860223")
            Name (_DDN, "PRM Test Device")
            Name (_STA, 0xF)
            Name (BUF1, Buffer(16)
            {
                0x5F, 0xAD, 0xF2, 0xD5, 0x47, 0xA3, 0x3E, 0x4D, //Guid_0
                0x87, 0xBC, 0xC2, 0xCE, 0x63, 0x02, 0x9C, 0xC8, //Guid_1
            })
            Name (BUF2, Buffer(16)
            {
                0xC3, 0xAD, 0xE7, 0xA9, 0xD0, 0x8C, 0x9A, 0x42, //Guid_0
                0x89, 0x15, 0x10, 0x94, 0x6E, 0xBD, 0xE3, 0x18, //Guid_1
            })
            Name (BUF3, Buffer(16)
            {
                0x14, 0xC2, 0x88, 0xB6, 0x81, 0x40, 0xEB, 0x4E, //Guid_0
                0x8D, 0x26, 0x1E, 0xB5, 0xA3, 0xBC, 0xF1, 0x1A, //Guid_1
            })
            Method (NTST)
            {
                \_SB.PRMB.SETV (BUF1)
            }
        }
    }

} // End of Definition Block



