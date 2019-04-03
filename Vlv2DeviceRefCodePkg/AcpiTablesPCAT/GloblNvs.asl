/**************************************************************************;
;*                                                                        *;
;*                                                                        *;
;*    Intel Corporation - ACPI Reference Code for the Baytrail            *;
;*    Family of Customer Reference Boards.                                *;
;*                                                                        *;
;*                                                                        *;
;*    Copyright (c)  1999  - 2016, Intel Corporation. All rights reserved   *;
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;*                                                                        *;
;*                                                                        *;
;**************************************************************************/



// Define a Global region of ACPI NVS Region that may be used for any
// type of implementation.  The starting offset and size will be fixed
// up by the System BIOS during POST.  Note that the Size must be a word
// in size to be fixed up correctly.

OperationRegion(GNVS,SystemMemory,0xFFFF0000,0xAA55)
Field(GNVS,AnyAcc,Lock,Preserve)
{
  Offset(0),       // Miscellaneous Dynamic Registers:
  OSYS,   16,      //   (00) Operating System
      ,   8,       //   (02)
      ,   8,       //   (03)
      ,   8,       //   (04)
      ,   8,       //   (05)
      ,   8,       //   (06)
      ,   8,       //   (07)
      ,   8,       //   (08)
      ,   8,       //   (09)
      ,   8,       //   (10)
  P80D,   32,      //   (11) Port 80 Debug Port Value
  LIDS,   8,       //   (15) Lid State (Lid Open = 1)
      ,   8,       //   (16)
      ,   8,       //   (17)
  Offset(18),      // Thermal Policy Registers:
      ,   8,       //   (18)
      ,   8,       //   (19)
  ACTT,   8,       //   (20) Active Trip Point
  PSVT,   8,       //   (21) Passive Trip Point
  TC1V,   8,       //   (22) Passive Trip Point TC1 Value
  TC2V,   8,       //   (23) Passive Trip Point TC2 Value
  TSPV,   8,       //   (24) Passive Trip Point TSP Value
  CRTT,   8,       //   (25) Critical Trip Point
  DTSE,   8,       //   (26) Digital Thermal Sensor Enable
  DTS1,   8,       //   (27) Digital Thermal Sensor 1 Reading
  DTS2,   8,       //   (28) Digital Thermal Sensor 2 Reading
  DTSF,   8,       //   (29) DTS SMI Function Call
  Offset(30),      // Battery Support Registers:
      ,   8,       //   (30)
      ,   8,       //   (31)
      ,   8,       //   (32)
      ,   8,       //   (33)
      ,   8,       //   (34)
      ,   8,       //   (35)
      ,   8,       //   (36)
  Offset(40),      // CPU Identification Registers:
  APIC,   8,       //   (40) APIC Enabled by SBIOS (APIC Enabled = 1)
  MPEN,   8,       //   (41) Number of Logical Processors if MP Enabled != 0
      ,   8,       //   (42)
      ,   8,       //   (43)
      ,   8,       //   (44)
      ,   32,      //   (45)
  Offset(50),      // SIO CMOS Configuration Registers:
      ,   8,       //   (50)
      ,   8,       //   (51)
      ,   8,       //   (52)
      ,   8,       //   (53)
      ,   8,       //   (54)
      ,   8,       //   (55)
      ,   8,       //   (56)
      ,   8,       //   (57)
      ,   8,       //   (58)
  Offset(60),      // Internal Graphics Registers:
      ,   8,       //   (60)
      ,   8,       //   (61)
  CADL,   8,       //   (62) Current Attached Device List
      ,   8,       //   (63)
  CSTE,   16,      //   (64) Current Display State
  NSTE,   16,      //   (66) Next Display State
      ,   16,      //   (68)
  NDID,   8,       //   (70) Number of Valid Device IDs
  DID1,   32,      //   (71) Device ID 1
  DID2,   32,      //   (75) Device ID 2
  DID3,   32,      //   (79) Device ID 3
  DID4,   32,      //   (83) Device ID 4
  DID5,   32,      //   (87) Device ID 5
      ,   32,      //   (91)
      ,   8,       //   (95) Fifth byte of AKSV (mannufacturing mode)
  Offset(103),     // Backlight Control Registers:
      ,   8,       //   (103)
  BRTL,   8,       //   (104) Brightness Level Percentage
  Offset(105),     // Ambiant Light Sensor Registers:
      ,   8,       //   (105)
      ,   8,       //   (106)
  LLOW,   8,       //   (107) LUX Low Value
      ,   8,       //   (108)
  Offset(110),     // EMA Registers:
      ,   8,       //   (110)
      ,   16,      //   (111)
      ,   16,      //   (113)
  Offset(116),     // MEF Registers:
      ,   8,       //   (116) MEF Enable
  Offset(117),     // PCIe Dock:
      ,   8,       //   (117)
  Offset(120),     // TPM Registers:
      ,   8,       //   (120)
      ,   8,       //   (121)
      ,   8,       //   (122)
      ,   8,       //   (123)
      ,   32,      //   (124)
      ,   8,       //   (125)
      ,   8,       //   (129)
  Offset(130),     //
      ,   56,      //   (130)
      ,   56,      //   (137)
      ,   8,       //   (144)
      ,   56,      //   (145)
  Offset(170),     // IGD OpRegion/Software SCI base address
  ASLB,   32,      //   (170) IGD OpRegion base address
  Offset(174),     // IGD OpRegion/Software SCI shared data
  IBTT,   8,       //   (174) IGD Boot Display Device
  IPAT,   8,       //   (175) IGD Panel Type CMOs option
  ITVF,   8,       //   (176) IGD TV Format CMOS option
  ITVM,   8,       //   (177) IGD TV Minor Format CMOS option
  IPSC,   8,       //   (178) IGD Panel Scaling
  IBLC,   8,       //   (179) IGD BLC Configuration
  IBIA,   8,       //   (180) IGD BIA Configuration
  ISSC,   8,       //   (181) IGD SSC Configuration
  I409,   8,       //   (182) IGD 0409 Modified Settings Flag
  I509,   8,       //   (183) IGD 0509 Modified Settings Flag
  I609,   8,       //   (184) IGD 0609 Modified Settings Flag
  I709,   8,       //   (185) IGD 0709 Modified Settings Flag
  IDMM,   8,       //   (186) IGD DVMT Mode
  IDMS,   8,       //   (187) IGD DVMT Memory Size
  IF1E,   8,       //   (188) IGD Function 1 Enable
  HVCO,   8,       //   (189) HPLL VCO
  NXD1,   32,      //   (190) Next state DID1 for _DGS
  NXD2,   32,      //   (194) Next state DID2 for _DGS
  NXD3,   32,      //   (198) Next state DID3 for _DGS
  NXD4,   32,      //   (202) Next state DID4 for _DGS
  NXD5,   32,      //   (206) Next state DID5 for _DGS
  NXD6,   32,      //   (210) Next state DID6 for _DGS
  NXD7,   32,      //   (214) Next state DID7 for _DGS
  NXD8,   32,      //   (218) Next state DID8 for _DGS
  GSMI,   8,       //   (222) GMCH SMI/SCI mode (0=SCI)
  PAVP,   8,       //   (223) IGD PAVP data
  Offset(225),
  OSCC,   8,       //   (225) PCIE OSC Control
  NEXP,   8,       //   (226) Native PCIE Setup Value
  Offset(235), // Global Variables
  DSEN,   8,       //   (235) _DOS Display Support Flag.
  ECON,   8,       //   (236) Embedded Controller Availability Flag.
  GPIC,   8,       //   (237) Global IOAPIC/8259 Interrupt Mode Flag.
  CTYP,   8,       //   (238) Global Cooling Type Flag.
  L01C,   8,       //   (239) Global L01 Counter.
  VFN0,   8,       //   (240) Virtual Fan0 Status.
  VFN1,   8,       //   (241) Virtual Fan1 Status.
  Offset(256),
  NVGA,   32,  //   (256) NVIG opregion address
  NVHA,   32,  //   (260) NVHM opregion address
  AMDA,   32,  //   (264) AMDA opregion address
  DID6,   32,  //   (268) Device ID 6
  DID7,   32,  //   (272) Device ID 7
  DID8,   32,  //   (276) Device ID 8
  Offset(332),
  USEL,   8,    // (332) UART Selection
  PU1E,   8,    // (333) PCU UART 1 Enabled
  PU2E,   8,    // (334) PCU UART 2 Enabled

  LPE0, 32,     // (335) LPE Bar0
  LPE1, 32,     // (339) LPE Bar1
  LPE2, 32,     // (343) LPE Bar2

  Offset(347),
      ,   8,    // (347)
      ,   8,    // (348)
  PFLV,   8,    // (349) Platform Flavor

  Offset(351),
  ICNF,   8,   //   (351) ISCT / AOAC Configuration
  XHCI,   8,   //   (352) xHCI controller mode
  PMEN,   8,   //   (353) PMIC enable/disable

  LPEE,   8,   //   (354) LPE enable/disable
  ISPA,   32,  //   (355) ISP Base Addr
  ISPD,   8,    //  (359) ISP Device Selection 0: Disabled; 1: PCI Device 2; 2: PCI Device 3

  offset(360),  // ((4+8+6)*4+2)*4=296
  //
  // Lpss controllers
  //
  PCIB,     32,
  PCIT,     32,
  D10A,     32,  //DMA1
  D10L,     32,
  D11A,     32,
  D11L,     32,
  P10A,     32,  //  PWM1
  P10L,     32,
  P11A,     32,
  P11L,     32,
  P20A,     32,  //  PWM2
  P20L,     32,
  P21A,     32,
  P21L,     32,
  U10A,     32,  // UART1
  U10L,     32,
  U11A,     32,
  U11L,     32,
  U20A,     32,  // UART2
  U20L,     32,
  U21A,     32,
  U21L,     32,
  SP0A,     32,  // SPI
  SP0L,     32,
  SP1A,     32,
  SP1L,     32,

  D20A,     32,  //DMA2
  D20L,     32,
  D21A,     32,
  D21L,     32,
  I10A,     32,  //  I2C1
  I10L,     32,
  I11A,     32,
  I11L,     32,
  I20A,     32,  //  I2C2
  I20L,     32,
  I21A,     32,
  I21L,     32,
  I30A,     32,  //  I2C3
  I30L,     32,
  I31A,     32,
  I31L,     32,
  I40A,     32,  //  I2C4
  I40L,     32,
  I41A,     32,
  I41L,     32,
  I50A,     32,  //  I2C5
  I50L,     32,
  I51A,     32,
  I51L,     32,
  I60A,     32,  //  I2C6
  I60L,     32,
  I61A,     32,
  I61L,     32,
  I70A,     32,  //  I2C7
  I70L,     32,
  I71A,     32,
  I71L,     32,
  //
  // Scc controllers
  //
  eM0A,     32,  //  EMMC
  eM0L,     32,
  eM1A,     32,
  eM1L,     32,
  SI0A,     32,  //  SDIO
  SI0L,     32,
  SI1A,     32,
  SI1L,     32,
  SD0A,     32,  //  SDCard
  SD0L,     32,
  SD1A,     32,
  SD1L,     32,
  MH0A,     32,  //
  MH0L,     32,
  MH1A,     32,
  MH1L,     32,

  offset(656),
  SDRM,     8,
  offset(657),
  HLPS,     8,   //(657) Hide Devices
  offset(658),
  OSEL,     8,      //(658) OS Seletion - Windows/Android

  offset(659),  // VLV1 DPTF
  SDP1,     8,      //(659) An enumerated value corresponding to SKU
  DPTE,     8,      //(660) DPTF Enable
  THM0,     8,      //(661) System Thermal 0
  THM1,     8,      //(662) System Thermal 1
  THM2,     8,      //(663) System Thermal 2
  THM3,     8,      //(664) System Thermal 3
  THM4,     8,      //(665) System Thermal 3
  CHGR,     8,      //(666) DPTF Changer Device
  DDSP,     8,      //(667) DPTF Display Device
  DSOC,     8,      //(668) DPTF SoC device
  DPSR,     8,      //(669) DPTF Processor device
  DPCT,     32,     //(670) DPTF Processor participant critical temperature
  DPPT,     32,     //(674) DPTF Processor participant passive temperature
  DGC0,     32,     //(678) DPTF Generic sensor0 participant critical temperature
  DGP0,     32,     //(682) DPTF Generic sensor0 participant passive temperature
  DGC1,     32,     //(686) DPTF Generic sensor1 participant critical temperature
  DGP1,     32,     //(690) DPTF Generic sensor1 participant passive temperature
  DGC2,     32,     //(694) DPTF Generic sensor2 participant critical temperature
  DGP2,     32,     //(698) DPTF Generic sensor2 participant passive temperature
  DGC3,     32,     //(702) DPTF Generic sensor3 participant critical temperature
  DGP3,     32,     //(706) DPTF Generic sensor3 participant passive temperature
  DGC4,     32,     //(710)DPTF Generic sensor3 participant critical temperature
  DGP4,     32,     //(714)DPTF Generic sensor3 participant passive temperature
  DLPM,     8,      //(718) DPTF Current low power mode setting
  DSC0,     32,     //(719) DPTF Critical threshold0 for SCU
  DSC1,     32,     //(723) DPTF Critical threshold1 for SCU
  DSC2,     32,     //(727) DPTF Critical threshold2 for SCU
  DSC3,     32,     //(731) DPTF Critical threshold3 for SCU
  DSC4,     32,     //(735) DPTF Critical threshold3 for SCU
  DDBG,     8,      //(739) DPTF Super Debug option. 0 - Disabled, 1 - Enabled
  LPOE,     32,     //(740) DPTF LPO Enable
  LPPS,     32,     //(744) P-State start index
  LPST,     32,     //(748) Step size
  LPPC,     32,     //(752) Power control setting
  LPPF,     32,     //(756) Performance control setting
  DPME,     8,      //(760) DPTF DPPM enable/disable
  BCSL,     8,      //(761) Battery charging solution 0-CLV 1-ULPMC
  NFCS,     8,      //(762) NFCx Select 1: NFC1    2:NFC2
  PCIM,     8,      //(763) EMMC device 0-ACPI mode, 1-PCI mode
  TPMA,     32,     //(764)
  TPML,     32,     //(768)
  ITSA,      8,     //(772) I2C Touch Screen Address
  S0IX,     8,      //(773) S0ix status
  SDMD,     8,      //(774) SDIO Mode
  EMVR,     8,      //(775) eMMC controller version
  BMBD,     32,     //(776) BM Bound
  FSAS,     8,      //(780) FSA Status
  BDID,     8,      //(781) Board ID
  FBID,     8,      //(782) FAB ID
  OTGM,     8,      //(783) OTG mode
  STEP,     8,      //(784) Stepping ID
  WITT,     8,      //(785) Enable Test Device connected to I2C for WHCK test.
  SOCS,     8,      //(786) provide the SoC stepping infomation
  AMTE,     8,      //(787) Ambient Trip point change
  UTS,      8,      //(788) Enable Test Device connected to URT for WHCK test.
  SCPE,     8,      //(789) Allow higher performance on AC/USB - Enable/Disable
  Offset(792),
  EDPV,     8,      //(792) Check for eDP display device
  DIDX,     32,     //(793) Device ID for eDP device
  IOT,      8,      //(794) MinnowBoard Max JP1 is configured for MSFT IOT project.
  BATT,     8,      //(795) The Flag of RTC Battery Prensent.
  LPAD,     8,      //(796)
}

