/** @file
  IGD OpRegion definition from Intel Integrated Graphics Device OpRegion
  Specification.

  https://01.org/sites/default/files/documentation/skl_opregion_rev0p5.pdf

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _IGD_OPREGION_H_
#define _IGD_OPREGION_H_

#define IGD_OPREGION_HEADER_SIGN "IntelGraphicsMem"
#define IGD_OPREGION_HEADER_MBOX1 BIT0
#define IGD_OPREGION_HEADER_MBOX2 BIT1
#define IGD_OPREGION_HEADER_MBOX3 BIT2
#define IGD_OPREGION_HEADER_MBOX4 BIT3
#define IGD_OPREGION_HEADER_MBOX5 BIT4

/**
  OpRegion structures:
  Sub-structures define the different parts of the OpRegion followed by the
  main structure representing the entire OpRegion.

  @note These structures are packed to 1 byte offsets because the exact
  data location is required by the supporting design specification due to
  the fact that the data is used by ASL and Graphics driver code compiled
  separately.
**/
#pragma pack(1)
///
/// OpRegion Mailbox 0 Header structure. The OpRegion Header is used to
/// identify a block of memory as the graphics driver OpRegion.
/// Offset 0x0, Size 0x100
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
  UINT32 PCON;          ///< Offset 0x60 Platform Configuration
  CHAR16 DVER[0x10];    ///< Offset 0x64 GOP Version
  UINT8  RM01[0x7C];    ///< Offset 0x84 Reserved Must be zero
} IGD_OPREGION_HEADER;

///
/// OpRegion Mailbox 1 - Public ACPI Methods
/// Offset 0x100, Size 0x100
///
typedef struct {
  UINT32 DRDY;          ///< Offset 0x100 Driver Readiness
  UINT32 CSTS;          ///< Offset 0x104 Status
  UINT32 CEVT;          ///< Offset 0x108 Current Event
  UINT8  RM11[0x14];    ///< Offset 0x10C Reserved Must be Zero
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
  UINT8  DID2[0x1C];    ///< Offset 0x1C4 Extended Supported Devices ID List (DOD)
  UINT8  CPD2[0x1C];    ///< Offset 0x1E0 Extended Attached Display Devices List
  UINT8  RM12[4];       ///< Offset 0x1FC - 0x1FF Reserved Must be zero
} IGD_OPREGION_MBOX1;

///
/// OpRegion Mailbox 2 - Software SCI Interface
/// Offset 0x200, Size 0x100
///
typedef struct {
  UINT32 SCIC;          ///< Offset 0x200 Software SCI Command / Status / Data
  UINT32 PARM;          ///< Offset 0x204 Software SCI Parameters
  UINT32 DSLP;          ///< Offset 0x208 Driver Sleep Time Out
  UINT8  RM21[0xF4];    ///< Offset 0x20C - 0x2FF Reserved Must be zero
} IGD_OPREGION_MBOX2;

///
/// OpRegion Mailbox 3 - BIOS/Driver Notification - ASLE Support
/// Offset 0x300, Size 0x100
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
  UINT32 PCFT;          ///< Offset 0x39E Power Conservation Features
  UINT32 SROT;          ///< Offset 0x3A2 Supported Rotation Angles
  UINT32 IUER;          ///< Offset 0x3A6 Intel Ultrabook(TM) Event Register
  UINT64 FDSS;          ///< Offset 0x3AA DSS Buffer address allocated for IFFS feature
  UINT32 FDSP;          ///< Offset 0x3B2 Size of DSS buffer
  UINT32 STAT;          ///< Offset 0x3B6 State Indicator
  UINT64 RVDA;          ///< Offset 0x3BA Physical address of Raw VBT data. Added from Spec Version 0.90 to support VBT greater than 6KB.
  UINT32 RVDS;          ///< Offset 0x3C2 Size of Raw VBT data. Added from Spec Version 0.90 to support VBT greater than 6KB.
  UINT8  RM32[0x3A];    ///< Offset 0x3C6 - 0x3FF  Reserved Must be zero.
} IGD_OPREGION_MBOX3;

///
/// OpRegion Mailbox 4 - VBT Video BIOS Table
/// Offset 0x400, Size 0x1800
///
typedef struct {
  UINT8  RVBT[0x1800];  ///< Offset 0x400 - 0x1BFF Raw VBT Data
} IGD_OPREGION_MBOX4;

///
/// OpRegion Mailbox 5 - BIOS/Driver Notification - Data storage BIOS to Driver data sync
/// Offset 0x1C00, Size 0x400
///
typedef struct {
  UINT32 PHED;          ///< Offset 0x1C00 Panel Header
  UINT8  BDDC[0x100];   ///< Offset 0x1C04 Panel EDID (DDC data)
  UINT8  RM51[0x2FC];   ///< Offset 0x1D04 - 0x1FFF Reserved Must be zero
} IGD_OPREGION_MBOX5;

///
/// IGD OpRegion Structure
///
typedef struct {
  IGD_OPREGION_HEADER Header; ///< OpRegion header (Offset 0x0, Size 0x100)
  IGD_OPREGION_MBOX1  MBox1;  ///< Mailbox 1: Public ACPI Methods (Offset 0x100, Size 0x100)
  IGD_OPREGION_MBOX2  MBox2;  ///< Mailbox 2: Software SCI Interface (Offset 0x200, Size 0x100)
  IGD_OPREGION_MBOX3  MBox3;  ///< Mailbox 3: BIOS to Driver Notification (Offset 0x300, Size 0x100)
  IGD_OPREGION_MBOX4  MBox4;  ///< Mailbox 4: Video BIOS Table (VBT) (Offset 0x400, Size 0x1800)
  IGD_OPREGION_MBOX5  MBox5;  ///< Mailbox 5: BIOS to Driver Notification Extension (Offset 0x1C00, Size 0x400)
} IGD_OPREGION_STRUCTURE;
#pragma pack()

#endif
