/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Sandy Bridge        *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c) 2012  - 2015, Intel Corporation. All rights reserved    *;
;
; This program and the accompanying materials are licensed and made available under
; the terms and conditions of the BSD License that accompanies this distribution.
; The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/

DefinitionBlock (
  "DSDT.aml",
  "DSDT",
  0x02,  // DSDT revision.
  "OEMID", // OEM ID (6 byte string)
  "VLV-SOC", // OEM table ID  (8 byte string)
  0x0 // OEM version of DSDT table (4 byte Integer)
)

// BEGIN OF ASL SCOPE
{
  External(MDBG, MethodObj)

  Method(ADBG, 1, Serialized)
  {

    If(CondRefOf(MDBG))   //check if SSDT is loaded
    {
      Return(MDBG(Arg0))
    }

    Return(0)
  }


// Miscellaneous services enabled in Project
  include ("token.asl")
  include ("GloblNvs.asl")
  include ("PciTree.asl")
  include ("Pch.asl")
  include ("Vlv.asl")
  include ("CPU.asl")
  include ("Platform.asl")
  include ("THERMAL.ASL")
  include ("PCI_DRC.ASL")
  include ("Video.asl")
  include ("Gpe.asl")
  //include ("IoTVirtualDevice.asl")

  // Sleep states supported by Chipset/Board.
  // SSx - BIOS setup controlled enabled _Sx Sleep state status
  // Values to be written to SLP_TYPE register are provided by SBACPI.SDL (South Bridge ACPI ModulePart)

  Name(\_S0, Package(4) {0x0,0x0,0,0}) // mandatory System state
  Name(\_S1, Package(4) {0x1,0x0,0,0})
  Name(\_S3, Package(4) {0x5,0x0,0,0})
  Name(\_S4, Package(4) {0x6,0x0,0,0})
  Name(\_S5, Package(4) {0x7,0x0,0,0}) // mandatory System state

  Method(PTS, 1)          // METHOD CALLED FROM _PTS PRIOR TO ENTER ANY SLEEP STATE
  {
    If(Arg0)            // entering any sleep state
    {
    }
  }
  Method(WAK, 1)          // METHOD CALLED FROM _WAK RIGHT AFTER WAKE UP
  {
  }

}// End of ASL File
