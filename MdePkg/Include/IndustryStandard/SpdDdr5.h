/** @file
  This file contains definitions for DDR5 SPD.

  Copyright (c) 2023, Qualcomm Innovation Center, Inc. All rights reserved.<BR>
  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - DDR5 Serial Presence Detect (SPD), Document Release 1.1 (Jan 2023)
      https://www.jedec.org/standards-documents/docs/jesd400-5a01
**/

#ifndef SPD_DDR5_H_
#define SPD_DDR5_H_

#include <Uefi/UefiBaseType.h>

#pragma pack (push, 1)

typedef union {
  struct {
    UINT8    BetaLevel3_0 :  4;                        ///< Bits 3:0
    UINT8    BytesTotal   :  3;                        ///< Bits 6:4
    UINT8    BetaLevel4   :  1;                        ///< Bits 7:7
  } Bits;
  UINT8    Data;
} SPD5_DEVICE_DESCRIPTION_STRUCT;

typedef union {
  struct {
    UINT8    Minor :  4;                               ///< Bits 3:0
    UINT8    Major :  4;                               ///< Bits 7:4
  } Bits;
  UINT8    Data;
} SPD5_REVISION_STRUCT;

typedef union {
  struct {
    UINT8    Type :  8;                                ///< Bits 7:0
  } Bits;
  UINT8    Data;
} SPD5_DEVICE_TYPE_STRUCT;

typedef union {
  struct {
    UINT8    ModuleType  :  4;                         ///< Bits 3:0
    UINT8    HybridMedia :  3;                         ///< Bits 6:4
    UINT8    Hybrid      :  1;                         ///< Bits 7:7
  } Bits;
  UINT8    Data;
} SPD5_MODULE_TYPE_STRUCT;

typedef union {
  struct {
    UINT8    Density :  5;                             ///< Bits 4:0
    UINT8    Die     :  3;                             ///< Bits 7:5
  } Bits;
  UINT8    Data;
} SPD5_SDRAM_DENSITY_PACKAGE_STRUCT;

typedef union {
  struct {
    UINT8    RowAddress    :  5;                       ///< Bits 4:0
    UINT8    ColumnAddress :  3;                       ///< Bits 7:5
  } Bits;
  UINT8    Data;
} SPD5_SDRAM_ADDRESSING_STRUCT;

typedef union {
  struct {
    UINT8    Reserved : 5;                             ///< Bits 4:0
    UINT8    IoWidth  : 3;                             ///< Bits 7:5
  } Bits;
  UINT8    Data;
} SPD5_SDRAM_IO_WIDTH_STRUCT;

typedef union {
  struct {
    UINT8    BanksPerBankGroup : 3;                    ///< Bits 2:0
    UINT8    Reserved          : 2;                    ///< Bits 4:3
    UINT8    BankGroups        : 3;                    ///< Bits 7:5
  } Bits;
  UINT8    Data;
} SPD5_SDRAM_BANKS_STRUCT;

typedef union {
  struct {
    UINT8    Reserved0       : 1;                      ///< Bits 0:0
    UINT8    Mbist_mPpr      : 1;                      ///< Bits 1:1
    UINT8    Reserved1       : 2;                      ///< Bits 3:2
    UINT8    BL32            : 1;                      ///< Bits 4:4
    UINT8    SPprUndo_Lock   : 1;                      ///< Bits 5:5
    UINT8    Reserved2       : 1;                      ///< Bits 6:6
    UINT8    SPprGranularity : 1;                      ///< Bits 7:7
  } Bits;
  UINT8    Data;
} SPD5_SDRAM_BL32_POST_PACKAGE_REPAIR_STRUCT;

typedef union {
  struct {
    UINT8    DcaTypesSupported : 2;                    ///< Bits 1:0
    UINT8    Reserved0         : 2;                    ///< Bits 3:2
    UINT8    Pasr              : 1;                    ///< Bits 4:4
    UINT8    Reserved1         : 3;                    ///< Bits 7:5
  } Bits;
  UINT8    Data;
} SPD5_SDRAM_DCA_AND_PASR_STRUCT;

typedef union {
  struct {
    UINT8    BoundedFault                    : 1;      ///< Bits 0:0
    UINT8    x4RmwEcsWbSuppressionMrSelector : 1;      ///< Bits 1:1
    UINT8    x4RmwEcsWriteBackSuppresion     : 1;      ///< Bits 2:2
    UINT8    WideTemperatureSense            : 1;      ///< Bits 3:3
    UINT8    Reserved                        : 4;      ///< Bits 7:4
  } Bits;
  UINT8    Data;
} SPD5_SDRAM_FAULT_HANDLING_TEMP_SENSE_STRUCT;

typedef union {
  struct {
    UINT8    Endurant    :  2;                         ///< Bits 1:0
    UINT8    Operational :  2;                         ///< Bits 3:2
    UINT8    Nominal     :  4;                         ///< Bits 7:4
  } Bits;
  UINT8    Data;
} SPD5_MODULE_NOMINAL_VOLTAGE_STRUCT;

typedef union {
  struct {
    UINT8    NonStandardCoreTimings :  1;              ///< Bits 0:0
    UINT8    Reserved               :  7;              ///< Bits 7:1
  } Bits;
  UINT8    Data;
} SPD5_SDRAM_TIMINGS_STRUCT;

typedef struct {
  UINT8    Lsb;
  UINT8    Msb;
} SPD5_SDRAM_T_STRUCT;

typedef union {
  struct {
    UINT32    Cl20     :  1;                           ///< Bits 0:0
    UINT32    Cl22     :  1;                           ///< Bits 1:1
    UINT32    Cl24     :  1;                           ///< Bits 2:2
    UINT32    Cl26     :  1;                           ///< Bits 3:3
    UINT32    Cl28     :  1;                           ///< Bits 4:4
    UINT32    Cl30     :  1;                           ///< Bits 5:5
    UINT32    Cl32     :  1;                           ///< Bits 6:6
    UINT32    Cl34     :  1;                           ///< Bits 7:7
    UINT32    Cl36     :  1;                           ///< Bits 8:8
    UINT32    Cl38     :  1;                           ///< Bits 9:9
    UINT32    Cl40     :  1;                           ///< Bits 10:10
    UINT32    Cl42     :  1;                           ///< Bits 11:11
    UINT32    Cl44     :  1;                           ///< Bits 12:12
    UINT32    Cl46     :  1;                           ///< Bits 13:13
    UINT32    Cl48     :  1;                           ///< Bits 14:14
    UINT32    Cl50     :  1;                           ///< Bits 15:15
    UINT32    Cl52     :  1;                           ///< Bits 16:16
    UINT32    Cl54     :  1;                           ///< Bits 17:17
    UINT32    Cl56     :  1;                           ///< Bits 18:18
    UINT32    Cl58     :  1;                           ///< Bits 19:19
    UINT32    Cl60     :  1;                           ///< Bits 20:20
    UINT32    Cl62     :  1;                           ///< Bits 21:21
    UINT32    Cl64     :  1;                           ///< Bits 22:22
    UINT32    Cl66     :  1;                           ///< Bits 23:23
    UINT32    Cl68     :  1;                           ///< Bits 24:24
    UINT32    Cl70     :  1;                           ///< Bits 25:25
    UINT32    Cl72     :  1;                           ///< Bits 26:26
    UINT32    Cl74     :  1;                           ///< Bits 27:27
    UINT32    Cl76     :  1;                           ///< Bits 28:28
    UINT32    Cl78     :  1;                           ///< Bits 29:29
    UINT32    Cl80     :  1;                           ///< Bits 30:30
    UINT32    Cl82     :  1;                           ///< Bits 31:31
  } Byte0_3;
  struct {
    UINT8    Cl84     :  1;                           ///< Bits 0:0
    UINT8    Cl86     :  1;                           ///< Bits 1:1
    UINT8    Cl88     :  1;                           ///< Bits 2:2
    UINT8    Cl90     :  1;                           ///< Bits 3:3
    UINT8    Cl92     :  1;                           ///< Bits 4:4
    UINT8    Cl94     :  1;                           ///< Bits 5:5
    UINT8    Cl96     :  1;                           ///< Bits 6:6
    UINT8    Cl98     :  1;                           ///< Bits 7:7
  } Byte4;
  UINT8    Data8[5];
} SPD5_CAS_LATENCIES_SUPPORTED_STRUCT;

typedef union {
  struct {
    UINT16    RfmRequired                  :  1;       ///< Bits 0:0
    UINT16    Raaimt                       :  4;       ///< Bits 4:1
    UINT16    Raammt                       :  3;       ///< Bits 7:5
    UINT16    DrfmSupported                :  1;       ///< Bits 8:8
    UINT16    RecommendedBoundedRefreshCfg :  2;       ///< Bits 10:9
    UINT16    BrcSupportLevel              :  1;       ///< Bits 11:11
    UINT16    ArfmLevel                    :  2;       ///< Bits 13:12
    UINT16    RfmRaaCounterDecPerRefCmd    :  2;       ///< Bits 15:14
  } Bits;
  UINT16    Data;
} SPD5_REFRESH_MANAGEMENT_STRUCT;

typedef struct {
  SPD5_DEVICE_DESCRIPTION_STRUCT               Description;                          ///< 0      Number of Bytes in SPD Device and Beta Level
  SPD5_REVISION_STRUCT                         Revision;                             ///< 1      SPD Revision for Base Configuration Parameters
  SPD5_DEVICE_TYPE_STRUCT                      HostBusCommandProtocolType;           ///< 2      Host Bus Command Protocol Type
  SPD5_MODULE_TYPE_STRUCT                      ModuleType;                           ///< 3      Module Type
  SPD5_SDRAM_DENSITY_PACKAGE_STRUCT            FirstSdramDensityAndPackage;          ///< 4      First SDRAM Density and Package
  SPD5_SDRAM_ADDRESSING_STRUCT                 FirstSdramAddressing;                 ///< 5      First SDRAM Addressing
  SPD5_SDRAM_IO_WIDTH_STRUCT                   FirstSdramIoWidth;                    ///< 6      First SDRAM I/O Width
  SPD5_SDRAM_BANKS_STRUCT                      FirstSdramBanks;                      ///< 7      First SDRAM Bank Groups and Banks Per Bank Group
  SPD5_SDRAM_DENSITY_PACKAGE_STRUCT            SecondSdramDensityAndPackage;         ///< 8      Second SDRAM Density and Package
  SPD5_SDRAM_ADDRESSING_STRUCT                 SecondSdramAddressing;                ///< 9      Second SDRAM Addressing
  SPD5_SDRAM_IO_WIDTH_STRUCT                   SecondSdramIoWidth;                   ///< 10     Second SDRAM I/O Width
  SPD5_SDRAM_BANKS_STRUCT                      SecondSdramBanks;                     ///< 11     Second SDRAM Bank Groups and Banks Per Bank Group
  SPD5_SDRAM_BL32_POST_PACKAGE_REPAIR_STRUCT   SdramBl32AndPostPackageRepair;        ///< 12     SDRAM BL32 and Post Package Repair
  SPD5_SDRAM_DCA_AND_PASR_STRUCT               SdramDcaAndPasr;                      ///< 13     SDRAM Duty Cycle Adjuster and Partial Array Self Refresh
  SPD5_SDRAM_FAULT_HANDLING_TEMP_SENSE_STRUCT  SdramFaultHandlingAndTempSense;       ///< 14     SDRAM Fault Handling and Temperature Sense
  UINT8                                        Reserved0;                            ///< 15     Reserved
  SPD5_MODULE_NOMINAL_VOLTAGE_STRUCT           ModuleNominalVoltageVdd;              ///< 16     SDRAM Nominal Voltage, VDD
  SPD5_MODULE_NOMINAL_VOLTAGE_STRUCT           ModuleNominalVoltageVddq;             ///< 17     SDRAM Nominal Voltage, VDDQ
  SPD5_MODULE_NOMINAL_VOLTAGE_STRUCT           ModuleNominalVoltageVpp;              ///< 18     SDRAM Nominal Voltage, VPP
  SPD5_SDRAM_TIMINGS_STRUCT                    SdramUsesNonStandardTimings;          ///< 19     SDRAM Timing
  SPD5_SDRAM_T_STRUCT                          tCKavgMin;                            ///< 20-21  SDRAM Minimum Cycle Time (tCK_avg_min)
  SPD5_SDRAM_T_STRUCT                          tCKavgMax;                            ///< 22-23  SDRAM Maximum Cycle Time (tCK_avg_max)
  SPD5_CAS_LATENCIES_SUPPORTED_STRUCT          CasLatenciesSupported;                ///< 24-28  SDRAM CAS Latenies Supported
  UINT8                                        Reserved1;                            ///< 29     Reserved
  SPD5_SDRAM_T_STRUCT                          tAA;                                  ///< 30-31  SDRAM Read Command to First Data (tAA)
  SPD5_SDRAM_T_STRUCT                          tRCD;                                 ///< 32-33  SDRAM Activate to Read or Write Command Delay (tRCD)
  SPD5_SDRAM_T_STRUCT                          tRP;                                  ///< 34-35  SDRAM Row Precharge Time (tRP)
  SPD5_SDRAM_T_STRUCT                          tRAS;                                 ///< 36-37  SDRAM Activate to Precharge Command Period (tRAS)
  SPD5_SDRAM_T_STRUCT                          tRC;                                  ///< 38-39  SDRAM Activate to Activate or Refresh Command Period (tRC)
  SPD5_SDRAM_T_STRUCT                          tWR;                                  ///< 40-41  SDRAM Write Recovery Time (tWR)
  SPD5_SDRAM_T_STRUCT                          tRFC1_tRFC1Slr;                       ///< 42-43  SDRAM Normal Refresh Recovery Time (tRFC1, tRFC1_slr)
  SPD5_SDRAM_T_STRUCT                          tRFC2_tRFC2Slr;                       ///< 44-45  SDRAM Fine Granularity Refresh Recovery Time (tRFC2, tRFC2_slr)
  SPD5_SDRAM_T_STRUCT                          tRFCsb_tRFCsbSlr;                     ///< 46-47  SDRAM Same Bank Refresh Recovery Time (tRFC_sb, tRFC_sb_slr)
  SPD5_SDRAM_T_STRUCT                          tRFC1Dlr;                             ///< 48-49  SDRAM Normal Refresh Recovery Time, 3DS Different Logical Rank (tRFC1_dlr)
  SPD5_SDRAM_T_STRUCT                          tRFC2Dlr;                             ///< 50-51  SDRAM Fine Granularity Refresh Recovery Time, 3DS Different Logical Rank (tRFC2_dir)
  SPD5_SDRAM_T_STRUCT                          tRFCsbDir;                            ///< 52-53  SDRAM Same Bank Refresh Recovery Time, 3DS Different Logical Rank (tRFC_sb_dlr)
  SPD5_REFRESH_MANAGEMENT_STRUCT               FirstSdramRefreshManagement;          ///< 54-55  SDRAM Refresh Management, First SDRAM
  SPD5_REFRESH_MANAGEMENT_STRUCT               SecondSdramRefreshManagement;         ///< 56-57  SDRAM Refresh Management, Second SDRAM
  SPD5_REFRESH_MANAGEMENT_STRUCT               FirstSdramAdaptiveRefreshMgmtLevelA;  ///< 58-59  SDRAM Adaptive Refresh Management, First DRAM, ARFM Level A
  SPD5_REFRESH_MANAGEMENT_STRUCT               SecondSdramAdaptiveRefreshMgmtLevelA; ///< 60-61  SDRAM Adaptive Refresh Management, Second SDRAM, ARFM Level A
  SPD5_REFRESH_MANAGEMENT_STRUCT               FirstSdramAdaptiveRefreshMgmtLevelB;  ///< 62-63  SDRAM Adaptive Refresh Management, First SDRAM, ARFM Level B
  SPD5_REFRESH_MANAGEMENT_STRUCT               SecondSdramAdaptiveRefreshMgmtLevelB; ///< 64-65  SDRAM Adaptive Refresh Management, Second SDRAM, ARFM Level B
  SPD5_REFRESH_MANAGEMENT_STRUCT               FirstSdramAdaptiveRefreshMgmtLevelC;  ///< 66-67  SDRAM Adaptive Refresh Management, First SDRAM, ARFM Level C
  SPD5_REFRESH_MANAGEMENT_STRUCT               SecondSdramAdaptiveRefreshMgmtLevelC; ///< 68-69  SDRAM Adaptive Refresh Management, Second SDRAM, ARFM Level C
  SPD5_SDRAM_T_STRUCT                          tRRD_L;                               ///< 70-71  SDRAM Activate to Activate Command Delay for Same Bank Group (tRRD_L)
  UINT8                                        tRRD_L_LowerClockLimit;               ///< 72     SDRAM Activate to Activate Command Delay for Same Bank Group, Lower Clock Limit
  SPD5_SDRAM_T_STRUCT                          tCCD_L;                               ///< 73-74  SDRAM Read to Read Command Delay for Same Bank Group (tCCD_L)
  UINT8                                        tCCD_L_LowerClockLimit;               ///< 75     SDRAM Read to Read Command Delay for Same Bank Group, Lower Clock Limit
  SPD5_SDRAM_T_STRUCT                          tCCD_L_WR;                            ///< 76-77  SDRAM Write to Write Command Delay for Same Bank Group (tCCD_L_WR)
  UINT8                                        tCCD_L_WR_LowerClockLimit;            ///< 78     SDRAM Write to Write Command Delay for Same Bank Group, Lower Clock Limit
  SPD5_SDRAM_T_STRUCT                          tCCD_L_WR2;                           ///< 79-80  SDRAM Write to Write Command Delay for Same Bank Group, Second Write not RMW (tCCD_L_WR2)
  UINT8                                        tCCD_L_WR2_LowerClockLimit;           ///< 81     SDRAM Write to Write Command Delay for Same Bank Group, Second Write not RMW, Lower Clock Limit
  SPD5_SDRAM_T_STRUCT                          tFAW;                                 ///< 82-83  SDRAM Four Activate Window (tFAW)
  UINT8                                        tFAW_LowerClockLimit;                 ///< 84     SDRAM Four Activate Window, Lower Clock Limit
  SPD5_SDRAM_T_STRUCT                          tCCD_L_WTR;                           ///< 85-86  SDRAM Write to Read Command Delay for Same Bank (tCCD_L_WTR)
  UINT8                                        tCCD_L_WTR_LowerClockLimit;           ///< 87     SDRAM Write to Read Command Delay for Same Bank, Lower Clock Limit
  SPD5_SDRAM_T_STRUCT                          tCCD_S_WTR;                           ///< 88-89  SDRAM Write to Read Command Delay for Different Bank Group (tCCD_S_WTR)
  UINT8                                        tCCD_S_WTR_LowerClockLimit;           ///< 90     SDRAM Write to Read Command Delay for Different Bank Group, Lower Clock Limit
  SPD5_SDRAM_T_STRUCT                          tRTP_tRTPslr;                         ///< 91-92  SDRAM Read to Precharge Command Delay (tRTP, tRTP_slr)
  UINT8                                        tRTP_tRTPslr_LowerClockLimit;         ///< 93     SDRAM Read to Precharge Command Delay, Lower Clock Limit
  UINT8                                        Reserved2[127 - 94 + 1];              ///< 94-127 Reserved
} SPD5_BASE_SECTION;

STATIC_ASSERT (sizeof (SPD5_BASE_SECTION)   == 128, "sizeof (SPD5_BASE_SECTION) does not match DDR5 SPD 1.1 specification");

typedef union {
  struct {
    UINT8    SerialNumberHashingSequence : 3;       ///< Bits 2:0
    UINT8    Reserved                    : 5;       ///< Bits 7:3
  } Bits;
  UINT8    Data;
} SPD5_HASHING_SEQUENCE;

typedef union {
  struct {
    UINT16    ContinuationCount  :  7;              ///< Bits 6:0
    UINT16    ContinuationParity :  1;              ///< Bits 7:7
    UINT16    LastNonZeroByte    :  8;              ///< Bits 15:8
  } Bits;
  UINT16    Data;
  UINT8     Data8[2];
} SPD5_MANUFACTURER_ID_CODE;

typedef union {
  struct {
    UINT8    DeviceType      :  4;                  ///< Bits 3:0
    UINT8    Reserved0       :  2;                  ///< Bits 5:4
    UINT8    Rsvd_TempSensor :  1;                  ///< Bits 6:6
    UINT8    DeviceInstalled :  1;                  ///< Bits 7:7
  } Bits;
  UINT8    Data;
} SPD5_MODULE_DEVICE_TYPE;

typedef union {
  struct {
    UINT8    Height   :  5;                         ///< Bits 4:0
    UINT8    Reserved :  3;                         ///< Bits 7:5
  } Bits;
  UINT8    Data;
} SPD5_MODULE_NOMINAL_HEIGHT;

typedef union {
  struct {
    UINT8    FrontThickness :  4;                   ///< Bits 3:0
    UINT8    BackThickness  :  4;                   ///< Bits 7:4
  } Bits;
  UINT8    Data;
} SPD5_MODULE_NOMINAL_THICKNESS;

typedef union {
  struct {
    UINT8    Card      :  5;                        ///< Bits 4:0
    UINT8    Revision  :  3;                        ///< Bits 7:5
  } Bits;
  UINT8    Data;
} SPD5_MODULE_REFERENCE_RAW_CARD;

typedef union {
  struct {
    UINT8    DramRowCount       :  2;               ///< Bits 1:0
    UINT8    HeatSpreader       :  1;               ///< Bits 2:2
    UINT8    Reserved           :  1;               ///< Bits 3:3
    UINT8    OperatingTempRange :  4;               ///< Bits 7:4
  } Bits;
  UINT8    Data;
} SPD5_MODULE_DIMM_ATTRIBUTES;

typedef union {
  struct {
    UINT8    Reserved0          :  3;               ///< Bits 2:0
    UINT8    PackageRanksCount  :  3;               ///< Bits 5:3
    UINT8    RankMix            :  1;               ///< Bits 6:6
    UINT8    Reserved1          :  1;               ///< Bits 7:7
  } Bits;
  UINT8    Data;
} SPD5_MODULE_ORGANIZATION;

typedef union {
  struct {
    UINT8    PrimaryBusWidthPerSubChannel    :  3;  ///< Bits 2:0
    UINT8    BusWidthExtensionPerSubChannel  :  2;  ///< Bits 4:3
    UINT8    SubChannelsPerDimmCount         :  2;  ///< Bits 6:5
    UINT8    Reserved                        :  1;  ///< Bits 7:7
  } Bits;
  UINT8    Data;
} SPD5_MEMORY_CHANNEL_BUS_WIDTH;


typedef struct {
  SPD5_REVISION_STRUCT            SpdRevisionBytes192_447;     ///< 192     SPD Revision for SPD bytes 192-447
  SPD5_HASHING_SEQUENCE           HashingSequence;             ///< 193     Hashing Sequence
  SPD5_MANUFACTURER_ID_CODE       SpdManufacturer;             ///< 194-195 SPD Manufacturer ID Code
  SPD5_MODULE_DEVICE_TYPE         SpdDeviceType;               ///< 196     SPD Device Type
  SPD5_REVISION_STRUCT            SpdRevisionNumber;           ///< 197     SPD Device Revision Number
  SPD5_MANUFACTURER_ID_CODE       Pmic0Manufacturer;           ///< 198-199 PMIC 0 Manufacturer ID Code
  SPD5_MODULE_DEVICE_TYPE         Pmic0DeviceType;             ///< 200     PMIC 0 Device Type
  SPD5_REVISION_STRUCT            Pmic0RevisionNumber;         ///< 201     PMIC 0 Revision Number
  SPD5_MANUFACTURER_ID_CODE       Pmic1Manufacturer;           ///< 202-203 PMIC 1 Manufacturer ID Code
  SPD5_MODULE_DEVICE_TYPE         Pmic1DeviceType;             ///< 204     PMIC 1 Device Type
  SPD5_REVISION_STRUCT            Pmic1RevisionNumber;         ///< 205     PMIC 1 Revision Number
  SPD5_MANUFACTURER_ID_CODE       Pmic2Manufacturer;           ///< 206-207 PMIC 2 Manufacturer ID Code
  SPD5_MODULE_DEVICE_TYPE         Pmic2DeviceType;             ///< 208     PMIC 2 Device Type
  SPD5_REVISION_STRUCT            Pmic2RevisionNumber;         ///< 209     PMIC 2 Revision Number
  SPD5_MANUFACTURER_ID_CODE       ThermalSensorManufacturer;   ///< 210-211 Thermal Sensor Manufacturer ID Code
  SPD5_MODULE_DEVICE_TYPE         ThermalSensorDeviceType;     ///< 212     Thermal Sensor Revision Number
  SPD5_REVISION_STRUCT            ThermalSensorRevisionNumber; ///< 213     Thermal Sensor Revision Number
  UINT8                           Reserved0[229 - 214 + 1];    ///< 214-229 Reserved
  SPD5_MODULE_NOMINAL_HEIGHT      ModuleNominalHeight;         ///< 230     Module Nominal Height
  SPD5_MODULE_NOMINAL_THICKNESS   ModuleMaximumThickness;      ///< 231     Module Maximum Thickness
  SPD5_MODULE_REFERENCE_RAW_CARD  ReferenceRawCardUsed;        ///< 232     Reference Raw Card Used
  SPD5_MODULE_DIMM_ATTRIBUTES     DimmAttributes;              ///< 233     DIMM Attributes
  SPD5_MODULE_ORGANIZATION        ModuleOrganization;          ///< 234     Module Organization
  SPD5_MEMORY_CHANNEL_BUS_WIDTH   MemoryChannelBusWidth;       ///< 235     Memory Channel Bus Width
  UINT8                           Reserved1[239 - 236 + 1];    ///< 236-239 Reserved
} SPD5_MODULE_COMMON;

 STATIC_ASSERT (sizeof (SPD5_MODULE_COMMON)   == 48, "sizeof (SPD5_MODULE_COMMON) does not match DDR5 SPD 1.1 specification");

typedef struct {
  UINT8 Reserved[447 - 240 + 1]; ///< 240-447 Reserved
} SPD5_MODULE_SOLDER_DOWN;

STATIC_ASSERT (sizeof (SPD5_MODULE_SOLDER_DOWN)   == 208, "sizeof (SPD5_MODULE_SOLDER_DOWN) does not match DDR5 SPD 1.1 specification");

typedef struct {
  SPD5_MANUFACTURER_ID_CODE  ClockDriverManufacturerId;  ///< 240-241 Clock Driver Manufacturer ID Code
  SPD5_DEVICE_TYPE_STRUCT    ClockDriverDeviceType;      ///< 242     Clock Driver Device Type
  SPD5_REVISION_STRUCT       ClockDriverRevisionNumber;  ///< 243     Clock Driver Revision Number
  UINT8                      Reserved[447 - 244 + 1];
} SPD5_MODULE_UNBUFFERED;

STATIC_ASSERT (sizeof (SPD5_MODULE_UNBUFFERED)   == 208, "sizeof (SPD5_MODULE_UNBUFFERED) does not match DDR5 SPD 1.1 specification");

typedef struct {
  SPD5_MANUFACTURER_ID_CODE  RegisteringClockDriverManufacturerId;            ///< 240-241 Registering Clock Driver Manufacturer ID Code
  SPD5_DEVICE_TYPE_STRUCT    RegisterDeviceType;                              ///< 242     Register Device Type
  SPD5_REVISION_STRUCT       RegisterRevisionNumber;                          ///< 243     Register Revision Number
  SPD5_MANUFACTURER_ID_CODE  DataBufferManufacturerId;                        ///< 244-245 Data Buffer Manufacturer ID Code
  SPD5_DEVICE_TYPE_STRUCT    DataBufferDeviceType;                            ///< 246     Data Buffer Device Type
  SPD5_REVISION_STRUCT       DataBufferRevisionNumber;                        ///< 247     Data Buffer Revision Number
  UINT8                      RcdRw08ClockDriverEnable;                        ///< 248     RCD-RW08 Clock Driver Enable
  UINT8                      RcdRw09OutputAddressAndControlEnable;            ///< 249     RCD-RW09 Output Address and Control Enable
  UINT8                      RcdRw0AQckDriverCharacteristics;                 ///< 250     RCD-RW0A QCK Driver Characteristics
  UINT8                      RcdRw0B;                                         ///< 251     RCD-RW0B
  UINT8                      RcdRw0CQxCaAndQcsnDriverCharacteristics;         ///< 252     RCD-RW0C QxCA and QxCS_n Driver Characteristics
  UINT8                      RcdRw0DDataBufferInterfaceDriverCharacteristics; ///< 253     RCD-RW0D Data Buffer Interface Driver Characteristics
  UINT8                      RcdRw0EQckQcaAndQcsOutputSlewRate;               ///< 254     RCD-RW0E QCK, QCA and QCS Output Slew Rate
  UINT8                      RcdRw0FBckBcomAndBcsOutputSlewRate;              ///< 255     RCD-RW0F BCK, BCOM, and BCS Output Slew Rate
  UINT8                      DbRw86DqsRttParkTermination;                     ///< 256     DB-RW86 DQS RTT Park Termination
  UINT8                      Reserved[447 - 257 + 1];                         ///< 257-447 Reserved
} SPD5_MODULE_REGISTERED;

STATIC_ASSERT (sizeof (SPD5_MODULE_REGISTERED)   == 208, "sizeof (SPD5_MODULE_REGISTERED) does not match DDR5 SPD 1.1 specification");

typedef struct {
  SPD5_MANUFACTURER_ID_CODE  MultiplexRegisteringClockDriverManufacturerId;   ///< 240-241 Multiplex Registering Clock Driver Manufacturer ID Code
  SPD5_DEVICE_TYPE_STRUCT    RegisterDeviceType;                              ///< 242     Register Device Type
  SPD5_REVISION_STRUCT       RegisterRevisionNumber;                          ///< 243     Register Revision Number
  SPD5_MANUFACTURER_ID_CODE  MultiplexDataBufferManufacturerId;               ///< 244-245 Multiplex Data Buffer Manufacturer ID Code
  SPD5_DEVICE_TYPE_STRUCT    DataBufferDeviceType;                            ///< 246     Data Buffer Device Type
  SPD5_REVISION_STRUCT       DataBufferRevisionNumber;                        ///< 247     Data Buffer Revision Number
  UINT8                      Reserved[447 - 248 + 1];                         ///< 248-447 Reserved
} SPD5_MODULE_MULTIPLEXED_RANK;

STATIC_ASSERT (sizeof (SPD5_MODULE_MULTIPLEXED_RANK)   == 208, "sizeof (SPD5_MODULE_MULTIPLEXED_RANK) does not match DDR5 SPD 1.1 specification");

typedef struct {
  SPD5_MANUFACTURER_ID_CODE DifferentialMemoryBufferManufacturerId;           ///< 240-241 Differential Memory Buffer Manufacturer ID Code
  SPD5_DEVICE_TYPE_STRUCT   DifferentialMemoryBufferDeviceType;               ///< 242     Differential Memory Buffer Device Type
  SPD5_REVISION_STRUCT      DifferentialMemoryBufferRevisionNumber;           ///< 243     Differential Memory Buffer Revision Number
  UINT8                     Reserved[447 - 244 + 1];                          ///< 244-447 Reserved
} SPD5_MODULE_DIFFERENTIAL;

STATIC_ASSERT (sizeof (SPD5_MODULE_DIFFERENTIAL)   == 208, "sizeof (SPD5_MODULE_DIFFERENTIAL) does not match DDR5 SPD 1.1 specification");

typedef struct {
  SPD5_MANUFACTURER_ID_CODE  RegisteringClockDriverManufacturerId; ///< 240-241 Registering Clock Driver Manufacturer ID Code
  SPD5_DEVICE_TYPE_STRUCT    RegisterDeviceType;                   ///< 242     Register Device Type
  SPD5_REVISION_STRUCT       RegisterRevisionNumber;               ///< 243     Register Revision Number
  SPD5_MANUFACTURER_ID_CODE  DataBufferManufacturerId;             ///< 244-245 Data Buffer Manufacturer ID Code
  SPD5_DEVICE_TYPE_STRUCT    DataBufferDeviceType;                 ///< 246     Data Buffer Device Type
  SPD5_REVISION_STRUCT       DataBufferRevisionNumber;             ///< 247     Data Buffer Revision Number
  UINT8                      Reserved[447 - 248 + 1];              ///< 248-447 Reserved
} SPD5_MODULE_NVDIMM_N;

STATIC_ASSERT (sizeof (SPD5_MODULE_NVDIMM_N)   == 208, "sizeof (SPD5_MODULE_NVDIMM_N) does not match DDR5 SPD 1.1 specification");

typedef struct {
  SPD5_MANUFACTURER_ID_CODE  RegisteringClockDriverManufacturerId; ///< 240-241 Registering Clock Driver Manufacturer ID Code
  SPD5_DEVICE_TYPE_STRUCT    RegisterDeviceType;                   ///< 242     Register Device Type
  SPD5_REVISION_STRUCT       RegisterRevisionNumber;               ///< 243     Register Revision Number
  SPD5_MANUFACTURER_ID_CODE  DataBufferManufacturerId;             ///< 244-245 Data Buffer Manufacturer ID Code
  SPD5_DEVICE_TYPE_STRUCT    DataBufferDeviceType;                 ///< 246     Data Buffer Device Type
  SPD5_REVISION_STRUCT       DataBufferRevisionNumber;             ///< 247     Data Buffer Revision Number
  UINT8                      ModuleStorageCapacity[2];             ///< 248-249 Module Storage Capacity
  UINT8                      ProtocolProfile;                      ///< 250     Protocol Profile
  UINT8                      Reserved[447 - 251 + 1];              ///< 251-447 Reserved
} SPD5_MODULE_NVDIMM_P;

STATIC_ASSERT (sizeof (SPD5_MODULE_NVDIMM_P)   == 208, "sizeof (SPD5_MODULE_NVDIMM_P) does not match DDR5 SPD 1.1 specification");

typedef union {
  SPD5_MODULE_SOLDER_DOWN       SolderDown;      ///< 240-447 Solder Down Memory Module Types
  SPD5_MODULE_UNBUFFERED        Unbuffered;      ///< 240-447 Unbuffered Memory Module Types
  SPD5_MODULE_REGISTERED        Registered;      ///< 240-447 Registered Memory Module Types
  SPD5_MODULE_MULTIPLEXED_RANK  MultiplexedRank; ///< 240-447 Mutiplexed Rank Memory Module Types
  SPD5_MODULE_DIFFERENTIAL      Differential;    ///< 240-447 Differentual (DDR-D) Memory Module Types
  SPD5_MODULE_NVDIMM_N          NvDimmN;         ///< 240-447 Non-Volatile (NVDIMM-N) Hybrid Memory Parameters
  SPD5_MODULE_NVDIMM_P          NvDimmP;         ///< 240-447 Non-Volatile (NVDIMM-P) Hybrid Memory Parameters
} SPD5_MODULE_SPECIFIC;

STATIC_ASSERT (sizeof (SPD5_MODULE_SPECIFIC)   == 208, "sizeof (SPD5_MODULE_SPECIFIC) does not match DDR5 SPD 1.1 specification");

typedef struct {
  UINT8    ManufacturerSpecificData[639 - 555 + 1]; ///< 639-555 Manufacturer's Specific Data
} SPD5_MANUFACTURER_SPECIFIC;

STATIC_ASSERT (sizeof (SPD5_MANUFACTURER_SPECIFIC)   == 85, "sizeof (SPD5_MANUFACTURER_SPECIFIC) does not match DDR5 SPD 1.1 specification");

typedef struct {
  UINT8    Year;                                 ///< Year represented in BCD (00h = 2000)
  UINT8    Week;                                 ///< Year represented in BCD (47h = week 47)
} SPD5_MANUFACTURING_DATE;

typedef union {
  UINT32    SerialNumber32;                      ///< 517-520 Serial Number
  UINT16    SerialNumber16[2];
  UINT8     SerialNumber8[4];
} SPD5_MANUFACTURER_SERIAL_NUMBER;

typedef struct {
  UINT8    ModulePartNumber[550 - 521 + 1];      ///< 521-550 Module Part Number
} SPD5_MODULE_PART_NUMBER;

typedef UINT8 SPD5_MANUFACTURING_LOCATION;       ///< 514 Module Manufacturing Location
typedef UINT8 SPD5_MODULE_REVISION_CODE;         ///< 551 Module Revision Code
typedef UINT8 SPD5_DRAM_STEPPING;                ///< 554 DRAM Stepping

typedef struct {
  SPD5_MANUFACTURER_ID_CODE        ModuleManufacturer;          ///< 512-513 Module Manufacturer ID Code
  SPD5_MANUFACTURING_LOCATION      ModuleManufacturingLocation; ///< 514     Module Manufacuring Location
  SPD5_MANUFACTURING_DATE          ModuleManufacturingDate;     ///< 515-516 Module Manufacturing Date
  SPD5_MANUFACTURER_SERIAL_NUMBER  ModuleSerialNumber;          ///< 517-520 Module Serial Number
  SPD5_MODULE_PART_NUMBER          ModulePartNumber;            ///< 521-550 Module Part Number
  SPD5_MODULE_REVISION_CODE        ModuleRevisionCode;          ///< 551     Module Revision Code
  SPD5_MANUFACTURER_ID_CODE        DramManufacturer;            ///< 552-553 DRAM Manufacturer ID Code
  SPD5_DRAM_STEPPING               DramStepping;                ///< 554     DRAM Stepping
  SPD5_MANUFACTURER_SPECIFIC       ManufacturerSpecificData;    ///< 555-639 Manufacturer's Specific Data
} SPD5_MANUFACTURING_DATA;

STATIC_ASSERT (sizeof (SPD5_MANUFACTURING_DATA)   == 128, "sizeof (SPD5_MANUFACTURING_DATA) does not match DDR5 SPD 1.1 specification");

typedef struct {
  UINT8 EndUserBytes[1023 - 640 + 1];                ///< 640-1023 End User Programmable Bytes
} SPD5_END_USER_SECTION;

STATIC_ASSERT (sizeof (SPD5_END_USER_SECTION)   == 384, "sizeof (SPD5_END_USER_SECTION) does not match DDR5 SPD 1.1 specification");

///
/// DDR5 Serial Presence Detect structure
///
typedef struct {
  SPD5_BASE_SECTION        Base;                     ///<   0-127 Base Configuration and DRAM Parameters
  UINT8                    Reserved0[191 - 128 + 1]; ///< 128-191 Reserved
  SPD5_MODULE_COMMON       Common;                   ///< 192-239 Common Module Parameters
  SPD5_MODULE_SPECIFIC     Module;                   ///< 240-447 Module Type Specific Information
  UINT8                    Reserved1[509 - 448 + 1]; ///< 448-509 Reserved for future use
  UINT8                    Crc[2];                   ///< 510-511 CRC for bytes 0-509
  SPD5_MANUFACTURING_DATA  ManufacturerInfo;         ///< 512-639 Manufacturing Information
  SPD5_END_USER_SECTION    EndUser;                  ///< 640-1023 End User Programmable
} SPD_DDR5;

STATIC_ASSERT (sizeof (SPD_DDR5)   == 1024, "sizeof (SPD_DDR5) does not match DDR5 SPD 1.1 specification");

#pragma pack (pop)

#endif /* SPD_DDR5_H_ */
