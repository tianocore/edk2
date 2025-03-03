/** @file

  Defines the X64 Namespace Object.

  Copyright (C) 2024 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - X64 or x64 - X64 Architecture
**/

#ifndef X64_NAMESPACE_OBJECTS_H_
#define X64_NAMESPACE_OBJECTS_H_

#include <AcpiObjects.h>
#include <StandardNameSpaceObjects.h>
#include <ArchCommonNameSpaceObjects.h>

#pragma pack(1)

/** The LOCAL_APIC_MODE enum describes the Local APIC
    mode in the X64 Namespace
*/
typedef enum {
  LocalApicModeInvalid = 0,
  LocalApicModeXApic,
  LocalApicModeX2Apic
} LOCAL_APIC_MODE;

/** The EX64_OBJECT_ID enum describes the Object IDs
    in the X64 Namespace
*/
typedef enum X64ObjectID {
  EX64ObjReserved,                    ///<  0 - Reserved
  EX64ObjFadtSciInterrupt,            ///<  1 - FADT SCI Interrupt information
  EX64ObjFadtSciCmdInfo,              ///<  2 - FADT SCI CMD information
  EX64ObjFadtPmBlockInfo,             ///<  3 - FADT Power management block info
  EX64ObjFadtGpeBlockInfo,            ///<  4 - FADT GPE block info
  EX64ObjFadtXpmBlockInfo,            ///<  5 - FADT 64-bit Power Management block info
  EX64ObjFadtXgpeBlockInfo,           ///<  6 - FADT 64-bit GPE block info
  EX64ObjFadtSleepBlockInfo,          ///<  7 - FADT Sleep block info
  EX64ObjFadtResetBlockInfo,          ///<  8 - FADT Reset block info
  EX64ObjFadtMiscInfo,                ///<  9 - FADT Legacy fields info
  EX64ObjWsmtFlagsInfo,               ///< 10 - WSMT protection flags info
  EX64ObjHpetInfo,                    ///< 11 - HPET device info
  EX64ObjMadtInfo,                    ///< 12 - MADT info
  EX64ObjLocalApicX2ApicInfo,         ///< 13 - Local APIC and X2APIC info
  EX64ObjIoApicInfo,                  ///< 14 - IO APIC info
  EX64ObjIntrSourceOverrideInfo,      ///< 15 - Interrupt Source Override info
  EX64ObjLocalApicX2ApicNmiInfo,      ///< 16 - Local APIC and X2APIC NMI info
  EX64ObjFacsInfo,                    ///< 17 - FACS info
  EX64ObjLocalApicX2ApicAffinityInfo, ///< 18 - Local APIC and X2APIC Affinity info
  EX64ObjMax                          ///< 19 - Maximum Object ID
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

/**
  A structure that describes the MADT information.

  ID: EX64ObjMadtInfo
*/
typedef struct CmX64MadtInfo {
  UINT32             LocalApicAddress;
  UINT32             Flags;
  LOCAL_APIC_MODE    ApicMode;
} CM_X64_MADT_INFO;

/**
  A structure that describes the Local APIC and X2APIC information.
  This structure includes fields from the ACPI_6_5_LOCAL_APIC_STRUCTURE
  and ACPI_6_5_LOCAL_X2APIC_STRUCTURE from the ACPI specifications.
  Additional fields are included to support CPU SSDT topology generation.

  ID: EX64ObjLocalApicX2ApicInfo
*/
typedef struct CmX64LocalApicX2ApicInfo {
  UINT32             ApicId;
  UINT32             Flags;
  UINT32             AcpiProcessorUid;

  /** Optional field: Reference Token for the Cst info of this processor.
      i.e. a token referencing a CM_ARCH_COMMON_OBJ_REF,
      which in turn refers to an array of CM_ARCH_COMMON_OBJ_REF objects,
      each pointing to individual CM_ARCH_COMMON_CST_INFO objects.
  */
  CM_OBJECT_TOKEN    CstToken;

  /** Optional field: Reference Token for the Csd info of this processor.
      i.e. a token referencing a CM_ARCH_COMMON_CSD_INFO object.
  */
  CM_OBJECT_TOKEN    CsdToken;

  /** Optional field: Reference Token for the Pct info of this processor.
      i.e. a token referencing a CM_ARCH_COMMON_Pct_INFO object.
  */
  CM_OBJECT_TOKEN    PctToken;

  /** Optional field: Reference Token for the Pss info of this processor.
      i.e. a token referencing a CM_ARCH_COMMON_PSS_INFO object.
  */
  CM_OBJECT_TOKEN    PssToken;

  /** Optional field: Reference Token for the Ppc info of this processor.
      i.e. a token referencing a CM_ARCH_COMMON_PPC_INFO object.
  */
  CM_OBJECT_TOKEN    PpcToken;

  /** Optional field: Reference Token for the Psd info of this processor.
      i.e. a token referencing a CM_ARCH_COMMON_PSD_INFO object.
  */
  CM_OBJECT_TOKEN    PsdToken;

  /** Optional field: Reference Token for the Cpc info of this processor.
      i.e. a token referencing a CM_ARCH_COMMON_CPC_INFO object.
  */
  CM_OBJECT_TOKEN    CpcToken;

  /** Optional field: Reference Token for _STA info of this processor.
      i.e. a token referencing a CM_ARCH_COMMON_STA_INFO object.
   */
  CM_OBJECT_TOKEN    StaToken;
} CM_X64_LOCAL_APIC_X2APIC_INFO;

/**
  A structure that describes the IO APIC information.

  ID: EX64ObjIoApicInfo
*/
typedef struct CmX64IoApicInfo {
  UINT8     IoApicId;
  UINT32    IoApicAddress;
  UINT32    GlobalSystemInterruptBase;
} CM_X64_IO_APIC_INFO;

/**
  A structure that describes the Interrupt Source Override information.

  ID: EX64ObjIntrSourceOverrideInfo
*/
typedef struct CmX64IntrSourceOverrideInfo {
  UINT8     Bus;
  UINT8     Source;
  UINT32    GlobalSystemInterrupt;
  UINT16    Flags;
} CM_X64_INTR_SOURCE_OVERRIDE_INFO;

/**
  A structure that describes the Local APIC NMI information.

  ID: EX64ObjLocalApicX2ApicNmiInfo
*/
typedef struct CmX64LocalApicX2ApicNmiInfo {
  UINT16    Flags;
  UINT32    AcpiProcessorUid;
  UINT8     LocalApicLint;
} CM_X64_LOCAL_APIC_X2APIC_NMI_INFO;

/**
  A structure that describes the FACS information.

  ID: EX64ObjFacsInfo
*/
typedef struct CmX64FacsInfo {
  UINT32    FirmwareWakingVector;
  UINT32    Flags;
  UINT64    XFirmwareWakingVector;
  UINT32    OspmFlags;
} CM_X64_FACS_INFO;

/**
  A structure that describes the Local APIC and X2APIC Affinity information.

  ID: EX64ObjLocalApicX2ApicAffinityInfo
 */
typedef struct CmX64LocalApicX2ApicAffinityInfo {
  LOCAL_APIC_MODE    ApicMode;
  UINT32             ApicId;
  UINT32             ProximityDomain;
  UINT32             Flags;
  UINT32             ClockDomain;
} CM_X64_LOCAL_APIC_X2APIC_AFFINITY_INFO;

#pragma pack()
#endif // X64_NAMESPACE_OBJECTS_H_
