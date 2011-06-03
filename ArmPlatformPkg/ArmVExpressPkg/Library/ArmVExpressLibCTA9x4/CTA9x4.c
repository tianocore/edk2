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
#include <Library/ArmTrustZoneLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Drivers/PL341Dmc.h>
#include <Drivers/PL301Axi.h>
#include <Drivers/PL310L2Cache.h>
#include <Library/SerialPortLib.h>

#define SerialPrint(txt)  SerialPortWrite (txt, AsciiStrLen(txt)+1);

// DDR2 timings
struct pl341_dmc_config ddr_timings = {
    .base		= ARM_VE_DMC_BASE,
    .has_qos		= 1,
    .refresh_prd	= 0x3D0,
    .cas_latency	= 0x8,
    .write_latency	= 0x3,
    .t_mrd		= 0x2,
    .t_ras		= 0xA,
    .t_rc		= 0xE,
    .t_rcd		= 0x104,
    .t_rfc		= 0x2f32,
    .t_rp		= 0x14,
    .t_rrd		= 0x2,
    .t_wr		= 0x4,
    .t_wtr		= 0x2,
    .t_xp		= 0x2,
    .t_xsr		= 0xC8,
    .t_esr		= 0x14,
    .memory_cfg		= DMC_MEMORY_CONFIG_ACTIVE_CHIP_1 | DMC_MEMORY_CONFIG_BURST_4 |
                          DMC_MEMORY_CONFIG_ROW_ADDRESS_15 | DMC_MEMORY_CONFIG_COLUMN_ADDRESS_10,
    .memory_cfg2	= DMC_MEMORY_CFG2_DQM_INIT | DMC_MEMORY_CFG2_CKE_INIT |
						  DMC_MEMORY_CFG2_BANK_BITS_3 | DMC_MEMORY_CFG2_MEM_WIDTH_32,
    .memory_cfg3	= 0x00000001,
    .chip_cfg0		= 0x00010000,
    .t_faw		= 0x00000A0D,
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
  return (MmioRead32(ARM_VE_SYS_CFGRW1_REG) & ARM_VE_CFGRW1_TZASC_EN_BIT_MASK);
}

/**
  Initialize the Secure peripherals and memory regions

  If Trustzone is supported by your platform then this function makes the required initialization
  of the secure peripherals and memory regions.

**/
VOID ArmPlatformTrustzoneInit(VOID) {
    //
    // Setup TZ Protection Controller
    //
    
    // Set Non Secure access for all devices
    TZPCSetDecProtBits(ARM_VE_TZPC_BASE, TZPC_DECPROT_0, 0xFFFFFFFF);
    TZPCSetDecProtBits(ARM_VE_TZPC_BASE, TZPC_DECPROT_1, 0xFFFFFFFF);
    TZPCSetDecProtBits(ARM_VE_TZPC_BASE, TZPC_DECPROT_2, 0xFFFFFFFF);

    // Remove Non secure access to secure devices
    TZPCClearDecProtBits(ARM_VE_TZPC_BASE, TZPC_DECPROT_0,
         ARM_VE_DECPROT_BIT_TZPC | ARM_VE_DECPROT_BIT_DMC_TZASC | ARM_VE_DECPROT_BIT_NMC_TZASC | ARM_VE_DECPROT_BIT_SMC_TZASC);

    TZPCClearDecProtBits(ARM_VE_TZPC_BASE, TZPC_DECPROT_2,
         ARM_VE_DECPROT_BIT_EXT_MAST_TZ | ARM_VE_DECPROT_BIT_DMC_TZASC_LOCK | ARM_VE_DECPROT_BIT_NMC_TZASC_LOCK | ARM_VE_DECPROT_BIT_SMC_TZASC_LOCK);


    //
    // Setup TZ Address Space Controller for the SMC. Create 5 Non Secure regions (NOR0, NOR1, SRAM, SMC Peripheral regions)
    //

    // NOR Flash 0 non secure (BootMon)
    TZASCSetRegion(ARM_VE_TZASC_BASE,1,TZASC_REGION_ENABLED,
        ARM_VE_SMB_NOR0_BASE,0,
        TZASC_REGION_SIZE_64MB, TZASC_REGION_SECURITY_NSRW);

    // NOR Flash 1. The first half of the NOR Flash1 must be secure for the secure firmware (sec_uefi.bin)
#if EDK2_ARMVE_SECURE_SYSTEM
    //Note: Your OS Kernel must be aware of the secure regions before to enable this region
    TZASCSetRegion(ARM_VE_TZASC_BASE,2,TZASC_REGION_ENABLED,
        ARM_VE_SMB_NOR1_BASE + SIZE_32MB,0,
        TZASC_REGION_SIZE_32MB, TZASC_REGION_SECURITY_NSRW);
#else
    TZASCSetRegion(ARM_VE_TZASC_BASE,2,TZASC_REGION_ENABLED,
        ARM_VE_SMB_NOR1_BASE,0,
        TZASC_REGION_SIZE_64MB, TZASC_REGION_SECURITY_NSRW);
#endif

    // Base of SRAM. Only half of SRAM in Non Secure world
    // First half non secure (16MB) + Second Half secure (16MB) = 32MB of SRAM
#if EDK2_ARMVE_SECURE_SYSTEM
    //Note: Your OS Kernel must be aware of the secure regions before to enable this region
    TZASCSetRegion(ARM_VE_TZASC_BASE,3,TZASC_REGION_ENABLED,
        ARM_VE_SMB_SRAM_BASE,0,
        TZASC_REGION_SIZE_16MB, TZASC_REGION_SECURITY_NSRW);
#else
    TZASCSetRegion(ARM_VE_TZASC_BASE,3,TZASC_REGION_ENABLED,
        ARM_VE_SMB_SRAM_BASE,0,
        TZASC_REGION_SIZE_32MB, TZASC_REGION_SECURITY_NSRW);
#endif

    // Memory Mapped Peripherals. All in non secure world
    TZASCSetRegion(ARM_VE_TZASC_BASE,4,TZASC_REGION_ENABLED,
        ARM_VE_SMB_PERIPH_BASE,0,
        TZASC_REGION_SIZE_64MB, TZASC_REGION_SECURITY_NSRW);

    // MotherBoard Peripherals and On-chip peripherals.
    TZASCSetRegion(ARM_VE_TZASC_BASE,5,TZASC_REGION_ENABLED,
        ARM_VE_SMB_MB_ON_CHIP_PERIPH_BASE,0,
        TZASC_REGION_SIZE_256MB, TZASC_REGION_SECURITY_NSRW);
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
    UINT32 val32  = MmioRead32(ARM_VE_SYS_CFGRW1_REG); //Scc - CFGRW1
    // we remap the DRAM to 0x0
    MmioWrite32(ARM_VE_SYS_CFGRW1_REG, (val32 & 0x0FFFFFFF) | ARM_VE_CFGRW1_REMAP_DRAM);
}

/**
  Initialize controllers that must setup at the early stage

  Some peripherals must be initialized in Secure World.
  For example, some L2x0 requires to be initialized in Secure World

**/
VOID
ArmPlatformSecInitialize (
  VOID
  ) {
  // The L2x0 controller must be intialize in Secure World
  L2x0CacheInit(PcdGet32(PcdL2x0ControllerBase),
      PL310_TAG_LATENCIES(L2x0_LATENCY_8_CYCLES,L2x0_LATENCY_8_CYCLES,L2x0_LATENCY_8_CYCLES),
      PL310_DATA_LATENCIES(L2x0_LATENCY_8_CYCLES,L2x0_LATENCY_8_CYCLES,L2x0_LATENCY_8_CYCLES),
      0,~0, // Use default setting for the Auxiliary Control Register
      FALSE);
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
  PL341DmcInit(&ddr_timings);
  PL301AxiInit(ARM_VE_FAXI_BASE);
}
