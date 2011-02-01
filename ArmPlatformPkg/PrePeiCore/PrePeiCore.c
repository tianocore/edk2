/** @file
*  Main file supporting the transition to PEI Core in Normal World for Versatile Express
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

#include <PiPei.h>
#include <Ppi/TemporaryRamSupport.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ArmLib.h>
#include <Chipset/ArmV7.h>

EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );

VOID
SecSwitchStack (
  INTN    StackDelta
  );

TEMPORARY_RAM_SUPPORT_PPI   mSecTemporaryRamSupportPpi = {SecTemporaryRamSupport};

EFI_PEI_PPI_DESCRIPTOR      gSecPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiTemporaryRamSupportPpiGuid,
    &mSecTemporaryRamSupportPpi
  }
};

// Vector Table for Pei Phase
VOID  PeiVectorTable (VOID);


VOID
CEntryPoint (
  IN  UINTN                     CoreId,
  IN  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint
  )
{
  //Clean Data cache
  ArmCleanInvalidateDataCache();

  //Invalidate instruction cache
  ArmInvalidateInstructionCache();

  // Enable Instruction & Data caches
  ArmEnableDataCache();
  ArmEnableInstructionCache();

  //
  // Note: Doesn't have to Enable CPU interface in non-secure world,
  // as Non-secure interface is already enabled in Secure world.
  //

  // Write VBAR - The Vector table must be 32-byte aligned
  ASSERT(((UINT32)PeiVectorTable & ((1 << 5)-1)) == 0);
  ArmWriteVBar((UINT32)PeiVectorTable);

  //Note: The MMU will be enabled by MemoryPeim. Only the primary core will have the MMU on.

  //If not primary Jump to Secondary Main
  if(0 == CoreId) {
    //Goto primary Main.
    primary_main(PeiCoreEntryPoint);
  } else {
    secondary_main(CoreId);
  }

  // PEI Core should always load and never return
  ASSERT (FALSE);
}

EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  //
  // Migrate the whole temporary memory to permenent memory.
  // 
  CopyMem (
    (VOID*)(UINTN)PermanentMemoryBase, 
    (VOID*)(UINTN)TemporaryMemoryBase, 
    CopySize
    );

  SecSwitchStack((UINTN)(PermanentMemoryBase - TemporaryMemoryBase));

  return EFI_SUCCESS;
}

VOID PeiCommonExceptionEntry(UINT32 Entry, UINT32 LR) {
  switch (Entry) {
  case 0:
    DEBUG((EFI_D_ERROR,"Reset Exception at 0x%X\n",LR));
    break;
  case 1:
    DEBUG((EFI_D_ERROR,"Undefined Exception at 0x%X\n",LR));
    break;
  case 2:
    DEBUG((EFI_D_ERROR,"SWI Exception at 0x%X\n",LR));
    break;
  case 3:
    DEBUG((EFI_D_ERROR,"PrefetchAbort Exception at 0x%X\n",LR));
    break;
  case 4:
    DEBUG((EFI_D_ERROR,"DataAbort Exception at 0x%X\n",LR));
    break;
  case 5:
    DEBUG((EFI_D_ERROR,"Reserved Exception at 0x%X\n",LR));
    break;
  case 6:
    DEBUG((EFI_D_ERROR,"IRQ Exception at 0x%X\n",LR));
    break;
  case 7:
    DEBUG((EFI_D_ERROR,"FIQ Exception at 0x%X\n",LR));
    break;
  default:
    DEBUG((EFI_D_ERROR,"Unknown Exception at 0x%X\n",LR));
    break;
  }
  while(1);
}
