/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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

#include <Drivers/SP804Timer.h>

#include <Ppi/ArmMpCoreInfo.h>

#include <ArmPlatform.h>

ARM_CORE_INFO mVersatileExpressMpCoreInfoTable[] = {
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

/**
  Return if Trustzone is supported by your platform

  A non-zero value must be returned if you want to support a Secure World on your platform.
  ArmVExpressTrustzoneInit() will later set up the secure regions.
  This function can return 0 even if Trustzone is supported by your processor. In this case,
  the platform will continue to run in Secure World.

  @return   A non-zero value if Trustzone supported.

**/
UINTN
ArmPlatformTrustzoneSupported (
  VOID
  )
{
  // Not supported yet but model does have Secure SRAM (but no TZPC/TZASC) so we could support it
  return FALSE;
}

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
  return BOOT_WITH_FULL_CONFIGURATION;
}

/**
  Remap the memory at 0x0

  Some platform requires or gives the ability to remap the memory at the address 0x0.
  This function can do nothing if this feature is not relevant to your platform.

**/
VOID
ArmPlatformBootRemapping (
  VOID
  )
{
  // Disable memory remapping and return to normal mapping
  MmioOr32 (SP810_CTRL_BASE, BIT8);
}

/**
  Initialize controllers that must setup in the normal world

  This function is called by the ArmPlatformPkg/Pei or ArmPlatformPkg/Pei/PlatformPeim
  in the PEI phase.

**/
VOID
ArmPlatformNormalInitialize (
  VOID
  )
{
  // Nothing to do here
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
  // Configure periodic timer (TIMER0) for 1MHz operation
  MmioOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, SP810_SYS_CTRL_TIMER0_TIMCLK);
  // Configure 1MHz clock
  MmioOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, SP810_SYS_CTRL_TIMER1_TIMCLK);
  // configure SP810 to use 1MHz clock and disable
  MmioAndThenOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, ~SP810_SYS_CTRL_TIMER2_EN, SP810_SYS_CTRL_TIMER2_TIMCLK);
  // Configure SP810 to use 1MHz clock and disable
  MmioAndThenOr32 (SP810_CTRL_BASE + SP810_SYS_CTRL_REG, ~SP810_SYS_CTRL_TIMER3_EN, SP810_SYS_CTRL_TIMER3_TIMCLK);
}

EFI_STATUS
PrePeiCoreGetMpCoreInfo (
  OUT UINTN                   *CoreCount,
  OUT ARM_CORE_INFO           **ArmCoreTable
  )
{
  UINT32   ProcType;

  ProcType = MmioRead32 (ARM_VE_SYS_PROCID0_REG) & ARM_VE_SYS_PROC_ID_MASK;
  if (ProcType == ARM_VE_SYS_PROC_ID_CORTEX_A9) {
    // Only support one cluster
    *CoreCount    = ArmGetCpuCountPerCluster ();
    *ArmCoreTable = mVersatileExpressMpCoreInfoTable;
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
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
