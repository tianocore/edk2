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

#include <Library/IoLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/PrintLib.h>
#include <Library/ArmLib.h>
#include <Library/SerialPortLib.h>
#include <Chipset/ArmV7.h>

#include "PrePeiCore.h"

EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI   mSecTemporaryRamSupportPpi = {SecTemporaryRamSupport};

EFI_PEI_PPI_DESCRIPTOR      gSecPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiTemporaryRamSupportPpiGuid,
    &mSecTemporaryRamSupportPpi
  }
};

VOID
CEntryPoint (
  IN  UINTN                     MpId,
  IN  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint
  )
{
  //Clean Data cache
  ArmCleanInvalidateDataCache();

  //Invalidate instruction cache
  ArmInvalidateInstructionCache();

  // Enable Instruction & Data caches
  ArmEnableDataCache ();
  ArmEnableInstructionCache ();

  //
  // Note: Doesn't have to Enable CPU interface in non-secure world,
  // as Non-secure interface is already enabled in Secure world.
  //

  // Write VBAR - The Vector table must be 32-byte aligned
  ASSERT(((UINT32)PeiVectorTable & ((1 << 5)-1)) == 0);
  ArmWriteVBar((UINT32)PeiVectorTable);

  //Note: The MMU will be enabled by MemoryPeim. Only the primary core will have the MMU on.

  //If not primary Jump to Secondary Main
  if (IS_PRIMARY_CORE(MpId)) {
    // Initialize the Debug Agent for Source Level Debugging
    InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, NULL, NULL);
    SaveAndSetDebugTimerInterrupt (TRUE);

    // Goto primary Main.
    PrimaryMain (PeiCoreEntryPoint);
  } else {
    SecondaryMain (MpId);
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

VOID
PeiCommonExceptionEntry (
  IN UINT32 Entry,
  IN UINT32 LR
  )
{
  CHAR8           Buffer[100];
  UINTN           CharCount;

  switch (Entry) {
  case 0:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Reset Exception at 0x%X\n\r",LR);
    break;
  case 1:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Undefined Exception at 0x%X\n\r",LR);
    break;
  case 2:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"SWI Exception at 0x%X\n\r",LR);
    break;
  case 3:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"PrefetchAbort Exception at 0x%X\n\r",LR);
    break;
  case 4:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"DataAbort Exception at 0x%X\n\r",LR);
    break;
  case 5:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Reserved Exception at 0x%X\n\r",LR);
    break;
  case 6:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"IRQ Exception at 0x%X\n\r",LR);
    break;
  case 7:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"FIQ Exception at 0x%X\n\r",LR);
    break;
  default:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Unknown Exception at 0x%X\n\r",LR);
    break;
  }
  SerialPortWrite ((UINT8 *) Buffer, CharCount);
  while(1);
}
