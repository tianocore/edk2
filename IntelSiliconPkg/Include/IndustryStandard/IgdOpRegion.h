/** @file
  IGD OpRegion definition from Intel Integrated Graphics Device OpRegion
  Specification.

  https://01.org/sites/default/files/documentation/acpi_igd_opregion_spec_0.pdf

  There are some mismatch between the specification and the implementation.
  The definition follows the latest implementation.
  1) INTEL_IGD_OPREGION_HEADER.RSV1[0xA0]
  2) INTEL_IGD_OPREGION_MBOX1.RSV3[0x3C]
  3) INTEL_IGD_OPREGION_MBOX3.RSV5[0x62]
  4) INTEL_IGD_OPREGION_VBT.RVBT[0x1C00]

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _IGD_OPREGION_H_
#define _IGD_OPREGION_H_

/**
 OpRegion structures:
 Sub-structures define the different parts of the OpRegion followed by the
 main structure representing the entire OpRegion.

 Note: These structures are packed to 1 byte offsets because the exact
 data location is requred by the supporting design specification due to
 the fact that the data is used by ASL and Graphics driver code compiled
 separatly.
**/
#pragma pack(1)
///
/// OpRegion header (mailbox 0) structure and defines.
///
typedef struct {
  CHAR8   SIGN[0x10]; ///< Offset 0    OpRegion Signature
  UINT32  SIZE;       ///< Offset 16   OpRegion Size
  UINT32  OVER;       ///< Offset 20   OpRegion Structure Version
  UINT8   SVER[0x20]; ///< Offset 24   System BIOS Build Version
  UINT8   VVER[0x10]; ///< Offset 56   Video BIOS Build Version
  UINT8   GVER[0x10]; ///< Offset 72   Graphic Driver Build Version
  UINT32  MBOX;       ///< Offset 88   Supported Mailboxes
  UINT32  DMOD;       ///< Offset 92   Driver Model
  UINT8   RSV1[0xA0]; ///< Offset 96   Reserved
} INTEL_IGD_OPREGION_HEADER;

///
/// OpRegion mailbox 1 (public ACPI Methods).
///
typedef struct {
  UINT32  DRDY;       ///< Offset 0    Driver Readiness
  UINT32  CSTS;       ///< Offset 4    Status
  UINT32  CEVT;       ///< Offset 8    Current Event
  UINT8   RSV2[0x14]; ///< Offset 12   Reserved
  UINT32  DIDL[8];    ///< Offset 32   Supported Display Devices ID List
  UINT32  CPDL[8];    ///< Offset 64   Currently Attached Display Devices List
  UINT32  CADL[8];    ///< Offset 96   Currently Active Display Devices List
  UINT32  NADL[8];    ///< Offset 128  Next Active Devices List
  UINT32  ASLP;       ///< Offset 160  ASL Sleep Time Out
  UINT32  TIDX;       ///< Offset 164  Toggle Table Index
  UINT32  CHPD;       ///< Offset 168  Current Hotplug Enable Indicator
  UINT32  CLID;       ///< Offset 172  Current Lid State Indicator
  UINT32  CDCK;       ///< Offset 176  Current Docking State Indicator
  UINT32  SXSW;       ///< Offset 180  Display Switch Notification on Sx State Resume
  UINT32  EVTS;       ///< Offset 184  Events supported by ASL
  UINT32  CNOT;       ///< Offset 188  Current OS Notification
  UINT32  NRDY;       ///< Offset 192  Driver Status
  UINT8   RSV3[0x3C]; ///< Offset 196  Reserved
} INTEL_IGD_OPREGION_MBOX1;

///
/// OpRegion mailbox 2 (Software SCI Interface).
///
typedef struct {
  UINT32  SCIC;       ///< Offset 0    Software SCI Command / Status / Data
  UINT32  PARM;       ///< Offset 4    Software SCI Parameters
  UINT32  DSLP;       ///< Offset 8    Driver Sleep Time Out
  UINT8   RSV4[0xF4]; ///< Offset 12   Reserved
} INTEL_IGD_OPREGION_MBOX2;

///
/// OpRegion mailbox 3 (BIOS/Driver Communication - ASLE Support).
///
typedef struct {
  UINT32  ARDY;       ///< Offset 0    Driver Readiness
  UINT32  ASLC;       ///< Offset 4    ASLE Interrupt Command / Status
  UINT32  TCHE;       ///< Offset 8    Technology Enabled Indicator
  UINT32  ALSI;       ///< Offset 12   Current ALS Luminance Reading
  UINT32  BCLP;       ///< Offset 16   Requested Backlight Britness
  UINT32  PFIT;       ///< Offset 20   Panel Fitting State or Request
  UINT32  CBLV;       ///< Offset 24   Current Brightness Level
  UINT16  BCLM[0x14]; ///< Offset 28   Backlight Brightness Levels Duty Cycle Mapping Table
  UINT32  CPFM;       ///< Offset 68   Current Panel Fitting Mode
  UINT32  EPFM;       ///< Offset 72   Enabled Panel Fitting Modes
  UINT8   PLUT[0x4A]; ///< Offset 76   Panel Look Up Table & Identifier
  UINT32  PFMB;       ///< Offset 150  PWM Frequency and Minimum Brightness
  UINT32  CCDV;       ///< Offset 154  Color Correction Default Values
  UINT8   RSV5[0x62]; ///< Offset 158  Reserved
} INTEL_IGD_OPREGION_MBOX3;

///
/// OpRegion mailbox 4 (VBT).
///
typedef struct {
  UINT8 RVBT[0x1C00]; ///< Offset 0    Raw VBT Data
} INTEL_IGD_OPREGION_VBT;

///
/// IGD OpRegion Structure
///
typedef struct {
  INTEL_IGD_OPREGION_HEADER Header; ///< OpRegion header
  INTEL_IGD_OPREGION_MBOX1  MBox1;  ///< Mailbox 1: Public ACPI Methods
  INTEL_IGD_OPREGION_MBOX2  MBox2;  ///< Mailbox 2: Software SCI Inteface
  INTEL_IGD_OPREGION_MBOX3  MBox3;  ///< Mailbox 3: BIOS/Driver Communication
  INTEL_IGD_OPREGION_VBT    VBT;    ///< VBT: Video BIOS Table (OEM customizable data)
} IGD_IGD_OPREGION_STRUCTURE;
#pragma pack()

#endif
