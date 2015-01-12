/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


  This file includes a memory call back function notified when MRC is done,
  following action is performed in this file,
    1. ICH initialization after MRC.
    2. SIO initialization.
    3. Install ResetSystem and FinvFv PPI.
    4. Set MTRR for PEI
    5. Create FV HOB and Flash HOB


**/


#include "CommonHeader.h"
#include "Platform.h"
#include <Ppi/Cache.h>
#include <Library/BaseCryptLib.h>
#include <Library/PciLib.h>
#include "VlvAccess.h"


EFI_PEI_PPI_DESCRIPTOR  mPpiListRecoveryBootMode[] = {
{ (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiBootInRecoveryModePpiGuid,
  NULL
}
};

#if 0
STATIC
EFI_STATUS
GetMemorySize (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  OUT UINT64              *LowMemoryLength,
  OUT UINT64              *HighMemoryLength
  )
{
  EFI_STATUS              Status;
  EFI_PEI_HOB_POINTERS    Hob;

  *HighMemoryLength = 0;
  *LowMemoryLength = 0x100000;
  //
  // Get the HOB list for processing
  //
  Status = (*PeiServices)->GetHobList (PeiServices, (void **)&Hob.Raw);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Collect memory ranges
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
        //
        // Need memory above 1MB to be collected here
        //
        if (Hob.ResourceDescriptor->PhysicalStart >= 0x100000 &&
            Hob.ResourceDescriptor->PhysicalStart < (EFI_PHYSICAL_ADDRESS) 0x100000000) {
          *LowMemoryLength += (UINT64) (Hob.ResourceDescriptor->ResourceLength);
        } else if (Hob.ResourceDescriptor->PhysicalStart >= (EFI_PHYSICAL_ADDRESS) 0x100000000) {
          *HighMemoryLength += (UINT64) (Hob.ResourceDescriptor->ResourceLength);
        }
      }
    }
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return EFI_SUCCESS;
}

#endif
/**
  This function will be called when MRC is done.

  @param  PeiServices General purpose services available to every PEIM.
  @param  NotifyDescriptor Information about the notify event..
  @param  Ppi The notify context.

  @retval EFI_SUCCESS If the function completed successfully.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{

  EFI_STATUS       Status;
  EFI_BOOT_MODE    BootMode;
  UINT32           Pages;
  VOID*            Memory;
  UINTN            Size;

  //
  // Allocate LM memory and configure PDM if enabled by user.
  // ConfigureLM(PeiServices);
  //
  Status = (*PeiServices)->GetBootMode (
                             (const EFI_PEI_SERVICES **)PeiServices,
                             &BootMode
                             );

  if (BootMode != BOOT_ON_S3_RESUME) {
    Size = (PcdGet32 (PcdFlashFvRecovery2Base) - PcdGet32 (PcdFlashFvMainBase)) + FixedPcdGet32(PcdFlashFvRecovery2Size);
    Pages=  Size/0x1000;

    Memory = AllocatePages ( Pages );
    CopyMem(Memory , (VOID *) FixedPcdGet32(PcdFlashFvMainBase) , Size);

    //
    // We don't verify just load
    //
    PeiServicesInstallFvInfoPpi (
      NULL,
      (VOID *) ((UINTN) Memory + (PcdGet32 (PcdFlashFvRecovery2Base) - PcdGet32 (PcdFlashFvMainBase))),
      PcdGet32 (PcdFlashFvRecovery2Size),
      NULL,
      NULL
      );

    PeiServicesInstallFvInfoPpi (
      NULL,
      (VOID *)  Memory,
      PcdGet32 (PcdFlashFvMainSize),
      NULL,
      NULL
      );

  }

  if (BootMode == BOOT_ON_S3_RESUME) {
    PeiServicesInstallFvInfoPpi (
    NULL,
    (VOID *) (UINTN) (PcdGet32 (PcdFlashFvRecovery2Base)),
    PcdGet32 (PcdFlashFvRecovery2Size),
    NULL,
    NULL
    );
  }

  return EFI_SUCCESS;
}
