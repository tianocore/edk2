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
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>

// DDR attributes
#define DDR_ATTRIBUTES_CACHED           ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED         ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED
#define DDR_ATTRIBUTES_SECURE_CACHED    ARM_MEMORY_REGION_ATTRIBUTE_SECURE_WRITE_BACK
#define DDR_ATTRIBUTES_SECURE_UNCACHED  ARM_MEMORY_REGION_ATTRIBUTE_SECURE_UNCACHED_UNBUFFERED

/**
  Return the information about the memory region in permanent memory used by PEI

  One of the PEI Module must install the permament memory used by PEI. This function returns the
  information about this region for your platform to this PEIM module.

  @param[out]   PeiMemoryBase       Base of the memory region used by PEI core and modules
  @param[out]   PeiMemorySize       Size of the memory region used by PEI core and modules

**/
VOID ArmPlatformGetPeiMemory (
    OUT UINTN*                                   PeiMemoryBase,
    OUT UINTN*                                   PeiMemorySize
    ) {
    ASSERT((PeiMemoryBase != NULL) && (PeiMemorySize != NULL));
    
    *PeiMemoryBase = ARM_EB_DRAM_BASE + ARM_EB_EFI_FIX_ADDRESS_REGION_SZ;
    *PeiMemorySize = ARM_EB_EFI_MEMORY_REGION_SZ;
}

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

**/
VOID ArmPlatformGetVirtualMemoryMap(ARM_MEMORY_REGION_DESCRIPTOR** VirtualMemoryMap) {
    UINT32                        CacheAttributes;
    BOOLEAN                       bTrustzoneSupport = FALSE;
    UINTN                         Index = 0;
    ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;

    ASSERT(VirtualMemoryMap != NULL);

    VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR*)AllocatePages(sizeof(ARM_MEMORY_REGION_DESCRIPTOR) * 9);
    if (VirtualMemoryTable == NULL) {
        return;
    }

    if (FeaturePcdGet(PcdCacheEnable) == TRUE) {
        CacheAttributes = (bTrustzoneSupport ? DDR_ATTRIBUTES_CACHED : DDR_ATTRIBUTES_SECURE_CACHED);
    } else {
        CacheAttributes = (bTrustzoneSupport ? DDR_ATTRIBUTES_UNCACHED : DDR_ATTRIBUTES_SECURE_UNCACHED);
    }

    // ReMap (Either NOR Flash or DRAM)
    VirtualMemoryTable[Index].PhysicalBase = ARM_EB_REMAP_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_EB_REMAP_BASE;
    VirtualMemoryTable[Index].Length       = ARM_EB_REMAP_SZ;
    VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;

    // DDR
    VirtualMemoryTable[++Index].PhysicalBase = ARM_EB_DRAM_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_EB_DRAM_BASE;
    VirtualMemoryTable[Index].Length       = ARM_EB_DRAM_SZ;
    VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;

    // SMC CS7
    VirtualMemoryTable[++Index].PhysicalBase = ARM_EB_SMB_MB_ON_CHIP_PERIPH_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_EB_SMB_MB_ON_CHIP_PERIPH_BASE;
    VirtualMemoryTable[Index].Length       = ARM_EB_SMB_MB_ON_CHIP_PERIPH_SZ;
    VirtualMemoryTable[Index].Attributes   = (bTrustzoneSupport ? ARM_MEMORY_REGION_ATTRIBUTE_DEVICE : ARM_MEMORY_REGION_ATTRIBUTE_SECURE_DEVICE);

    // SMB CS0-CS1 - NOR Flash 1 & 2
    VirtualMemoryTable[++Index].PhysicalBase = ARM_EB_SMB_NOR_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_EB_SMB_NOR_BASE;
    VirtualMemoryTable[Index].Length       = ARM_EB_SMB_NOR_SZ + ARM_EB_SMB_DOC_SZ;
    VirtualMemoryTable[Index].Attributes   = (bTrustzoneSupport ? ARM_MEMORY_REGION_ATTRIBUTE_DEVICE : ARM_MEMORY_REGION_ATTRIBUTE_SECURE_DEVICE);

    // SMB CS2 - SRAM
    VirtualMemoryTable[++Index].PhysicalBase = ARM_EB_SMB_SRAM_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_EB_SMB_SRAM_BASE;
    VirtualMemoryTable[Index].Length       = ARM_EB_SMB_SRAM_SZ;
    VirtualMemoryTable[Index].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;

    // SMB CS3-CS6 - Motherboard Peripherals
    VirtualMemoryTable[++Index].PhysicalBase = ARM_EB_SMB_PERIPH_BASE;
    VirtualMemoryTable[Index].VirtualBase  = ARM_EB_SMB_PERIPH_BASE;
    VirtualMemoryTable[Index].Length       = ARM_EB_SMB_PERIPH_SZ;
    VirtualMemoryTable[Index].Attributes   = (bTrustzoneSupport ? ARM_MEMORY_REGION_ATTRIBUTE_DEVICE : ARM_MEMORY_REGION_ATTRIBUTE_SECURE_DEVICE);

    // If a Logic Tile is connected to The ARM Versatile Express Motherboard
    if (MmioRead32(ARM_EB_SYS_PROCID1_REG) != 0) {
        VirtualMemoryTable[++Index].PhysicalBase = ARM_EB_LOGIC_TILE_BASE;
        VirtualMemoryTable[Index].VirtualBase  = ARM_EB_LOGIC_TILE_BASE;
        VirtualMemoryTable[Index].Length       = ARM_EB_LOGIC_TILE_SZ;
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
VOID ArmPlatformGetEfiMemoryMap (
    OUT ARM_SYSTEM_MEMORY_REGION_DESCRIPTOR** EfiMemoryMap
) {
    EFI_RESOURCE_ATTRIBUTE_TYPE     Attributes;
    UINT64                          MemoryBase;
    UINTN                           Index = 0;
    ARM_SYSTEM_MEMORY_REGION_DESCRIPTOR  *EfiMemoryTable;

    ASSERT(EfiMemoryMap != NULL);

    EfiMemoryTable = (ARM_SYSTEM_MEMORY_REGION_DESCRIPTOR*)AllocatePages(sizeof(ARM_SYSTEM_MEMORY_REGION_DESCRIPTOR) * 6);

    Attributes =
    (
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED
    );
    MemoryBase = ARM_EB_DRAM_BASE;
  
    // Memory Reserved for fixed address allocations (such as Exception Vector Table)
    EfiMemoryTable[Index].ResourceAttribute = Attributes;
    EfiMemoryTable[Index].PhysicalStart = MemoryBase;
    EfiMemoryTable[Index].NumberOfBytes = ARM_EB_EFI_FIX_ADDRESS_REGION_SZ;
    
    MemoryBase += ARM_EB_EFI_FIX_ADDRESS_REGION_SZ;

    // Memory declared to PEI as permanent memory for PEI and DXE
    EfiMemoryTable[++Index].ResourceAttribute = Attributes;
    EfiMemoryTable[Index].PhysicalStart = MemoryBase;
    EfiMemoryTable[Index].NumberOfBytes = ARM_EB_EFI_MEMORY_REGION_SZ;

    MemoryBase += ARM_EB_EFI_MEMORY_REGION_SZ;

    // We must reserve the memory used by the Firmware Volume copied in DRAM at 0x80000000
    if (FeaturePcdGet(PcdStandalone) == FALSE) {
        // Chunk between the EFI Memory region and the firmware
        EfiMemoryTable[++Index].ResourceAttribute = Attributes;
        EfiMemoryTable[Index].PhysicalStart = MemoryBase;
        EfiMemoryTable[Index].NumberOfBytes = PcdGet32(PcdNormalFdBaseAddress) - MemoryBase;

        // Chunk reserved by the firmware in DRAM
        EfiMemoryTable[++Index].ResourceAttribute = Attributes & (~EFI_RESOURCE_ATTRIBUTE_PRESENT);
        EfiMemoryTable[Index].PhysicalStart = PcdGet32(PcdNormalFdBaseAddress);
        EfiMemoryTable[Index].NumberOfBytes = PcdGet32(PcdNormalFdSize);

        MemoryBase = PcdGet32(PcdNormalFdBaseAddress) + PcdGet32(PcdNormalFdSize);
    }
      
    // We allocate all the remain memory as untested system memory
    EfiMemoryTable[++Index].ResourceAttribute = Attributes & (~EFI_RESOURCE_ATTRIBUTE_TESTED);
    EfiMemoryTable[Index].PhysicalStart = MemoryBase;
    EfiMemoryTable[Index].NumberOfBytes = ARM_EB_DRAM_SZ - (MemoryBase-ARM_EB_DRAM_BASE);

    EfiMemoryTable[++Index].ResourceAttribute = 0;
    EfiMemoryTable[Index].PhysicalStart = 0;
    EfiMemoryTable[Index].NumberOfBytes = 0;

    *EfiMemoryMap = EfiMemoryTable;
}
