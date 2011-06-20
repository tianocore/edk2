/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2011 Hewlett Packard Corporation. All rights reserved.<BR>
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

EFI_STATUS
FindMainMemory(
  OUT UINT32    *PhysicalBase,
  OUT UINT32    *Length
  )
{
  EFI_PEI_HOB_POINTERS      NextHob;

  // look at the resource descriptor hobs, choose the first system memory one
  NextHob.Raw = GetHobList ();
  while ((NextHob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, NextHob.Raw)) != NULL) {
    if(NextHob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY)
    {
      *PhysicalBase = (UINT32)NextHob.ResourceDescriptor->PhysicalStart;
      *Length = (UINT32)NextHob.ResourceDescriptor->ResourceLength;
      return EFI_SUCCESS;
    }

    NextHob.Raw = GET_NEXT_HOB (NextHob);
  }

  return EFI_NOT_FOUND;
}

VOID
ConfigureMmu ( VOID )
{
  EFI_STATUS                 Status;
  UINTN                         Idx;
  UINT32                        CacheAttributes;
  UINT32                        SystemMemoryBase;
  UINT32                        SystemMemoryLength;
  UINT32                        SystemMemoryLastAddress;
  ARM_MEMORY_REGION_DESCRIPTOR  MemoryTable[4];
  VOID                          *TranslationTableBase;
  UINTN                         TranslationTableSize;

  if (FeaturePcdGet(PcdCacheEnable) == TRUE) {
    CacheAttributes = DDR_ATTRIBUTES_CACHED;
  } else {
    CacheAttributes = DDR_ATTRIBUTES_UNCACHED;
  }

  Idx = 0;
  
  // Main Memory
  Status = FindMainMemory (&SystemMemoryBase, &SystemMemoryLength);
  ASSERT_EFI_ERROR (Status);

  SystemMemoryLastAddress = SystemMemoryBase + (SystemMemoryLength-1);

  // if system memory does not begin at 0
  if(SystemMemoryBase > 0) {
    MemoryTable[Idx].PhysicalBase = 0;
    MemoryTable[Idx].VirtualBase  = 0;
    MemoryTable[Idx].Length       = SystemMemoryBase;
    MemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    Idx++;
  }

  MemoryTable[Idx].PhysicalBase = SystemMemoryBase;
  MemoryTable[Idx].VirtualBase  = SystemMemoryBase;
  MemoryTable[Idx].Length       = SystemMemoryLength;
  MemoryTable[Idx].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)CacheAttributes;
  Idx++;

  // if system memory does not go to the last address (0xFFFFFFFF)
  if( SystemMemoryLastAddress < MAX_ADDRESS ) {
    MemoryTable[Idx].PhysicalBase = SystemMemoryLastAddress + 1;
    MemoryTable[Idx].VirtualBase  = MemoryTable[Idx].PhysicalBase;
    MemoryTable[Idx].Length       = MAX_ADDRESS - MemoryTable[Idx].PhysicalBase + 1;
    MemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    Idx++;
  }

  // End of Table
  MemoryTable[Idx].PhysicalBase = 0;
  MemoryTable[Idx].VirtualBase  = 0;
  MemoryTable[Idx].Length       = 0;
  MemoryTable[Idx].Attributes   = (ARM_MEMORY_REGION_ATTRIBUTES)0;
   
  DEBUG ((EFI_D_INFO, "Enabling MMU, setting 0x%08x + %d MB to %a\n",
    SystemMemoryBase, SystemMemoryLength/1024/1024,
    (CacheAttributes == DDR_ATTRIBUTES_CACHED) ? "cacheable" : "uncacheable"));

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

  ConfigureMmu();

  return EFI_SUCCESS;
}
