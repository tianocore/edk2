/** @file
  Task priority (TPL) functions.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeMain.h"
#include "Event.h"

/**
  Set Interrupt State.

  @param  Enable  The state of enable or disable interrupt

**/
VOID
CoreSetInterruptState (
  IN BOOLEAN      Enable
  )
{
  EFI_STATUS  Status;
  BOOLEAN     InSmm;
  
  if (gCpu == NULL) {
    return;
  }
  if (!Enable) {
    gCpu->DisableInterrupt (gCpu);
    return;
  }
  if (gSmmBase2 == NULL) {
    gCpu->EnableInterrupt (gCpu);
    return;
  }
  Status = gSmmBase2->InSmm (gSmmBase2, &InSmm);
  if (!EFI_ERROR (Status) && !InSmm) {
    gCpu->EnableInterrupt(gCpu);
  }
}


/**
  Raise the task priority level to the new level.
  High level is implemented by disabling processor interrupts.

  @param  NewTpl  New task priority level

  @return The previous task priority level

**/
EFI_TPL
EFIAPI
CoreRaiseTpl (
  IN EFI_TPL      NewTpl
  )
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




/**
  Lowers the task priority to the previous value.   If the new
  priority unmasks events at a higher priority, they are dispatched.

  @param  NewTpl  New, lower, task priority

**/
VOID
EFIAPI
CoreRestoreTpl (
  IN EFI_TPL NewTpl
  )
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
  while (((-2 << NewTpl) & gEventPending) != 0) {
    gEfiCurrentTpl = (UINTN) HighBitSet64 (gEventPending);
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
