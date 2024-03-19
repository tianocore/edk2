/** @file
  FADT Table Generator

  Copyright (c) 2017 - 2023, Arm Limited. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.5 Specification, Aug 29, 2022

**/

#include <AcpiTableGenerator.h>
#include <ArchNameSpaceObjects.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <IndustryStandard/WindowsSmmSecurityMitigationTable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include "FadtUpdate.h"

/** Empty GAS defintion
*/
#define NULL_GAS  { EFI_ACPI_6_5_SYSTEM_MEMORY,  0, 0, EFI_ACPI_6_5_UNDEFINED, 0L }

/** The Creator ID for the ACPI tables generated using
  the standard ACPI table generators.
*/
#define TABLE_GENERATOR_CREATOR_ID_GENERIC  SIGNATURE_32('D', 'Y', 'N', 'T')

/** The AcpiFadt is a template EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE
    structure used for generating the FADT Table.
*/
STATIC
EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE  mAcpiFadt = {
  ACPI_HEADER (
    EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
    EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE,
    EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_REVISION
    ),
  // UINT32     FirmwareCtrl
  0,
  // UINT32     Dsdt
  0,
  // UINT8      Reserved0
  EFI_ACPI_RESERVED_BYTE,
  // UINT8      PreferredPmProfile
  EFI_ACPI_6_5_PM_PROFILE_UNSPECIFIED,
  // UINT16     SciInt
  0,
  // UINT32     SmiCmd
  0,
  // UINT8      AcpiEnable
  0,
  // UINT8      AcpiDisable
  0,
  // UINT8      S4BiosReq
  0,
  // UINT8      PstateCnt
  0,
  // UINT32     Pm1aEvtBlk
  0,
  // UINT32     Pm1bEvtBlk
  0,
  // UINT32     Pm1aCntBlk
  0,
  // UINT32     Pm1bCntBlk
  0,
  // UINT32     Pm2CntBlk
  0,
  // UINT32     PmTmrBlk
  0,
  // UINT32     Gpe0Blk
  0,
  // UINT32     Gpe1Blk
  0,
  // UINT8      Pm1EvtLen
  0,
  // UINT8      Pm1CntLen
  0,
  // UINT8      Pm2CntLen
  0,
  // UINT8      PmTmrLen
  0,
  // UINT8      Gpe0BlkLen
  0,
  // UINT8      Gpe1BlkLen
  0,
  // UINT8      Gpe1Base
  0,
  // UINT8      CstCnt
  0,
  // UINT16     PLvl2Lat
  0,
  // UINT16     PLvl3Lat
  0,
  // UINT16     FlushSize
  0,
  // UINT16     FlushStride
  0,
  // UINT8      DutyOffset
  0,
  // UINT8      DutyWidth
  0,
  // UINT8      DayAlrm
  0,
  // UINT8      MonAlrm
  0,
  // UINT8      Century
  0,
  // UINT16     IaPcBootArch
  0,
  // UINT8      Reserved1
  0,
  // UINT32     Flags
  0,
  // EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  ResetReg
  NULL_GAS,
  // UINT8      ResetValue
  0,
  // UINT16     ArmBootArch
  0,
  // UINT8      MinorRevision
  EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_MINOR_REVISION,
  // UINT64     XFirmwareCtrl
  0,
  // UINT64     XDsdt
  0,
  // EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  XPm1aEvtBlk
  NULL_GAS,
  // EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  XPm1bEvtBlk
  NULL_GAS,
  // EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  XPm1aCntBlk
  NULL_GAS,
  // EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  XPm1bCntBlk
  NULL_GAS,
  // EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  XPm2CntBlk
  NULL_GAS,
  // EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  XPmTmrBlk
  NULL_GAS,
  // EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  XGpe0Blk
  NULL_GAS,
  // EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  XGpe1Blk
  NULL_GAS,
  // EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  SleepControlReg
  NULL_GAS,
  // EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE  SleepStatusReg
  NULL_GAS,
  // UINT64     HypervisorVendorIdentity
  EFI_ACPI_RESERVED_QWORD
};

/** This macro expands to a function that retrieves the Power
    Management Profile Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtPreferredPmProfile,
  CM_ARCH_FADT_PREFERRED_PM_PROFILE
  );

/** This macro expands to a function that retrieves the
    SCI interrupt information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtSciInterrupt,
  CM_ARCH_FADT_SCI_INTERRUPT
  );

/** This macro expands to a function that retrieves the
    SCI command information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtSciCmdInfo,
  CM_ARCH_FADT_SCI_CMD_INFO
  );

/** This macro expands to a function that retrieves the
    legacy power management information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtPmBlockInfo,
  CM_ARCH_FADT_PM_BLOCK_INFO
  );

/** This macro expands to a function that retrieves the
    legacy GPE block information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtGpeBlockInfo,
  CM_ARCH_FADT_GPE_BLOCK_INFO
  );

/** This macro expands to a function that retrieves the
    FADT flags information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtFlags,
  CM_ARCH_FADT_FLAGS
  )

/** This macro expands to a function that retrieves the
    ARM boot arch information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtArmBootArch,
  CM_ARCH_FADT_ARM_BOOT_ARCH
  )

/** This macro expands to a function that retrieves the
    hypervisor vendor identity information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtHypervisorVendorId,
  CM_ARCH_FADT_HYPERVISOR_VENDOR_ID
  )

/** This macro expands to a function that retrieves the
    legacy level2 latency, level 3 latency, RTC information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtMiscInfo,
  CM_ARCH_FADT_MISC_INFO
  )

/** This macro expands to a function that retrieves the
    64-bit power management information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtXpmBlockInfo,
  CM_ARCH_FADT_X_PM_BLOCK_INFO
  );

/** This macro expands to a function that retrieves the
    64-bit GPE block information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtXgpeBlockInfo,
  CM_ARCH_FADT_X_GPE_BLOCK_INFO
  );

/** This macro expands to a function that retrieves the
    sleep block information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtSleepBlockInfo,
  CM_ARCH_FADT_SLEEP_BLOCK_INFO
  );

/** This macro expands to a function that retrieves the
    reset block information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArch,
  EArchObjFadtResetBlockInfo,
  CM_ARCH_FADT_RESET_BLOCK_INFO
  );

/** Construct the FADT table.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

  @param [in]  This           Pointer to the table generator.
  @param [in]  AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.
  @param [out] Table          Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
STATIC
EFI_STATUS
BuildFadtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                         Status;
  CM_ARCH_FADT_PREFERRED_PM_PROFILE  *PmProfile;
  CM_ARCH_FADT_SCI_INTERRUPT         *SciInterrupt;
  CM_ARCH_FADT_SCI_CMD_INFO          *SciCmdinfo;
  CM_ARCH_FADT_PM_BLOCK_INFO         *PmBlockInfo;
  CM_ARCH_FADT_GPE_BLOCK_INFO        *GpeBlockInfo;
  CM_ARCH_FADT_X_PM_BLOCK_INFO       *XpmBlockInfo;
  CM_ARCH_FADT_X_GPE_BLOCK_INFO      *XgpeBlockInfo;
  CM_ARCH_FADT_SLEEP_BLOCK_INFO      *SleepBlockInfo;
  CM_ARCH_FADT_RESET_BLOCK_INFO      *ResetBlockInfo;
  CM_ARCH_FADT_FLAGS                 *FadtFlags;
  CM_ARCH_FADT_ARM_BOOT_ARCH         *ArmBootArch;
  CM_ARCH_FADT_HYPERVISOR_VENDOR_ID  *HypervisorVendorId;
  CM_ARCH_FADT_MISC_INFO             *FadtMiscInfo;
  UINT64                             EArchObjFadtMask;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FADT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             (EFI_ACPI_DESCRIPTION_HEADER *)&mAcpiFadt,
             AcpiTableInfo,
             sizeof (EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FADT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    return Status;
  }

  EArchObjFadtMask = 0;
  GetPlatformNameSpaceMask (&EArchObjFadtMask);

  if (!(EArchObjFadtMask & (1 << EArchObjFadtPreferredPmProfile))) {
    // Get the Power Management Profile from the Platform Configuration Manager
    Status = GetEArchObjFadtPreferredPmProfile (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &PmProfile,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get Power Management Profile information." \
        " Status = %r\n",
        Status
        ));
    } else {
      mAcpiFadt.PreferredPmProfile = PmProfile->PreferredPmProfile;
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtSciInterrupt))) {
    // Get the SCI interrupt from the Platform Configuration Manager
    Status = GetEArchObjFadtSciInterrupt (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &SciInterrupt,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get SCI Interrupt information." \
        " Status = %r\n",
        Status
        ));
    } else {
      mAcpiFadt.SciInt = SciInterrupt->SciInterrupt;
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtSciCmdInfo))) {
    // Get the SCI CMD information from the Platform Configuration Manager
    Status = GetEArchObjFadtSciCmdInfo (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &SciCmdinfo,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get SCI CMD information." \
        " Status = %r\n",
        Status
        ));
    } else {
      mAcpiFadt.SmiCmd      = SciCmdinfo->SciCmd;
      mAcpiFadt.AcpiEnable  = SciCmdinfo->AcpiEnable;
      mAcpiFadt.AcpiDisable = SciCmdinfo->AcpiDisable;
      mAcpiFadt.S4BiosReq   = SciCmdinfo->S4BiosReq;
      mAcpiFadt.PstateCnt   = SciCmdinfo->PstateCnt;
      mAcpiFadt.CstCnt      = SciCmdinfo->CstCnt;
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtPmBlockInfo))) {
    // Get the SCI PM Block information from the Platform Configuration Manager
    Status = GetEArchObjFadtPmBlockInfo (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &PmBlockInfo,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get PM Block information." \
        " Status = %r\n",
        Status
        ));
    } else {
      mAcpiFadt.Pm1aEvtBlk = PmBlockInfo->Pm1aEvtBlk;
      mAcpiFadt.Pm1bEvtBlk = PmBlockInfo->Pm1bEvtBlk;
      mAcpiFadt.Pm1aCntBlk = PmBlockInfo->Pm1aCntBlk;
      mAcpiFadt.Pm1bCntBlk = PmBlockInfo->Pm1bCntBlk;
      mAcpiFadt.Pm2CntBlk  = PmBlockInfo->Pm2CntBlk;
      mAcpiFadt.PmTmrBlk   = PmBlockInfo->PmTmrBlk;
      mAcpiFadt.Pm1EvtLen  = PmBlockInfo->Pm1EvtLen;
      mAcpiFadt.Pm1CntLen  = PmBlockInfo->Pm1CntLen;
      mAcpiFadt.Pm2CntLen  = PmBlockInfo->Pm2CntLen;
      mAcpiFadt.PmTmrLen   = PmBlockInfo->PmTmrLen;
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtGpeBlockInfo))) {
    // Get the SCI PM Block information from the Platform Configuration Manager
    Status = GetEArchObjFadtGpeBlockInfo (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &GpeBlockInfo,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get PM Block information." \
        " Status = %r\n",
        Status
        ));
    } else {
      mAcpiFadt.Gpe0Blk    = GpeBlockInfo->Gpe0Blk;
      mAcpiFadt.Gpe1Blk    = GpeBlockInfo->Gpe1Blk;
      mAcpiFadt.Gpe0BlkLen = GpeBlockInfo->Gpe0BlkLen;
      mAcpiFadt.Gpe1BlkLen = GpeBlockInfo->Gpe1BlkLen;
      mAcpiFadt.Gpe1Base   = GpeBlockInfo->Gpe1Base;
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtFlags))) {
    // Get the Power Management Profile from the Platform Configuration Manager
    Status = GetEArchObjFadtFlags (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &FadtFlags,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get FADT flags information." \
        " Status = %r\n",
        Status
        ));
    } else {
      mAcpiFadt.Flags = FadtFlags->Flags;
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtArmBootArch))) {
    // Get the Arm boot arch information from the Platform Configuration Manager
    Status = GetEArchObjFadtArmBootArch (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &ArmBootArch,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get ARM boot arch flags information." \
        " Status = %r\n",
        Status
        ));
    } else {
      mAcpiFadt.ArmBootArch = ArmBootArch->ArmBootArch;
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtArmBootArch))) {
    // Get the Hypervisor Vendor Identity information from the Platform Configuration Manager
    Status = GetEArchObjFadtHypervisorVendorId (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &HypervisorVendorId,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get Hypervisor Vendor Identity flags information." \
        " Status = %r\n",
        Status
        ));
    } else {
      mAcpiFadt.HypervisorVendorIdentity = HypervisorVendorId->HypervisorVendorIdentity;
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtXpmBlockInfo))) {
    // Get the 64-bit PM Block information from the Platform Configuration Manager
    Status = GetEArchObjFadtXpmBlockInfo (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &XpmBlockInfo,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get 64-bit PM Block information." \
        " Status = %r\n",
        Status
        ));
    } else {
      CopyMem (&mAcpiFadt.XPm1aCntBlk, &XpmBlockInfo->XPm1aCntBlk, sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE));
      CopyMem (&mAcpiFadt.XPm1bEvtBlk, &XpmBlockInfo->XPm1bEvtBlk, sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE));
      CopyMem (&mAcpiFadt.XPm1aCntBlk, &XpmBlockInfo->XPm1aCntBlk, sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE));
      CopyMem (&mAcpiFadt.XPm1bCntBlk, &XpmBlockInfo->XPm1bCntBlk, sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE));
      CopyMem (&mAcpiFadt.XPm2CntBlk, &XpmBlockInfo->XPm2CntBlk, sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE));
      CopyMem (&mAcpiFadt.XPmTmrBlk, &XpmBlockInfo->XPmTmrBlk, sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE));
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtMiscInfo))) {
    // Get the various platform information from the Platform Configuration manager
    Status = GetEArchObjFadtMiscInfo (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &FadtMiscInfo,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get various platform information." \
        " Status = %r\n",
        Status
        ));
    } else {
      mAcpiFadt.PLvl2Lat    = FadtMiscInfo->PLvl2Lat;
      mAcpiFadt.PLvl3Lat    = FadtMiscInfo->PLvl3Lat;
      mAcpiFadt.FlushSize   = FadtMiscInfo->FlushSize;
      mAcpiFadt.FlushStride = FadtMiscInfo->FlushStride;
      mAcpiFadt.DutyOffset  = FadtMiscInfo->DutyOffset;
      mAcpiFadt.DutyWidth   = FadtMiscInfo->DutyWidth;
      mAcpiFadt.DayAlrm     = FadtMiscInfo->DayAlrm;
      mAcpiFadt.MonAlrm     = FadtMiscInfo->MonAlrm;
      mAcpiFadt.Century     = FadtMiscInfo->Century;
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtXgpeBlockInfo))) {
    // Get the 64-bit GPE Block information from the Platform Configuration Manager
    Status = GetEArchObjFadtXgpeBlockInfo (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &XgpeBlockInfo,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get 64-bit GPE Block information." \
        " Status = %r\n",
        Status
        ));
    } else {
      CopyMem (&mAcpiFadt.XGpe0Blk, &XgpeBlockInfo->XGpe0Blk, sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE));
      CopyMem (&mAcpiFadt.XGpe1Blk, &XgpeBlockInfo->XGpe1Blk, sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE));
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtSleepBlockInfo))) {
    // Get the sleep Block information from the Platform Configuration Manager
    Status = GetEArchObjFadtSleepBlockInfo (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &SleepBlockInfo,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get Sleep Block information." \
        " Status = %r\n",
        Status
        ));
    } else {
      CopyMem (&mAcpiFadt.SleepControlReg, &SleepBlockInfo->SleepControlReg, sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE));
      CopyMem (&mAcpiFadt.SleepStatusReg, &SleepBlockInfo->SleepStatusReg, sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE));
    }
  }

  if (!(EArchObjFadtMask & (1 << EArchObjFadtResetBlockInfo))) {
    // Get the sleep Block information from the Platform Configuration Manager
    Status = GetEArchObjFadtResetBlockInfo (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &ResetBlockInfo,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get Reset Block information." \
        " Status = %r\n",
        Status
        ));
    } else {
      CopyMem (&mAcpiFadt.ResetReg, &ResetBlockInfo->ResetReg, sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE));
      mAcpiFadt.ResetValue = ResetBlockInfo->ResetValue;
    }
  }

  Status = EFI_SUCCESS;

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)&mAcpiFadt;

  return Status;
}

/** This macro defines the FADT Table Generator revision.
*/
#define FADT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the FADT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  mFadtGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdFadt),
  // Generator Description
  L"ACPI.STD.FADT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_GENERIC,
  // Creator Revision
  FADT_GENERATOR_REVISION,
  // Build Table function
  BuildFadtTable,
  // No additional resources are allocated by the generator.
  // Hence the Free Resource function is not required.
  NULL,
  // Extended build function not needed
  NULL,
  // Extended build function not implemented by the generator.
  // Hence extended free resource function is not required.
  NULL
};

/** Register the Generator with the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
AcpiFadtLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&mFadtGenerator);
  DEBUG ((DEBUG_INFO, "FADT: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiFadtLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&mFadtGenerator);
  DEBUG ((DEBUG_INFO, "FADT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
