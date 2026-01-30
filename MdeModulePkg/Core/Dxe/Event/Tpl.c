/** @file
  Task priority (TPL) functions.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include "Event.h"

//
// Bit mask of TPLs that were interrupted (typically during RestoreTPL's
// event dispatching, though there are reports that the Windows boot loader
// executes stray STIs at TPL_HIGH_LEVEL).  CoreRaiseTpl() sets the
// OldTpl-th bit when it detects it was called from an interrupt handler,
// because the corresponding CoreRestoreTpl() needs different semantics for
// the CPU interrupt state.  See CoreRaiseTpl() and CoreRestoreTpl() below.
//
static UINTN  mInterruptedTplMask = 0;

/**
  Set Interrupt State.

  @param  Enable  The state of enable or disable interrupt

**/
VOID
CoreSetInterruptState (
  IN BOOLEAN  Enable
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
    gCpu->EnableInterrupt (gCpu);
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
  IN EFI_TPL  NewTpl
  )
{
  EFI_TPL  OldTpl;
  BOOLEAN  InterruptState;

  OldTpl = gEfiCurrentTpl;
  if (OldTpl > NewTpl) {
    DEBUG ((DEBUG_ERROR, "FATAL ERROR - RaiseTpl with OldTpl(0x%x) > NewTpl(0x%x)\n", OldTpl, NewTpl));
    ASSERT (FALSE);
  }

  ASSERT (VALID_TPL (NewTpl));

  //
  // If raising to high level, disable interrupts
  //
  if ((NewTpl >= TPL_HIGH_LEVEL) &&  (OldTpl < TPL_HIGH_LEVEL)) {
    //
    // When gCpu is NULL, assume we're not called from an interrupt handler.
    // Calling CoreSetInterruptState() with TRUE is safe as CoreSetInterruptState() will directly return
    // when gCpu is NULL.
    //
    InterruptState = TRUE;
    if (gCpu != NULL) {
      gCpu->GetInterruptState (gCpu, &InterruptState);
    }

    if (InterruptState) {
      //
      // Interrupts are currently enabled.
      // Keep them disabled while at TPL_HIGH_LEVEL.
      //
      CoreSetInterruptState (FALSE);
    } else {
      //
      // Within an interrupt handler.  Save the TPL that was interrupted;
      // It must be higher than the previously interrupted TPL, since
      // CoreRestoreTpl reset all bits up to and including the requested TPL.
      //
      ASSERT ((INTN)OldTpl > HighBitSet64 (mInterruptedTplMask));
      mInterruptedTplMask |= (UINTN)(1U << OldTpl);
    }
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
  IN EFI_TPL  NewTpl
  )
{
  EFI_TPL  OldTpl;
  EFI_TPL  PendingTpl;

  OldTpl = gEfiCurrentTpl;
  if (NewTpl > OldTpl) {
    DEBUG ((DEBUG_ERROR, "FATAL ERROR - RestoreTpl with NewTpl(0x%x) > OldTpl(0x%x)\n", NewTpl, OldTpl));
    ASSERT (FALSE);
  }

  ASSERT (VALID_TPL (NewTpl));

  if ((OldTpl >= TPL_HIGH_LEVEL) &&  (NewTpl < TPL_HIGH_LEVEL)) {
    gEfiCurrentTpl = TPL_HIGH_LEVEL;
  }

  //
  // Dispatch any pending events
  //
  while (gEventPending != 0) {
    PendingTpl = (UINTN)HighBitSet64 (gEventPending);
    if (PendingTpl <= NewTpl) {
      break;
    }

    gEfiCurrentTpl = PendingTpl;

    //
    // If lowering below TPL_HIGH_LEVEL, make sure interrupts are
    // enabled to avoid priority inversions.  Note however that
    // the TPL remains higher than the caller's.  This limits the
    // number of nested interrupts that can happen.
    //
    if (gEfiCurrentTpl < TPL_HIGH_LEVEL) {
      CoreSetInterruptState (TRUE);
    }

    CoreDispatchEventNotifies (gEfiCurrentTpl);
  }

  //
  // The CPU disables interrupts while handlers run, therefore the
  // interrupt handler wants to set TPL_HIGH_LEVEL while it runs,
  // for consistency.  However, when the handler calls RestoreTPL
  // before returning, we want to keep interrupts disabled.  This
  // restores the exact state at the beginning of the handler,
  // before the call to RaiseTPL(): low TPL and interrupts disabled.
  //
  // Disabling interrupts below TPL_HIGH_LEVEL is temporarily
  // inconsistent but, if we did not do so, another interrupt
  // could trigger in the small window between
  // CoreSetInterruptState (TRUE) and the IRET instruction.
  // The nested interrupt would start with the same TPL as the
  // outer one, and nothing would prevents infinite recursion and
  // a stack overflow.
  //
  // Instead, disable interrupts so that nested interrupt handlers
  // will only fire before gEfiCurrentTpl gets its final value.
  // This ensures that nested handlers see a TPL higher than
  // the outer handler, thus bounding the overall stack depth.
  //
  CoreSetInterruptState (FALSE);
  gEfiCurrentTpl = NewTpl;

  if ((INTN)NewTpl <= HighBitSet64 (mInterruptedTplMask)) {
    //
    // We were called from an interrupt handler.  Return with
    // interrupts disabled to ensure that the stack does
    // not blow up.
    //
    ASSERT (mInterruptedTplMask & (UINTN)(1U << NewTpl));
    ASSERT (GetInterruptState () == FALSE);
    mInterruptedTplMask &= (UINTN)(1U << NewTpl) - 1;
  } else if (NewTpl < TPL_HIGH_LEVEL) {
    //
    // Lowering below TPL_HIGH_LEVEL, make sure
    // interrupts are enabled
    //
    CoreSetInterruptState (TRUE);
  }
}
