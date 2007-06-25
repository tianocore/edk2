/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    tpl.c

Abstract:

    Task priority function    

--*/

#include <DxeMain.h>

STATIC
VOID
CoreSetInterruptState (
  IN BOOLEAN      Enable
  )
/*++

Routine Description:
  
  Set Interrupt State
  
Arguments:
  
  Enable - The state of enable or disable interrupt
  
Returns:
  
  None

--*/

{
  if (gCpu != NULL) {
    if (Enable) {
      gCpu->EnableInterrupt(gCpu);
    } else {
      gCpu->DisableInterrupt(gCpu);
    }
  }
}

//
// Return the highest set bit
//
UINTN
CoreHighestSetBit (
  IN UINTN     Number
  )
/*++

Routine Description:
  
  Return the highest set bit
  
Arguments:
  
  Number - The value to check
  
Returns:
  
  Bit position of the highest set bit

--*/
{
  UINTN   msb;
  
  msb = 31;
  while ((msb > 0) && ((Number & (UINTN)(1 << msb)) == 0)) {
    msb--;
  }

  return msb;
}



EFI_TPL
EFIAPI
CoreRaiseTpl (
  IN EFI_TPL      NewTpl
  )
/*++

Routine Description:

  Raise the task priority level to the new level.
  High level is implemented by disabling processor interrupts.

Arguments:

  NewTpl  - New task priority level
    
Returns:

  The previous task priority level

--*/
{
  EFI_TPL     OldTpl;

  OldTpl = gEfiCurrentTpl;
  ASSERT (OldTpl <= NewTpl);
  ASSERT (VALID_TPL (NewTpl));

  //
  // If raising to high level, disable interrupts
  //
  if (NewTpl >= TPL_HIGH_LEVEL  &&  OldTpl < TPL_HIGH_LEVEL) {
    CoreSetInterruptState (FALSE);
  }

  //
  // Set the new value
  //
  gEfiCurrentTpl = NewTpl;

  return OldTpl;
}



VOID
EFIAPI
CoreRestoreTpl (
  IN EFI_TPL NewTpl
  )
/*++

Routine Description:

  Lowers the task priority to the previous value.   If the new 
  priority unmasks events at a higher priority, they are dispatched.

Arguments:

  NewTpl  - New, lower, task priority
    
Returns:

  None

--*/
{
  EFI_TPL     OldTpl;

  OldTpl = gEfiCurrentTpl;
  ASSERT (NewTpl <= OldTpl);
  ASSERT (VALID_TPL (NewTpl));

  //
  // If lowering below HIGH_LEVEL, make sure
  // interrupts are enabled
  //

  if (OldTpl >= TPL_HIGH_LEVEL  &&  NewTpl < TPL_HIGH_LEVEL) {
    gEfiCurrentTpl = TPL_HIGH_LEVEL;  
  }

  //
  // Dispatch any pending events
  //

  while ((-2 << NewTpl) & gEventPending) {
    gEfiCurrentTpl = CoreHighestSetBit (gEventPending);
    if (gEfiCurrentTpl < TPL_HIGH_LEVEL) {
      CoreSetInterruptState (TRUE);
    }
    CoreDispatchEventNotifies (gEfiCurrentTpl);
  }

  //
  // Set the new value
  //

  gEfiCurrentTpl = NewTpl;

  //
  // If lowering below HIGH_LEVEL, make sure
  // interrupts are enabled
  //
  if (gEfiCurrentTpl < TPL_HIGH_LEVEL) {
    CoreSetInterruptState (TRUE);
  }

}
