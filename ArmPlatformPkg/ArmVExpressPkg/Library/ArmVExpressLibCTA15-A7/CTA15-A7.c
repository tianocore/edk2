/** @file
*
*  Copyright (c) 2012, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/IoLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <Ppi/ArmMpCoreInfo.h>

#include <ArmPlatform.h>

ARM_CORE_INFO mVersatileExpressCTA15A7InfoTable[] = {
  {
    // Cluster 0, Core 0
    0x0, 0x0,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR0,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR0,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR0,
    (UINT64)0
  },
  {
    // Cluster 0, Core 1
    0x0, 0x1,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR1,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR1,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR1,
    (UINT64)0
  },
#ifndef ARM_BIGLITTLE_TC2
  {
    // Cluster 0, Core 2
    0x0, 0x2,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR2,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR2,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR2,
    (UINT64)0
  },
  {
    // Cluster 0, Core 3
    0x0, 0x3,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR3,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR3,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A15_BX_ADDR3,
    (UINT64)0
  },
#endif
  {
    // Cluster 1, Core 0
    0x1, 0x0,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR0,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR0,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR0,
    (UINT64)0
  },
  {
    // Cluster 1, Core 1
    0x1, 0x1,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR1,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR1,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR1,
    (UINT64)0
  },
  {
    // Cluster 1, Core 2
    0x1, 0x2,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR2,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR2,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR2,
    (UINT64)0
  }
#ifndef ARM_BIGLITTLE_TC2
  ,{
    // Cluster 1, Core 3
    0x1, 0x3,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR3,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR3,
    (EFI_PHYSICAL_ADDRESS)ARM_CTA15A7_SPC_A7_BX_ADDR3,
    (UINT64)0
  }
#endif
};

/**
  Return the current Boot Mode

  This function returns the boot reason on the platform

  @return   Return the current Boot Mode of the platform

**/
EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
  if (MmioRead32(ARM_CTA15A7_SCC_SYSINFO) & ARM_CTA15A7_SCC_SYSINFO_UEFI_RESTORE_DEFAULT_NORFLASH) {
    return BOOT_WITH_DEFAULT_SETTINGS;
  } else {
    return BOOT_WITH_FULL_CONFIGURATION;
  }
}

/**
  Initialize controllers that must setup in the normal world

  This function is called by the ArmPlatformPkg/Pei or ArmPlatformPkg/Pei/PlatformPeim
  in the PEI phase.

**/
RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN                     MpId
  )
{
  if (!ArmPlatformIsPrimaryCore (MpId)) {
    return RETURN_SUCCESS;
  }

  // Nothing to do here

  return RETURN_SUCCESS;
}

/**
  Initialize the system (or sometimes called permanent) memory

  This memory is generally represented by the DRAM.

**/
VOID
ArmPlatformInitializeSystemMemory (
  VOID
  )
{
}

EFI_STATUS
PrePeiCoreGetMpCoreInfo (
  OUT UINTN                   *CoreCount,
  OUT ARM_CORE_INFO           **ArmCoreTable
  )
{
  // Only support one cluster
  *CoreCount    = sizeof(mVersatileExpressCTA15A7InfoTable) / sizeof(ARM_CORE_INFO);
  *ArmCoreTable = mVersatileExpressCTA15A7InfoTable;
  return EFI_SUCCESS;
}

// Needs to be declared in the file. Otherwise gArmMpCoreInfoPpiGuid is undefined in the contect of PrePeiCore
EFI_GUID mArmMpCoreInfoPpiGuid = ARM_MP_CORE_INFO_PPI_GUID;
ARM_MP_CORE_INFO_PPI mMpCoreInfoPpi = { PrePeiCoreGetMpCoreInfo };

EFI_PEI_PPI_DESCRIPTOR      gPlatformPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &mArmMpCoreInfoPpiGuid,
    &mMpCoreInfoPpi
  }
};

VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  *PpiListSize = sizeof(gPlatformPpiTable);
  *PpiList = gPlatformPpiTable;
}
