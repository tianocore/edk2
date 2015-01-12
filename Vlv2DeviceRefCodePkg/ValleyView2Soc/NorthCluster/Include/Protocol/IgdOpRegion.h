
/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  IgdOpRegion.h

Abstract:

  This file is part of the IGD OpRegion Implementation.  The IGD OpRegion is
  an interface between system BIOS, ASL code, and Graphics drivers.

  Supporting Specifiction: IGD OpRegion/Software SCI SPEC

  Note:  Data structures defined in this protocol are packed not naturally
    aligned.

  GUID forms:
    {CDC5DDDF-E79D-41ec-A9B0-6565490DB9D3}
    (0xcdc5dddf, 0xe79d, 0x41ec, 0xa9, 0xb0, 0x65, 0x65, 0x49, 0xd, 0xb9, 0xd3);

  Acronyms:
    NVS:        ACPI Non Volatile Storage
    OpRegion:   ACPI Operational Region
    VBT:        Video BIOS Table (OEM customizable data)

--*/

#ifndef _IGD_OPREGION_PROTOCOL_H_
#define _IGD_OPREGION_PROTOCOL_H_

//
// OpRegion / Software SCI protocol GUID
//
#define IGD_OPREGION_PROTOCOL_GUID \
  { \
    0xcdc5dddf, 0xe79d, 0x41ec, 0xa9, 0xb0, 0x65, 0x65, 0x49, 0xd, 0xb9, 0xd3 \
  }

//
// Extern the GUID for protocol users.
//
extern EFI_GUID gIgdOpRegionProtocolGuid;

//
// Forward reference for pure ANSI compatability
//
typedef struct _IGD_OPREGION_PROTOCOL IGD_OPREGION_PROTOCOL;

//
// Protocol data definitions
//

//
// OpRegion structures:
// Sub-structures define the different parts of the OpRegion followed by the
// main structure representing the entire OpRegion.
//
// Note: These structures are packed to 1 byte offsets because the exact
// data location is requred by the supporting design specification due to
// the fact that the data is used by ASL and Graphics driver code compiled
// separatly.
//

//
// OpRegion header (mailbox 0) structure and #defines.
//
#pragma pack (1)
typedef struct {
  CHAR8   SIGN[0x10]; // 0      OpRegion signature
  UINT32  SIZE;       // 0x10   OpRegion size
  UINT32  OVER;       // 0x14   OpRegion structure version
  UINT8   SVER[0x20]; // 0x18   System BIOS build version
  UINT8   VVER[0x10]; // 0x38   Video BIOS build version
  UINT8   GVER[0x10]; // 0x48   Graphic driver build version
  UINT32  MBOX;       // 0x58   Mailboxes supported
  UINT32  DMOD;       // 0x5C   Driver Model
  UINT32  PCON;       // 0x60   Platform Configuration Info
  CHAR8   GOPV[0x20]; // 0X64   GOP build version
  UINT8   RSV[0x7C];  //        Reserved
} OPREGION_HEADER;
#pragma pack ()

//
// OpRegion mailbox 1 (public ACPI Methods).
//
#pragma pack (1)
typedef struct {
  UINT32  DRDY;     // 0    Driver readiness
  UINT32  CSTS;     // 4    Status
  UINT32  CEVT;     // 8    Current event
  UINT8   RM11[0x14]; // 12   Reserved
  UINT32  DIDL;       // 32   Supported display devices list
  UINT32  DDL2;       //  8 Devices.
  UINT32  DDL3;
  UINT32  DDL4;
  UINT32  DDL5;
  UINT32  DDL6;
  UINT32  DDL7;
  UINT32  DDL8;
  UINT32  CPDL;       // 64   Currently present display devices list
  UINT32  CPL2;       //  8 Devices.
  UINT32  CPL3;
  UINT32  CPL4;
  UINT32  CPL5;
  UINT32  CPL6;
  UINT32  CPL7;
  UINT32  CPL8;
  UINT32  CADL;       // 96   Currently active display devices list
  UINT32  CAL2;       //  8 Devices.
  UINT32  CAL3;
  UINT32  CAL4;
  UINT32  CAL5;
  UINT32  CAL6;
  UINT32  CAL7;
  UINT32  CAL8;
  UINT32  NADL;       // 128  Next active device list
  UINT32  NDL2;       //   8 Devices.
  UINT32  NDL3;
  UINT32  NDL4;
  UINT32  NDL5;
  UINT32  NDL6;
  UINT32  NDL7;
  UINT32  NDL8;
  UINT32  ASLP;     // 160  ASL sleep timeout
  UINT32  TIDX;     // 164  Toggle table index
  UINT32  CHPD;     // 168  Current hot plug enable indicator
  UINT32  CLID;     // 172  Current lid state indicator
  UINT32  CDCK;     // 176  Current docking state indicator
  UINT32  SXSW;     // 180  Display Switch notification on Sx State resume
  UINT32  EVTS;     // 184  Events supported by ASL
  UINT32  CNOT;     // 188  Current OS Notification
  UINT32  NRDY;     // 192  Reasons for DRDY = 0
  UINT8   RM12[0x3C]; // 196  Reserved
} OPREGION_MBOX1;
#pragma pack ()

//
// OpRegion mailbox 2 (Software SCI Interface).
//
#pragma pack (1)
typedef struct {
  UINT32  SCIC;       // 0    Software SCI function number parameters
  UINT32  PARM;       // 4    Software SCI additional parameters
  UINT32  DSLP;       // 8    Driver sleep timeout
  UINT8   RM21[0xF4]; // 12   Reserved
} OPREGION_MBOX2;
#pragma pack ()

//
// OpRegion mailbox 3 (Power Conservation).
//
#pragma pack (1)
typedef struct {
  UINT32  ARDY;       // 0    Driver readiness
  UINT32  ASLC;       // 4    ASLE interrupt command / status
  UINT32  TCHE;       // 8    Technology enabled indicator
  UINT32  ALSI;       // 12   Current ALS illuminance reading
  UINT32  BCLP;       // 16   Backlight britness to set
  UINT32  PFIT;       // 20   Panel fitting Current State or Request
  UINT32  CBLV;       // 24   Brightness Current State
  UINT16  BCLM[0x14]; // 28   Backlight Brightness Level Duty Cycle Mapping Table
  UINT32  CPFM;       // 68   Panel Fitting Current Mode
  UINT32  EPFM;       // 72   Enabled Panel Fitting Mode
  UINT8   PLUT[0x4A]; // 76   Panel Look Up Table
  UINT32  PFMB;       // 150  PWM Frequency and Minimum Brightness
  UINT32  CCDV;       // 154  Color Correction Default Values
  UINT32  PCFT;       // 158  Power Conservation Features
  UINT8   RM31[0x5E]; // 162  Reserved
} OPREGION_MBOX3;
#pragma pack ()

//
// OpRegion mailbox 4 (VBT).
//
#pragma pack (1)
typedef struct {
  UINT8 GVD1[0x1800]; // 6K Reserved
} OPREGION_VBT;
#pragma pack ()

#pragma pack (1)
typedef struct {
  UINT8 EDIDOVRD[0x400]; // 6K Edid overriding data
} OPREGION_MBOX5;
#pragma pack ()
//
// Entire OpRegion
//
#pragma pack (1)
typedef struct {
  OPREGION_HEADER  Header; // OpRegion header
  OPREGION_MBOX1   MBox1;  // Mailbox 1: Public ACPI Methods
  OPREGION_MBOX2   MBox2;  // Mailbox 2: Software SCI Inteface
  OPREGION_MBOX3   MBox3;  // Mailbox 3: Power Conservation
  OPREGION_VBT        VBT;    // VBT: Video BIOS Table (OEM customizable data)
  OPREGION_MBOX5   MBox5;
} IGD_OPREGION_STRUC;
#pragma pack ()

//
// Protocol data structure definition
//
struct _IGD_OPREGION_PROTOCOL {
  IGD_OPREGION_STRUC    *OpRegion;
};

#endif
