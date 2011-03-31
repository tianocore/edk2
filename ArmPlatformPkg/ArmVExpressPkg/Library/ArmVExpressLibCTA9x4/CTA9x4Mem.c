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

#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>

// DDR attributes
#define DDR_ATTRIBUTES_CACHED           ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED         ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED
#define DDR_ATTRIBUTES_SECURE_CACHED    ARM_MEMORY_REGION_ATTRIBUTE_SECURE_WRITE_BACK
#define DDR_ATTRIBUTES_SECURE_UNCACHED  ARM_MEMORY_REGION_ATTRIBUTE_SECURE_UNCACHED_UNBUFFERED

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

**/
VOID ArmPlatformGetVirtualMemoryMap(ARM_MEMORY_REGION_DESCRIPTOR** VirtualMemoryMap) {
    UINT32                        val32;
    UINT32                        CacheAttributes;
    BOOLEAN                       bTrustzoneSupport;
    UINTN                         Index = 0;
    ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;

    ASSERT(VirtualMemoryMap != NULL);

    VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR*)AllocatePages(sizeof(ARM_MEMORY_REGION_DESCRIPTOR) * 9);
    if (VirtualMemoryTable == NULL) {
        return;
    }

    // Check if SMC TZASC is enabled. If Trustzone not enabled then all the entries remain in Secure World.
    // As this value can be changed in the Board Configuration file, the UEFI firmware needs to work for both case
    val32 = MmioRead32(ARM_VE_SYS_CFGRW1_REG);
    if (ARM_VE_CFGRW1_TZASC_EN_BIT_MASK & val32) {
        bTrustzoneSupport = TRUE;
    } else {
        bTrustzoneSupport = FALSE;
    }

    if (FeaturePcdGet(PcdCacheEnable) == TRUE) {
        CacheAttributes = (bTrustzoneSupport ? DDR_ATTRIBUTES_CACHED : DDR_ATTRIBUTES_SECURE_CACHED);
    } else {
        CacheAttributes = (bTrustzoneSupport ? DDR_ATTRIBUTES_UNCACHED : DDR_ATTRIBUTES_SECURE_UNCACHED);
    }

    // ReMap (Either NOR Flash or DRAM)
    VirtualMemoryTable[Index].PhysicalBase = ARM_VE_REMAP_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_VE_REMAP_BASE;
    VirtualMemoryTable[Index].Length       = ARM_VE_REMAP_SZ;
    VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;

    // DDR
    VirtualMemoryTable[++Index].PhysicalBase = ARM_VE_DRAM_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_VE_DRAM_BASE;
    VirtualMemoryTable[Index].Length       = ARM_VE_DRAM_SZ;
    VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;

    // SMC CS7
    VirtualMemoryTable[++Index].PhysicalBase = ARM_VE_SMB_MB_ON_CHIP_PERIPH_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_VE_SMB_MB_ON_CHIP_PERIPH_BASE;
    VirtualMemoryTable[Index].Length       = ARM_VE_SMB_MB_ON_CHIP_PERIPH_SZ;
    VirtualMemoryTable[Index].Attributes   = (bTrustzoneSupport ? ARM_MEMORY_REGION_ATTRIBUTE_DEVICE : ARM_MEMORY_REGION_ATTRIBUTE_SECURE_DEVICE);

    // SMB CS0-CS1 - NOR Flash 1 & 2
    VirtualMemoryTable[++Index].PhysicalBase = ARM_VE_SMB_NOR0_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_VE_SMB_NOR0_BASE;
    VirtualMemoryTable[Index].Length       = ARM_VE_SMB_NOR0_SZ + ARM_VE_SMB_NOR1_SZ;
    VirtualMemoryTable[Index].Attributes   = (bTrustzoneSupport ? ARM_MEMORY_REGION_ATTRIBUTE_DEVICE : ARM_MEMORY_REGION_ATTRIBUTE_SECURE_DEVICE);

    // SMB CS2 - SRAM
    VirtualMemoryTable[++Index].PhysicalBase = ARM_VE_SMB_SRAM_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_VE_SMB_SRAM_BASE;
    VirtualMemoryTable[Index].Length       = ARM_VE_SMB_SRAM_SZ;
    VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;

    // SMB CS3-CS6 - Motherboard Peripherals
    VirtualMemoryTable[++Index].PhysicalBase = ARM_VE_SMB_PERIPH_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_VE_SMB_PERIPH_BASE;
    VirtualMemoryTable[Index].Length       = ARM_VE_SMB_PERIPH_SZ;
    VirtualMemoryTable[Index].Attributes   = (bTrustzoneSupport ? ARM_MEMORY_REGION_ATTRIBUTE_DEVICE : ARM_MEMORY_REGION_ATTRIBUTE_SECURE_DEVICE);

    // If a Logic Tile is connected to The ARM Versatile Express Motherboard
    if (MmioRead32(ARM_VE_SYS_PROCID1_REG) != 0) {
        VirtualMemoryTable[++Index].PhysicalBase = ARM_VE_EXT_AXI_BASE;
        VirtualMemoryTable[Index].VirtualBase  = ARM_VE_EXT_AXI_BASE;
        VirtualMemoryTable[Index].Length       = ARM_VE_EXT_AXI_SZ;
        VirtualMemoryTable[Index].Attributes   = (bTrustzoneSupport ? ARM_MEMORY_REGION_ATTRIBUTE_DEVICE : ARM_MEMORY_REGION_ATTRIBUTE_SECURE_DEVICE);
    }

    // End of Table
    VirtualMemoryTable[++Index].PhysicalBase = 0;
    VirtualMemoryTable[Index].VirtualBase  = 0;
    VirtualMemoryTable[Index].Length       = 0;
    VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)0;

    *VirtualMemoryMap = VirtualMemoryTable;
}

/**
  Return the EFI Memory Map of your platform

  This EFI Memory Map of the System Memory is used by MemoryInitPei module to create the Resource
  Descriptor HOBs used by DXE core.

  @param[out]   EfiMemoryMap        Array of ARM_SYSTEM_MEMORY_REGION_DESCRIPTOR describing an
                                    EFI Memory region. This array must be ended by a zero-filled entry

**/
EFI_STATUS
ArmPlatformGetAdditionalSystemMemory (
  OUT ARM_SYSTEM_MEMORY_REGION_DESCRIPTOR** EfiMemoryMap
  )
{
  return EFI_UNSUPPORTED;
}
