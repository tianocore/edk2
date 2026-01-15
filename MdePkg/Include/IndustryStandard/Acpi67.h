/** @file                                                                                         // [CODE_FIRST] 11148
  ACPI 6.7 definitions from the ACPI Specification Revision 6.7.                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  Copyright (c) 2017 - 2025, Intel Corporation. All rights reserved.<BR>                          // [CODE_FIRST] 11148
  Copyright (c) 2019 - 2025, ARM Ltd. All rights reserved.<BR>                                    // [CODE_FIRST] 11148
  Copyright (c) 2023, Loongson Technology Corporation Limited. All rights reserved.<BR>           // [CODE_FIRST] 11148
  Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  SPDX-License-Identifier: BSD-2-Clause-Patent                                                    // [CODE_FIRST] 11148
**/// [CODE_FIRST] 11148
// [CODE_FIRST] 11148

#ifndef ACPI_6_7_H_                                                                               // [CODE_FIRST] 11148
#define ACPI_6_7_H_  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
#include <IndustryStandard/Acpi66.h> // [CODE_FIRST] 11148
                                     // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Ensure proper structure formats                                                                // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#pragma pack(1)                                                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// _STA bit definitions ACPI 6.7 s6.3.7                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define ACPI_AML_STA_DEVICE_STATUS_PRESET       0x1                                               // [CODE_FIRST] 11148
#define ACPI_AML_STA_DEVICE_STATUS_ENABLED      0x2                                               // [CODE_FIRST] 11148
#define ACPI_AML_STA_DEVICE_STATUS_UI           0x4                                               // [CODE_FIRST] 11148
#define ACPI_AML_STA_DEVICE_STATUS_FUNCTIONING  0x8                                               // [CODE_FIRST] 11148
#define ACPI_AML_STA_DEVICE_STATUS_BATTERY      0x10                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// _CSD Revision for ACPI 6.7                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_AML_CSD_REVISION  0                                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// _CSD NumEntries for ACPI 6.7                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_AML_CSD_NUM_ENTRIES  6                                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// _PSD Revision for ACPI 6.7                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_AML_PSD_REVISION  0                                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// _CPC Revision for ACPI 6.7                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_AML_CPC_REVISION  3                                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ACPI 6.7 Generic Address Space definition                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     AddressSpaceId;                                                                       // [CODE_FIRST] 11148
  UINT8     RegisterBitWidth;                                                                     // [CODE_FIRST] 11148
  UINT8     RegisterBitOffset;                                                                    // [CODE_FIRST] 11148
  UINT8     AccessSize;                                                                           // [CODE_FIRST] 11148
  UINT64    Address;                                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE;                                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Generic Address Space Address IDs                                                              // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SYSTEM_MEMORY                   0x00                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SYSTEM_IO                       0x01                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCI_CONFIGURATION_SPACE         0x02                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EMBEDDED_CONTROLLER             0x03                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SMBUS                           0x04                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SYSTEM_CMOS                     0x05                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCI_BAR_TARGET                  0x06                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IPMI                            0x07                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GENERAL_PURPOSE_IO              0x08                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GENERIC_SERIAL_BUS              0x09                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLATFORM_COMMUNICATION_CHANNEL  0x0A                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLATFORM_RUNTIME_MECHANISM      0x0B                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FUNCTIONAL_FIXED_HARDWARE       0x7F                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Generic Address Space Access Sizes                                                             // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_UNDEFINED  0                                                                 // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_BYTE       1                                                                 // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_WORD       2                                                                 // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_DWORD      3                                                                 // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_QWORD      4                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// ACPI 6.7 table structures                                                                      // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Root System Description Pointer Structure                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT64    Signature;                                                                            // [CODE_FIRST] 11148
  UINT8     Checksum;                                                                             // [CODE_FIRST] 11148
  UINT8     OemId[6];                                                                             // [CODE_FIRST] 11148
  UINT8     Revision;                                                                             // [CODE_FIRST] 11148
  UINT32    RsdtAddress;                                                                          // [CODE_FIRST] 11148
  UINT32    Length;                                                                               // [CODE_FIRST] 11148
  UINT64    XsdtAddress;                                                                          // [CODE_FIRST] 11148
  UINT8     ExtendedChecksum;                                                                     // [CODE_FIRST] 11148
  UINT8     Reserved[3];                                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_ROOT_SYSTEM_DESCRIPTION_POINTER;                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// RSD_PTR Revision (as defined in ACPI 6.7 spec.)                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION  0x02 ///< ACPISpec (Revision 6.7) says current value is 2  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Common table header, this prefaces all ACPI tables, including FACS, but                       // [CODE_FIRST] 11148
/// excluding the RSD PTR structure                                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    Signature;                                                                            // [CODE_FIRST] 11148
  UINT32    Length;                                                                               // [CODE_FIRST] 11148
} EFI_ACPI_6_7_COMMON_HEADER;                                                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Root System Description Table                                                                  // [CODE_FIRST] 11148
// No definition needed as it is a common description table header, the same with                 // [CODE_FIRST] 11148
// EFI_ACPI_DESCRIPTION_HEADER, followed by a variable number of UINT32 table pointers.           // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// RSDT Revision (as defined in ACPI 6.7 spec.)                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ROOT_SYSTEM_DESCRIPTION_TABLE_REVISION  0x01                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Extended System Description Table                                                              // [CODE_FIRST] 11148
// No definition needed as it is a common description table header, the same with                 // [CODE_FIRST] 11148
// EFI_ACPI_DESCRIPTION_HEADER, followed by a variable number of UINT64 table pointers.           // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// XSDT Revision (as defined in ACPI 6.7 spec.)                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EXTENDED_SYSTEM_DESCRIPTION_TABLE_REVISION  0x01                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Fixed ACPI Description Table Structure (FADT)                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER               Header;                                               // [CODE_FIRST] 11148
  UINT32                                    FirmwareCtrl;                                         // [CODE_FIRST] 11148
  UINT32                                    Dsdt;                                                 // [CODE_FIRST] 11148
  UINT8                                     Reserved0;                                            // [CODE_FIRST] 11148
  UINT8                                     PreferredPmProfile;                                   // [CODE_FIRST] 11148
  UINT16                                    SciInt;                                               // [CODE_FIRST] 11148
  UINT32                                    SmiCmd;                                               // [CODE_FIRST] 11148
  UINT8                                     AcpiEnable;                                           // [CODE_FIRST] 11148
  UINT8                                     AcpiDisable;                                          // [CODE_FIRST] 11148
  UINT8                                     S4BiosReq;                                            // [CODE_FIRST] 11148
  UINT8                                     PstateCnt;                                            // [CODE_FIRST] 11148
  UINT32                                    Pm1aEvtBlk;                                           // [CODE_FIRST] 11148
  UINT32                                    Pm1bEvtBlk;                                           // [CODE_FIRST] 11148
  UINT32                                    Pm1aCntBlk;                                           // [CODE_FIRST] 11148
  UINT32                                    Pm1bCntBlk;                                           // [CODE_FIRST] 11148
  UINT32                                    Pm2CntBlk;                                            // [CODE_FIRST] 11148
  UINT32                                    PmTmrBlk;                                             // [CODE_FIRST] 11148
  UINT32                                    Gpe0Blk;                                              // [CODE_FIRST] 11148
  UINT32                                    Gpe1Blk;                                              // [CODE_FIRST] 11148
  UINT8                                     Pm1EvtLen;                                            // [CODE_FIRST] 11148
  UINT8                                     Pm1CntLen;                                            // [CODE_FIRST] 11148
  UINT8                                     Pm2CntLen;                                            // [CODE_FIRST] 11148
  UINT8                                     PmTmrLen;                                             // [CODE_FIRST] 11148
  UINT8                                     Gpe0BlkLen;                                           // [CODE_FIRST] 11148
  UINT8                                     Gpe1BlkLen;                                           // [CODE_FIRST] 11148
  UINT8                                     Gpe1Base;                                             // [CODE_FIRST] 11148
  UINT8                                     CstCnt;                                               // [CODE_FIRST] 11148
  UINT16                                    PLvl2Lat;                                             // [CODE_FIRST] 11148
  UINT16                                    PLvl3Lat;                                             // [CODE_FIRST] 11148
  UINT16                                    FlushSize;                                            // [CODE_FIRST] 11148
  UINT16                                    FlushStride;                                          // [CODE_FIRST] 11148
  UINT8                                     DutyOffset;                                           // [CODE_FIRST] 11148
  UINT8                                     DutyWidth;                                            // [CODE_FIRST] 11148
  UINT8                                     DayAlrm;                                              // [CODE_FIRST] 11148
  UINT8                                     MonAlrm;                                              // [CODE_FIRST] 11148
  UINT8                                     Century;                                              // [CODE_FIRST] 11148
  UINT16                                    IaPcBootArch;                                         // [CODE_FIRST] 11148
  UINT8                                     Reserved1;                                            // [CODE_FIRST] 11148
  UINT32                                    Flags;                                                // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    ResetReg;                                             // [CODE_FIRST] 11148
  UINT8                                     ResetValue;                                           // [CODE_FIRST] 11148
  UINT16                                    ArmBootArch;                                          // [CODE_FIRST] 11148
  UINT8                                     MinorVersion;                                         // [CODE_FIRST] 11148
  UINT64                                    XFirmwareCtrl;                                        // [CODE_FIRST] 11148
  UINT64                                    XDsdt;                                                // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    XPm1aEvtBlk;                                          // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    XPm1bEvtBlk;                                          // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    XPm1aCntBlk;                                          // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    XPm1bCntBlk;                                          // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    XPm2CntBlk;                                           // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    XPmTmrBlk;                                            // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    XGpe0Blk;                                             // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    XGpe1Blk;                                             // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    SleepControlReg;                                      // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    SleepStatusReg;                                       // [CODE_FIRST] 11148
  UINT64                                    HypervisorVendorIdentity;                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FIXED_ACPI_DESCRIPTION_TABLE;                                                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FADT Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FIXED_ACPI_DESCRIPTION_TABLE_REVISION        0x06                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FIXED_ACPI_DESCRIPTION_TABLE_MINOR_REVISION  0x06                            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Fixed ACPI Description Table Preferred Power Management Profile                                // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PM_PROFILE_UNSPECIFIED         0                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PM_PROFILE_DESKTOP             1                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PM_PROFILE_MOBILE              2                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PM_PROFILE_WORKSTATION         3                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PM_PROFILE_ENTERPRISE_SERVER   4                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PM_PROFILE_SOHO_SERVER         5                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PM_PROFILE_APPLIANCE_PC        6                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PM_PROFILE_PERFORMANCE_SERVER  7                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PM_PROFILE_TABLET              8                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Fixed ACPI Description Table Boot Architecture Flags                                           // [CODE_FIRST] 11148
// All other bits are reserved and must be set to 0.                                              // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_LEGACY_DEVICES        BIT0                                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_8042                  BIT1                                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_VGA_NOT_PRESENT       BIT2                                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MSI_NOT_SUPPORTED     BIT3                                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCIE_ASPM_CONTROLS    BIT4                                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CMOS_RTC_NOT_PRESENT  BIT5                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Fixed ACPI Description Table Arm Boot Architecture Flags                                       // [CODE_FIRST] 11148
// All other bits are reserved and must be set to 0.                                              // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ARM_PSCI_COMPLIANT  BIT0                                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ARM_PSCI_USE_HVC    BIT1                                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Fixed ACPI Description Table Fixed Feature Flags                                               // [CODE_FIRST] 11148
// All other bits are reserved and must be set to 0.                                              // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_WBINVD                                BIT0                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_WBINVD_FLUSH                          BIT1                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PROC_C1                               BIT2                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_P_LVL2_UP                             BIT3                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PWR_BUTTON                            BIT4                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SLP_BUTTON                            BIT5                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FIX_RTC                               BIT6                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RTC_S4                                BIT7                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_TMR_VAL_EXT                           BIT8                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_DCK_CAP                               BIT9                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RESET_REG_SUP                         BIT10                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SEALED_CASE                           BIT11                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HEADLESS                              BIT12                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CPU_SW_SLP                            BIT13                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCI_EXP_WAK                           BIT14                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_USE_PLATFORM_CLOCK                    BIT15                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_S4_RTC_STS_VALID                      BIT16                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_REMOTE_POWER_ON_CAPABLE               BIT17                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FORCE_APIC_CLUSTER_MODEL              BIT18                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FORCE_APIC_PHYSICAL_DESTINATION_MODE  BIT19                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HW_REDUCED_ACPI                       BIT20                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_LOW_POWER_S0_IDLE_CAPABLE             BIT21                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Firmware ACPI Control Structure                                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    Signature;                                                                            // [CODE_FIRST] 11148
  UINT32    Length;                                                                               // [CODE_FIRST] 11148
  UINT32    HardwareSignature;                                                                    // [CODE_FIRST] 11148
  UINT32    FirmwareWakingVector;                                                                 // [CODE_FIRST] 11148
  UINT32    GlobalLock;                                                                           // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT64    XFirmwareWakingVector;                                                                // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT8     Reserved0[3];                                                                         // [CODE_FIRST] 11148
  UINT32    OspmFlags;                                                                            // [CODE_FIRST] 11148
  UINT8     Reserved1[24];                                                                        // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FIRMWARE_ACPI_CONTROL_STRUCTURE;                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FACS Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION  0x02                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Firmware Control Structure Feature Flags                                                      // [CODE_FIRST] 11148
/// All other bits are reserved and must be set to 0.                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_S4BIOS_F                BIT0                                                 // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_64BIT_WAKE_SUPPORTED_F  BIT1                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// OSPM Enabled Firmware Control Structure Flags                                                 // [CODE_FIRST] 11148
/// All other bits are reserved and must be set to 0.                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_OSPM_64BIT_WAKE_F  BIT0                                                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Differentiated System Description Table,                                                       // [CODE_FIRST] 11148
// Secondary System Description Table                                                             // [CODE_FIRST] 11148
// and Persistent System Description Table,                                                       // [CODE_FIRST] 11148
// no definition needed as they are common description table header, the same with                // [CODE_FIRST] 11148
// EFI_ACPI_DESCRIPTION_HEADER, followed by a definition block.                                   // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_REVISION  0x02                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SECONDARY_SYSTEM_DESCRIPTION_TABLE_REVISION       0x02                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Multiple APIC Description Table header definition.  The rest of the table                     // [CODE_FIRST] 11148
/// must be defined in a platform specific manner.                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT32                         LocalApicAddress;                                                // [CODE_FIRST] 11148
  UINT32                         Flags;                                                           // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER;                                            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// MADT Revision (as defined in ACPI 6.7 spec.)                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION  0x08                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Multiple APIC Flags                                                                           // [CODE_FIRST] 11148
/// All other bits are reserved and must be set to 0.                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCAT_COMPAT  BIT0                                                            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Multiple APIC Description Table APIC structure types                                           // [CODE_FIRST] 11148
// All other values between 0x18 and 0x7F are reserved and                                        // [CODE_FIRST] 11148
// will be ignored by OSPM. 0x80 ~ 0xFF are reserved for OEM.                                     // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PROCESSOR_LOCAL_APIC           0x00                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IO_APIC                        0x01                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_INTERRUPT_SOURCE_OVERRIDE      0x02                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NON_MASKABLE_INTERRUPT_SOURCE  0x03                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_LOCAL_APIC_NMI                 0x04                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_LOCAL_APIC_ADDRESS_OVERRIDE    0x05                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IO_SAPIC                       0x06                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_LOCAL_SAPIC                    0x07                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLATFORM_INTERRUPT_SOURCES     0x08                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PROCESSOR_LOCAL_X2APIC         0x09                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_LOCAL_X2APIC_NMI               0x0A                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC                            0x0B                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GICD                           0x0C                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_MSI_FRAME                  0x0D                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GICR                           0x0E                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_ITS                        0x0F                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MULTIPROCESSOR_WAKEUP          0x10                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CORE_PIC                       0x11                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_LIO_PIC                        0x12                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HT_PIC                         0x13                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EIO_PIC                        0x14                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MSI_PIC                        0x15                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_BIO_PIC                        0x16                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_LPC_PIC                        0x17                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RINTC                          0x18                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IMSIC                          0x19                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_APLIC                          0x1A                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLIC                           0x1B                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_IRS                        0x1C                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_ITSV5                      0x1D                                          // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_ITSV5_TRANSLATE_FRAME      0x1E                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// APIC Structure Definitions                                                                     // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Processor Local APIC Structure Definition                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     AcpiProcessorUid;                                                                     // [CODE_FIRST] 11148
  UINT8     ApicId;                                                                               // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PROCESSOR_LOCAL_APIC_STRUCTURE;                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Local APIC Flags.  All other bits are reserved and must be 0.                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_LOCAL_APIC_ENABLED         BIT0                                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_LOCAL_APIC_ONLINE_CAPABLE  BIT1                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// IO APIC Structure                                                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     IoApicId;                                                                             // [CODE_FIRST] 11148
  UINT8     Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    IoApicAddress;                                                                        // [CODE_FIRST] 11148
  UINT32    GlobalSystemInterruptBase;                                                            // [CODE_FIRST] 11148
} EFI_ACPI_6_7_IO_APIC_STRUCTURE;                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Interrupt Source Override Structure                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Bus;                                                                                  // [CODE_FIRST] 11148
  UINT8     Source;                                                                               // [CODE_FIRST] 11148
  UINT32    GlobalSystemInterrupt;                                                                // [CODE_FIRST] 11148
  UINT16    Flags;                                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE;                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Platform Interrupt Sources Structure Definition                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Flags;                                                                                // [CODE_FIRST] 11148
  UINT8     InterruptType;                                                                        // [CODE_FIRST] 11148
  UINT8     ProcessorId;                                                                          // [CODE_FIRST] 11148
  UINT8     ProcessorEid;                                                                         // [CODE_FIRST] 11148
  UINT8     IoSapicVector;                                                                        // [CODE_FIRST] 11148
  UINT32    GlobalSystemInterrupt;                                                                // [CODE_FIRST] 11148
  UINT32    PlatformInterruptSourceFlags;                                                         // [CODE_FIRST] 11148
  UINT8     CpeiProcessorOverride;                                                                // [CODE_FIRST] 11148
  UINT8     Reserved[31];                                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PLATFORM_INTERRUPT_APIC_STRUCTURE;                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// MPS INTI flags.                                                                                // [CODE_FIRST] 11148
// All other bits are reserved and must be set to 0.                                              // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_POLARITY      (3 << 0)                                                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_TRIGGER_MODE  (3 << 2)                                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Non-Maskable Interrupt Source Structure                                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    GlobalSystemInterrupt;                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NON_MASKABLE_INTERRUPT_SOURCE_STRUCTURE;                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Local APIC NMI Structure                                                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     AcpiProcessorUid;                                                                     // [CODE_FIRST] 11148
  UINT16    Flags;                                                                                // [CODE_FIRST] 11148
  UINT8     LocalApicLint;                                                                        // [CODE_FIRST] 11148
} EFI_ACPI_6_7_LOCAL_APIC_NMI_STRUCTURE;                                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Local APIC Address Override Structure                                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Reserved;                                                                             // [CODE_FIRST] 11148
  UINT64    LocalApicAddress;                                                                     // [CODE_FIRST] 11148
} EFI_ACPI_6_7_LOCAL_APIC_ADDRESS_OVERRIDE_STRUCTURE;                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// IO SAPIC Structure                                                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     IoApicId;                                                                             // [CODE_FIRST] 11148
  UINT8     Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    GlobalSystemInterruptBase;                                                            // [CODE_FIRST] 11148
  UINT64    IoSapicAddress;                                                                       // [CODE_FIRST] 11148
} EFI_ACPI_6_7_IO_SAPIC_STRUCTURE;                                                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Local SAPIC Structure                                                                         // [CODE_FIRST] 11148
/// This struct followed by a null-terminated ASCII string - ACPI Processor UID String            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     AcpiProcessorId;                                                                      // [CODE_FIRST] 11148
  UINT8     LocalSapicId;                                                                         // [CODE_FIRST] 11148
  UINT8     LocalSapicEid;                                                                        // [CODE_FIRST] 11148
  UINT8     Reserved[3];                                                                          // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    ACPIProcessorUIDValue;                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PROCESSOR_LOCAL_SAPIC_STRUCTURE;                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Platform Interrupt Sources Structure                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Flags;                                                                                // [CODE_FIRST] 11148
  UINT8     InterruptType;                                                                        // [CODE_FIRST] 11148
  UINT8     ProcessorId;                                                                          // [CODE_FIRST] 11148
  UINT8     ProcessorEid;                                                                         // [CODE_FIRST] 11148
  UINT8     IoSapicVector;                                                                        // [CODE_FIRST] 11148
  UINT32    GlobalSystemInterrupt;                                                                // [CODE_FIRST] 11148
  UINT32    PlatformInterruptSourceFlags;                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PLATFORM_INTERRUPT_SOURCES_STRUCTURE;                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Platform Interrupt Source Flags.                                                              // [CODE_FIRST] 11148
/// All other bits are reserved and must be set to 0.                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CPEI_PROCESSOR_OVERRIDE  BIT0                                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Processor Local x2APIC Structure Definition                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Reserved[2];                                                                          // [CODE_FIRST] 11148
  UINT32    X2ApicId;                                                                             // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    AcpiProcessorUid;                                                                     // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PROCESSOR_LOCAL_X2APIC_STRUCTURE;                                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Local x2APIC NMI Structure                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    AcpiProcessorUid;                                                                     // [CODE_FIRST] 11148
  UINT8     LocalX2ApicLint;                                                                      // [CODE_FIRST] 11148
  UINT8     Reserved[3];                                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_LOCAL_X2APIC_NMI_STRUCTURE;                                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GIC Structure                                                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    CPUInterfaceNumber;                                                                   // [CODE_FIRST] 11148
  UINT32    AcpiProcessorUid;                                                                     // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    ParkingProtocolVersion;                                                               // [CODE_FIRST] 11148
  UINT32    PerformanceInterruptGsiv;                                                             // [CODE_FIRST] 11148
  UINT64    ParkedAddress;                                                                        // [CODE_FIRST] 11148
  UINT64    PhysicalBaseAddress;                                                                  // [CODE_FIRST] 11148
  UINT64    GICV;                                                                                 // [CODE_FIRST] 11148
  UINT64    GICH;                                                                                 // [CODE_FIRST] 11148
  UINT32    VGICMaintenanceInterrupt;                                                             // [CODE_FIRST] 11148
  UINT64    GICRBaseAddress;                                                                      // [CODE_FIRST] 11148
  UINT64    MPIDR;                                                                                // [CODE_FIRST] 11148
  UINT8     ProcessorPowerEfficiencyClass;                                                        // [CODE_FIRST] 11148
  UINT8     Reserved2;                                                                            // [CODE_FIRST] 11148
  UINT16    SpeOverflowInterrupt;                                                                 // [CODE_FIRST] 11148
  UINT16    TrbeInterrupt;                                                                        // [CODE_FIRST] 11148
  UINT16    IAffId;                                                                               // [CODE_FIRST] 11148
  UINT32    IrsId;                                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GIC_STRUCTURE;                                                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GIC Flags.  All other bits are reserved and must be 0.                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_ENABLED                            BIT0                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PERFORMANCE_INTERRUPT_MODEL            BIT1                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_VGIC_MAINTENANCE_INTERRUPT_MODE_FLAGS  BIT2                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_ONLINE_CAPABLE                     BIT3                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GIC Distributor Structure                                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Reserved1;                                                                            // [CODE_FIRST] 11148
  UINT32    GicId;                                                                                // [CODE_FIRST] 11148
  UINT64    PhysicalBaseAddress;                                                                  // [CODE_FIRST] 11148
  UINT32    SystemVectorBase;                                                                     // [CODE_FIRST] 11148
  UINT8     GicVersion;                                                                           // [CODE_FIRST] 11148
  UINT8     Reserved2[3];                                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GIC_DISTRIBUTOR_STRUCTURE;                                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GIC Version                                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_V1  0x01                                                                 // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_V2  0x02                                                                 // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_V3  0x03                                                                 // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_V4  0x04                                                                 // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_V5  0x05                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GIC MSI Frame Structure                                                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Reserved1;                                                                            // [CODE_FIRST] 11148
  UINT32    GicMsiFrameId;                                                                        // [CODE_FIRST] 11148
  UINT64    PhysicalBaseAddress;                                                                  // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT16    SPICount;                                                                             // [CODE_FIRST] 11148
  UINT16    SPIBase;                                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GIC_MSI_FRAME_STRUCTURE;                                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GIC MSI Frame Flags.  All other bits are reserved and must be 0.                              // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SPI_COUNT_BASE_SELECT  BIT0                                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GICR Structure                                                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Reserved;                                                                             // [CODE_FIRST] 11148
  UINT64    DiscoveryRangeBaseAddress;                                                            // [CODE_FIRST] 11148
  UINT32    DiscoveryRangeLength;                                                                 // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GICR_STRUCTURE;                                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GIC Interrupt Translation Service Structure                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    GicItsId;                                                                             // [CODE_FIRST] 11148
  UINT64    PhysicalBaseAddress;                                                                  // [CODE_FIRST] 11148
  UINT32    Reserved2;                                                                            // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GIC_ITS_STRUCTURE;                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Multiprocessor Wakeup Structure                                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    MailBoxVersion;                                                                       // [CODE_FIRST] 11148
  UINT32    Reserved;                                                                             // [CODE_FIRST] 11148
  UINT64    MailBoxAddress;                                                                       // [CODE_FIRST] 11148
  UINT64    ResetVector;                                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MULTIPROCESSOR_WAKEUP_STRUCTURE;                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Multiprocessor Wakeup Mailbox Structure                                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Command;                                                                              // [CODE_FIRST] 11148
  UINT16    Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    AcpiId;                                                                               // [CODE_FIRST] 11148
  UINT64    WakeupVector;                                                                         // [CODE_FIRST] 11148
  UINT8     ReservedForOs[2032];                                                                  // [CODE_FIRST] 11148
  UINT8     ReservedForFirmware[2048];                                                            // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MULTIPROCESSOR_WAKEUP_MAILBOX_STRUCTURE;                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MULTIPROCESSOR_WAKEUP_MAILBOX_COMMAND_NOOP    0x0000                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MULTIPROCESSOR_WAKEUP_MAILBOX_COMMAND_WAKEUP  0x0001                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MULTIPROCESSOR_WAKEUP_MAILBOX_COMMAND_TEST    0x0002                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Core Programmable Interrupt Controller                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT32    ProcessorId;                                                                          // [CODE_FIRST] 11148
  UINT32    CoreId;                                                                               // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_CORE_PIC_STRUCTURE;                                                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Legacy I/O Programmable Interrupt Controller                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT64    Address;                                                                              // [CODE_FIRST] 11148
  UINT16    Size;                                                                                 // [CODE_FIRST] 11148
  UINT8     Cascade[2];                                                                           // [CODE_FIRST] 11148
  UINT32    CascadeMap[2];                                                                        // [CODE_FIRST] 11148
} EFI_ACPI_6_7_LIO_PIC_STRUCTURE;                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// HyperTransport Programmable Interrupt Controller                                              // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT64    Address;                                                                              // [CODE_FIRST] 11148
  UINT16    Size;                                                                                 // [CODE_FIRST] 11148
  UINT8     Cascade[8];                                                                           // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HT_PIC_STRUCTURE;                                                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Extend I/O Programmable Interrupt Controller                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT8     Cascade;                                                                              // [CODE_FIRST] 11148
  UINT8     Node;                                                                                 // [CODE_FIRST] 11148
  UINT64    NodeMap;                                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_EIO_PIC_STRUCTURE;                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// MSI Programmable Interrupt Controller                                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT64    MsgAddress;                                                                           // [CODE_FIRST] 11148
  UINT32    Start;                                                                                // [CODE_FIRST] 11148
  UINT32    Count;                                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MSI_PIC_STRUCTURE;                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Bridge I/O Programmable Interrupt Controller                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT64    Address;                                                                              // [CODE_FIRST] 11148
  UINT16    Size;                                                                                 // [CODE_FIRST] 11148
  UINT16    Id;                                                                                   // [CODE_FIRST] 11148
  UINT16    GsiBase;                                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_BIO_PIC_STRUCTURE;                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Low Pin Count Programmable Interrupt Controller                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT64    Address;                                                                              // [CODE_FIRST] 11148
  UINT16    Size;                                                                                 // [CODE_FIRST] 11148
  UINT8     Cascade;                                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_LPC_PIC_STRUCTURE;                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// RISC-V INTC (RINTC)                                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT8     Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT64    HartId;                                                                               // [CODE_FIRST] 11148
  UINT32    Uid;                                                                                  // [CODE_FIRST] 11148
  UINT32    ExtIntcId;                                                                            // [CODE_FIRST] 11148
  UINT64    ImsicAddr;                                                                            // [CODE_FIRST] 11148
  UINT32    ImsicSize;                                                                            // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RINTC_STRUCTURE;                                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RINTC_STRUCTURE_VERSION  1                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RINTC_FLAG_ENABLE          BIT0                                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RINTC_FLAG_ONLINE_CAPABLE  BIT1                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// RISC-V Incoming MSI Controller (IMSIC)                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT8     Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT16    NumIds;                                                                               // [CODE_FIRST] 11148
  UINT16    NumGuestIds;                                                                          // [CODE_FIRST] 11148
  UINT8     GuestIndexBits;                                                                       // [CODE_FIRST] 11148
  UINT8     HartIndexBits;                                                                        // [CODE_FIRST] 11148
  UINT8     GroupIndexBits;                                                                       // [CODE_FIRST] 11148
  UINT8     GroupIndexShift;                                                                      // [CODE_FIRST] 11148
} EFI_ACPI_6_7_IMSIC_STRUCTURE;                                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IMSIC_STRUCTURE_VERSION  1                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define IMSIC_MIN_NUM_IDS            63                                                           // [CODE_FIRST] 11148
#define IMSIC_MAX_NUM_IDS            2047                                                         // [CODE_FIRST] 11148
#define IMSIC_MIN_NUM_GUEST_IDS      63                                                           // [CODE_FIRST] 11148
#define IMSIC_MAX_NUM_GUEST_IDS      2047                                                         // [CODE_FIRST] 11148
#define IMSIC_MIN_GUEST_INDEX_BITS   0                                                            // [CODE_FIRST] 11148
#define IMSIC_MAX_GUEST_INDEX_BITS   7                                                            // [CODE_FIRST] 11148
#define IMSIC_MIN_HART_INDEX_BITS    0                                                            // [CODE_FIRST] 11148
#define IMSIC_MAX_HART_INDEX_BITS    15                                                           // [CODE_FIRST] 11148
#define IMSIC_MIN_GROUP_INDEX_BITS   0                                                            // [CODE_FIRST] 11148
#define IMSIC_MAX_GROUP_INDEX_BITS   7                                                            // [CODE_FIRST] 11148
#define IMSIC_MIN_GROUP_INDEX_SHIFT  0                                                            // [CODE_FIRST] 11148
#define IMSIC_MAX_GROUP_INDEX_SHIFT  55                                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// RISC-V APLIC                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT8     Id;                                                                                   // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT8     HwId[8];                                                                              // [CODE_FIRST] 11148
  UINT16    NumIdcs;                                                                              // [CODE_FIRST] 11148
  UINT16    NumSources;                                                                           // [CODE_FIRST] 11148
  UINT32    GsiBase;                                                                              // [CODE_FIRST] 11148
  UINT64    BaseAddr;                                                                             // [CODE_FIRST] 11148
  UINT32    Size;                                                                                 // [CODE_FIRST] 11148
} EFI_ACPI_6_7_APLIC_STRUCTURE;                                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_APLIC_STRUCTURE_VERSION  1                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// RISC-V PLIC                                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Version;                                                                              // [CODE_FIRST] 11148
  UINT8     Id;                                                                                   // [CODE_FIRST] 11148
  UINT8     HwId[8];                                                                              // [CODE_FIRST] 11148
  UINT16    NumIrqs;                                                                              // [CODE_FIRST] 11148
  UINT16    MaxPrio;                                                                              // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    Size;                                                                                 // [CODE_FIRST] 11148
  UINT64    BaseAddr;                                                                             // [CODE_FIRST] 11148
  UINT32    GsiBase;                                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PLIC_STRUCTURE;                                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLIC_STRUCTURE_VERSION  1                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GIC IRS                                                                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     GicVersion;                                                                           // [CODE_FIRST] 11148
  UINT8     Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    IrsId;                                                                                // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    Reserved2;                                                                            // [CODE_FIRST] 11148
  UINT64    IrsConfigFrameBase;                                                                   // [CODE_FIRST] 11148
  UINT64    IrsSetLpiFrameBase;                                                                   // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GIC_IRS_STRUCTURE;                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_IRS_FLAG_FULLY_COHERENT  (0)                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_IRS_FLAG_NOT_COHERENT    (BIT0)                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GIC ITSv5                                                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Flags;                                                                                // [CODE_FIRST] 11148
  UINT8     Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    GicItsId;                                                                             // [CODE_FIRST] 11148
  UINT64    PhysicalBaseAddress;                                                                  // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GIC_ITSV5_STRUCTURE;                                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_ITSV5_FLAG_FULLY_COHERENT  (0)                                           // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_ITSV5_FLAG_NOT_COHERENT    (BIT0)                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GIC ITSv5 translate frame structure                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    LinkedGicItsId;                                                                       // [CODE_FIRST] 11148
  UINT32    ItsTranslateId;                                                                       // [CODE_FIRST] 11148
  UINT32    Reserved2;                                                                            // [CODE_FIRST] 11148
  UINT64    ItsTranslateFrameBase;                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GIC_ITSV5_TRANSLATE_FRAME_STRUCTURE;                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Smart Battery Description Table (SBST)                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT32                         WarningEnergyLevel;                                              // [CODE_FIRST] 11148
  UINT32                         LowEnergyLevel;                                                  // [CODE_FIRST] 11148
  UINT32                         CriticalEnergyLevel;                                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_SMART_BATTERY_DESCRIPTION_TABLE;                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// SBST Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SMART_BATTERY_DESCRIPTION_TABLE_REVISION  0x01                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Embedded Controller Boot Resources Table (ECDT)                                               // [CODE_FIRST] 11148
/// The table is followed by a null terminated ASCII string that contains                         // [CODE_FIRST] 11148
/// a fully qualified reference to the name space object.                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER               Header;                                               // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    EcControl;                                            // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    EcData;                                               // [CODE_FIRST] 11148
  UINT32                                    Uid;                                                  // [CODE_FIRST] 11148
  UINT8                                     GpeBit;                                               // [CODE_FIRST] 11148
} EFI_ACPI_6_7_EMBEDDED_CONTROLLER_BOOT_RESOURCES_TABLE;                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ECDT Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EMBEDDED_CONTROLLER_BOOT_RESOURCES_TABLE_REVISION  0x01                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// System Resource Affinity Table (SRAT).  The rest of the table                                 // [CODE_FIRST] 11148
/// must be defined in a platform specific manner.                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;    // [CODE_FIRST] 11148
  UINT32                         Reserved1; ///< Must be set to 1                                 // [CODE_FIRST] 11148
  UINT64                         Reserved2; // [CODE_FIRST] 11148
} EFI_ACPI_6_7_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER;                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// SRAT Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SYSTEM_RESOURCE_AFFINITY_TABLE_REVISION  0x04                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// SRAT structure types.                                                                          // [CODE_FIRST] 11148
// All other values between 0x06 an 0xFF are reserved and                                         // [CODE_FIRST] 11148
// will be ignored by OSPM.                                                                       // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY  0x00                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MEMORY_AFFINITY                      0x01                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PROCESSOR_LOCAL_X2APIC_AFFINITY      0x02                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GICC_AFFINITY                        0x03                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_ITS_AFFINITY                     0x04                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GENERIC_INITIATOR_AFFINITY           0x05                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GENERIC_PORT_AFFINITY                0x06                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RINTC_AFFINITY                       0x07                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GIC_IRS_AFFINITY                     0x08                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Processor Local APIC/SAPIC Affinity Structure Definition                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     ProximityDomain7To0;                                                                  // [CODE_FIRST] 11148
  UINT8     ApicId;                                                                               // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT8     LocalSapicEid;                                                                        // [CODE_FIRST] 11148
  UINT8     ProximityDomain31To8[3];                                                              // [CODE_FIRST] 11148
  UINT32    ClockDomain;                                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY_STRUCTURE;                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Local APIC/SAPIC Flags.  All other bits are reserved and must be 0.                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PROCESSOR_LOCAL_APIC_SAPIC_ENABLED  (1 << 0)                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Memory Affinity Structure Definition                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT32    ProximityDomain;                                                                      // [CODE_FIRST] 11148
  UINT16    Reserved1;                                                                            // [CODE_FIRST] 11148
  UINT32    AddressBaseLow;                                                                       // [CODE_FIRST] 11148
  UINT32    AddressBaseHigh;                                                                      // [CODE_FIRST] 11148
  UINT32    LengthLow;                                                                            // [CODE_FIRST] 11148
  UINT32    LengthHigh;                                                                           // [CODE_FIRST] 11148
  UINT32    Reserved2;                                                                            // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT64    Reserved3;                                                                            // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MEMORY_AFFINITY_STRUCTURE;                                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Memory Flags.  All other bits are reserved and must be 0.                                      // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MEMORY_ENABLED        (1 << 0)                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MEMORY_HOT_PLUGGABLE  (1 << 1)                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MEMORY_NONVOLATILE    (1 << 2)                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Processor Local x2APIC Affinity Structure Definition                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Reserved1[2];                                                                         // [CODE_FIRST] 11148
  UINT32    ProximityDomain;                                                                      // [CODE_FIRST] 11148
  UINT32    X2ApicId;                                                                             // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    ClockDomain;                                                                          // [CODE_FIRST] 11148
  UINT8     Reserved2[4];                                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PROCESSOR_LOCAL_X2APIC_AFFINITY_STRUCTURE;                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GICC Affinity Structure Definition                                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT32    ProximityDomain;                                                                      // [CODE_FIRST] 11148
  UINT32    AcpiProcessorUid;                                                                     // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    ClockDomain;                                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GICC_AFFINITY_STRUCTURE;                                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GICC Flags.  All other bits are reserved and must be 0.                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GICC_ENABLED  (1 << 0)                                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GIC Interrupt Translation Service (ITS) Affinity Structure Definition                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT32    ProximityDomain;                                                                      // [CODE_FIRST] 11148
  UINT8     Reserved[2];                                                                          // [CODE_FIRST] 11148
  UINT32    ItsId;                                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GIC_ITS_AFFINITY_STRUCTURE;                                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// RINTC Affinity Structure Definition                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    ProximityDomain;                                                                      // [CODE_FIRST] 11148
  UINT32    AcpiProcessorUid;                                                                     // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    ClockDomain;                                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RINTC_AFFINITY_STRUCTURE;                                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// IRS Affinity Structure Definition                                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    ProximityDomain;                                                                      // [CODE_FIRST] 11148
  UINT32    IrsId;                                                                                // [CODE_FIRST] 11148
  UINT32    Reserved2;                                                                            // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GIC_IRS_AFFINITY_STRUCTURE;                                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Generic Initiator Affinity Structure Device Handle Types                                       // [CODE_FIRST] 11148
// All other values between 0x02 an 0xFF are reserved and                                         // [CODE_FIRST] 11148
// will be ignored by OSPM.                                                                       // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ACPI_DEVICE_HANDLE  0x00                                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCI_DEVICE_HANDLE   0x01                                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Device Handle - ACPI                                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT64    AcpiHid;                                                                              // [CODE_FIRST] 11148
  UINT32    AcpiUid;                                                                              // [CODE_FIRST] 11148
  UINT8     Reserved[4];                                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_DEVICE_HANDLE_ACPI;                                                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Device Handle - PCI                                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    PciSegment;                                                                           // [CODE_FIRST] 11148
  UINT16    PciBdfNumber;                                                                         // [CODE_FIRST] 11148
  UINT8     Reserved[12];                                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_DEVICE_HANDLE_PCI;                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Device Handle                                                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef union {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_DEVICE_HANDLE_ACPI    Acpi;                                                        // [CODE_FIRST] 11148
  EFI_ACPI_6_7_DEVICE_HANDLE_PCI     Pci;                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_DEVICE_HANDLE;                                                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Generic Initiator Affinity Structure                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8                         Type;                                                             // [CODE_FIRST] 11148
  UINT8                         Length;                                                           // [CODE_FIRST] 11148
  UINT8                         Reserved1;                                                        // [CODE_FIRST] 11148
  UINT8                         DeviceHandleType;                                                 // [CODE_FIRST] 11148
  UINT32                        ProximityDomain;                                                  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_DEVICE_HANDLE    DeviceHandle;                                                     // [CODE_FIRST] 11148
  UINT32                        Flags;                                                            // [CODE_FIRST] 11148
  UINT8                         Reserved2[4];                                                     // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GENERIC_INITIATOR_AFFINITY_STRUCTURE;                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Generic Initiator Affinity Structure Flags. All other bits are reserved                       // [CODE_FIRST] 11148
/// and must be 0.                                                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GENERIC_INITIATOR_AFFINITY_STRUCTURE_ENABLED                     BIT0        // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GENERIC_INITIATOR_AFFINITY_STRUCTURE_ARCHITECTURAL_TRANSACTIONS  BIT1        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// System Locality Distance Information Table (SLIT).                                            // [CODE_FIRST] 11148
/// The rest of the table is a matrix.                                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT64                         NumberOfSystemLocalities;                                        // [CODE_FIRST] 11148
} EFI_ACPI_6_7_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER;                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// SLIT Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_REVISION  0x01                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Corrected Platform Error Polling Table (CPEP)                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT8                          Reserved[8];                                                     // [CODE_FIRST] 11148
} EFI_ACPI_6_7_CORRECTED_PLATFORM_ERROR_POLLING_TABLE_HEADER;                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// CPEP Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CORRECTED_PLATFORM_ERROR_POLLING_TABLE_REVISION  0x01                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// CPEP processor structure types.                                                                // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CPEP_PROCESSOR_APIC_SAPIC  0x00                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Corrected Platform Error Polling Processor Structure Definition                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     ProcessorId;                                                                          // [CODE_FIRST] 11148
  UINT8     ProcessorEid;                                                                         // [CODE_FIRST] 11148
  UINT32    PollingInterval;                                                                      // [CODE_FIRST] 11148
} EFI_ACPI_6_7_CPEP_PROCESSOR_APIC_SAPIC_STRUCTURE;                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Maximum System Characteristics Table (MSCT)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT32                         OffsetProxDomInfo;                                               // [CODE_FIRST] 11148
  UINT32                         MaximumNumberOfProximityDomains;                                 // [CODE_FIRST] 11148
  UINT32                         MaximumNumberOfClockDomains;                                     // [CODE_FIRST] 11148
  UINT64                         MaximumPhysicalAddress;                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_HEADER;                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// MSCT Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_REVISION  0x01                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Maximum Proximity Domain Information Structure Definition                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Revision;                                                                             // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT32    ProximityDomainRangeLow;                                                              // [CODE_FIRST] 11148
  UINT32    ProximityDomainRangeHigh;                                                             // [CODE_FIRST] 11148
  UINT32    MaximumProcessorCapacity;                                                             // [CODE_FIRST] 11148
  UINT64    MaximumMemoryCapacity;                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE;                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ACPI RAS Feature Table definition.                                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT8                          PlatformCommunicationChannelIdentifier[12];                      // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RAS_FEATURE_TABLE;                                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// RASF Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RAS_FEATURE_TABLE_REVISION  0x01                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ACPI RASF Platform Communication Channel Shared Memory Region definition.                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    Signature;                                                                            // [CODE_FIRST] 11148
  UINT16    Command;                                                                              // [CODE_FIRST] 11148
  UINT16    Status;                                                                               // [CODE_FIRST] 11148
  UINT16    Version;                                                                              // [CODE_FIRST] 11148
  UINT8     RASCapabilities[16];                                                                  // [CODE_FIRST] 11148
  UINT8     SetRASCapabilities[16];                                                               // [CODE_FIRST] 11148
  UINT16    NumberOfRASFParameterBlocks;                                                          // [CODE_FIRST] 11148
  UINT32    SetRASCapabilitiesStatus;                                                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RASF_PLATFORM_COMMUNICATION_CHANNEL_SHARED_MEMORY_REGION;                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ACPI RASF PCC command code                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RASF_PCC_COMMAND_CODE_EXECUTE_RASF_COMMAND  0x01                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ACPI RASF Platform RAS Capabilities                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RASF_PLATFORM_RAS_CAPABILITY_HARDWARE_BASED_PATROL_SCRUB_SUPPORTED                          BIT0  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RASF_PLATFORM_RAS_CAPABILITY_HARDWARE_BASED_PATROL_SCRUB_SUPPORTED_AND_EXPOSED_TO_SOFTWARE  BIT1  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RASF_PLATFORM_RAS_CAPABILITY_CPU_CACHE_FLUSH_TO_NVDIMM_DURABILITY_ON_POWER_LOSS             BIT2  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RASF_PLATFORM_RAS_CAPABILITY_MEMORY_CONTROLLER_FLUSH_TO_NVDIMM_DURABILITY_ON_POWER_LOSS     BIT3  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RASF_PLATFORM_RAS_CAPABILITY_BYTE_ADDRESSABLE_PERSISTENT_MEMORY_HARDWARE_MIRRORING          BIT4  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ACPI RASF Parameter Block structure for PATROL_SCRUB                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    Version;                                                                              // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT16    PatrolScrubCommand;                                                                   // [CODE_FIRST] 11148
  UINT64    RequestedAddressRange[2];                                                             // [CODE_FIRST] 11148
  UINT64    ActualAddressRange[2];                                                                // [CODE_FIRST] 11148
  UINT16    Flags;                                                                                // [CODE_FIRST] 11148
  UINT8     RequestedSpeed;                                                                       // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RASF_PATROL_SCRUB_PLATFORM_BLOCK_STRUCTURE;                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ACPI RASF Patrol Scrub command                                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RASF_PATROL_SCRUB_COMMAND_GET_PATROL_PARAMETERS  0x01                        // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RASF_PATROL_SCRUB_COMMAND_START_PATROL_SCRUBBER  0x02                        // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RASF_PATROL_SCRUB_COMMAND_STOP_PATROL_SCRUBBER   0x03                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ACPI RAS2 Feature Table definition.                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT16                         Reserved;                                                        // [CODE_FIRST] 11148
  UINT16                         PccCount;                                                        // [CODE_FIRST] 11148
  // EFI_ACPI_RAS2_PCC_DESCRIPTOR Descriptors[PccCount];                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RAS2_FEATURE_TABLE;                                                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Memory Power State Table definition.                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT8                          PlatformCommunicationChannelIdentifier;                          // [CODE_FIRST] 11148
  UINT8                          Reserved[3];                                                     // [CODE_FIRST] 11148
  // Memory Power Node Structure                                                                  // [CODE_FIRST] 11148
  // Memory Power State Characteristics                                                           // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MEMORY_POWER_STATUS_TABLE;                                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// MPST Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MEMORY_POWER_STATE_TABLE_REVISION  0x01                                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// MPST Platform Communication Channel Shared Memory Region definition.                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    Signature;                                                                            // [CODE_FIRST] 11148
  UINT16    Command;                                                                              // [CODE_FIRST] 11148
  UINT16    Status;                                                                               // [CODE_FIRST] 11148
  UINT32    MemoryPowerCommandRegister;                                                           // [CODE_FIRST] 11148
  UINT32    MemoryPowerStatusRegister;                                                            // [CODE_FIRST] 11148
  UINT32    PowerStateId;                                                                         // [CODE_FIRST] 11148
  UINT32    MemoryPowerNodeId;                                                                    // [CODE_FIRST] 11148
  UINT64    MemoryEnergyConsumed;                                                                 // [CODE_FIRST] 11148
  UINT64    ExpectedAveragePowerComsuned;                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MPST_PLATFORM_COMMUNICATION_CHANNEL_SHARED_MEMORY_REGION;                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ACPI MPST PCC command code                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MPST_PCC_COMMAND_CODE_EXECUTE_MPST_COMMAND  0x03                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ACPI MPST Memory Power command                                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MPST_MEMORY_POWER_COMMAND_GET_MEMORY_POWER_STATE      0x01                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MPST_MEMORY_POWER_COMMAND_SET_MEMORY_POWER_STATE      0x02                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MPST_MEMORY_POWER_COMMAND_GET_AVERAGE_POWER_CONSUMED  0x03                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MPST_MEMORY_POWER_COMMAND_GET_MEMORY_ENERGY_CONSUMED  0x04                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// MPST Memory Power Node Table                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8    PowerStateValue;                                                                       // [CODE_FIRST] 11148
  UINT8    PowerStateInformationIndex;                                                            // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MPST_MEMORY_POWER_STATE;                                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Flag;                                                                                 // [CODE_FIRST] 11148
  UINT8     Reserved;                                                                             // [CODE_FIRST] 11148
  UINT16    MemoryPowerNodeId;                                                                    // [CODE_FIRST] 11148
  UINT32    Length;                                                                               // [CODE_FIRST] 11148
  UINT64    AddressBase;                                                                          // [CODE_FIRST] 11148
  UINT64    AddressLength;                                                                        // [CODE_FIRST] 11148
  UINT32    NumberOfPowerStates;                                                                  // [CODE_FIRST] 11148
  UINT32    NumberOfPhysicalComponents;                                                           // [CODE_FIRST] 11148
  // EFI_ACPI_6_7_MPST_MEMORY_POWER_STATE              MemoryPowerState[NumberOfPowerStates];     // [CODE_FIRST] 11148
  // UINT16                                            PhysicalComponentIdentifier[NumberOfPhysicalComponents];  // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MPST_MEMORY_POWER_STRUCTURE;                                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MPST_MEMORY_POWER_STRUCTURE_FLAG_ENABLE         0x01                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MPST_MEMORY_POWER_STRUCTURE_FLAG_POWER_MANAGED  0x02                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MPST_MEMORY_POWER_STRUCTURE_FLAG_HOT_PLUGGABLE  0x04                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    MemoryPowerNodeCount;                                                                 // [CODE_FIRST] 11148
  UINT8     Reserved[2];                                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MPST_MEMORY_POWER_NODE_TABLE;                                                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// MPST Memory Power State Characteristics Table                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     PowerStateStructureID;                                                                // [CODE_FIRST] 11148
  UINT8     Flag;                                                                                 // [CODE_FIRST] 11148
  UINT16    Reserved;                                                                             // [CODE_FIRST] 11148
  UINT32    AveragePowerConsumedInMPS0;                                                           // [CODE_FIRST] 11148
  UINT32    RelativePowerSavingToMPS0;                                                            // [CODE_FIRST] 11148
  UINT64    ExitLatencyToMPS0;                                                                    // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MPST_MEMORY_POWER_STATE_CHARACTERISTICS_STRUCTURE;                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MPST_MEMORY_POWER_STATE_CHARACTERISTICS_STRUCTURE_FLAG_MEMORY_CONTENT_PRESERVED             0x01  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MPST_MEMORY_POWER_STATE_CHARACTERISTICS_STRUCTURE_FLAG_AUTONOMOUS_MEMORY_POWER_STATE_ENTRY  0x02  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MPST_MEMORY_POWER_STATE_CHARACTERISTICS_STRUCTURE_FLAG_AUTONOMOUS_MEMORY_POWER_STATE_EXIT   0x04  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    MemoryPowerStateCharacteristicsCount;                                                 // [CODE_FIRST] 11148
  UINT8     Reserved[2];                                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_MPST_MEMORY_POWER_STATE_CHARACTERISTICS_TABLE;                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Platform Memory Topology Table definition.                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT32                         NumberOfMemoryDevices;                                           // [CODE_FIRST] 11148
  // EFI_ACPI_6_7_PMTT_COMMON_MEMORY_DEVICE  MemoryDeviceStructure[NumberOfMemoryDevices];        // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PLATFORM_MEMORY_TOPOLOGY_TABLE;                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PMTT Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MEMORY_TOPOLOGY_TABLE_REVISION  0x02                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Common Memory Device.                                                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Reserved;                                                                             // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Flags;                                                                                // [CODE_FIRST] 11148
  UINT16    Reserved1;                                                                            // [CODE_FIRST] 11148
  UINT32    NumberOfMemoryDevices;                                                                // [CODE_FIRST] 11148
  // UINT8                                   TypeSpecificData[];                                  // [CODE_FIRST] 11148
  // EFI_ACPI_6_7_PMTT_COMMON_MEMORY_DEVICE  MemoryDeviceStructure[NumberOfMemoryDevices];        // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PMTT_COMMON_MEMORY_DEVICE;                                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Memory Device Type.                                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PMTT_MEMORY_DEVICE_TYPE_SOCKET                0x0                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PMTT_MEMORY_DEVICE_TYPE_MEMORY_CONTROLLER     0x1                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PMTT_MEMORY_DEVICE_TYPE_DIMM                  0x2                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PMTT_MEMORY_DEVICE_TYPE_VENDOR_SPECIFIC_TYPE  0xFF                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Socket Type Data.                                                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_PMTT_COMMON_MEMORY_DEVICE    CommonMemoryDeviceHeader;                             // [CODE_FIRST] 11148
  UINT16                                    SocketIdentifier;                                     // [CODE_FIRST] 11148
  UINT16                                    Reserved;                                             // [CODE_FIRST] 11148
  // EFI_ACPI_6_7_PMTT_COMMON_MEMORY_DEVICE  MemoryDeviceStructure[];                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PMTT_SOCKET_TYPE_DATA;                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Memory Controller Type Data.                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_PMTT_COMMON_MEMORY_DEVICE    CommonMemoryDeviceHeader;                             // [CODE_FIRST] 11148
  UINT16                                    MemoryControllerIdentifier;                           // [CODE_FIRST] 11148
  UINT16                                    Reserved;                                             // [CODE_FIRST] 11148
  // EFI_ACPI_6_7_PMTT_COMMON_MEMORY_DEVICE  MemoryDeviceStructure[];                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PMTT_MEMORY_CONTROLLER_TYPE_DATA;                                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// DIMM Type Specific Data.                                                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_PMTT_COMMON_MEMORY_DEVICE    CommonMemoryDeviceHeader;                             // [CODE_FIRST] 11148
  UINT32                                    SmbiosHandle;                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PMTT_DIMM_TYPE_SPECIFIC_DATA;                                                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Vendor Specific Type Data.                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_PMTT_COMMON_MEMORY_DEVICE    CommonMemoryDeviceHeader;                             // [CODE_FIRST] 11148
  UINT8                                     TypeUuid[16];                                         // [CODE_FIRST] 11148
  // EFI_ACPI_6_7_PMTT_VENDOR_SPECIFIC_TYPE_DATA   VendorSpecificData[];                          // [CODE_FIRST] 11148
  // EFI_ACPI_6_7_PMTT_COMMON_MEMORY_DEVICE        MemoryDeviceStructure[];                       // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PMTT_VENDOR_SPECIFIC_TYPE_DATA;                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Boot Graphics Resource Table definition.                                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// 2-bytes (16 bit) version ID. This value must be 1.                                          // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT16                         Version;                                                         // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// 1-byte status field indicating current status about the table.                              // [CODE_FIRST] 11148
  ///     Bits[7:3] = Reserved (must be zero)                                                     // [CODE_FIRST] 11148
  ///     Bits[2:1] = Orientation Offset. These bits describe the clockwise                       // [CODE_FIRST] 11148
  ///                 degree offset from the image's default orientation.                         // [CODE_FIRST] 11148
  ///                 [00] = 0, no offset                                                         // [CODE_FIRST] 11148
  ///                 [01] = 90                                                                   // [CODE_FIRST] 11148
  ///                 [10] = 180                                                                  // [CODE_FIRST] 11148
  ///                 [11] = 270                                                                  // [CODE_FIRST] 11148
  ///     Bit [0] = Displayed. A one indicates the boot image graphic is                          // [CODE_FIRST] 11148
  ///               displayed.                                                                    // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT8     Status;                                                                               // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// 1-byte enumerated type field indicating format of the image.                                // [CODE_FIRST] 11148
  ///     0 = Bitmap                                                                              // [CODE_FIRST] 11148
  ///     1 - 255  Reserved (for future use)                                                      // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT8     ImageType;                                                                            // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// 8-byte (64 bit) physical address pointing to the firmware's in-memory copy                  // [CODE_FIRST] 11148
  /// of the image bitmap.                                                                        // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64    ImageAddress;                                                                         // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// A 4-byte (32-bit) unsigned long describing the display X-offset of the boot image.          // [CODE_FIRST] 11148
  /// (X, Y) display offset of the top left corner of the boot image.                             // [CODE_FIRST] 11148
  /// The top left corner of the display is at offset (0, 0).                                     // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT32    ImageOffsetX;                                                                         // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// A 4-byte (32-bit) unsigned long describing the display Y-offset of the boot image.          // [CODE_FIRST] 11148
  /// (X, Y) display offset of the top left corner of the boot image.                             // [CODE_FIRST] 11148
  /// The top left corner of the display is at offset (0, 0).                                     // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT32    ImageOffsetY;                                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_BOOT_GRAPHICS_RESOURCE_TABLE;                                                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// BGRT Revision                                                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_BOOT_GRAPHICS_RESOURCE_TABLE_REVISION  1                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// BGRT Version                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_BGRT_VERSION  0x01                                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// BGRT Status                                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_BGRT_STATUS_NOT_DISPLAYED  0x00                                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_BGRT_STATUS_DISPLAYED      0x01                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// BGRT Image Type                                                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_BGRT_IMAGE_TYPE_BMP  0x00                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FIRMWARE_PERFORMANCE_DATA_TABLE_REVISION  0x01                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Performance Record Types                                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_RECORD_TYPE_FIRMWARE_BASIC_BOOT_POINTER   0x0000                        // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_RECORD_TYPE_S3_PERFORMANCE_TABLE_POINTER  0x0001                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Performance Record Revision                                                              // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_RECORD_REVISION_FIRMWARE_BASIC_BOOT_POINTER   0x01                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_RECORD_REVISION_S3_PERFORMANCE_TABLE_POINTER  0x01                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Runtime Performance Record Types                                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_RUNTIME_RECORD_TYPE_S3_RESUME            0x0000                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_RUNTIME_RECORD_TYPE_S3_SUSPEND           0x0001                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_RUNTIME_RECORD_TYPE_FIRMWARE_BASIC_BOOT  0x0002                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Runtime Performance Record Revision                                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_RUNTIME_RECORD_REVISION_S3_RESUME            0x01                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_RUNTIME_RECORD_REVISION_S3_SUSPEND           0x01                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_RUNTIME_RECORD_REVISION_FIRMWARE_BASIC_BOOT  0x02                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Performance Record header                                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Revision;                                                                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FPDT_PERFORMANCE_RECORD_HEADER;                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Performance Table header                                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    Signature;                                                                            // [CODE_FIRST] 11148
  UINT32    Length;                                                                               // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FPDT_PERFORMANCE_TABLE_HEADER;                                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Firmware Basic Boot Performance Pointer Record Structure                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_FPDT_PERFORMANCE_RECORD_HEADER    Header;                                          // [CODE_FIRST] 11148
  UINT32                                         Reserved;                                        // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// 64-bit processor-relative physical address of the Basic Boot Performance Table.             // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64                                         BootPerformanceTablePointer;                     // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FPDT_BOOT_PERFORMANCE_TABLE_POINTER_RECORD;                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT S3 Performance Table Pointer Record Structure                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_FPDT_PERFORMANCE_RECORD_HEADER    Header;                                          // [CODE_FIRST] 11148
  UINT32                                         Reserved;                                        // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// 64-bit processor-relative physical address of the S3 Performance Table.                     // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64                                         S3PerformanceTablePointer;                       // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FPDT_S3_PERFORMANCE_TABLE_POINTER_RECORD;                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Firmware Basic Boot Performance Record Structure                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_FPDT_PERFORMANCE_RECORD_HEADER    Header;                                          // [CODE_FIRST] 11148
  UINT32                                         Reserved;                                        // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// Timer value logged at the beginning of firmware image execution.                            // [CODE_FIRST] 11148
  /// This may not always be zero or near zero.                                                   // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64                                         ResetEnd;                                        // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// Timer value logged just prior to loading the OS boot loader into memory.                    // [CODE_FIRST] 11148
  /// For non-UEFI compatible boots, this field must be zero.                                     // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64                                         OsLoaderLoadImageStart;                          // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// Timer value logged just prior to launching the previously loaded OS boot loader image.      // [CODE_FIRST] 11148
  /// For non-UEFI compatible boots, the timer value logged will be just prior                    // [CODE_FIRST] 11148
  /// to the INT 19h handler invocation.                                                          // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64                                         OsLoaderStartImageStart;                         // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// Timer value logged at the point when the OS loader calls the                                // [CODE_FIRST] 11148
  /// ExitBootServices function for UEFI compatible firmware.                                     // [CODE_FIRST] 11148
  /// For non-UEFI compatible boots, this field must be zero.                                     // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64                                         ExitBootServicesEntry;                           // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// Timer value logged at the point just prior towhen the OS loader gaining                     // [CODE_FIRST] 11148
  /// control back from calls the ExitBootServices function for UEFI compatible firmware.         // [CODE_FIRST] 11148
  /// For non-UEFI compatible boots, this field must be zero.                                     // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64                                         ExitBootServicesExit;                            // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FPDT_FIRMWARE_BASIC_BOOT_RECORD;                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Firmware Basic Boot Performance Table signature                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_BOOT_PERFORMANCE_TABLE_SIGNATURE  SIGNATURE_32('F', 'B', 'P', 'T')      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// FPDT Firmware Basic Boot Performance Table                                                     // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_FPDT_PERFORMANCE_TABLE_HEADER    Header;                                           // [CODE_FIRST] 11148
  //                                                                                              // [CODE_FIRST] 11148
  // one or more Performance Records.                                                             // [CODE_FIRST] 11148
  //                                                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FPDT_FIRMWARE_BASIC_BOOT_TABLE;                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT "S3PT" S3 Performance Table                                                              // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FPDT_S3_PERFORMANCE_TABLE_SIGNATURE  SIGNATURE_32('S', '3', 'P', 'T')        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// FPDT Firmware S3 Boot Performance Table                                                        // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_FPDT_PERFORMANCE_TABLE_HEADER    Header;                                           // [CODE_FIRST] 11148
  //                                                                                              // [CODE_FIRST] 11148
  // one or more Performance Records.                                                             // [CODE_FIRST] 11148
  //                                                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FPDT_FIRMWARE_S3_BOOT_TABLE;                                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Basic S3 Resume Performance Record                                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_FPDT_PERFORMANCE_RECORD_HEADER    Header;                                          // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// A count of the number of S3 resume cycles since the last full boot sequence.                // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT32                                         ResumeCount;                                     // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// Timer recorded at the end of BIOS S3 resume, just prior to handoff to the                   // [CODE_FIRST] 11148
  /// OS waking vector. Only the most recent resume cycle's time is retained.                     // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64                                         FullResume;                                      // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// Average timer value of all resume cycles logged since the last full boot                    // [CODE_FIRST] 11148
  /// sequence, including the most recent resume.  Note that the entire log of                    // [CODE_FIRST] 11148
  /// timer values does not need to be retained in order to calculate this average.               // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64                                         AverageResume;                                   // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FPDT_S3_RESUME_RECORD;                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// FPDT Basic S3 Suspend Performance Record                                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_FPDT_PERFORMANCE_RECORD_HEADER    Header;                                          // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// Timer value recorded at the OS write to SLP_TYP upon entry to S3.                           // [CODE_FIRST] 11148
  /// Only the most recent suspend cycle's timer value is retained.                               // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64                                         SuspendStart;                                    // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  /// Timer value recorded at the final firmware write to SLP_TYP (or other                       // [CODE_FIRST] 11148
  /// mechanism) used to trigger hardware entry to S3.                                            // [CODE_FIRST] 11148
  /// Only the most recent suspend cycle's timer value is retained.                               // [CODE_FIRST] 11148
  ///                                                                                             // [CODE_FIRST] 11148
  UINT64                                         SuspendEnd;                                      // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FPDT_S3_SUSPEND_RECORD;                                                            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Firmware Performance Record Table definition.                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_FIRMWARE_PERFORMANCE_RECORD_TABLE;                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Generic Timer Description Table definition.                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT64                         CntControlBasePhysicalAddress;                                   // [CODE_FIRST] 11148
  UINT32                         Reserved;                                                        // [CODE_FIRST] 11148
  UINT32                         SecurePL1TimerGSIV;                                              // [CODE_FIRST] 11148
  UINT32                         SecurePL1TimerFlags;                                             // [CODE_FIRST] 11148
  UINT32                         NonSecurePL1TimerGSIV;                                           // [CODE_FIRST] 11148
  UINT32                         NonSecurePL1TimerFlags;                                          // [CODE_FIRST] 11148
  UINT32                         VirtualTimerGSIV;                                                // [CODE_FIRST] 11148
  UINT32                         VirtualTimerFlags;                                               // [CODE_FIRST] 11148
  UINT32                         NonSecurePL2TimerGSIV;                                           // [CODE_FIRST] 11148
  UINT32                         NonSecurePL2TimerFlags;                                          // [CODE_FIRST] 11148
  UINT64                         CntReadBasePhysicalAddress;                                      // [CODE_FIRST] 11148
  UINT32                         PlatformTimerCount;                                              // [CODE_FIRST] 11148
  UINT32                         PlatformTimerOffset;                                             // [CODE_FIRST] 11148
  UINT32                         VirtualPL2TimerGSIV;                                             // [CODE_FIRST] 11148
  UINT32                         VirtualPL2TimerFlags;                                            // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GENERIC_TIMER_DESCRIPTION_TABLE;                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GTDT Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GENERIC_TIMER_DESCRIPTION_TABLE_REVISION  0x03                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Timer Flags.  All other bits are reserved and must be 0.                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_TIMER_FLAG_TIMER_INTERRUPT_MODE      BIT0                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_TIMER_FLAG_TIMER_INTERRUPT_POLARITY  BIT1                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_TIMER_FLAG_ALWAYS_ON_CAPABILITY      BIT2                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Platform Timer Type                                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_GT_BLOCK              0                                                 // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_ARM_GENERIC_WATCHDOG  1                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GT Block Structure                                                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Reserved;                                                                             // [CODE_FIRST] 11148
  UINT64    CntCtlBase;                                                                           // [CODE_FIRST] 11148
  UINT32    GTBlockTimerCount;                                                                    // [CODE_FIRST] 11148
  UINT32    GTBlockTimerOffset;                                                                   // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GTDT_GT_BLOCK_STRUCTURE;                                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GT Block Timer Structure                                                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     GTFrameNumber;                                                                        // [CODE_FIRST] 11148
  UINT8     Reserved[3];                                                                          // [CODE_FIRST] 11148
  UINT64    CntBaseX;                                                                             // [CODE_FIRST] 11148
  UINT64    CntEL0BaseX;                                                                          // [CODE_FIRST] 11148
  UINT32    GTxPhysicalTimerGSIV;                                                                 // [CODE_FIRST] 11148
  UINT32    GTxPhysicalTimerFlags;                                                                // [CODE_FIRST] 11148
  UINT32    GTxVirtualTimerGSIV;                                                                  // [CODE_FIRST] 11148
  UINT32    GTxVirtualTimerFlags;                                                                 // [CODE_FIRST] 11148
  UINT32    GTxCommonFlags;                                                                       // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GTDT_GT_BLOCK_TIMER_STRUCTURE;                                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// GT Block Physical Timers and Virtual Timers Flags.  All other bits are reserved and must be 0.  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_GT_BLOCK_TIMER_FLAG_TIMER_INTERRUPT_MODE      BIT0                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_GT_BLOCK_TIMER_FLAG_TIMER_INTERRUPT_POLARITY  BIT1                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Common Flags Flags.  All other bits are reserved and must be 0.                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_GT_BLOCK_COMMON_FLAG_SECURE_TIMER          BIT0                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_GT_BLOCK_COMMON_FLAG_ALWAYS_ON_CAPABILITY  BIT1                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Arm Generic Watchdog Structure                                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT8     Reserved;                                                                             // [CODE_FIRST] 11148
  UINT64    RefreshFramePhysicalAddress;                                                          // [CODE_FIRST] 11148
  UINT64    WatchdogControlFramePhysicalAddress;                                                  // [CODE_FIRST] 11148
  UINT32    WatchdogTimerGSIV;                                                                    // [CODE_FIRST] 11148
  UINT32    WatchdogTimerFlags;                                                                   // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GTDT_ARM_GENERIC_WATCHDOG_STRUCTURE;                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Arm Generic Watchdog Timer Flags.  All other bits are reserved and must be 0.                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_ARM_GENERIC_WATCHDOG_FLAG_TIMER_INTERRUPT_MODE      BIT0                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_ARM_GENERIC_WATCHDOG_FLAG_TIMER_INTERRUPT_POLARITY  BIT1                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GTDT_ARM_GENERIC_WATCHDOG_FLAG_SECURE_TIMER              BIT2                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// NVDIMM Firmware Interface Table definition.                                                    // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT32                         Reserved;                                                        // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NVDIMM_FIRMWARE_INTERFACE_TABLE;                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// NFIT Version (as defined in ACPI 6.7 spec.)                                                    // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NVDIMM_FIRMWARE_INTERFACE_TABLE_REVISION  0x1                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Definition for NFIT Table Structure Types                                                      // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE_TYPE    0                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_NVDIMM_REGION_MAPPING_STRUCTURE_TYPE            1                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_INTERLEAVE_STRUCTURE_TYPE                       2                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_SMBIOS_MANAGEMENT_INFORMATION_STRUCTURE_TYPE    3                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE_TYPE            4                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_NVDIMM_BLOCK_DATA_WINDOW_REGION_STRUCTURE_TYPE  5                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_FLUSH_HINT_ADDRESS_STRUCTURE_TYPE               6                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_PLATFORM_CAPABILITIES_STRUCTURE_TYPE            7                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Definition for NFIT Structure Header                                                           // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NFIT_STRUCTURE_HEADER;                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Definition for System Physical Address Range Structure                                         // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_FLAGS_CONTROL_REGION_FOR_MANAGEMENT  BIT0  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_FLAGS_PROXIMITY_DOMAIN_VALID         BIT1  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_FLAGS_SPA_LOCATION_COOKIE_VALID      BIT2  // [CODE_FIRST] 11148
                                                                                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_GUID_VOLATILE_MEMORY_REGION                              { 0x7305944F, 0xFDDA, 0x44E3, { 0xB1, 0x6C, 0x3F, 0x22, 0xD2, 0x52, 0xE5, 0xD0 }}  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_GUID_BYTE_ADDRESSABLE_PERSISTENT_MEMORY_REGION           { 0x66F0D379, 0xB4F3, 0x4074, { 0xAC, 0x43, 0x0D, 0x33, 0x18, 0xB7, 0x8C, 0xDB }}  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_GUID_NVDIMM_CONTROL_REGION                               { 0x92F701F6, 0x13B4, 0x405D, { 0x91, 0x0B, 0x29, 0x93, 0x67, 0xE8, 0x23, 0x4C }}  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_GUID_NVDIMM_BLOCK_DATA_WINDOW_REGION                     { 0x91AF0530, 0x5D86, 0x470E, { 0xA6, 0xB0, 0x0A, 0x2D, 0xB9, 0x40, 0x82, 0x49 }}  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_GUID_RAM_DISK_SUPPORTING_VIRTUAL_DISK_REGION_VOLATILE    { 0x77AB535A, 0x45FC, 0x624B, { 0x55, 0x60, 0xF7, 0xB2, 0x81, 0xD1, 0xF9, 0x6E }}  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_GUID_RAM_DISK_SUPPORTING_VIRTUAL_CD_REGION_VOLATILE      { 0x3D5ABD30, 0x4175, 0x87CE, { 0x6D, 0x64, 0xD2, 0xAD, 0xE5, 0x23, 0xC4, 0xBB }}  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_GUID_RAM_DISK_SUPPORTING_VIRTUAL_DISK_REGION_PERSISTENT  { 0x5CEA02C9, 0x4D07, 0x69D3, { 0x26, 0x9F ,0x44, 0x96, 0xFB, 0xE0, 0x96, 0xF9 }}  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_GUID_RAM_DISK_SUPPORTING_VIRTUAL_CD_REGION_PERSISTENT    { 0x08018188, 0x42CD, 0xBB48, { 0x10, 0x0F, 0x53, 0x87, 0xD5, 0x3D, 0xED, 0x3D }}  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT16    SPARangeStructureIndex;                                                               // [CODE_FIRST] 11148
  UINT16    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    Reserved_8;                                                                           // [CODE_FIRST] 11148
  UINT32    ProximityDomain;                                                                      // [CODE_FIRST] 11148
  GUID      AddressRangeTypeGUID;                                                                 // [CODE_FIRST] 11148
  UINT64    SystemPhysicalAddressRangeBase;                                                       // [CODE_FIRST] 11148
  UINT64    SystemPhysicalAddressRangeLength;                                                     // [CODE_FIRST] 11148
  UINT64    AddressRangeMemoryMappingAttribute;                                                   // [CODE_FIRST] 11148
  UINT64    SPALocationCookie;                                                                    // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE;                                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Definition for Memory Device to System Physical Address Range Mapping Structure                // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    DIMMNumber          : 4;                                                              // [CODE_FIRST] 11148
  UINT32    MemoryChannelNumber : 4;                                                              // [CODE_FIRST] 11148
  UINT32    MemoryControllerID  : 4;                                                              // [CODE_FIRST] 11148
  UINT32    SocketID            : 4;                                                              // [CODE_FIRST] 11148
  UINT32    NodeControllerID    : 12;                                                             // [CODE_FIRST] 11148
  UINT32    Reserved_28         : 4;                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NFIT_DEVICE_HANDLE;                                                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_MEMORY_DEVICE_STATE_FLAGS_PREVIOUS_SAVE_FAIL                                      BIT0  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_MEMORY_DEVICE_STATE_FLAGS_LAST_RESTORE_FAIL                                       BIT1  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_MEMORY_DEVICE_STATE_FLAGS_PLATFORM_FLUSH_FAIL                                     BIT2  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_MEMORY_DEVICE_STATE_FLAGS_NOT_ARMED_PRIOR_TO_OSPM_HAND_OFF                        BIT3  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_MEMORY_DEVICE_STATE_FLAGS_SMART_HEALTH_EVENTS_PRIOR_OSPM_HAND_OFF                 BIT4  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_MEMORY_DEVICE_STATE_FLAGS_FIRMWARE_ENABLED_TO_NOTIFY_OSPM_ON_SMART_HEALTH_EVENTS  BIT5  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_MEMORY_DEVICE_STATE_FLAGS_FIRMWARE_NOT_MAP_NVDIMM_TO_SPA                          BIT6  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16                             Type;                                                        // [CODE_FIRST] 11148
  UINT16                             Length;                                                      // [CODE_FIRST] 11148
  EFI_ACPI_6_7_NFIT_DEVICE_HANDLE    NFITDeviceHandle;                                            // [CODE_FIRST] 11148
  UINT16                             NVDIMMPhysicalID;                                            // [CODE_FIRST] 11148
  UINT16                             NVDIMMRegionID;                                              // [CODE_FIRST] 11148
  UINT16                             SPARangeStructureIndex;                                      // [CODE_FIRST] 11148
  UINT16                             NVDIMMControlRegionStructureIndex;                           // [CODE_FIRST] 11148
  UINT64                             NVDIMMRegionSize;                                            // [CODE_FIRST] 11148
  UINT64                             RegionOffset;                                                // [CODE_FIRST] 11148
  UINT64                             NVDIMMPhysicalAddressRegionBase;                             // [CODE_FIRST] 11148
  UINT16                             InterleaveStructureIndex;                                    // [CODE_FIRST] 11148
  UINT16                             InterleaveWays;                                              // [CODE_FIRST] 11148
  UINT16                             NVDIMMStateFlags;                                            // [CODE_FIRST] 11148
  UINT16                             Reserved_46;                                                 // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NFIT_NVDIMM_REGION_MAPPING_STRUCTURE;                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Definition for Interleave Structure                                                            // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT16    InterleaveStructureIndex;                                                             // [CODE_FIRST] 11148
  UINT16    Reserved_6;                                                                           // [CODE_FIRST] 11148
  UINT32    NumberOfLines;                                                                        // [CODE_FIRST] 11148
  UINT32    LineSize;                                                                             // [CODE_FIRST] 11148
  // UINT32                                      LineOffset[NumberOfLines];                       // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NFIT_INTERLEAVE_STRUCTURE;                                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Definition for SMBIOS Management Information Structure                                         // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT32    Reserved_4;                                                                           // [CODE_FIRST] 11148
  // UINT8                                       Data[];                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NFIT_SMBIOS_MANAGEMENT_INFORMATION_STRUCTURE;                                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Definition for NVDIMM Control Region Structure                                                 // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_NVDIMM_CONTROL_REGION_VALID_FIELDS_MANUFACTURING  BIT0                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_NVDIMM_CONTROL_REGION_FLAGS_BLOCK_DATA_WINDOWS_BUFFERED  BIT0           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT16    NVDIMMControlRegionStructureIndex;                                                    // [CODE_FIRST] 11148
  UINT16    VendorID;                                                                             // [CODE_FIRST] 11148
  UINT16    DeviceID;                                                                             // [CODE_FIRST] 11148
  UINT16    RevisionID;                                                                           // [CODE_FIRST] 11148
  UINT16    SubsystemVendorID;                                                                    // [CODE_FIRST] 11148
  UINT16    SubsystemDeviceID;                                                                    // [CODE_FIRST] 11148
  UINT16    SubsystemRevisionID;                                                                  // [CODE_FIRST] 11148
  UINT8     ValidFields;                                                                          // [CODE_FIRST] 11148
  UINT8     ManufacturingLocation;                                                                // [CODE_FIRST] 11148
  UINT16    ManufacturingDate;                                                                    // [CODE_FIRST] 11148
  UINT8     Reserved_22[2];                                                                       // [CODE_FIRST] 11148
  UINT32    SerialNumber;                                                                         // [CODE_FIRST] 11148
  UINT16    RegionFormatInterfaceCode;                                                            // [CODE_FIRST] 11148
  UINT16    NumberOfBlockControlWindows;                                                          // [CODE_FIRST] 11148
  UINT64    SizeOfBlockControlWindow;                                                             // [CODE_FIRST] 11148
  UINT64    CommandRegisterOffsetInBlockControlWindow;                                            // [CODE_FIRST] 11148
  UINT64    SizeOfCommandRegisterInBlockControlWindows;                                           // [CODE_FIRST] 11148
  UINT64    StatusRegisterOffsetInBlockControlWindow;                                             // [CODE_FIRST] 11148
  UINT64    SizeOfStatusRegisterInBlockControlWindows;                                            // [CODE_FIRST] 11148
  UINT16    NVDIMMControlRegionFlag;                                                              // [CODE_FIRST] 11148
  UINT8     Reserved_74[6];                                                                       // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE;                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Definition for NVDIMM Block Data Window Region Structure                                       // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT16    NVDIMMControlRegionStructureIndex;                                                    // [CODE_FIRST] 11148
  UINT16    NumberOfBlockDataWindows;                                                             // [CODE_FIRST] 11148
  UINT64    BlockDataWindowStartOffset;                                                           // [CODE_FIRST] 11148
  UINT64    SizeOfBlockDataWindow;                                                                // [CODE_FIRST] 11148
  UINT64    BlockAccessibleMemoryCapacity;                                                        // [CODE_FIRST] 11148
  UINT64    BeginningAddressOfFirstBlockInBlockAccessibleMemory;                                  // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NFIT_NVDIMM_BLOCK_DATA_WINDOW_REGION_STRUCTURE;                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Definition for Flush Hint Address Structure                                                    // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16                             Type;                                                        // [CODE_FIRST] 11148
  UINT16                             Length;                                                      // [CODE_FIRST] 11148
  EFI_ACPI_6_7_NFIT_DEVICE_HANDLE    NFITDeviceHandle;                                            // [CODE_FIRST] 11148
  UINT16                             NumberOfFlushHintAddresses;                                  // [CODE_FIRST] 11148
  UINT8                              Reserved_10[6];                                              // [CODE_FIRST] 11148
  // UINT64                                      FlushHintAddress[NumberOfFlushHintAddresses];    // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NFIT_FLUSH_HINT_ADDRESS_STRUCTURE;                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Definition for Platform Capabilities Structure                                                 // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT8     HighestValidCapability;                                                               // [CODE_FIRST] 11148
  UINT8     Reserved_5[3];                                                                        // [CODE_FIRST] 11148
  UINT32    Capabilities;                                                                         // [CODE_FIRST] 11148
  UINT8     Reserved_12[4];                                                                       // [CODE_FIRST] 11148
} EFI_ACPI_6_7_NFIT_PLATFORM_CAPABILITIES_STRUCTURE;                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_PLATFORM_CAPABILITY_CPU_CACHE_FLUSH_TO_NVDIMM_DURABILITY_ON_POWER_LOSS          BIT0  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_PLATFORM_CAPABILITY_MEMORY_CONTROLLER_FLUSH_TO_NVDIMM_DURABILITY_ON_POWER_LOSS  BIT1  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NFIT_PLATFORM_CAPABILITY_BYTE_ADDRESSABLE_PERSISTENT_MEMORY_HARDWARE_MIRRORING       BIT2  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Secure DEVices Table (SDEV)                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_SECURE_DEVICES_TABLE_HEADER;                                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// SDEV Revision (as defined in ACPI 6.7 spec.)                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SECURE_DEVICES_TABLE_REVISION  0x01                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Secure Device types                                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SDEV_TYPE_ACPI_NAMESPACE_DEVICE  0x00                                        // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SDEV_TYPE_PCIE_ENDPOINT_DEVICE   0x01                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Secure Device flags                                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SDEV_FLAG_ALLOW_HANDOFF                     BIT0                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SDEV_FLAG_SECURE_ACCESS_COMPONENTS_PRESENT  BIT1                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// SDEV Structure Header                                                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Flags;                                                                                // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
} EFI_ACPI_6_7_SDEV_STRUCTURE_HEADER;                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ACPI_NAMESPACE_DEVICE based Secure Device Structure                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_SDEV_STRUCTURE_HEADER    Header;                                                   // [CODE_FIRST] 11148
  UINT16                                DeviceIdentifierOffset;                                   // [CODE_FIRST] 11148
  UINT16                                DeviceIdentifierLength;                                   // [CODE_FIRST] 11148
  UINT16                                VendorSpecificDataOffset;                                 // [CODE_FIRST] 11148
  UINT16                                VendorSpecificDataLength;                                 // [CODE_FIRST] 11148
  UINT16                                SecureAccessComponentsOffset;                             // [CODE_FIRST] 11148
  UINT16                                SecureAccessComponentsLength;                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_SDEV_STRUCTURE_ACPI_NAMESPACE_DEVICE;                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Secure Access Component Types                                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SDEV_SECURE_ACCESS_COMPONENT_TYPE_IDENTIFICATION  0x00                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SDEV_SECURE_ACCESS_COMPONENT_TYPE_MEMORY          0x01                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Identification Based Secure Access Component                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_SDEV_STRUCTURE_HEADER    Header;                                                   // [CODE_FIRST] 11148
  UINT16                                HardwareIdentifierOffset;                                 // [CODE_FIRST] 11148
  UINT16                                HardwareIdentifierLength;                                 // [CODE_FIRST] 11148
  UINT16                                SubsystemIdentifierOffset;                                // [CODE_FIRST] 11148
  UINT16                                SubsystemIdentifierLength;                                // [CODE_FIRST] 11148
  UINT16                                HardwareRevision;                                         // [CODE_FIRST] 11148
  UINT8                                 HardwareRevisionPresent;                                  // [CODE_FIRST] 11148
  UINT8                                 ClassCodePresent;                                         // [CODE_FIRST] 11148
  UINT8                                 PciCompatibleBaseClass;                                   // [CODE_FIRST] 11148
  UINT8                                 PciCompatibleSubClass;                                    // [CODE_FIRST] 11148
  UINT8                                 PciCompatibleProgrammingInterface;                        // [CODE_FIRST] 11148
} EFI_ACPI_6_7_SDEV_SECURE_ACCESS_COMPONENT_IDENTIFICATION_STRUCTURE;                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Memory-based Secure Access Component                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_SDEV_STRUCTURE_HEADER    Header;                                                   // [CODE_FIRST] 11148
  UINT32                                Reserved;                                                 // [CODE_FIRST] 11148
  UINT64                                MemoryAddressBase;                                        // [CODE_FIRST] 11148
  UINT64                                MemoryLength;                                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_SDEV_SECURE_ACCESS_COMPONENT_MEMORY_STRUCTURE;                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PCIe Endpoint Device based Secure Device Structure                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_SDEV_STRUCTURE_HEADER    Header;                                                   // [CODE_FIRST] 11148
  UINT16                                PciSegmentNumber;                                         // [CODE_FIRST] 11148
  UINT16                                StartBusNumber;                                           // [CODE_FIRST] 11148
  UINT16                                PciPathOffset;                                            // [CODE_FIRST] 11148
  UINT16                                PciPathLength;                                            // [CODE_FIRST] 11148
  UINT16                                VendorSpecificDataOffset;                                 // [CODE_FIRST] 11148
  UINT16                                VendorSpecificDataLength;                                 // [CODE_FIRST] 11148
} EFI_ACPI_6_7_SDEV_STRUCTURE_PCIE_ENDPOINT_DEVICE;                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Boot Error Record Table (BERT)                                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT32                         BootErrorRegionLength;                                           // [CODE_FIRST] 11148
  UINT64                         BootErrorRegion;                                                 // [CODE_FIRST] 11148
} EFI_ACPI_6_7_BOOT_ERROR_RECORD_TABLE_HEADER;                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// BERT Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_BOOT_ERROR_RECORD_TABLE_REVISION  0x01                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Boot Error Region Block Status Definition                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    UncorrectableErrorValid     : 1;                                                      // [CODE_FIRST] 11148
  UINT32    CorrectableErrorValid       : 1;                                                      // [CODE_FIRST] 11148
  UINT32    MultipleUncorrectableErrors : 1;                                                      // [CODE_FIRST] 11148
  UINT32    MultipleCorrectableErrors   : 1;                                                      // [CODE_FIRST] 11148
  UINT32    ErrorDataEntryCount         : 10;                                                     // [CODE_FIRST] 11148
  UINT32    Reserved                    : 18;                                                     // [CODE_FIRST] 11148
} EFI_ACPI_6_7_ERROR_BLOCK_STATUS;                                                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Boot Error Region Definition                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_ERROR_BLOCK_STATUS    BlockStatus;                                                 // [CODE_FIRST] 11148
  UINT32                             RawDataOffset;                                               // [CODE_FIRST] 11148
  UINT32                             RawDataLength;                                               // [CODE_FIRST] 11148
  UINT32                             DataLength;                                                  // [CODE_FIRST] 11148
  UINT32                             ErrorSeverity;                                               // [CODE_FIRST] 11148
} EFI_ACPI_6_7_BOOT_ERROR_REGION_STRUCTURE;                                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Boot Error Severity types                                                                      // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_SEVERITY_RECOVERABLE  0x00                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_SEVERITY_FATAL        0x01                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_SEVERITY_CORRECTED    0x02                                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_SEVERITY_NONE         0x03                                             // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// The term 'Correctable' is no longer being used as an error severity of the                     // [CODE_FIRST] 11148
// reported error since ACPI Specification Version 5.1 Errata B.                                  // [CODE_FIRST] 11148
// The below macro is considered as deprecated and should no longer be used.                      // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_SEVERITY_CORRECTABLE  0x00                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Generic Error Data Entry Definition                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     SectionType[16];                                                                      // [CODE_FIRST] 11148
  UINT32    ErrorSeverity;                                                                        // [CODE_FIRST] 11148
  UINT16    Revision;                                                                             // [CODE_FIRST] 11148
  UINT8     ValidationBits;                                                                       // [CODE_FIRST] 11148
  UINT8     Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    ErrorDataLength;                                                                      // [CODE_FIRST] 11148
  UINT8     FruId[16];                                                                            // [CODE_FIRST] 11148
  UINT8     FruText[20];                                                                          // [CODE_FIRST] 11148
  UINT8     Timestamp[8];                                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GENERIC_ERROR_DATA_ENTRY_STRUCTURE;                                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Generic Error Data Entry Version (as defined in ACPI 6.7 spec.)                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GENERIC_ERROR_DATA_ENTRY_REVISION  0x0300                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// HEST - Hardware Error Source Table                                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT32                         ErrorSourceCount;                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HARDWARE_ERROR_SOURCE_TABLE_HEADER;                                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// HEST Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_SOURCE_TABLE_REVISION  0x02                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Error Source structure types.                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION  0x00                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK  0x01                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IA32_ARCHITECTURE_NMI_ERROR                0x02                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCI_EXPRESS_ROOT_PORT_AER                  0x06                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCI_EXPRESS_DEVICE_AER                     0x07                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCI_EXPRESS_BRIDGE_AER                     0x08                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GENERIC_HARDWARE_ERROR                     0x09                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GENERIC_HARDWARE_ERROR_VERSION_2           0x0A                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK   0x0B                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Error Source structure flags.                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_SOURCE_FLAG_FIRMWARE_FIRST  (1 << 0)                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_SOURCE_FLAG_GLOBAL          (1 << 1)                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_SOURCE_FLAG_GHES_ASSIST     (1 << 2)                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// IA-32 Architecture Machine Check Exception Structure Definition                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    SourceId;                                                                             // [CODE_FIRST] 11148
  UINT8     Reserved0[2];                                                                         // [CODE_FIRST] 11148
  UINT8     Flags;                                                                                // [CODE_FIRST] 11148
  UINT8     Enabled;                                                                              // [CODE_FIRST] 11148
  UINT32    NumberOfRecordsToPreAllocate;                                                         // [CODE_FIRST] 11148
  UINT32    MaxSectionsPerRecord;                                                                 // [CODE_FIRST] 11148
  UINT64    GlobalCapabilityInitData;                                                             // [CODE_FIRST] 11148
  UINT64    GlobalControlInitData;                                                                // [CODE_FIRST] 11148
  UINT8     NumberOfHardwareBanks;                                                                // [CODE_FIRST] 11148
  UINT8     Reserved1[7];                                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_IA32_ARCHITECTURE_MACHINE_CHECK_EXCEPTION_STRUCTURE;                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// IA-32 Architecture Machine Check Bank Structure Definition                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     BankNumber;                                                                           // [CODE_FIRST] 11148
  UINT8     ClearStatusOnInitialization;                                                          // [CODE_FIRST] 11148
  UINT8     StatusDataFormat;                                                                     // [CODE_FIRST] 11148
  UINT8     Reserved0;                                                                            // [CODE_FIRST] 11148
  UINT32    ControlRegisterMsrAddress;                                                            // [CODE_FIRST] 11148
  UINT64    ControlInitData;                                                                      // [CODE_FIRST] 11148
  UINT32    StatusRegisterMsrAddress;                                                             // [CODE_FIRST] 11148
  UINT32    AddressRegisterMsrAddress;                                                            // [CODE_FIRST] 11148
  UINT32    MiscRegisterMsrAddress;                                                               // [CODE_FIRST] 11148
} EFI_ACPI_6_7_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_BANK_STRUCTURE;                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// IA-32 Architecture Machine Check Bank Structure MCA data format                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_DATA_FORMAT_IA32     0x00              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_DATA_FORMAT_INTEL64  0x01              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IA32_ARCHITECTURE_MACHINE_CHECK_ERROR_DATA_FORMAT_AMD64    0x02              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Hardware Error Notification types. All other values are reserved                               // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_POLLED                        0x00               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_EXTERNAL_INTERRUPT            0x01               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_LOCAL_INTERRUPT               0x02               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_SCI                           0x03               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_NMI                           0x04               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_CMCI                          0x05               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_MCE                           0x06               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_GPIO_SIGNAL                   0x07               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_ARMV8_SEA                     0x08               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_ARMV8_SEI                     0x09               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_GSIV                          0x0A               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_SOFTWARE_DELEGATED_EXCEPTION  0x0B               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Hardware Error Notification Configuration Write Enable Structure Definition                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type                           : 1;                                                   // [CODE_FIRST] 11148
  UINT16    PollInterval                   : 1;                                                   // [CODE_FIRST] 11148
  UINT16    SwitchToPollingThresholdValue  : 1;                                                   // [CODE_FIRST] 11148
  UINT16    SwitchToPollingThresholdWindow : 1;                                                   // [CODE_FIRST] 11148
  UINT16    ErrorThresholdValue            : 1;                                                   // [CODE_FIRST] 11148
  UINT16    ErrorThresholdWindow           : 1;                                                   // [CODE_FIRST] 11148
  UINT16    Reserved                       : 10;                                                  // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_CONFIGURATION_WRITE_ENABLE_STRUCTURE;                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Hardware Error Notification Structure Definition                                              // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8                                                                            Type;                           // [CODE_FIRST] 11148
  UINT8                                                                            Length;                         // [CODE_FIRST] 11148
  EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_CONFIGURATION_WRITE_ENABLE_STRUCTURE    ConfigurationWriteEnable;       // [CODE_FIRST] 11148
  UINT32                                                                           PollInterval;                   // [CODE_FIRST] 11148
  UINT32                                                                           Vector;                         // [CODE_FIRST] 11148
  UINT32                                                                           SwitchToPollingThresholdValue;  // [CODE_FIRST] 11148
  UINT32                                                                           SwitchToPollingThresholdWindow; // [CODE_FIRST] 11148
  UINT32                                                                           ErrorThresholdValue;            // [CODE_FIRST] 11148
  UINT32                                                                           ErrorThresholdWindow;           // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_STRUCTURE;                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// IA-32 Architecture Corrected Machine Check Structure Definition                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16                                                Type;                                     // [CODE_FIRST] 11148
  UINT16                                                SourceId;                                 // [CODE_FIRST] 11148
  UINT8                                                 Reserved0[2];                             // [CODE_FIRST] 11148
  UINT8                                                 Flags;                                    // [CODE_FIRST] 11148
  UINT8                                                 Enabled;                                  // [CODE_FIRST] 11148
  UINT32                                                NumberOfRecordsToPreAllocate;             // [CODE_FIRST] 11148
  UINT32                                                MaxSectionsPerRecord;                     // [CODE_FIRST] 11148
  EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_STRUCTURE    NotificationStructure;                    // [CODE_FIRST] 11148
  UINT8                                                 NumberOfHardwareBanks;                    // [CODE_FIRST] 11148
  UINT8                                                 Reserved1[3];                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_IA32_ARCHITECTURE_CORRECTED_MACHINE_CHECK_STRUCTURE;                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// IA-32 Architecture NMI Error Structure Definition                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    SourceId;                                                                             // [CODE_FIRST] 11148
  UINT8     Reserved0[2];                                                                         // [CODE_FIRST] 11148
  UINT32    NumberOfRecordsToPreAllocate;                                                         // [CODE_FIRST] 11148
  UINT32    MaxSectionsPerRecord;                                                                 // [CODE_FIRST] 11148
  UINT32    MaxRawDataLength;                                                                     // [CODE_FIRST] 11148
} EFI_ACPI_6_7_IA32_ARCHITECTURE_NMI_ERROR_STRUCTURE;                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PCI Express Root Port AER Structure Definition                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    SourceId;                                                                             // [CODE_FIRST] 11148
  UINT8     Reserved0[2];                                                                         // [CODE_FIRST] 11148
  UINT8     Flags;                                                                                // [CODE_FIRST] 11148
  UINT8     Enabled;                                                                              // [CODE_FIRST] 11148
  UINT32    NumberOfRecordsToPreAllocate;                                                         // [CODE_FIRST] 11148
  UINT32    MaxSectionsPerRecord;                                                                 // [CODE_FIRST] 11148
  UINT32    Bus;                                                                                  // [CODE_FIRST] 11148
  UINT16    Device;                                                                               // [CODE_FIRST] 11148
  UINT16    Function;                                                                             // [CODE_FIRST] 11148
  UINT16    DeviceControl;                                                                        // [CODE_FIRST] 11148
  UINT8     Reserved1[2];                                                                         // [CODE_FIRST] 11148
  UINT32    UncorrectableErrorMask;                                                               // [CODE_FIRST] 11148
  UINT32    UncorrectableErrorSeverity;                                                           // [CODE_FIRST] 11148
  UINT32    CorrectableErrorMask;                                                                 // [CODE_FIRST] 11148
  UINT32    AdvancedErrorCapabilitiesAndControl;                                                  // [CODE_FIRST] 11148
  UINT32    RootErrorCommand;                                                                     // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCI_EXPRESS_ROOT_PORT_AER_STRUCTURE;                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PCI Express Device AER Structure Definition                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    SourceId;                                                                             // [CODE_FIRST] 11148
  UINT8     Reserved0[2];                                                                         // [CODE_FIRST] 11148
  UINT8     Flags;                                                                                // [CODE_FIRST] 11148
  UINT8     Enabled;                                                                              // [CODE_FIRST] 11148
  UINT32    NumberOfRecordsToPreAllocate;                                                         // [CODE_FIRST] 11148
  UINT32    MaxSectionsPerRecord;                                                                 // [CODE_FIRST] 11148
  UINT32    Bus;                                                                                  // [CODE_FIRST] 11148
  UINT16    Device;                                                                               // [CODE_FIRST] 11148
  UINT16    Function;                                                                             // [CODE_FIRST] 11148
  UINT16    DeviceControl;                                                                        // [CODE_FIRST] 11148
  UINT8     Reserved1[2];                                                                         // [CODE_FIRST] 11148
  UINT32    UncorrectableErrorMask;                                                               // [CODE_FIRST] 11148
  UINT32    UncorrectableErrorSeverity;                                                           // [CODE_FIRST] 11148
  UINT32    CorrectableErrorMask;                                                                 // [CODE_FIRST] 11148
  UINT32    AdvancedErrorCapabilitiesAndControl;                                                  // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCI_EXPRESS_DEVICE_AER_STRUCTURE;                                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PCI Express Bridge AER Structure Definition                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    SourceId;                                                                             // [CODE_FIRST] 11148
  UINT8     Reserved0[2];                                                                         // [CODE_FIRST] 11148
  UINT8     Flags;                                                                                // [CODE_FIRST] 11148
  UINT8     Enabled;                                                                              // [CODE_FIRST] 11148
  UINT32    NumberOfRecordsToPreAllocate;                                                         // [CODE_FIRST] 11148
  UINT32    MaxSectionsPerRecord;                                                                 // [CODE_FIRST] 11148
  UINT32    Bus;                                                                                  // [CODE_FIRST] 11148
  UINT16    Device;                                                                               // [CODE_FIRST] 11148
  UINT16    Function;                                                                             // [CODE_FIRST] 11148
  UINT16    DeviceControl;                                                                        // [CODE_FIRST] 11148
  UINT8     Reserved1[2];                                                                         // [CODE_FIRST] 11148
  UINT32    UncorrectableErrorMask;                                                               // [CODE_FIRST] 11148
  UINT32    UncorrectableErrorSeverity;                                                           // [CODE_FIRST] 11148
  UINT32    CorrectableErrorMask;                                                                 // [CODE_FIRST] 11148
  UINT32    AdvancedErrorCapabilitiesAndControl;                                                  // [CODE_FIRST] 11148
  UINT32    SecondaryUncorrectableErrorMask;                                                      // [CODE_FIRST] 11148
  UINT32    SecondaryUncorrectableErrorSeverity;                                                  // [CODE_FIRST] 11148
  UINT32    SecondaryAdvancedErrorCapabilitiesAndControl;                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCI_EXPRESS_BRIDGE_AER_STRUCTURE;                                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Generic Hardware Error Source Structure Definition                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16                                                Type;                                     // [CODE_FIRST] 11148
  UINT16                                                SourceId;                                 // [CODE_FIRST] 11148
  UINT16                                                RelatedSourceId;                          // [CODE_FIRST] 11148
  UINT8                                                 Flags;                                    // [CODE_FIRST] 11148
  UINT8                                                 Enabled;                                  // [CODE_FIRST] 11148
  UINT32                                                NumberOfRecordsToPreAllocate;             // [CODE_FIRST] 11148
  UINT32                                                MaxSectionsPerRecord;                     // [CODE_FIRST] 11148
  UINT32                                                MaxRawDataLength;                         // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE                ErrorStatusAddress;                       // [CODE_FIRST] 11148
  EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_STRUCTURE    NotificationStructure;                    // [CODE_FIRST] 11148
  UINT32                                                ErrorStatusBlockLength;                   // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE;                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Generic Hardware Error Source Version 2 Structure Definition                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16                                                Type;                                     // [CODE_FIRST] 11148
  UINT16                                                SourceId;                                 // [CODE_FIRST] 11148
  UINT16                                                RelatedSourceId;                          // [CODE_FIRST] 11148
  UINT8                                                 Flags;                                    // [CODE_FIRST] 11148
  UINT8                                                 Enabled;                                  // [CODE_FIRST] 11148
  UINT32                                                NumberOfRecordsToPreAllocate;             // [CODE_FIRST] 11148
  UINT32                                                MaxSectionsPerRecord;                     // [CODE_FIRST] 11148
  UINT32                                                MaxRawDataLength;                         // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE                ErrorStatusAddress;                       // [CODE_FIRST] 11148
  EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_STRUCTURE    NotificationStructure;                    // [CODE_FIRST] 11148
  UINT32                                                ErrorStatusBlockLength;                   // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE                ReadAckRegister;                          // [CODE_FIRST] 11148
  UINT64                                                ReadAckPreserve;                          // [CODE_FIRST] 11148
  UINT64                                                ReadAckWrite;                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GENERIC_HARDWARE_ERROR_SOURCE_VERSION_2_STRUCTURE;                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Generic Error Status Definition                                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_ERROR_BLOCK_STATUS    BlockStatus;                                                 // [CODE_FIRST] 11148
  UINT32                             RawDataOffset;                                               // [CODE_FIRST] 11148
  UINT32                             RawDataLength;                                               // [CODE_FIRST] 11148
  UINT32                             DataLength;                                                  // [CODE_FIRST] 11148
  UINT32                             ErrorSeverity;                                               // [CODE_FIRST] 11148
} EFI_ACPI_6_7_GENERIC_ERROR_STATUS_STRUCTURE;                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// IA-32 Architecture Deferred Machine Check Structure Definition                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16                                                Type;                                     // [CODE_FIRST] 11148
  UINT16                                                SourceId;                                 // [CODE_FIRST] 11148
  UINT8                                                 Reserved0[2];                             // [CODE_FIRST] 11148
  UINT8                                                 Flags;                                    // [CODE_FIRST] 11148
  UINT8                                                 Enabled;                                  // [CODE_FIRST] 11148
  UINT32                                                NumberOfRecordsToPreAllocate;             // [CODE_FIRST] 11148
  UINT32                                                MaxSectionsPerRecord;                     // [CODE_FIRST] 11148
  EFI_ACPI_6_7_HARDWARE_ERROR_NOTIFICATION_STRUCTURE    NotificationStructure;                    // [CODE_FIRST] 11148
  UINT8                                                 NumberOfHardwareBanks;                    // [CODE_FIRST] 11148
  UINT8                                                 Reserved1[3];                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_IA32_ARCHITECTURE_DEFERRED_MACHINE_CHECK_STRUCTURE;                                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// HMAT - Heterogeneous Memory Attribute Table                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT8                          Reserved[4];                                                     // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_HEADER;                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// HMAT Revision (as defined in ACPI 6.7 spec.)                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_REVISION  0x02                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// HMAT types                                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HMAT_TYPE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES          0x00                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HMAT_TYPE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO  0x01                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HMAT_TYPE_MEMORY_SIDE_CACHE_INFO                      0x02                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// HMAT Structure Header                                                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT8     Reserved[2];                                                                          // [CODE_FIRST] 11148
  UINT32    Length;                                                                               // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HMAT_STRUCTURE_HEADER;                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Memory Proximity Domain Attributes Structure flags                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    InitiatorProximityDomainValid : 1;                                                    // [CODE_FIRST] 11148
  UINT16    Reserved                      : 15;                                                   // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES_FLAGS;                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Memory Proximity Domain Attributes Structure                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16                                                                  Type;                     // [CODE_FIRST] 11148
  UINT8                                                                   Reserved[2];              // [CODE_FIRST] 11148
  UINT32                                                                  Length;                   // [CODE_FIRST] 11148
  EFI_ACPI_6_7_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES_FLAGS    Flags;                    // [CODE_FIRST] 11148
  UINT8                                                                   Reserved1[2];             // [CODE_FIRST] 11148
  UINT32                                                                  InitiatorProximityDomain; // [CODE_FIRST] 11148
  UINT32                                                                  MemoryProximityDomain;    // [CODE_FIRST] 11148
  UINT8                                                                   Reserved2[20];            // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HMAT_STRUCTURE_MEMORY_PROXIMITY_DOMAIN_ATTRIBUTES;                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// System Locality Latency and Bandwidth Information Structure flags                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8    MemoryHierarchy  : 4;                                                                  // [CODE_FIRST] 11148
  UINT8    AccessAttributes : 2;                                                                  // [CODE_FIRST] 11148
  UINT8    Reserved         : 2;                                                                  // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO_FLAGS;                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// System Locality Latency and Bandwidth Information Structure                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16                                                                          Type;                              // [CODE_FIRST] 11148
  UINT8                                                                           Reserved[2];                       // [CODE_FIRST] 11148
  UINT32                                                                          Length;                            // [CODE_FIRST] 11148
  EFI_ACPI_6_7_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO_FLAGS    Flags;                             // [CODE_FIRST] 11148
  UINT8                                                                           DataType;                          // [CODE_FIRST] 11148
  UINT8                                                                           MinTransferSize;                   // [CODE_FIRST] 11148
  UINT8                                                                           Reserved1;                         // [CODE_FIRST] 11148
  UINT32                                                                          NumberOfInitiatorProximityDomains; // [CODE_FIRST] 11148
  UINT32                                                                          NumberOfTargetProximityDomains;    // [CODE_FIRST] 11148
  UINT8                                                                           Reserved2[4];                      // [CODE_FIRST] 11148
  UINT64                                                                          EntryBaseUnit;                     // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HMAT_STRUCTURE_SYSTEM_LOCALITY_LATENCY_AND_BANDWIDTH_INFO;                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Memory Side Cache Information Structure cache attributes                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    TotalCacheLevels   : 4;                                                               // [CODE_FIRST] 11148
  UINT32    CacheLevel         : 4;                                                               // [CODE_FIRST] 11148
  UINT32    CacheAssociativity : 4;                                                               // [CODE_FIRST] 11148
  UINT32    WritePolicy        : 4;                                                               // [CODE_FIRST] 11148
  UINT32    CacheLineSize      : 16;                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO_CACHE_ATTRIBUTES;                            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Memory Side Cache Information Structure                                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16                                                                 Type;                    // [CODE_FIRST] 11148
  UINT8                                                                  Reserved[2];             // [CODE_FIRST] 11148
  UINT32                                                                 Length;                  // [CODE_FIRST] 11148
  UINT32                                                                 MemoryProximityDomain;   // [CODE_FIRST] 11148
  UINT8                                                                  Reserved1[4];            // [CODE_FIRST] 11148
  UINT64                                                                 MemorySideCacheSize;     // [CODE_FIRST] 11148
  EFI_ACPI_6_7_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO_CACHE_ATTRIBUTES    CacheAttributes;         // [CODE_FIRST] 11148
  UINT8                                                                  Reserved2[2];            // [CODE_FIRST] 11148
  UINT16                                                                 NumberOfSmbiosHandles;   // [CODE_FIRST] 11148
} EFI_ACPI_6_7_HMAT_STRUCTURE_MEMORY_SIDE_CACHE_INFO;                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ERST - Error Record Serialization Table                                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT32                         SerializationHeaderSize;                                         // [CODE_FIRST] 11148
  UINT8                          Reserved0[4];                                                    // [CODE_FIRST] 11148
  UINT32                         InstructionEntryCount;                                           // [CODE_FIRST] 11148
} EFI_ACPI_6_7_ERROR_RECORD_SERIALIZATION_TABLE_HEADER;                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ERST Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_RECORD_SERIALIZATION_TABLE_REVISION  0x01                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ERST Serialization Actions                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_BEGIN_WRITE_OPERATION                   0x00                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_BEGIN_READ_OPERATION                    0x01                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_BEGIN_CLEAR_OPERATION                   0x02                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_END_OPERATION                           0x03                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_SET_RECORD_OFFSET                       0x04                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_EXECUTE_OPERATION                       0x05                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_CHECK_BUSY_STATUS                       0x06                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_GET_COMMAND_STATUS                      0x07                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_GET_RECORD_IDENTIFIER                   0x08                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_SET_RECORD_IDENTIFIER                   0x09                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_GET_RECORD_COUNT                        0x0A                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_BEGIN_DUMMY_WRITE_OPERATION             0x0B                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_GET_ERROR_LOG_ADDRESS_RANGE             0x0D                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_GET_ERROR_LOG_ADDRESS_RANGE_LENGTH      0x0E                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_GET_ERROR_LOG_ADDRESS_RANGE_ATTRIBUTES  0x0F                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_GET_EXECUTE_OPERATION_TIMINGS           0x10                            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ERST Action Command Status                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_STATUS_SUCCESS                 0x00                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_STATUS_NOT_ENOUGH_SPACE        0x01                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_STATUS_HARDWARE_NOT_AVAILABLE  0x02                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_STATUS_FAILED                  0x03                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_STATUS_RECORD_STORE_EMPTY      0x04                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_STATUS_RECORD_NOT_FOUND        0x05                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ERST Serialization Instructions                                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_READ_REGISTER                  0x00                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_READ_REGISTER_VALUE            0x01                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_WRITE_REGISTER                 0x02                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_WRITE_REGISTER_VALUE           0x03                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_NOOP                           0x04                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_LOAD_VAR1                      0x05                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_LOAD_VAR2                      0x06                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_STORE_VAR1                     0x07                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_ADD                            0x08                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_SUBTRACT                       0x09                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_ADD_VALUE                      0x0A                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_SUBTRACT_VALUE                 0x0B                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_STALL                          0x0C                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_STALL_WHILE_TRUE               0x0D                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_SKIP_NEXT_INSTRUCTION_IF_TRUE  0x0E                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_GOTO                           0x0F                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_SET_SRC_ADDRESS_BASE           0x10                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_SET_DST_ADDRESS_BASE           0x11                                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_MOVE_DATA                      0x12                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ERST Instruction Flags                                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERST_PRESERVE_REGISTER  0x01                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// ERST Serialization Instruction Entry                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8                                     SerializationAction;                                  // [CODE_FIRST] 11148
  UINT8                                     Instruction;                                          // [CODE_FIRST] 11148
  UINT8                                     Flags;                                                // [CODE_FIRST] 11148
  UINT8                                     Reserved0;                                            // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    RegisterRegion;                                       // [CODE_FIRST] 11148
  UINT64                                    Value;                                                // [CODE_FIRST] 11148
  UINT64                                    Mask;                                                 // [CODE_FIRST] 11148
} EFI_ACPI_6_7_ERST_SERIALIZATION_INSTRUCTION_ENTRY;                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// EINJ - Error Injection Table                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT32                         InjectionHeaderSize;                                             // [CODE_FIRST] 11148
  UINT8                          InjectionFlags;                                                  // [CODE_FIRST] 11148
  UINT8                          Reserved0[3];                                                    // [CODE_FIRST] 11148
  UINT32                         InjectionEntryCount;                                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_ERROR_INJECTION_TABLE_HEADER;                                                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// EINJ Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_INJECTION_TABLE_REVISION  0x02                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// EINJ Error Injection Actions                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_BEGIN_INJECTION_OPERATION       0x00                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_GET_TRIGGER_ERROR_ACTION_TABLE  0x01                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_SET_ERROR_TYPE                  0x02                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_GET_ERROR_TYPE                  0x03                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_END_OPERATION                   0x04                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_EXECUTE_OPERATION               0x05                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_CHECK_BUSY_STATUS               0x06                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_GET_COMMAND_STATUS              0x07                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_SET_ERROR_TYPE_WITH_ADDRESS     0x08                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_GET_EXECUTE_OPERATION_TIMINGS   0x09                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_EINJV2_SET_ERROR_TYPE           0x10                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_EINJV2_GET_ERROR_TYPE           0x11                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_TRIGGER_ERROR                   0xFF                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// EINJ Action Command Status                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_STATUS_SUCCESS          0x00                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_STATUS_UNKNOWN_FAILURE  0x01                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_STATUS_INVALID_ACCESS   0x02                                            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// EINJ Error Type Definition                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_PROCESSOR_CORRECTABLE               (1 << 0)                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_PROCESSOR_UNCORRECTABLE_NONFATAL    (1 << 1)                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_PROCESSOR_UNCORRECTABLE_FATAL       (1 << 2)                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_MEMORY_CORRECTABLE                  (1 << 3)                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_MEMORY_UNCORRECTABLE_NONFATAL       (1 << 4)                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_MEMORY_UNCORRECTABLE_FATAL          (1 << 5)                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_PCI_EXPRESS_CORRECTABLE             (1 << 6)                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_PCI_EXPRESS_UNCORRECTABLE_NONFATAL  (1 << 7)                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_PCI_EXPRESS_UNCORRECTABLE_FATAL     (1 << 8)                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_PLATFORM_CORRECTABLE                (1 << 9)                      // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_PLATFORM_UNCORRECTABLE_NONFATAL     (1 << 10)                     // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_ERROR_PLATFORM_UNCORRECTABLE_FATAL        (1 << 11)                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// EINJ Injection Instructions                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_READ_REGISTER         0x00                                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_READ_REGISTER_VALUE   0x01                                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_WRITE_REGISTER        0x02                                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_WRITE_REGISTER_VALUE  0x03                                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_NOOP                  0x04                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// EINJ Instruction Flags                                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EINJ_PRESERVE_REGISTER  0x01                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// EINJ Injection Instruction Entry                                                              // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8                                     InjectionAction;                                      // [CODE_FIRST] 11148
  UINT8                                     Instruction;                                          // [CODE_FIRST] 11148
  UINT8                                     Flags;                                                // [CODE_FIRST] 11148
  UINT8                                     Reserved0;                                            // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    RegisterRegion;                                       // [CODE_FIRST] 11148
  UINT64                                    Value;                                                // [CODE_FIRST] 11148
  UINT64                                    Mask;                                                 // [CODE_FIRST] 11148
} EFI_ACPI_6_7_EINJ_INJECTION_INSTRUCTION_ENTRY;                                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// EINJ Trigger Action Table                                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    HeaderSize;                                                                           // [CODE_FIRST] 11148
  UINT32    Revision;                                                                             // [CODE_FIRST] 11148
  UINT32    TableSize;                                                                            // [CODE_FIRST] 11148
  UINT32    EntryCount;                                                                           // [CODE_FIRST] 11148
} EFI_ACPI_6_7_EINJ_TRIGGER_ACTION_TABLE;                                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Platform Communications Channel Table (PCCT)                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT32                         Flags;                                                           // [CODE_FIRST] 11148
  UINT64                         Reserved;                                                        // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER;                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PCCT Version (as defined in ACPI 6.7 spec.)                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLATFORM_COMMUNICATION_CHANNEL_TABLE_REVISION  0x02                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PCCT Global Flags                                                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCCT_FLAGS_PLATFORM_INTERRUPT  BIT0                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// PCCT Subspace type                                                                             // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCCT_SUBSPACE_TYPE_GENERIC                        0x00                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCCT_SUBSPACE_TYPE_1_HW_REDUCED_COMMUNICATIONS    0x01                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCCT_SUBSPACE_TYPE_2_HW_REDUCED_COMMUNICATIONS    0x02                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCCT_SUBSPACE_TYPE_3_EXTENDED_PCC                 0x03                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCCT_SUBSPACE_TYPE_4_EXTENDED_PCC                 0x04                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCCT_SUBSPACE_TYPE_5_HW_REGISTERS_COMMUNICATIONS  0x05                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PCC Subspace Structure Header                                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8    Type;                                                                                  // [CODE_FIRST] 11148
  UINT8    Length;                                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCCT_SUBSPACE_HEADER;                                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Generic Communications Subspace Structure                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8                                     Type;                                                 // [CODE_FIRST] 11148
  UINT8                                     Length;                                               // [CODE_FIRST] 11148
  UINT8                                     Reserved[6];                                          // [CODE_FIRST] 11148
  UINT64                                    BaseAddress;                                          // [CODE_FIRST] 11148
  UINT64                                    AddressLength;                                        // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    DoorbellRegister;                                     // [CODE_FIRST] 11148
  UINT64                                    DoorbellPreserve;                                     // [CODE_FIRST] 11148
  UINT64                                    DoorbellWrite;                                        // [CODE_FIRST] 11148
  UINT32                                    NominalLatency;                                       // [CODE_FIRST] 11148
  UINT32                                    MaximumPeriodicAccessRate;                            // [CODE_FIRST] 11148
  UINT16                                    MinimumRequestTurnaroundTime;                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCCT_SUBSPACE_GENERIC;                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Generic Communications Channel Shared Memory Region                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8    Command;                                                                               // [CODE_FIRST] 11148
  UINT8    Reserved           : 7;                                                                // [CODE_FIRST] 11148
  UINT8    NotifyOnCompletion : 1;                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCCT_GENERIC_SHARED_MEMORY_REGION_COMMAND;                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8    CommandComplete      : 1;                                                              // [CODE_FIRST] 11148
  UINT8    PlatformInterrupt    : 1;                                                              // [CODE_FIRST] 11148
  UINT8    Error                : 1;                                                              // [CODE_FIRST] 11148
  UINT8    PlatformNotification : 1;                                                              // [CODE_FIRST] 11148
  UINT8    Reserved             : 4;                                                              // [CODE_FIRST] 11148
  UINT8    Reserved1;                                                                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCCT_GENERIC_SHARED_MEMORY_REGION_STATUS;                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32                                                    Signature;                            // [CODE_FIRST] 11148
  EFI_ACPI_6_7_PCCT_GENERIC_SHARED_MEMORY_REGION_COMMAND    Command;                              // [CODE_FIRST] 11148
  EFI_ACPI_6_7_PCCT_GENERIC_SHARED_MEMORY_REGION_STATUS     Status;                               // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCCT_GENERIC_SHARED_MEMORY_REGION_HEADER;                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCCT_SUBSPACE_PLATFORM_INTERRUPT_FLAGS_POLARITY  BIT0                        // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCCT_SUBSPACE_PLATFORM_INTERRUPT_FLAGS_MODE      BIT1                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Type 1 HW-Reduced Communications Subspace Structure                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8                                     Type;                                                 // [CODE_FIRST] 11148
  UINT8                                     Length;                                               // [CODE_FIRST] 11148
  UINT32                                    PlatformInterrupt;                                    // [CODE_FIRST] 11148
  UINT8                                     PlatformInterruptFlags;                               // [CODE_FIRST] 11148
  UINT8                                     Reserved;                                             // [CODE_FIRST] 11148
  UINT64                                    BaseAddress;                                          // [CODE_FIRST] 11148
  UINT64                                    AddressLength;                                        // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    DoorbellRegister;                                     // [CODE_FIRST] 11148
  UINT64                                    DoorbellPreserve;                                     // [CODE_FIRST] 11148
  UINT64                                    DoorbellWrite;                                        // [CODE_FIRST] 11148
  UINT32                                    NominalLatency;                                       // [CODE_FIRST] 11148
  UINT32                                    MaximumPeriodicAccessRate;                            // [CODE_FIRST] 11148
  UINT16                                    MinimumRequestTurnaroundTime;                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCCT_SUBSPACE_1_HW_REDUCED_COMMUNICATIONS;                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Type 2 HW-Reduced Communications Subspace Structure                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8                                     Type;                                                 // [CODE_FIRST] 11148
  UINT8                                     Length;                                               // [CODE_FIRST] 11148
  UINT32                                    PlatformInterrupt;                                    // [CODE_FIRST] 11148
  UINT8                                     PlatformInterruptFlags;                               // [CODE_FIRST] 11148
  UINT8                                     Reserved;                                             // [CODE_FIRST] 11148
  UINT64                                    BaseAddress;                                          // [CODE_FIRST] 11148
  UINT64                                    AddressLength;                                        // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    DoorbellRegister;                                     // [CODE_FIRST] 11148
  UINT64                                    DoorbellPreserve;                                     // [CODE_FIRST] 11148
  UINT64                                    DoorbellWrite;                                        // [CODE_FIRST] 11148
  UINT32                                    NominalLatency;                                       // [CODE_FIRST] 11148
  UINT32                                    MaximumPeriodicAccessRate;                            // [CODE_FIRST] 11148
  UINT16                                    MinimumRequestTurnaroundTime;                         // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    PlatformInterruptAckRegister;                         // [CODE_FIRST] 11148
  UINT64                                    PlatformInterruptAckPreserve;                         // [CODE_FIRST] 11148
  UINT64                                    PlatformInterruptAckWrite;                            // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS;                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Type 3 Extended PCC Subspace Structure                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8                                     Type;                                                 // [CODE_FIRST] 11148
  UINT8                                     Length;                                               // [CODE_FIRST] 11148
  UINT32                                    PlatformInterrupt;                                    // [CODE_FIRST] 11148
  UINT8                                     PlatformInterruptFlags;                               // [CODE_FIRST] 11148
  UINT8                                     Reserved;                                             // [CODE_FIRST] 11148
  UINT64                                    BaseAddress;                                          // [CODE_FIRST] 11148
  UINT32                                    AddressLength;                                        // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    DoorbellRegister;                                     // [CODE_FIRST] 11148
  UINT64                                    DoorbellPreserve;                                     // [CODE_FIRST] 11148
  UINT64                                    DoorbellWrite;                                        // [CODE_FIRST] 11148
  UINT32                                    NominalLatency;                                       // [CODE_FIRST] 11148
  UINT32                                    MaximumPeriodicAccessRate;                            // [CODE_FIRST] 11148
  UINT32                                    MinimumRequestTurnaroundTime;                         // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    PlatformInterruptAckRegister;                         // [CODE_FIRST] 11148
  UINT64                                    PlatformInterruptAckPreserve;                         // [CODE_FIRST] 11148
  UINT64                                    PlatformInterruptAckSet;                              // [CODE_FIRST] 11148
  UINT8                                     Reserved1[8];                                         // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    CommandCompleteCheckRegister;                         // [CODE_FIRST] 11148
  UINT64                                    CommandCompleteCheckMask;                             // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    CommandCompleteUpdateRegister;                        // [CODE_FIRST] 11148
  UINT64                                    CommandCompleteUpdatePreserve;                        // [CODE_FIRST] 11148
  UINT64                                    CommandCompleteUpdateSet;                             // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    ErrorStatusRegister;                                  // [CODE_FIRST] 11148
  UINT64                                    ErrorStatusMask;                                      // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCCT_SUBSPACE_3_EXTENDED_PCC;                                                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Type 4 Extended PCC Subspace Structure                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef EFI_ACPI_6_7_PCCT_SUBSPACE_3_EXTENDED_PCC EFI_ACPI_6_7_PCCT_SUBSPACE_4_EXTENDED_PCC;      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCCT_MASTER_SLAVE_COMMUNICATIONS_CHANNEL_FLAGS_NOTIFY_ON_COMPLETION  BIT0    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    Signature;                                                                            // [CODE_FIRST] 11148
  UINT32    Flags;                                                                                // [CODE_FIRST] 11148
  UINT32    Length;                                                                               // [CODE_FIRST] 11148
  UINT32    Command;                                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCCT_EXTENDED_PCC_SHARED_MEMORY_REGION_HEADER;                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Type 5 HW Registers based Communications Subspace Structure                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8                                     Type;                                                 // [CODE_FIRST] 11148
  UINT8                                     Length;                                               // [CODE_FIRST] 11148
  UINT16                                    Version;                                              // [CODE_FIRST] 11148
  UINT64                                    BaseAddress;                                          // [CODE_FIRST] 11148
  UINT64                                    SharedMemoryRangeLength;                              // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    DoorbellRegister;                                     // [CODE_FIRST] 11148
  UINT64                                    DoorbellPreserve;                                     // [CODE_FIRST] 11148
  UINT64                                    DoorbellWrite;                                        // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    CommandCompleteCheckRegister;                         // [CODE_FIRST] 11148
  UINT64                                    CommandCompleteCheckMask;                             // [CODE_FIRST] 11148
  EFI_ACPI_6_7_GENERIC_ADDRESS_STRUCTURE    ErrorStatusRegister;                                  // [CODE_FIRST] 11148
  UINT64                                    ErrorStatusMask;                                      // [CODE_FIRST] 11148
  UINT32                                    NominalLatency;                                       // [CODE_FIRST] 11148
  UINT32                                    MinimumRequestTurnaroundTime;                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PCCT_SUBSPACE_5_HW_REGISTERS_COMMUNICATIONS;                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Reduced PCC Subspace Shared Memory Region                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    Signature;                                                                            // [CODE_FIRST] 11148
  // UINT8       CommunicationSubspace[];                                                         // [CODE_FIRST] 11148
} EFI_6_7_PCCT_REDUCED_PCC_SUBSPACE_SHARED_MEMORY_REGION;                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Platform Debug Trigger Table (PDTT)                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT8                          TriggerCount;                                                    // [CODE_FIRST] 11148
  UINT8                          Reserved[3];                                                     // [CODE_FIRST] 11148
  UINT32                         TriggerIdentifierArrayOffset;                                    // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PLATFORM_DEBUG_TRIGGER_TABLE_HEADER;                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PDTT Revision (as defined in ACPI 6.7 spec.)                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLATFORM_DEBUG_TRIGGER_TABLE_REVISION  0x00                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PDTT Platform Communication Channel Identifier Structure                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    SubChannelIdentifer : 8;                                                              // [CODE_FIRST] 11148
  UINT16    Runtime             : 1;                                                              // [CODE_FIRST] 11148
  UINT16    WaitForCompletion   : 1;                                                              // [CODE_FIRST] 11148
  UINT16    TriggerOrder        : 1;                                                              // [CODE_FIRST] 11148
  UINT16    Reserved            : 5;                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PDTT_PCC_IDENTIFIER;                                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PCC Commands Codes used by Platform Debug Trigger Table                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PDTT_PCC_COMMAND_DOORBELL_ONLY    0x00                                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PDTT_PCC_COMMAND_VENDOR_SPECIFIC  0x01                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PDTT Platform Communication Channel                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef EFI_ACPI_6_7_PCCT_GENERIC_SHARED_MEMORY_REGION_HEADER EFI_ACPI_6_7_PDTT_PCC;              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Processor Properties Topology Table (PPTT)                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER;                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PPTT Revision (as defined in ACPI 6.7 spec.)                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION  0x03                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PPTT types                                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_TYPE_PROCESSOR  0x00                                                    // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_TYPE_CACHE      0x01                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PPTT Structure Header                                                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8    Type;                                                                                  // [CODE_FIRST] 11148
  UINT8    Length;                                                                                // [CODE_FIRST] 11148
  UINT8    Reserved[2];                                                                           // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PPTT_STRUCTURE_HEADER;                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// For PPTT struct processor flags                                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_PACKAGE_NOT_PHYSICAL          0x0                                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_PACKAGE_PHYSICAL              0x1                                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_PROCESSOR_ID_INVALID          0x0                                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_PROCESSOR_ID_VALID            0x1                                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_PROCESSOR_IS_NOT_THREAD       0x0                                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_PROCESSOR_IS_THREAD           0x1                                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_NODE_IS_NOT_LEAF              0x0                                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_NODE_IS_LEAF                  0x1                                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_IMPLEMENTATION_NOT_IDENTICAL  0x0                                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_IMPLEMENTATION_IDENTICAL      0x1                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Processor hierarchy node structure flags                                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    PhysicalPackage         : 1;                                                          // [CODE_FIRST] 11148
  UINT32    AcpiProcessorIdValid    : 1;                                                          // [CODE_FIRST] 11148
  UINT32    ProcessorIsAThread      : 1;                                                          // [CODE_FIRST] 11148
  UINT32    NodeIsALeaf             : 1;                                                          // [CODE_FIRST] 11148
  UINT32    IdenticalImplementation : 1;                                                          // [CODE_FIRST] 11148
  UINT32    Reserved                : 27;                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PPTT_STRUCTURE_PROCESSOR_FLAGS;                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Processor hierarchy node structure                                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8                                          Type;                                            // [CODE_FIRST] 11148
  UINT8                                          Length;                                          // [CODE_FIRST] 11148
  UINT8                                          Reserved[2];                                     // [CODE_FIRST] 11148
  EFI_ACPI_6_7_PPTT_STRUCTURE_PROCESSOR_FLAGS    Flags;                                           // [CODE_FIRST] 11148
  UINT32                                         Parent;                                          // [CODE_FIRST] 11148
  UINT32                                         AcpiProcessorId;                                 // [CODE_FIRST] 11148
  UINT32                                         NumberOfPrivateResources;                        // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PPTT_STRUCTURE_PROCESSOR;                                                          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// For PPTT struct cache flags                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_CACHE_SIZE_INVALID       0x0                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_CACHE_SIZE_VALID         0x1                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_NUMBER_OF_SETS_INVALID   0x0                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_NUMBER_OF_SETS_VALID     0x1                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_ASSOCIATIVITY_INVALID    0x0                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_ASSOCIATIVITY_VALID      0x1                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_ALLOCATION_TYPE_INVALID  0x0                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_ALLOCATION_TYPE_VALID    0x1                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_CACHE_TYPE_INVALID       0x0                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_CACHE_TYPE_VALID         0x1                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_WRITE_POLICY_INVALID     0x0                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_WRITE_POLICY_VALID       0x1                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_LINE_SIZE_INVALID        0x0                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_LINE_SIZE_VALID          0x1                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_CACHE_ID_INVALID         0x0                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PPTT_CACHE_ID_VALID           0x1                                            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Cache Type Structure flags                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT32    SizePropertyValid   : 1;                                                              // [CODE_FIRST] 11148
  UINT32    NumberOfSetsValid   : 1;                                                              // [CODE_FIRST] 11148
  UINT32    AssociativityValid  : 1;                                                              // [CODE_FIRST] 11148
  UINT32    AllocationTypeValid : 1;                                                              // [CODE_FIRST] 11148
  UINT32    CacheTypeValid      : 1;                                                              // [CODE_FIRST] 11148
  UINT32    WritePolicyValid    : 1;                                                              // [CODE_FIRST] 11148
  UINT32    LineSizeValid       : 1;                                                              // [CODE_FIRST] 11148
  UINT32    CacheIdValid        : 1;                                                              // [CODE_FIRST] 11148
  UINT32    Reserved            : 24;                                                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PPTT_STRUCTURE_CACHE_FLAGS;                                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// For cache attributes                                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CACHE_ATTRIBUTES_ALLOCATION_READ             0x0                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CACHE_ATTRIBUTES_ALLOCATION_WRITE            0x1                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE       0x2                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CACHE_ATTRIBUTES_CACHE_TYPE_DATA             0x0                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION      0x1                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED          0x2                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK     0x0                             // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_THROUGH  0x1                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Cache Type Structure cache attributes                                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8    AllocationType : 2;                                                                    // [CODE_FIRST] 11148
  UINT8    CacheType      : 2;                                                                    // [CODE_FIRST] 11148
  UINT8    WritePolicy    : 1;                                                                    // [CODE_FIRST] 11148
  UINT8    Reserved       : 3;                                                                    // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PPTT_STRUCTURE_CACHE_ATTRIBUTES;                                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Cache Type Structure                                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8                                           Type;                                           // [CODE_FIRST] 11148
  UINT8                                           Length;                                         // [CODE_FIRST] 11148
  UINT8                                           Reserved[2];                                    // [CODE_FIRST] 11148
  EFI_ACPI_6_7_PPTT_STRUCTURE_CACHE_FLAGS         Flags;                                          // [CODE_FIRST] 11148
  UINT32                                          NextLevelOfCache;                               // [CODE_FIRST] 11148
  UINT32                                          Size;                                           // [CODE_FIRST] 11148
  UINT32                                          NumberOfSets;                                   // [CODE_FIRST] 11148
  UINT8                                           Associativity;                                  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_PPTT_STRUCTURE_CACHE_ATTRIBUTES    Attributes;                                     // [CODE_FIRST] 11148
  UINT16                                          LineSize;                                       // [CODE_FIRST] 11148
  UINT32                                          CacheId;                                        // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PPTT_STRUCTURE_CACHE;                                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Platform Health Assessment Table (PHAT) Format                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  // UINT8                         PlatformTelemetryRecords[];                                    // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PLATFORM_HEALTH_ASSESSMENT_TABLE;                                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLATFORM_HEALTH_ASSESSMENT_TABLE_REVISION  0x01                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PHAT Record Format                                                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    PlatformHealthAssessmentRecordType;                                                   // [CODE_FIRST] 11148
  UINT16    RecordLength;                                                                         // [CODE_FIRST] 11148
  UINT8     Revision;                                                                             // [CODE_FIRST] 11148
  // UINT8   Data[];                                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PHAT_RECORD;                                                                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PHAT Record Type Format                                                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RECORD_TYPE_FIRMWARE_VERSION_DATA_RECORD  0x0000                        // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RECORD_TYPE_FIRMWARE_HEALTH_DATA_RECORD   0x0001                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PHAT Version Element                                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  GUID      ComponentId;                                                                          // [CODE_FIRST] 11148
  UINT64    VersionValue;                                                                         // [CODE_FIRST] 11148
  UINT32    ProducerId;                                                                           // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PHAT_VERSION_ELEMENT;                                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// PHAT Firmware Version Data Record                                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    PlatformRecordType;                                                                   // [CODE_FIRST] 11148
  UINT16    RecordLength;                                                                         // [CODE_FIRST] 11148
  UINT8     Revision;                                                                             // [CODE_FIRST] 11148
  UINT8     Reserved[3];                                                                          // [CODE_FIRST] 11148
  UINT32    RecordCount;                                                                          // [CODE_FIRST] 11148
  // UINT8   PhatVersionElement[];                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PHAT_FIRMWARE_VERISON_DATA_RECORD;                                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_FIRMWARE_VERSION_DATA_RECORD_REVISION  0x01                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Firmware Health Data Record Structure                                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    PlatformRecordType;                                                                   // [CODE_FIRST] 11148
  UINT16    RecordLength;                                                                         // [CODE_FIRST] 11148
  UINT8     Revision;                                                                             // [CODE_FIRST] 11148
  UINT16    Reserved;                                                                             // [CODE_FIRST] 11148
  UINT8     AmHealthy;                                                                            // [CODE_FIRST] 11148
  GUID      DeviceSignature;                                                                      // [CODE_FIRST] 11148
  UINT32    DeviceSpecificDataOffset;                                                             // [CODE_FIRST] 11148
  // UINT8   DevicePath[];                                                                        // [CODE_FIRST] 11148
  // UINT8   DeviceSpecificData[];                                                                // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PHAT_FIRMWARE_HEALTH_DATA_RECORD_STRUCTURE;                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_FIRMWARE_HEALTH_DATA_RECORD_REVISION  0x01                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Firmware Health Data Record device health state                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_FIRMWARE_HEALTH_DATA_RECORD_ERRORS_FOUND     0x00                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_FIRMWARE_HEALTH_DATA_RECORD_NO_ERRORS_FOUND  0x01                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_FIRMWARE_HEALTH_DATA_RECORD_UNKNOWN          0x02                       // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_FIRMWARE_HEALTH_DATA_RECORD_ADVISORY         0x03                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Reset Reason Health Record Vendor Data Entry                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  GUID      VendorDataID;                                                                         // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Revision;                                                                             // [CODE_FIRST] 11148
  // UINTN   Data[];                                                                              // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PHAT_RESET_REASON_HEALTH_RECORD_VENDOR_DATA_ENTRY;                                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// Reset Reason Health Record Structure                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT8     SupportedSources;                                                                     // [CODE_FIRST] 11148
  UINT8     Source;                                                                               // [CODE_FIRST] 11148
  UINT8     SubSource;                                                                            // [CODE_FIRST] 11148
  UINT8     Reason;                                                                               // [CODE_FIRST] 11148
  UINT16    VendorCount;                                                                          // [CODE_FIRST] 11148
  // EFI_ACPI_6_7_PHAT_RESET_REASON_HEALTH_RECORD_VENDOR_DATA_ENTRY   VendorSpecificResetReasonEntry[];  // [CODE_FIRST] 11148
} EFI_ACPI_6_7_PHAT_RESET_REASON_HEALTH_RECORD_STRUCTURE;                                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_HEADER_GUID  { 0x7a014ce2, 0xf263, 0x4b77, { 0xb8, 0x8a, 0xe6, 0x33, 0x6b, 0x78, 0x2c, 0x14 }}  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_SUPPORTED_SOURCES_UNKNOWN     BIT0                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_SUPPORTED_SOURCES_HARDWARE    BIT1                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_SUPPORTED_SOURCES_FIRMWARE    BIT2                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_SUPPORTED_SOURCES_SOFTWARE    BIT3                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_SUPPORTED_SOURCES_SUPERVISOR  BIT4                         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_SOURCES_UNKNOWN     BIT0                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_SOURCES_HARDWARE    BIT1                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_SOURCES_FIRMWARE    BIT2                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_SOURCES_SOFTWARE    BIT3                                   // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_SOURCES_SUPERVISOR  BIT4                                   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_REASON_UNKNOWN           0x00                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_REASON_COLD_BOOT         0x01                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_REASON_COLD_RESET        0x02                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_REASON_WARM_RESET        0x03                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_REASON_UPDATE            0x04                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_REASON_UNEXPECTED_RESET  0x20                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_REASON_FAULT             0x21                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_REASON_TIMEOUT           0x22                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_REASON_THERMAL           0x23                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_REASON_POWER_LOSS        0x24                              // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PHAT_RESET_REASON_REASON_POWER_BUTTON      0x25                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_DESCRIPTION_HEADER    Header;                                                          // [CODE_FIRST] 11148
  UINT32                         Flags;                                                           // [CODE_FIRST] 11148
  UINT64                         TimeBaseFreq;                                                    // [CODE_FIRST] 11148
  UINT32                         NodeCount;                                                       // [CODE_FIRST] 11148
  UINT32                         NodeOffset;                                                      // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RISCV_HART_CAPABILITIES_TABLE;                                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_TABLE_REVISION  1                                                       // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// RHCT Flags                                                                                     // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_FLAG_TIMER_CANNOT_WAKEUP_CPU  BIT0                                      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// RHCT subtables                                                                                 // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  UINT16    Type;                                                                                 // [CODE_FIRST] 11148
  UINT16    Length;                                                                               // [CODE_FIRST] 11148
  UINT16    Revision;                                                                             // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RHCT_NODE_HEADER;                                                                  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
/* Values for RHCT subtable Type above */                                                         // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_NODE_TYPE_ISA_STRING  0x0000                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_NODE_TYPE_CMO         0x0001                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_NODE_TYPE_MMU         0x0002                                            // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_NODE_TYPE_HART_INFO   0xFFFF                                            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// ISA string node structure                                                                      // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_RHCT_NODE_HEADER    Node;                                                          // [CODE_FIRST] 11148
  UINT16                           IsaLength;                                                     // [CODE_FIRST] 11148
  char                             Isa[];                                                         // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RHCT_ISA_STRING_NODE;                                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_ISA_NODE_STRUCTURE_VERSION  1                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// CMO node structure                                                                             // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_RHCT_NODE_HEADER    Node;                                                          // [CODE_FIRST] 11148
  UINT8                            Reserved;                                                      // [CODE_FIRST] 11148
  UINT8                            CbomBlockSize;                                                 // [CODE_FIRST] 11148
  UINT8                            CbopBlockSize;                                                 // [CODE_FIRST] 11148
  UINT8                            CbozBlockSize;                                                 // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RHCT_CMO_NODE;                                                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_CMO_NODE_STRUCTURE_VERSION  1                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// MMU node structure                                                                             // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_RHCT_NODE_HEADER    Node;                                                          // [CODE_FIRST] 11148
  UINT8                            Reserved;                                                      // [CODE_FIRST] 11148
  UINT8                            MmuType;                                                       // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RHCT_MMU_NODE;                                                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_MMU_NODE_STRUCTURE_VERSION  1                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_MMU_TYPE_SV39  0                                                        // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_MMU_TYPE_SV48  1                                                        // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_MMU_TYPE_SV57  2                                                        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Hart Info node structure                                                                       // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
typedef struct {
  // [CODE_FIRST] 11148
  EFI_ACPI_6_7_RHCT_NODE_HEADER    Node;                                                          // [CODE_FIRST] 11148
  UINT16                           NumOffsets;                                                    // [CODE_FIRST] 11148
  UINT32                           Uid;                                                           // [CODE_FIRST] 11148
  UINT32                           Offsets[];                                                     // [CODE_FIRST] 11148
} EFI_ACPI_6_7_RHCT_HART_INFO_NODE;                                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RHCT_HART_INFO_NODE_STRUCTURE_VERSION  1                                     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// Known table signatures                                                                         // [CODE_FIRST] 11148
//                                                                                                // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "RSD PTR " Root System Description Pointer                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE  SIGNATURE_64('R', 'S', 'D', ' ', 'P', 'T', 'R', ' ')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "APIC" Multiple APIC Description Table                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE  SIGNATURE_32('A', 'P', 'I', 'C')  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "APMT" Arm Performance Monitoring Unit Table                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ARM_PERFORMANCE_MONITORING_UNIT_TABLE_SIGNATURE  SIGNATURE_32('A', 'P', 'M', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "BERT" Boot Error Record Table                                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_BOOT_ERROR_RECORD_TABLE_SIGNATURE  SIGNATURE_32('B', 'E', 'R', 'T')          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "BGRT" Boot Graphics Resource Table                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_BOOT_GRAPHICS_RESOURCE_TABLE_SIGNATURE  SIGNATURE_32('B', 'G', 'R', 'T')     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "CDIT" Component Distance Information Table                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_COMPONENT_DISTANCE_INFORMATION_TABLE_SIGNATURE  SIGNATURE_32('C', 'D', 'I', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "CPEP" Corrected Platform Error Polling Table                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CORRECTED_PLATFORM_ERROR_POLLING_TABLE_SIGNATURE  SIGNATURE_32('C', 'P', 'E', 'P')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "CRAT" Component Resource Attribute Table                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_COMPONENT_RESOURCE_ATTRIBUTE_TABLE_SIGNATURE  SIGNATURE_32('C', 'R', 'A', 'T')  // [CODE_FIRST] 11148
                                                                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "DSDT" Differentiated System Description Table                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE  SIGNATURE_32('D', 'S', 'D', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "ECDT" Embedded Controller Boot Resources Table                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EMBEDDED_CONTROLLER_BOOT_RESOURCES_TABLE_SIGNATURE  SIGNATURE_32('E', 'C', 'D', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "EINJ" Error Injection Table                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_INJECTION_TABLE_SIGNATURE  SIGNATURE_32('E', 'I', 'N', 'J')            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "ERST" Error Record Serialization Table                                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ERROR_RECORD_SERIALIZATION_TABLE_SIGNATURE  SIGNATURE_32('E', 'R', 'S', 'T')  // [CODE_FIRST] 11148
                                                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "FACP" Fixed ACPI Description Table                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE  SIGNATURE_32('F', 'A', 'C', 'P')     // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "FACS" Firmware ACPI Control Structure                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE  SIGNATURE_32('F', 'A', 'C', 'S')  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "FPDT" Firmware Performance Data Table                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_FIRMWARE_PERFORMANCE_DATA_TABLE_SIGNATURE  SIGNATURE_32('F', 'P', 'D', 'T')  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "GTDT" Generic Timer Description Table                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE  SIGNATURE_32('G', 'T', 'D', 'T')  // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "HEST" Hardware Error Source Table                                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HARDWARE_ERROR_SOURCE_TABLE_SIGNATURE  SIGNATURE_32('H', 'E', 'S', 'T')      // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "HMAT" Heterogeneous Memory Attribute Table                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HETEROGENEOUS_MEMORY_ATTRIBUTE_TABLE_SIGNATURE  SIGNATURE_32('H', 'M', 'A', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "MPST" Memory Power State Table                                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MEMORY_POWER_STATE_TABLE_SIGNATURE  SIGNATURE_32('M', 'P', 'S', 'T')         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "MSCT" Maximum System Characteristics Table                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_SIGNATURE  SIGNATURE_32('M', 'S', 'C', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "NFIT" NVDIMM Firmware Interface Table                                                        // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_NVDIMM_FIRMWARE_INTERFACE_TABLE_STRUCTURE_SIGNATURE  SIGNATURE_32('N', 'F', 'I', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "PDTT" Platform Debug Trigger Table                                                           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLATFORM_DEBUG_TRIGGER_TABLE_STRUCTURE_SIGNATURE  SIGNATURE_32('P', 'D', 'T', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "PMTT" Platform Memory Topology Table                                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLATFORM_MEMORY_TOPOLOGY_TABLE_SIGNATURE  SIGNATURE_32('P', 'M', 'T', 'T')   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "PPTT" Processor Properties Topology Table                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE  SIGNATURE_32('P', 'P', 'T', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "PSDT" Persistent System Description Table                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PERSISTENT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE  SIGNATURE_32('P', 'S', 'D', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "RAS2" ACPI RAS2 Feature Table                                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ACPI_RAS2_FEATURE_TABLE_SIGNATURE  SIGNATURE_32('R', 'A', 'S', '2')          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "RASF" ACPI RAS Feature Table                                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ACPI_RAS_FEATURE_TABLE_SIGNATURE  SIGNATURE_32('R', 'A', 'S', 'F')           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "RSDT" Root System Description Table                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ROOT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE  SIGNATURE_32('R', 'S', 'D', 'T')    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "SBST" Smart Battery Specification Table                                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SMART_BATTERY_SPECIFICATION_TABLE_SIGNATURE  SIGNATURE_32('S', 'B', 'S', 'T')  // [CODE_FIRST] 11148
                                                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "SDEV" Secure DEVices Table                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SECURE_DEVICES_TABLE_SIGNATURE  SIGNATURE_32('S', 'D', 'E', 'V')             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "SLIT" System Locality Information Table                                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SYSTEM_LOCALITY_INFORMATION_TABLE_SIGNATURE  SIGNATURE_32('S', 'L', 'I', 'T')  // [CODE_FIRST] 11148
                                                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "SRAT" System Resource Affinity Table                                                         // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SYSTEM_RESOURCE_AFFINITY_TABLE_SIGNATURE  SIGNATURE_32('S', 'R', 'A', 'T')   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "SSDT" Secondary System Description Table                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE  SIGNATURE_32('S', 'S', 'D', 'T')  // [CODE_FIRST] 11148
                                                                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "XSDT" Extended System Description Table                                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE  SIGNATURE_32('X', 'S', 'D', 'T')  // [CODE_FIRST] 11148
                                                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "BOOT" MS Simple Boot Spec                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SIMPLE_BOOT_FLAG_TABLE_SIGNATURE  SIGNATURE_32('B', 'O', 'O', 'T')           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "CSRT" MS Core System Resource Table                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_CORE_SYSTEM_RESOURCE_TABLE_SIGNATURE  SIGNATURE_32('C', 'S', 'R', 'T')       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "DBG2" MS Debug Port 2 Spec                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_DEBUG_PORT_2_TABLE_SIGNATURE  SIGNATURE_32('D', 'B', 'G', '2')               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "DBGP" MS Debug Port Spec                                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_DEBUG_PORT_TABLE_SIGNATURE  SIGNATURE_32('D', 'B', 'G', 'P')                 // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "DMAR" DMA Remapping Table                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_DMA_REMAPPING_TABLE_SIGNATURE  SIGNATURE_32('D', 'M', 'A', 'R')              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "DRTM" Dynamic Root of Trust for Measurement Table                                            // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_DYNAMIC_ROOT_OF_TRUST_FOR_MEASUREMENT_TABLE_SIGNATURE  SIGNATURE_32('D', 'R', 'T', 'M')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "ETDT" Event Timer Description Table                                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_EVENT_TIMER_DESCRIPTION_TABLE_SIGNATURE  SIGNATURE_32('E', 'T', 'D', 'T')    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "HPET" IA-PC High Precision Event Timer Table                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_HIGH_PRECISION_EVENT_TIMER_TABLE_SIGNATURE  SIGNATURE_32('H', 'P', 'E', 'T')  // [CODE_FIRST] 11148
                                                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "iBFT" iSCSI Boot Firmware Table                                                              // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_ISCSI_BOOT_FIRMWARE_TABLE_SIGNATURE  SIGNATURE_32('i', 'B', 'F', 'T')        // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "IORT" I/O Remapping Table                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IO_REMAPPING_TABLE_SIGNATURE  SIGNATURE_32('I', 'O', 'R', 'T')               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "IOVT" I/O Virtualization Table                                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IO_VIRTUALIZATION_TABLE_SIGNATURE  SIGNATURE_32('I', 'O', 'V', 'T')          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "IVRS" I/O Virtualization Reporting Structure                                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_IO_VIRTUALIZATION_REPORTING_STRUCTURE_SIGNATURE  SIGNATURE_32('I', 'V', 'R', 'S')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "LPIT" Low Power Idle Table                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_LOW_POWER_IDLE_TABLE_STRUCTURE_SIGNATURE  SIGNATURE_32('L', 'P', 'I', 'T')   // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "MCFG" PCI Express Memory Mapped Configuration Space Base Address Description Table           // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE  SIGNATURE_32('M', 'C', 'F', 'G')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "MCHI" Management Controller Host Interface Table                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_MANAGEMENT_CONTROLLER_HOST_INTERFACE_TABLE_SIGNATURE  SIGNATURE_32('M', 'C', 'H', 'I')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "MSDM" MS Data Management Table                                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_DATA_MANAGEMENT_TABLE_SIGNATURE  SIGNATURE_32('M', 'S', 'D', 'M')            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "PCCT" Platform Communications Channel Table                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLATFORM_COMMUNICATIONS_CHANNEL_TABLE_SIGNATURE  SIGNATURE_32('P', 'C', 'C', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "PHAT" Platform Health Assessment Table                                                       // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLATFORM_HEALTH_ASSESSMENT_TABLE_SIGNATURE  SIGNATURE_32('P', 'H', 'A', 'T')  // [CODE_FIRST] 11148
                                                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "RIMT" RISC-V IO Mapping Table                                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RIMT_TABLE_SIGNATURE  SIGNATURE_32('R', 'I', 'M', 'T')                       // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "RHCT" RISC-V Hart Capabilities Table (RHCT)                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_RISCV_HART_CAPABILITIES_TABLE_SIGNATURE  SIGNATURE_32('R', 'H', 'C', 'T')    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "SDEI" Software Delegated Exceptions Interface Table                                          // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SOFTWARE_DELEGATED_EXCEPTIONS_INTERFACE_TABLE_SIGNATURE  SIGNATURE_32('S', 'D', 'E', 'I')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "SLIC" MS Software Licensing Table Specification                                              // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SOFTWARE_LICENSING_TABLE_SIGNATURE  SIGNATURE_32('S', 'L', 'I', 'C')         // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "SPCR" Serial Port Concole Redirection Table                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE  SIGNATURE_32('S', 'P', 'C', 'R')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "SPMI" Server Platform Management Interface Table                                             // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_SERVER_PLATFORM_MANAGEMENT_INTERFACE_TABLE_SIGNATURE  SIGNATURE_32('S', 'P', 'M', 'I')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "STAO" _STA Override Table                                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_STA_OVERRIDE_TABLE_SIGNATURE  SIGNATURE_32('S', 'T', 'A', 'O')               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "TCPA" Trusted Computing Platform Alliance Capabilities Table                                 // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_TRUSTED_COMPUTING_PLATFORM_ALLIANCE_CAPABILITIES_TABLE_SIGNATURE  SIGNATURE_32('T', 'C', 'P', 'A')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "TPM2" Trusted Computing Platform 1 Table                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE  SIGNATURE_32('T', 'P', 'M', '2')  // [CODE_FIRST] 11148
                                                                                                     // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "UEFI" UEFI ACPI Data Table                                                                   // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_UEFI_ACPI_DATA_TABLE_SIGNATURE  SIGNATURE_32('U', 'E', 'F', 'I')             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "WAET" Windows ACPI Emulated Devices Table                                                    // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_WINDOWS_ACPI_EMULATED_DEVICES_TABLE_SIGNATURE  SIGNATURE_32('W', 'A', 'E', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "WDAT" Watchdog Action Table                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_WATCHDOG_ACTION_TABLE_SIGNATURE  SIGNATURE_32('W', 'D', 'A', 'T')            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "WDRT" Watchdog Resource Table                                                                // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_WATCHDOG_RESOURCE_TABLE_SIGNATURE  SIGNATURE_32('W', 'D', 'R', 'T')          // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "WPBT" MS Platform Binary Table                                                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_PLATFORM_BINARY_TABLE_SIGNATURE  SIGNATURE_32('W', 'P', 'B', 'T')            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "WSMT" Windows SMM Security Mitigation Table                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_WINDOWS_SMM_SECURITY_MITIGATION_TABLE_SIGNATURE  SIGNATURE_32('W', 'S', 'M', 'T')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "XENV" Xen Project Table                                                                      // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_6_7_XEN_PROJECT_TABLE_SIGNATURE  SIGNATURE_32('X', 'E', 'N', 'V')                // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
/// "MPAM" Memory System Resource Partitioning and Monitoring Table                               // [CODE_FIRST] 11148
///                                                                                               // [CODE_FIRST] 11148
#define EFI_ACPI_MEMORY_SYSTEM_RESOURCE_PARTITIONING_AND_MONITORING_TABLE_SIGNATURE  SIGNATURE_32('M', 'P', 'A', 'M')  // [CODE_FIRST] 11148
// [CODE_FIRST] 11148
#pragma pack()                                                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
#endif // [CODE_FIRST] 11148
