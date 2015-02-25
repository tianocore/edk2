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

#include <Library/ArmLib.h>

/**
  This structure is used to describe a region of the EFI memory map

  Every EFI regions of the system memory described by their physical start address and their size
  can have different attributes. Some regions can be tested and other untested.

**/
typedef struct {
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute;
  EFI_PHYSICAL_ADDRESS         PhysicalStart;
  UINT64                       NumberOfBytes;
} ARM_SYSTEM_MEMORY_REGION_DESCRIPTOR;

/**
  Return the core per cluster. The method may differ per core type

  This function might be called from assembler before any stack is set.

  @return   Return the core count per cluster

**/
UINTN
ArmGetCpuCountPerCluster (
  VOID
  );

/**
  Return the core position from the value of its MpId register

  This function returns the core position from the position 0 in the processor.
  This function might be called from assembler before any stack is set.

  @return   Return the core position

**/
UINTN
ArmPlatformGetCorePosition (
  IN UINTN MpId
  );

/**
  Return a non-zero value if the callee is the primary core

  This function returns a non-zero value if the callee is the primary core.
  The primary core is the core responsible to initialize the hardware and run UEFI.
  This function might be called from assembler before any stack is set.

  @return   Return a non-zero value if the callee is the primary core.

**/
UINTN
ArmPlatformIsPrimaryCore (
  IN UINTN MpId
  );

/**
  Return the MpId of the primary core

  This function returns the MpId of the primary core.
  This function might be called from assembler before any stack is set.

  @return   Return the MpId of the primary core

**/
UINTN
ArmPlatformGetPrimaryCoreMpId (
  VOID
  );

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
  First platform specific function to be called in the PEI phase

  This function is actually the first function called by the PrePi
  or PrePeiCore modules. It allows to retrieve arguments passed to
  the UEFI firmware through the CPU registers.

  This function might be written into assembler as no stack are set
  when the function is invoked.

**/
VOID
ArmPlatformPeiBootAction (
  VOID
  );

/**
  Initialize controllers that must setup in the normal world

  This function is called by the ArmPlatformPkg/PrePi or ArmPlatformPkg/PlatformPei
  in the PEI phase.

**/
RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN                     MpId
  );

/**
  Initialize the system (or sometimes called permanent) memory

  This memory is generally represented by the DRAM.

**/
VOID
ArmPlatformInitializeSystemMemory (
  VOID
  );

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

**/
VOID
ArmPlatformGetVirtualMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR** VirtualMemoryMap
  );

/**
  Return the Platform specific PPIs

  This function exposes the Platform Specific PPIs. They can be used by any PrePi modules or passed
  to the PeiCore by PrePeiCore.

  @param[out]   PpiListSize         Size in Bytes of the Platform PPI List
  @param[out]   PpiList             Platform PPI List

**/
VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  );

#endif
