/** @file

  Defines the X64 Namespace Object.

  Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - X64 or x64 - X64 Architecture
**/

#ifndef X64_NAMESPACE_OBJECTS_H_
#define X64_NAMESPACE_OBJECTS_H_

#include <IndustryStandard/Acpi.h>

/** The EX64_OBJECT_ID enum describes the Object IDs
    in the X64 Namespace
*/
typedef enum X64ObjectID {
  EX64ObjReserved,               ///<  0 - Reserved
  EX64ObjFadtSciInterrupt,       ///<  1 - FADT SCI Interrupt information
  EX64ObjFadtSciCmdInfo,         ///<  2 - FADT SCI CMD information
  EX64ObjFadtPmBlockInfo,        ///<  3 - FADT Power management block info
  EX64ObjFadtGpeBlockInfo,       ///<  4 - FADT GPE block info
  EX64ObjFadtXpmBlockInfo,       ///<  5 - FADT 64-bit Power Management block info
  EX64ObjFadtXgpeBlockInfo,      ///<  6 - FADT 64-bit GPE block info
  EX64ObjFadtSleepBlockInfo,     ///<  7 - FADT Sleep block info
  EX64ObjFadtResetBlockInfo,     ///<  8 - FADT Reset block info
  EX64ObjFadtMiscInfo,           ///<  9 - FADT Legacy fields info
  EX64ObjWsmtFlagsInfo,          ///< 10 - WSMT protection flags info
  EX64ObjHpetInfo,               ///< 11 - HPET device info
  EX64ObjMax                     ///< 12 - Maximum Object ID
} EX64_OBJECT_ID;

/** A structure that describes the
    SCI interrupt Information for the Platform.

    ID: EX64ObjFadtSciInterrupt
*/
typedef struct CmX64FadtSciInterrupt {
  /** This is the SCI interrupt field of the FADT Table
      described in the ACPI Specification
  */
  UINT16    SciInterrupt;
} CM_X64_FADT_SCI_INTERRUPT;

/** A structure that describes the
    SCI CMD Information for the Platform.

    ID: EX64ObjFadtSciCmdInfo
*/
typedef struct CmX64FadtSciCmdInfo {
  /** This is the System control interrupt command information of the FADT Table
      described in the ACPI Specification
  */
  UINT32    SciCmd;
  UINT8     AcpiEnable;
  UINT8     AcpiDisable;
  UINT8     S4BiosReq;
  UINT8     PstateCnt;
  UINT8     CstCnt;
} CM_X64_FADT_SCI_CMD_INFO;

/** A structure that describes the
    power management block information.

    ID: EX64ObjFadtPmBlockInfo
*/
typedef struct CmX64FadtPmBlockInfo {
  /** This is the PM event block information of the FADT Table
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
} CM_X64_FADT_PM_BLOCK_INFO;

/** A structure that describes the
    GPE block information.

    ID: EX64ObjFadtGpeBlockInfo
*/
typedef struct CmX64FadtGpeBlockInfo {
  /** This is the GPE Block information of the FADT Table
      described in the ACPI Specification
  */
  UINT32    Gpe0Blk;
  UINT32    Gpe1Blk;
  UINT8     Gpe0BlkLen;
  UINT8     Gpe1BlkLen;
  UINT8     Gpe1Base;
} CM_X64_FADT_GPE_BLOCK_INFO;

/** A structure that describes the
    64bit power management block information.

    ID: EX64ObjFadtXpmBlockInfo
*/
typedef struct CmX64FadtXpmBlockInfo {
  /** This is the System control interrupt command information of the FADT Table
      described in the ACPI Specification
  */
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPm1aEvtBlk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPm1bEvtBlk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPm1aCntBlk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPm1bCntBlk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPm2CntBlk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XPmTmrBlk;
} CM_X64_FADT_X_PM_BLOCK_INFO;

/** A structure that describes the
    64-bit GPE block information.

    ID: EX64ObjFadtXgpeBlockInfo
*/
typedef struct CmX64FadtXgpeBlockInfo {
  /** This is the GPE Block information of the FADT Table
      described in the ACPI Specification
  */
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XGpe0Blk;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    XGpe1Blk;
} CM_X64_FADT_X_GPE_BLOCK_INFO;

/** A structure that describes the
    sleep control block information.

    ID: EX64ObjFadtSleepBlockInfo
*/
typedef struct CmX64FadtSleepBlockInfo {
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    SleepControlReg;
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    SleepStatusReg;
} CM_X64_FADT_SLEEP_BLOCK_INFO;

/** A structure that describes the
    Reset control block information.

    ID: EX64ObjFadtResetBlockInfo
*/
typedef struct CmX64FadtResetBlockInfo {
  EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE    ResetReg;
  UINT8                                     ResetValue;
} CM_X64_FADT_RESET_BLOCK_INFO;

/** A structure that describes the
    miscellaneous FADT fields information.

    ID: EX64ObjFadtMiscInfo
*/
typedef struct CmX64FadtFadtMiscInfo {
  UINT16    PLvl2Lat;
  UINT16    PLvl3Lat;
  UINT16    FlushSize;
  UINT16    FlushStride;
  UINT8     DutyOffset;
  UINT8     DutyWidth;
  UINT8     DayAlrm;
  UINT8     MonAlrm;
  UINT8     Century;
} CM_X64_FADT_MISC_INFO;

/**
  A structure that describes the WSMT protection flags information.

  ID: EX64ObjWsmtFlagsInfo
*/
typedef struct CmX64WsmtFlagsInfo {
  UINT32    ProtectionFlags;
} CM_X64_WSMT_FLAGS_INFO;

/**
  A structure that describes the HPET device information.

  ID: EX64ObjHpetInfo
*/
typedef struct CmX64HpetInfo {
  UINT32    BaseAddressLower32Bit;
  UINT16    MainCounterMinimumClockTickInPeriodicMode;
  UINT8     PageProtectionAndOemAttribute;
} CM_X64_HPET_INFO;
#endif // X64_NAMESPACE_OBJECTS_H_
