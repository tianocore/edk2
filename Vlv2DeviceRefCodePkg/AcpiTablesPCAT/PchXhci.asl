/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Haswell             *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c)  2010  - 2014, Intel Corporation. All rights reserved   *;
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

//scope is \_SB.PCI0.XHC
Device(XHC1)
{
  Name(_ADR, 0x00140000)                     //Device 20, Function 0

  //When it is in Host mode, USH core is connected to USB3 microAB(USB3 P1 and USB2 P0)
  Name (_DDN, "Baytrail XHCI controller (CCG core/Host only)" )

  Method(XDEP, 0)
  {
    If(LEqual(OSYS,2013))
    {
      Name(_DEP, Package(0x1)
      {
        PEPD
      })
    }
  }

  Name (_STR, Unicode ("Baytrail XHCI controller (CCG core/Host only)"))
  Name(_PRW, Package() {0xD,4})

  Method(_PSW,1)
  {
    If (LAnd (PMES, PMEE)) {
       Store (0, PMEE)
       Store (1, PMES)
    }
  }

  OperationRegion (PMEB, PCI_Config, 0x74, 0x04)  // Power Management Control/Status
  Field (PMEB, WordAcc, NoLock, Preserve)
  {
    ,   8,
    PMEE,   1,   //bit8 PME_En
    ,   6,
    PMES,   1    //bit15 PME_Status
  }

  Method(_STA, 0)
  {
    If(LNotEqual(XHCI, 0))      //NVS variable controls present of XHCI controller
    {
      Return (0xF)
    } Else
    {
      Return (0x0)
    }
  }

  OperationRegion(XPRT,PCI_Config,0xD0,0x10)
  Field(XPRT,DWordAcc,NoLock,Preserve)       //usbx_top.doc.xml
  {
    PR2,  32,                              //bit[8:0] USB2HCSEL
    PR2M, 32,                              //bit[8:0] USB2HCSELM
    PR3,  32,                              //bit[3:0] USB3SSEN
    PR3M, 32                               //bit[3:0] USB3SSENM
  }

  Device(RHUB)
  {
    Name(_ADR, Zero)         //address 0 is reserved for root hub

    //
    // Super Speed Ports - must match _UPC declarations of the coresponding Full Speed Ports.
    //   Paired with Port 1
    Device(SSP1)
    {
      Name(_ADR, 0x07)

      Method(_UPC,0,Serialized)
      {
        Name(UPCP, Package()
        {
          0xFF,                                      // Port is connectable if non-zero
          0x06,                                      // USB3 uAB connector
          0x00,
          0x00
        })
        Return(UPCP)
      }

      Method(_PLD,0,Serialized)
      {
        Name(PLDP, Package()       //pls check ACPI 5.0 section 6.1.8
        {
          Buffer(0x14)
          {
            //31:0   - Bit[6:0]=2 revision is 0x2, Bit[7]=1 Ignore Color Bit[31:8]=0 RGB color is ignored
            0x82, 0x00, 0x00, 0x00,
            //63:32 - Bit[47:32]=0 width: 0x0000  Bit[63:48]=0 Height:0x0000
            0x00, 0x00, 0x00, 0x00,
            //95:64 - bit[66:64]=b'011 visiable/docking/no lid bit[69:67]=b'001 bottom panel bit[71:70]=b'01 Center  bit[73:72]=b'01 Center
            //           bit[77:74]=6 Horizontal Trapezoid bit[78]=0 bit[86:79]=0 bit[94:87]='0 no group info' bit[95]=0 not a bay
            0x4B, 0x19, 0x00, 0x00,
            //127:96 -bit[96]=1 Ejectable bit[97]=1 OSPM Ejection required Bit[105:98]=0 no Cabinet Number
            //            bit[113:106]=0 no Card cage Number bit[114]=0 no reference shape Bit[118:115]=0 no rotation Bit[123:119]=0 no order
            0x03, 0x00, 0x00, 0x00,
            //159:128  Vert. and Horiz. Offsets not supplied
            0xFF, 0xFF, 0xFF, 0xFF
          }
        })
        Return (PLDP)
      }
    }
    //
    // High Speed Ports
    // pair port with port 7 (SS)
    //    The UPC declarations for LS/FS/HS and SS ports that are paired to form a USB3.0 compatible connector.
    //    A "pair" is defined by two ports that declare _PLDs with identical Panel, Vertical Position, Horizontal Postion, Shape, Group Orientation
    //    and Group Token
    Device(HS01)
    {
      Name(_ADR, 0x01)

      Method(_UPC,0,Serialized)
      {
        Name(UPCP, Package() { 0xFF,0x06,0x00,0x00 })
        Return(UPCP)
      }

      Method(_PLD,0,Serialized)
      {
        Name(PLDP, Package()       //pls check ACPI 5.0 section 6.1.8
        {
          Buffer(0x14)
          {
            //31:0   - Bit[6:0]=2 revision is 0x2, Bit[7]=1 Ignore Color Bit[31:8]=0 RGB color is ignored
            0x82, 0x00, 0x00, 0x00,
            //63:32 - Bit[47:32]=0 width: 0x0000  Bit[63:48]=0 Height:0x0000
            0x00, 0x00, 0x00, 0x00,
            //95:64 - bit[66:64]=b'011 visiable/docking/no lid bit[69:67]=b'001 bottom panel bit[71:70]=b'01 Center  bit[73:72]=b'01 Center
            //           bit[77:74]=6 Horizontal Trapezoid bit[78]=0 bit[86:79]=0 bit[94:87]='0 no group info' bit[95]=0 not a bay
            0x4B, 0x19, 0x00, 0x00,
            //127:96 -bit[96]=1 Ejectable bit[97]=1 OSPM Ejection required Bit[105:98]=0 no Cabinet Number
            //            bit[113:106]=0 no Card cage Number bit[114]=0 no reference shape Bit[118:115]=0 no rotation Bit[123:119]=0 no order
            0x03, 0x00, 0x00, 0x00,
            //159:128  Vert. and Horiz. Offsets not supplied
            0xFF, 0xFF, 0xFF, 0xFF
          }
        })
        Return (PLDP)
      }
    }//end of HS01

    // USB2 Type-A/USB2 only
    // EHCI debug capable
    Device(HS02)
    {
      Name(_ADR, 0x02)                                   // 0 is for root hub so physical port index starts from 1 (it is port1 in schematic)

      Method(_UPC,0,Serialized)
      {
        Name(UPCP, Package()
        {
          0xFF,                     // connectable
          0xFF,                     //
          0x00,
          0x00
        })

        Return(UPCP)
      }

      Method(_PLD,0,Serialized)
      {
        Name(PLDP, Package()
        {
          Buffer(0x14)
          {
            //31:0   - Bit[6:0]=2 revision is 0x2, Bit[7]=1 Ignore Color Bit[31:8]=0 RGB color is ignored
            0x82, 0x00, 0x00, 0x00,
            //63:32 - Bit[47:32]=0 width: 0x0000  Bit[63:48]=0 Height:0x0000
            0x00, 0x00, 0x00, 0x00,
            //95:64 - bit[66:64]=b'000 not visiable/no docking/no lid bit[69:67]=b'000 top bit[71:70]=b'01 Center  bit[73:72]=b'00 Left
            //           bit[77:74]=2 Square bit[78]=0 bit[86:79]=0 bit[94:87]='0 no group info' bit[95]=0 not a bay
            0x40, 0x08, 0x00, 0x00,
            //127:96 -bit[96]=0 not Ejectable bit[97]=0 no OSPM Ejection required Bit[105:98]=0 no Cabinet Number
            //            bit[113:106]=0 no Card cage Number bit[114]=0 no reference shape Bit[118:115]=0 no rotation Bit[123:119]=0 no order
            0x00, 0x00, 0x00, 0x00,
            //159:128  Vert. and Horiz. Offsets not supplied
            0xFF, 0xFF, 0xFF, 0xFF
          }
        })

        Return (PLDP)
      }
    }//end of HS02
    // high speed port 3
    Device(HS03)
    {
      Name(_ADR, 0x03)

      Method(_UPC,0,Serialized)
      {
        Name(UPCP, Package()
        {
          0xFF,                     //  connectable
          0xFF,
          0x00,
          0x00
        })

        Return(UPCP)
      }

      Method(_RMV, 0)                                    // for XHCICV debug purpose
      {
        Return(0x0)
      }

      Method(_PLD,0,Serialized)
      {
        Name(PLDP, Package()
        {
          Buffer(0x14)
          {
            //31:0   - Bit[6:0]=2 revision is 0x2, Bit[7]=1 Ignore Color Bit[31:8]=0 RGB color is ignored
            0x82, 0x00, 0x00, 0x00,
            //63:32 - Bit[47:32]=0 width: 0x0000  Bit[63:48]=0 Height:0x0000
            0x00, 0x00, 0x00, 0x00,
            //95:64 - bit[66:64]=b'000 not Visible/no docking/no lid bit[69:67]=6 (b'110) unknown(Vertical Position and  Horizontal Position will be ignored)
            //           bit[71:70]=b'00 Vertical Position ignore bit[73:72]=b'00 Horizontal Position ignore
            //           bit[77:74]=2 Square bit[78]=0 bit[86:79]=0 bit[94:87]='0 no group info' bit[95]=0 not a bay
            0x30, 0x08, 0x00, 0x00,
            //127:96 -bit[96]=0 not Ejectable bit[97]=0 OSPM Ejection not required Bit[105:98]=0 no Cabinet Number
            //            bit[113:106]=0 no Card cage Number bit[114]=0 no reference shape Bit[118:115]=0 no rotation Bit[123:119]=0 no order
            0x00, 0x00, 0x00, 0x00,
            //159:128  Vert. and Horiz. Offsets not supplied
            0xFF, 0xFF, 0xFF, 0xFF
          }
        })
        Return (PLDP)
      }
    }

    Device(HS04)
    {
      Name(_ADR, 0x04)

      Method(_UPC,0,Serialized)
      {
        Name(UPCP, Package()
        {
          0xFF,                     //connectable
          0xFF,                     //Proprietary connector (FPC connector)
          0x00,
          0x00
        })

        Return(UPCP)
      }
      Method(_PLD,0,Serialized)
      {
        Name(PLDP, Package()
        {
          Buffer(0x14)
          {
            //31:0   - Bit[6:0]=2 revision is 0x2, Bit[7]=1 Ignore Color Bit[31:8]=0 RGB color is ignored
            0x82, 0x00, 0x00, 0x00,
            //63:32 - Bit[47:32]=0 width: 0x0000  Bit[63:48]=0 Height:0x0000
            0x00, 0x00, 0x00, 0x00,
            //95:64 - bit[66:64]=b'000 not Visible/no docking/no lid bit[69:67]=6 (b'110) unknown(Vertical Position and  Horizontal Position will be ignored)
            //           bit[71:70]=b'00 Vertical Position ignore bit[73:72]=b'00 Horizontal Position ignore
            //           bit[77:74]=2 Square bit[78]=0 bit[86:79]=0 bit[94:87]='0 no group info' bit[95]=0 not a bay
            0x30, 0x08, 0x00, 0x00,
            //127:96 -bit[96]=0 not Ejectable bit[97]=0 OSPM Ejection not required Bit[105:98]=0 no Cabinet Number
            //            bit[113:106]=0 no Card cage Number bit[114]=0 no reference shape Bit[118:115]=0 no rotation Bit[123:119]=0 no order
            0x00, 0x00, 0x00, 0x00,
            //159:128  Vert. and Horiz. Offsets not supplied
            0xFF, 0xFF, 0xFF, 0xFF
          }
        })

        Return (PLDP)
      }
    }


    Device(HSC1)                                           // USB2 HSIC 01
    {
      Name(_ADR, 0x05)

      Method(_UPC,0,Serialized)
      {
        Name(UPCP, Package()
        {
          0xFF,                     //connectable
          0xFF,                     //Proprietary connector (FPC connector)
          0x00,
          0x00
        })

        Return(UPCP)
      }
      Method(_PLD,0,Serialized)
      {
        Name(PLDP, Package()
        {
          Buffer(0x14)
          {
            //31:0   - Bit[6:0]=2 revision is 0x2, Bit[7]=1 Ignore Color Bit[31:8]=0 RGB color is ignored
            0x82, 0x00, 0x00, 0x00,
            //63:32 - Bit[47:32]=0 width: 0x0000  Bit[63:48]=0 Height:0x0000
            0x00, 0x00, 0x00, 0x00,
            //95:64 - bit[66:64]=b'000 not Visible/no docking/no lid bit[69:67]=6 (b'110) unknown(Vertical Position and  Horizontal Position will be ignored)
            //           bit[71:70]=b'00 Vertical Position ignore bit[73:72]=b'00 Horizontal Position ignore
            //           bit[77:74]=2 Square bit[78]=0 bit[86:79]=0 bit[94:87]='0 no group info' bit[95]=0 not a bay
            0x30, 0x08, 0x00, 0x00,
            //127:96 -bit[96]=0 not Ejectable bit[97]=0 OSPM Ejection not required Bit[105:98]=0 no Cabinet Number
            //            bit[113:106]=0 no Card cage Number bit[114]=0 no reference shape Bit[118:115]=0 no rotation Bit[123:119]=0 no order
            0x00, 0x00, 0x00, 0x00,
            //159:128  Vert. and Horiz. Offsets not supplied
            0xFF, 0xFF, 0xFF, 0xFF
          }
        })
        Return (PLDP)
      }
    }

    Device(HSC2)                                           // USB2 HSIC 02
    {
      Name(_ADR, 0x06)

      Method(_UPC,0,Serialized)
      {
        Name(UPCP, Package()
        {
          0xFF,                     //connectable
          0xFF,                     //Proprietary connector (FPC connector)
          0x00,
          0x00
        })

        Return(UPCP)
      }
      Method(_PLD,0,Serialized)
      {
        Name(PLDP, Package()
        {
          Buffer(0x14)
          {
            //31:0   - Bit[6:0]=2 revision is 0x2, Bit[7]=1 Ignore Color Bit[31:8]=0 RGB color is ignored
            0x82, 0x00, 0x00, 0x00,
            //63:32 - Bit[47:32]=0 width: 0x0000  Bit[63:48]=0 Height:0x0000
            0x00, 0x00, 0x00, 0x00,
            //95:64 - bit[66:64]=b'000 not Visible/no docking/no lid bit[69:67]=6 (b'110) unknown(Vertical Position and  Horizontal Position will be ignored)
            //           bit[71:70]=b'00 Vertical Position ignore bit[73:72]=b'00 Horizontal Position ignore
            //           bit[77:74]=2 Square bit[78]=0 bit[86:79]=0 bit[94:87]='0 no group info' bit[95]=0 not a bay
            0x30, 0x08, 0x00, 0x00,
            //127:96 -bit[96]=0 not Ejectable bit[97]=0 OSPM Ejection not required Bit[105:98]=0 no Cabinet Number
            //            bit[113:106]=0 no Card cage Number bit[114]=0 no reference shape Bit[118:115]=0 no rotation Bit[123:119]=0 no order
            0x00, 0x00, 0x00, 0x00,
            //159:128  Vert. and Horiz. Offsets not supplied
            0xFF, 0xFF, 0xFF, 0xFF
          }
        })
        Return (PLDP)
      }
    }
  }  //end of root hub

} // end of XHC1

