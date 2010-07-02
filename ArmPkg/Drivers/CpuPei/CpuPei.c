/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MemoryInit.c
   
Abstract:

  PEIM to provide fake memory init

**/



//
// The package level header files this module uses
//
#include <PiPei.h>
//
// The protocols, PPI and GUID defintions for this module
//

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/ArmLib.h>

//
// Module globals
//

#define DDR_ATTRIBUTES_CACHED                ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED              ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

VOID
JamArmMmuConfig ( VOID )
{
  UINT32                        CacheAttributes;
  ARM_MEMORY_REGION_DESCRIPTOR  MemoryTable[3];
  VOID                          *TranslationTableBase;
  UINTN                         TranslationTableSize;

  if (FeaturePcdGet(PcdCacheEnable) == TRUE) {
    CacheAttributes = DDR_ATTRIBUTES_CACHED;
  } else {
    CacheAttributes = DDR_ATTRIBUTES_UNCACHED;
  }

  // DDR
  MemoryTable[0].PhysicalBase = 0;
  MemoryTable[0].VirtualBase  = 0;
  MemoryTable[0].Length       = 0x10000000;
  MemoryTable[0].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;

  // SOC Registers. L3 interconnects
  MemoryTable[1].PhysicalBase = 0x10000000;
  MemoryTable[1].VirtualBase  = 0x10000000;
  MemoryTable[1].Length       = 0xF0000000;
  MemoryTable[1].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

  // End of Table
  MemoryTable[2].PhysicalBase = 0;
  MemoryTable[2].VirtualBase  = 0;
  MemoryTable[2].Length       = 0;
  MemoryTable[2].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)0;
  
  ArmConfigureMmu (MemoryTable, &TranslationTableBase, &TranslationTableSize);
  
  BuildMemoryAllocationHob((EFI_PHYSICAL_ADDRESS)(UINTN)TranslationTableBase, TranslationTableSize, EfiBootServicesData);
}


EFI_STATUS
EFIAPI
InitializeCpuPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
/*++

Routine Description:

  

Arguments:

  FileHandle  - Handle of the file being invoked.
  PeiServices - Describes the list of possible PEI Services.
    
Returns:

  Status -  EFI_SUCCESS if the boot mode could be set

--*/
{
  // Enable program flow prediction, if supported.
  ArmEnableBranchPrediction ();

  JamArmMmuConfig();

  return EFI_SUCCESS;
}
