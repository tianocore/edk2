/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
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

#include <Drivers/PL341Dmc.h>
#include <Drivers/PL301Axi.h>
#include <Drivers/SP804Timer.h>

#include <Ppi/ArmMpCoreInfo.h>

#include <ArmPlatform.h>

ARM_CORE_INFO mVersatileExpressMpCoreInfoCTA9x4[] = {
  {
    // Cluster 0, Core 0
    0x0, 0x0,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_REG,
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_SET_REG,
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_CLR_REG,
    (UINT64)0xFFFFFFFF
  },
  {
    // Cluster 0, Core 1
    0x0, 0x1,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_REG,
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_SET_REG,
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_CLR_REG,
    (UINT64)0xFFFFFFFF
  },
  {
    // Cluster 0, Core 2
    0x0, 0x2,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_REG,
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_SET_REG,
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_CLR_REG,
    (UINT64)0xFFFFFFFF
  },
  {
    // Cluster 0, Core 3
    0x0, 0x3,

    // MP Core MailBox Set/Get/Clear Addresses and Clear Value
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_REG,
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_SET_REG,
    (EFI_PHYSICAL_ADDRESS)ARM_VE_SYS_FLAGS_CLR_REG,
    (UINT64)0xFFFFFFFF
  }
};

// DDR2 timings
PL341_DMC_CONFIG DDRTimings = {
  .MaxChip   = 1,
  .IsUserCfg = TRUE,
  .User0Cfg = 0x7C924924,
  .User2Cfg = (TC_UIOLHXC_VALUE << TC_UIOLHNC_SHIFT) | (TC_UIOLHXC_VALUE << TC_UIOLHPC_SHIFT) | (0x1 << TC_UIOHOCT_SHIFT) | (0x1 << TC_UIOHSTOP_SHIFT),
  .HasQos    = TRUE,
  .RefreshPeriod  = 0x3D0,
  .CasLatency  = 0x8,
  .WriteLatency  = 0x3,
  .t_mrd    = 0x2,
  .t_ras    = 0xA,
  .t_rc   = 0xE,
  .t_rcd    = 0x104,
  .t_rfc    = 0x2f32,
  .t_rp   = 0x14,
  .t_rrd    = 0x2,
  .t_wr   = 0x4,
  .t_wtr    = 0x2,
  .t_xp   = 0x2,
  .t_xsr    = 0xC8,
  .t_esr    = 0x14,
  .MemoryCfg   = DMC_MEMORY_CONFIG_ACTIVE_CHIP_1 | DMC_MEMORY_CONFIG_BURST_4 |
                        DMC_MEMORY_CONFIG_ROW_ADDRESS_15 | DMC_MEMORY_CONFIG_COLUMN_ADDRESS_10,
  .MemoryCfg2  = DMC_MEMORY_CFG2_DQM_INIT | DMC_MEMORY_CFG2_CKE_INIT |
            DMC_MEMORY_CFG2_BANK_BITS_3 | DMC_MEMORY_CFG2_MEM_WIDTH_32,
  .MemoryCfg3  = 0x00000001,
  .ChipCfg0    = 0x00010000,
  .t_faw    = 0x00000A0D,
  .ModeReg = DDR2_MR_BURST_LENGTH_4 | DDR2_MR_CAS_LATENCY_4 | DDR2_MR_WR_CYCLES_4,
  .ExtModeReg = DDR_EMR_RTT_50R | (DDR_EMR_ODS_VAL << DDR_EMR_ODS_MASK),
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
  if (MmioRead32(ARM_VE_SYS_FLAGS_NV_REG) == 0) {
    return BOOT_WITH_FULL_CONFIGURATION;
  } else {
    return BOOT_ON_S2_RESUME;
  }
}

/**
  Initialize controllers that must setup in the normal world

  This function is called by the ArmPlatformPkg/PrePi or ArmPlatformPkg/PlatformPei
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

  // Configure periodic timer (TIMER0) for 1MHz operation
  MmioOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, SP810_SYS_CTRL_TIMER0_TIMCLK);
  // Configure 1MHz clock
  MmioOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, SP810_SYS_CTRL_TIMER1_TIMCLK);
  // configure SP810 to use 1MHz clock and disable
  MmioAndThenOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, ~SP810_SYS_CTRL_TIMER2_EN, SP810_SYS_CTRL_TIMER2_TIMCLK);
  // Configure SP810 to use 1MHz clock and disable
  MmioAndThenOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, ~SP810_SYS_CTRL_TIMER3_EN, SP810_SYS_CTRL_TIMER3_TIMCLK);

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
  PL341DmcInit (ARM_VE_DMC_BASE, &DDRTimings);
  PL301AxiInit (ARM_VE_FAXI_BASE);
}

EFI_STATUS
PrePeiCoreGetMpCoreInfo (
  OUT UINTN                   *CoreCount,
  OUT ARM_CORE_INFO           **ArmCoreTable
  )
{
  *CoreCount    = sizeof(mVersatileExpressMpCoreInfoCTA9x4) / sizeof(ARM_CORE_INFO);
  *ArmCoreTable = mVersatileExpressMpCoreInfoCTA9x4;

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

