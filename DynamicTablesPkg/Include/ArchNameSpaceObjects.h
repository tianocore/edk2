/** @file
  ARCH Name space object definations.

  Defines namespace objects which are common across platform.
  Platform can implements these optional namespace depends on
  their requirements.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#ifndef ARCH_NAMESPACE_OBJECTS_H_
#define ARCH_NAMESPACE_OBJECTS_H_

#include <Uefi/UefiBaseType.h>
#include <IndustryStandard/Acpi65.h>

/** The ARCH_OBJECT_ID enum describes the Object IDs
    in the ARCH Namespace
*/
typedef enum ArchObjectID {
  ArchObjReserved,           ///<  0 - Reserved
  ArchObjPreferredPmProfile, ///<  1 - Preferred Power Management Profile Info
  ArchObjSciInterrupt,       ///<  2 - SCI Interrupt information
  ArchObjSciCmdInfo,         ///<  3 - SCI CMD information
  ArchObjPmBlockInfo,        ///<  4 - Power management block info
  ArchObjGpeBlockInfo,       ///<  5 - GPE block info
  ArchObjXpmBlockInfo,       ///<  6 - 64-bit Power Management block info
  ArchObjXgpeBlockInfo,      ///<  7 - 64-bit GPE block info
  ArchObjSleepBlockInfo,     ///<  8 - SLEEP block info
  ArchObjResetBlockInfo,     ///<  9 - Reset block info
  ArchObjFadtFlags,          ///< 10 - FADT flags
  ArchObjArmBootArch,        ///< 11 - ARM boot arch information
  ArchObjHypervisorVendorId, ///< 12 - Hypervisor vendor identity information
  ArchObjFadtMiscInfo,       ///< 13 - Legacy fields; RTC, latency, flush stride, etc
  ArchObjMax
} ARCH_OBJECT_ID;

/** A structure that describes the
    Power Management Profile Information for the Platform.

    ID: ArchObjPreferredPmProfile
*/
typedef struct CmArchPreferredPmProfile {
  /** This is the Preferred_PM_Profile field of the FADT Table
      described in the ACPI Specification
  */
  UINT8    PreferredPmProfile;
} CM_ARCH_PREFERRED_PM_PROFILE;

/** A structure that describes the
    SCI interrupt Information for the Platform.

    ID: ArchObjSciInterrupt
*/
typedef struct CmArchSciInterrupt {
  /** This is the Preferred_PM_Profile field of the FADT Table
      described in the ACPI Specification
  */
  UINT16    SciInterrupt;
} CM_ARCH_SCI_INTERRUPT;

/** A structure that describes the
    SCI CMD Information for the Platform.

    ID: ArchObjSciCmdInfo
*/
typedef struct CmArchSciCmdInfo {
  /** This is the System control interrupt command information of the FADT Table
      described in the ACPI Specification
  */
  UINT32    SciCmd;
  UINT8     AcpiEnable;
  UINT8     AcpiDisable;
  UINT8     S4BiosReq;
  UINT8     PstateCnt;
  UINT8     CstCnt;
} CM_ARCH_SCI_CMD_INFO;

/** A structure that describes the
    power management block information.

    ID: ArchObjPmBlockInfo
*/
typedef struct CmArchPmBlockInfo {
  /** This is the System control interrupt command information of the FADT Table
      described in the ACPI Specification
  */
  UINT32    Pm1aEvtBlk;
  UINT32    Pm1bEvtBlk;
  UINT32    Pm1aCntBlk;
  UINT32    Pm1bCntBlk;
  UINT32    Pm2CntBlk;
  UINT32    PmTmrBlk;
  UINT8     Pm1EvtLen;
  UINT8     Pm1CntLen;
  UINT8     Pm2CntLen;
  UINT8     PmTmrLen;
} CM_ARCH_PM_BLOCK_INFO;

/** A structure that describes the
    GPE block information.

    ID: ArchObjGpeBlockInfo
*/
typedef struct CmArchGpeBlockInfo {
  /** This is the GPE Block information of the FADT Table
      described in the ACPI Specification
  */
  UINT32    Gpe0Blk;
  UINT32    Gpe1Blk;
  UINT8     Gpe0BlkLen;
  UINT8     Gpe1BlkLen;
  UINT8     Gpe1Base;
} CM_ARCH_GPE_BLOCK_INFO;

/** A structure that describes the
    FADT flags Information for the Platform.

    ID: ArchObjFadtFlags
*/
typedef struct CmArchFadtFlags {
  UINT32    Flags;
} CM_ARCH_FADT_FLAGS;

/** A structure that describes the
    ARM Boot Architecture flags.

    ID: ArchObjArmBootArch
*/
typedef struct CmArchArmBootArch {
  UINT16    ArmBootArch;
} CM_ARCH_ARM_BOOT_ARCH;

/** A structure that describes the
    Hypervisor vendor identity information.

    ID: ArchObjHypervisorVendorId
*/
typedef struct CmArchHypervisorVendorId {
  UINT64    HypervisorVendorIdentity;
} CM_ARCH_HYPERVISOR_VENDOR_ID;

/** A structure that describes the
    64bit power management block information.

    ID: ArchObjXpmBlockInfo
*/
typedef struct CmArchXpmBlockInfo {
  /** This is the System control interrupt command information of the FADT Table
      described in the ACPI Specification
  */
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPm1aEvtBlk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPm1bEvtBlk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPm1aCntBlk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPm1bCntBlk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPm2CntBlk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPmTmrBlk;
} CM_ARCH_X_PM_BLOCK_INFO;

/** A structure that describes the
    64-bit GPE block information.

    ID: ArchObjXgpeBlockInfo
*/
typedef struct CmArchXgpeBlockInfo {
  /** This is the GPE Block information of the FADT Table
      described in the ACPI Specification
  */
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XGpe0Blk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XGpe1Blk;
} CM_ARCH_X_GPE_BLOCK_INFO;

/** A structure that describes the
    sleep control block information.

    ID: ArchObjSleepBlockInfo
*/
typedef struct CmArchSleepBlockInfo {
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    SleepControlReg;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    SleepStatusReg;
} CM_ARCH_SLEEP_BLOCK_INFO;

/** A structure that describes the
    Reset control block information.

    ID: ArchObjResetBlockInfo
*/
typedef struct CmArchResetBlockInfo {
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    ResetReg;
  UINT8                                     ResetValue;
} CM_ARCH_RESET_BLOCK_INFO;

/** A structure that describes the
    miscellaneous FADT fields information.

    ID: ArchObjFadtMiscInfo
*/
typedef struct CmArchFadtMiscInfo {
  UINT16    PLvl2Lat;
  UINT16    PLvl3Lat;
  UINT16    FlushSize;
  UINT16    FlushStride;
  UINT8     DutyOffset;
  UINT8     DutyWidth;
  UINT8     DayAlrm;
  UINT8     MonAlrm;
  UINT8     Century;
} CM_ARCH_FADT_MISC_INFO;

#endif
