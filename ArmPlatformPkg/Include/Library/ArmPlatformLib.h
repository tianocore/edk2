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

#ifndef _ARMPLATFORMLIB_H_
#define _ARMPLATFORMLIB_H_

//
// The package level header files this module uses
//
#include <PiPei.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Ppi/MasterBootMode.h>
#include <Ppi/BootInRecoveryMode.h>
#include <Guid/MemoryTypeInformation.h>

#include <Library/ArmLib.h>
#include <ArmPlatform.h>

/**
  This structure is used by ArmVExpressGetEfiMemoryMap to describes a region of the EFI memory map

  Every EFI regions of the system memory described by their physical start address and their size
  can have different attributes. Some regions can be tested and other untested.

**/
typedef struct {
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute;
  EFI_PHYSICAL_ADDRESS         PhysicalStart;
  UINT64                       NumberOfBytes;
} ARM_SYSTEM_MEMORY_REGION_DESCRIPTOR;

/**
  Called at the early stage of the Boot phase to know if the memory has already been initialized

  Running the code from the reset vector does not mean we start from cold boot. In some case, we
  can go through this code with the memory already initialized.
  Because this function is called at the early stage, the implementation must not use the stack.
  Its implementation must probably done in assembly to ensure this requirement.

  @return   Return the condition value into the 'Z' flag

**/
VOID ArmPlatformIsMemoryInitialized(VOID);

/**
  Initialize the memory where the initial stacks will reside

  This memory can contain the initial stacks (Secure and Secure Monitor stacks).
  In some platform, this region is already initialized and the implementation of this function can
  do nothing. This memory can also represent the Secure RAM.
  This function is called before the satck has been set up. Its implementation must ensure the stack
  pointer is not used (probably required to use assembly language)

**/
VOID ArmPlatformInitializeBootMemory(VOID);

/**
  Return the current Boot Mode

  This function returns the boot reason on the platform

  @return   Return the current Boot Mode of the platform

**/
EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  );

/**
  Initialize controllers that must setup at the early stage

  Some peripherals must be initialized in Secure World.
  For example, some L2x0 requires to be initialized in Secure World

**/
VOID
ArmPlatformInitialize (
  VOID
  );

/**
  Initialize the system (or sometimes called permanent) memory

  This memory is generally represented by the DRAM.

**/
VOID ArmPlatformInitializeSystemMemory(VOID);

/**
  Remap the memory at 0x0

  Some platform requires or gives the ability to remap the memory at the address 0x0.
  This function can do nothing if this feature is not relevant to your platform.

**/
VOID ArmPlatformBootRemapping(VOID);

/**
  Return if Trustzone is supported by your platform

  A non-zero value must be returned if you want to support a Secure World on your platform.
  ArmPlatformTrustzoneInit() will later set up the secure regions.
  This function can return 0 even if Trustzone is supported by your processor. In this case,
  the platform will continue to run in Secure World.

  @return   A non-zero value if Trustzone supported.

**/
UINTN ArmPlatformTrustzoneSupported(VOID);

/**
  Initialize the Secure peripherals and memory regions

  If Trustzone is supported by your platform then this function makes the required initialization
  of the secure peripherals and memory regions.

**/
VOID ArmPlatformTrustzoneInit(VOID);

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
    );

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

**/
VOID ArmPlatformGetVirtualMemoryMap (
    OUT ARM_MEMORY_REGION_DESCRIPTOR** VirtualMemoryMap
);

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
  );

#endif
