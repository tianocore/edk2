/** @file
  X64 FADT Table Helpers

  Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.5 Specification, Aug 29, 2022

**/

#include <X64NameSpaceObjects.h>
#include <Library/BaseMemoryLib.h>
#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include "FadtGenerator.h"

/** This macro expands to a function that retrieves the
    SCI interrupt information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjFadtSciInterrupt,
  CM_X64_FADT_SCI_INTERRUPT
  );

/** This macro expands to a function that retrieves the
    SCI command information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjFadtSciCmdInfo,
  CM_X64_FADT_SCI_CMD_INFO
  );

/** This macro expands to a function that retrieves the
    legacy power management information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjFadtPmBlockInfo,
  CM_X64_FADT_PM_BLOCK_INFO
  );

/** This macro expands to a function that retrieves the
    legacy GPE block information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjFadtGpeBlockInfo,
  CM_X64_FADT_GPE_BLOCK_INFO
  );

/** This macro expands to a function that retrieves the
    legacy level2 latency, level 3 latency, RTC information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjFadtMiscInfo,
  CM_X64_FADT_MISC_INFO
  );

/** This macro expands to a function that retrieves the
    64-bit power management information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjFadtXpmBlockInfo,
  CM_X64_FADT_X_PM_BLOCK_INFO
  );

/** This macro expands to a function that retrieves the
    64-bit GPE block information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjFadtXgpeBlockInfo,
  CM_X64_FADT_X_GPE_BLOCK_INFO
  );

/** This macro expands to a function that retrieves the
    sleep block information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjFadtSleepBlockInfo,
  CM_X64_FADT_SLEEP_BLOCK_INFO
  );

/** This macro expands to a function that retrieves the
    reset block information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjFadtResetBlockInfo,
  CM_X64_FADT_RESET_BLOCK_INFO
  );

/** Updates the Architecture specific information in the FADT Table.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.
  @param [in, out] Fadt       Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
EFI_STATUS
EFIAPI
FadtArchUpdate (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN   OUT EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE      *Fadt
  )
{
  EFI_STATUS                    Status;
  CM_X64_FADT_SCI_INTERRUPT     *SciInterrupt;
  CM_X64_FADT_SCI_CMD_INFO      *SciCmdinfo;
  CM_X64_FADT_PM_BLOCK_INFO     *PmBlockInfo;
  CM_X64_FADT_GPE_BLOCK_INFO    *GpeBlockInfo;
  CM_X64_FADT_X_PM_BLOCK_INFO   *XpmBlockInfo;
  CM_X64_FADT_X_GPE_BLOCK_INFO  *XgpeBlockInfo;
  CM_X64_FADT_SLEEP_BLOCK_INFO  *SleepBlockInfo;
  CM_X64_FADT_RESET_BLOCK_INFO  *ResetBlockInfo;
  CM_X64_FADT_MISC_INFO         *FadtMiscInfo;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Fadt != NULL);

  // Get the SCI interrupt from the Platform Configuration Manager
  Status = GetEX64ObjFadtSciInterrupt (
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
    Fadt->SciInt = SciInterrupt->SciInterrupt;
  }

  // Get the SCI CMD information from the Platform Configuration Manager
  Status = GetEX64ObjFadtSciCmdInfo (
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
    Fadt->SmiCmd      = SciCmdinfo->SciCmd;
    Fadt->AcpiEnable  = SciCmdinfo->AcpiEnable;
    Fadt->AcpiDisable = SciCmdinfo->AcpiDisable;
    Fadt->S4BiosReq   = SciCmdinfo->S4BiosReq;
    Fadt->PstateCnt   = SciCmdinfo->PstateCnt;
    Fadt->CstCnt      = SciCmdinfo->CstCnt;
  }

  // Get the SCI PM Block information from the Platform Configuration Manager
  Status = GetEX64ObjFadtPmBlockInfo (
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
    Fadt->Pm1aEvtBlk = PmBlockInfo->Pm1aEvtBlk;
    Fadt->Pm1bEvtBlk = PmBlockInfo->Pm1bEvtBlk;
    Fadt->Pm1aCntBlk = PmBlockInfo->Pm1aCntBlk;
    Fadt->Pm1bCntBlk = PmBlockInfo->Pm1bCntBlk;
    Fadt->Pm2CntBlk  = PmBlockInfo->Pm2CntBlk;
    Fadt->PmTmrBlk   = PmBlockInfo->PmTmrBlk;
    Fadt->Pm1EvtLen  = PmBlockInfo->Pm1EvtLen;
    Fadt->Pm1CntLen  = PmBlockInfo->Pm1CntLen;
    Fadt->Pm2CntLen  = PmBlockInfo->Pm2CntLen;
    Fadt->PmTmrLen   = PmBlockInfo->PmTmrLen;
  }

  // Get the SCI PM Block information from the Platform Configuration Manager
  Status = GetEX64ObjFadtGpeBlockInfo (
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
    Fadt->Gpe0Blk    = GpeBlockInfo->Gpe0Blk;
    Fadt->Gpe1Blk    = GpeBlockInfo->Gpe1Blk;
    Fadt->Gpe0BlkLen = GpeBlockInfo->Gpe0BlkLen;
    Fadt->Gpe1BlkLen = GpeBlockInfo->Gpe1BlkLen;
    Fadt->Gpe1Base   = GpeBlockInfo->Gpe1Base;
  }

  // Get the 64-bit PM Block information from the Platform Configuration Manager
  Status = GetEX64ObjFadtXpmBlockInfo (
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
    CopyMem (
      &Fadt->XPm1aCntBlk,
      &XpmBlockInfo->XPm1aCntBlk,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );
    CopyMem (
      &Fadt->XPm1bEvtBlk,
      &XpmBlockInfo->XPm1bEvtBlk,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );
    CopyMem (
      &Fadt->XPm1aCntBlk,
      &XpmBlockInfo->XPm1aCntBlk,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );
    CopyMem (
      &Fadt->XPm1bCntBlk,
      &XpmBlockInfo->XPm1bCntBlk,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );
    CopyMem (
      &Fadt->XPm2CntBlk,
      &XpmBlockInfo->XPm2CntBlk,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );
    CopyMem (
      &Fadt->XPmTmrBlk,
      &XpmBlockInfo->XPmTmrBlk,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );
  }

  // Get the various platform information from the Platform Configuration manager
  Status = GetEX64ObjFadtMiscInfo (
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
    Fadt->PLvl2Lat    = FadtMiscInfo->PLvl2Lat;
    Fadt->PLvl3Lat    = FadtMiscInfo->PLvl3Lat;
    Fadt->FlushSize   = FadtMiscInfo->FlushSize;
    Fadt->FlushStride = FadtMiscInfo->FlushStride;
    Fadt->DutyOffset  = FadtMiscInfo->DutyOffset;
    Fadt->DutyWidth   = FadtMiscInfo->DutyWidth;
    Fadt->DayAlrm     = FadtMiscInfo->DayAlrm;
    Fadt->MonAlrm     = FadtMiscInfo->MonAlrm;
    Fadt->Century     = FadtMiscInfo->Century;
  }

  // Get the 64-bit GPE Block information from the Platform Configuration Manager
  Status = GetEX64ObjFadtXgpeBlockInfo (
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
    CopyMem (
      &Fadt->XGpe0Blk,
      &XgpeBlockInfo->XGpe0Blk,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );
    CopyMem (
      &Fadt->XGpe1Blk,
      &XgpeBlockInfo->XGpe1Blk,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );
  }

  // Get the sleep Block information from the Platform Configuration Manager
  Status = GetEX64ObjFadtSleepBlockInfo (
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
    CopyMem (
      &Fadt->SleepControlReg,
      &SleepBlockInfo->SleepControlReg,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );
    CopyMem (
      &Fadt->SleepStatusReg,
      &SleepBlockInfo->SleepStatusReg,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );
  }

  // Get the sleep Block information from the Platform Configuration Manager
  Status = GetEX64ObjFadtResetBlockInfo (
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
    CopyMem (
      &Fadt->ResetReg,
      &ResetBlockInfo->ResetReg,
      sizeof (EFI_ACPI_6_5_GENERIC_ADDRESS_STRUCTURE)
      );
    Fadt->ResetValue = ResetBlockInfo->ResetValue;
  }

  return Status;
}
