/** @file
  IGD OpRegion definition from Intel Integrated Graphics Device OpRegion
  Specification.

  https://01.org/sites/default/files/documentation/acpi_igd_opregion_spec_0.pdf

  There are some mismatch between the specification and the implementation.
  The definition follows the latest implementation.
  1) INTEL_IGD_OPREGION_HEADER.RSV1[0xA0]
  2) INTEL_IGD_OPREGION_MBOX1.RSV3[0x3C]
  3) INTEL_IGD_OPREGION_MBOX3.RSV5[0x62]
  4) INTEL_IGD_OPREGION_VBT.RVBT[0x1800] Size is 6KB

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

  @note: These structures are packed to 1 byte offsets because the exact
  data location is required by the supporting design specification due to
  the fact that the data is used by ASL and Graphics driver code compiled
  separately.
**/
#pragma pack(1)
///
/// OpRegion header (mailbox 0) structure. The OpRegion Header is used to
/// identify a block of memory as the graphics driver OpRegion.
///
typedef struct {
  CHAR8  SIGN[0x10];    ///< Offset 0x00 OpRegion Signature
  UINT32 SIZE;          ///< Offset 0x10 OpRegion Size
  UINT32 OVER;          ///< Offset 0x14 OpRegion Structure Version
  UINT8  SVER[0x20];    ///< Offset 0x18 System BIOS Build Version
  UINT8  VVER[0x10];    ///< Offset 0x38 Video BIOS Build Version
  UINT8  GVER[0x10];    ///< Offset 0x48 Graphic Driver Build Version
  UINT32 MBOX;          ///< Offset 0x58 Supported Mailboxes
  UINT32 DMOD;          ///< Offset 0x5C Driver Model
  UINT8  RSV1[0xA0];    ///< Offset 0x60 Reserved
} INTEL_IGD_OPREGION_HEADER;

///
/// OpRegion mailbox 1 (public ACPI Methods)
///
typedef struct {
  UINT32 DRDY;          ///< Offset 0x100 Driver Readiness
  UINT32 CSTS;          ///< Offset 0x104 Status
  UINT32 CEVT;          ///< Offset 0x108 Current Event
  UINT8  RSV2[0x14];    ///< Offset 0x10C Reserved
  UINT32 DIDL[8];       ///< Offset 0x120 Supported Display Devices ID List
  UINT32 CPDL[8];       ///< Offset 0x140 Currently Attached Display Devices List
  UINT32 CADL[8];       ///< Offset 0x160 Currently Active Display Devices List
  UINT32 NADL[8];       ///< Offset 0x180 Next Active Devices List
  UINT32 ASLP;          ///< Offset 0x1A0 ASL Sleep Time Out
  UINT32 TIDX;          ///< Offset 0x1A4 Toggle Table Index
  UINT32 CHPD;          ///< Offset 0x1A8 Current Hotplug Enable Indicator
  UINT32 CLID;          ///< Offset 0x1AC Current Lid State Indicator
  UINT32 CDCK;          ///< Offset 0x1B0 Current Docking State Indicator
  UINT32 SXSW;          ///< Offset 0x1B4 Display Switch Notification on Sx State Resume
  UINT32 EVTS;          ///< Offset 0x1B8 Events supported by ASL
  UINT32 CNOT;          ///< Offset 0x1BC Current OS Notification
  UINT32 NRDY;          ///< Offset 0x1C0 Driver Status
  UINT8  RSV3[0x3C];    ///< Offset 0x1C4 - 0x1FF Reserved
} INTEL_IGD_OPREGION_MBOX1;

///
/// OpRegion mailbox 2 (Software SCI Interface).
///
typedef struct {
  UINT32 SCIC;          ///< Offset 0x200 Software SCI Command / Status / Data
  UINT32 PARM;          ///< Offset 0x204 Software SCI Parameters
  UINT32 DSLP;          ///< Offset 0x208 Driver Sleep Time Out
  UINT8  RSV4[0xF4];    ///< Offset 0x20C - 0x2FF Reserved
} INTEL_IGD_OPREGION_MBOX2;

///
/// OpRegion mailbox 3 (BIOS/Driver Communication - ASLE Support).
///
typedef struct {
  UINT32 ARDY;          ///< Offset 0x300 Driver Readiness
  UINT32 ASLC;          ///< Offset 0x304 ASLE Interrupt Command / Status
  UINT32 TCHE;          ///< Offset 0x308 Technology Enabled Indicator
  UINT32 ALSI;          ///< Offset 0x30C Current ALS Luminance Reading
  UINT32 BCLP;          ///< Offset 0x310 Requested Backlight Brightness
  UINT32 PFIT;          ///< Offset 0x314 Panel Fitting State or Request
  UINT32 CBLV;          ///< Offset 0x318 Current Brightness Level
  UINT16 BCLM[0x14];    ///< Offset 0x31C Backlight Brightness Levels Duty Cycle Mapping Table
  UINT32 CPFM;          ///< Offset 0x344 Current Panel Fitting Mode
  UINT32 EPFM;          ///< Offset 0x348 Enabled Panel Fitting Modes
  UINT8  PLUT[0x4A];    ///< Offset 0x34C Panel Look Up Table & Identifier
  UINT32 PFMB;          ///< Offset 0x396 PWM Frequency and Minimum Brightness
  UINT32 CCDV;          ///< Offset 0x39A Color Correction Default Values
  UINT8  RSV5[0x62];    ///< Offset 0x39E - 0x3FF  Reserved
} INTEL_IGD_OPREGION_MBOX3;

///
/// OpRegion mailbox 4 (VBT).
///
typedef struct {
  UINT8  RVBT[0x1800];  ///< Offset 0x400 - 0x1BFF Raw VBT Data
} INTEL_IGD_OPREGION_VBT;

///
/// IGD OpRegion Structure
///
typedef struct {
  INTEL_IGD_OPREGION_HEADER Header; ///< OpRegion header (Offset 0x0, Size 0x100)
  INTEL_IGD_OPREGION_MBOX1  MBox1;  ///< Mailbox 1: Public ACPI Methods (Offset 0x100, Size 0x100)
  INTEL_IGD_OPREGION_MBOX2  MBox2;  ///< Mailbox 2: Software SCI Interface (Offset 0x200, Size 0x100)
  INTEL_IGD_OPREGION_MBOX3  MBox3;  ///< Mailbox 3: BIOS to Driver Communication (Offset 0x300, Size 0x100)
  INTEL_IGD_OPREGION_VBT    VBT;    ///< Mailbox 4: Video BIOS Table (VBT) (Offset 0x400, Size 0x1200)
} IGD_IGD_OPREGION_STRUCTURE;
#pragma pack()

#endif
