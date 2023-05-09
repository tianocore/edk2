/** @file
  Handle raising and lowering TPL from within nested interrupt handlers.

  Allows interrupt handlers to safely raise and lower the TPL to
  dispatch event notifications, correctly allowing for nested
  interrupts to occur without risking stack exhaustion.

  Copyright (C) 2022, Fen Systems Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/NestedInterruptTplLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "Iret.h"

/**
  Raise the task priority level to TPL_HIGH_LEVEL.

  @param  None.

  @return The task priority level at which the interrupt occurred.
**/
EFI_TPL
EFIAPI
NestedInterruptRaiseTPL (
  VOID
  )
{
  EFI_TPL  InterruptedTPL;

  //
  // Raise TPL and assert that we were called from within an interrupt
  // handler (i.e. with interrupts already disabled before raising the
  // TPL).
  //
  ASSERT (GetInterruptState () == FALSE);
  InterruptedTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // At TPL_HIGH_LEVEL, CPU interrupts are disabled (as per the UEFI
  // specification) and so we should never encounter a situation in
  // which InterruptedTPL==TPL_HIGH_LEVEL.  The specification also
  // restricts usage of TPL_HIGH_LEVEL to the firmware itself.
  //
  // However, nothing actually prevents a UEFI application from
  // invalidly calling gBS->RaiseTPL(TPL_HIGH_LEVEL) and then
  // violating the invariant by enabling interrupts via the STI or
  // equivalent instruction.  Some versions of the Microsoft Windows
  // bootloader are known to do this.
  //
  if (InterruptedTPL >= TPL_HIGH_LEVEL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Interrupts enabled at TPL_HIGH_LEVEL!\n"));
  }

  return InterruptedTPL;
}

/**
  Lower the task priority back to the value at which the interrupt
  occurred.

  This is unfortunately messy.  UEFI requires us to support nested
  interrupts, but provides no way for an interrupt handler to call
  RestoreTPL() without implicitly re-enabling interrupts.  In a
  virtual machine, it is possible for a large burst of interrupts to
  arrive.  We must prevent such a burst from leading to stack
  exhaustion, while continuing to allow nested interrupts to occur.

  Since nested interrupts are permitted, an interrupt handler may be
  invoked as an inner interrupt handler while an outer instance of the
  same interrupt handler is still inside its call to RestoreTPL().

  To avoid stack exhaustion, this call may therefore (when provably
  safe to do so) defer the actual TPL lowering to be performed by an
  outer instance of the same interrupt handler.

  @param InterruptedTPL        The task priority level at which the interrupt
                               occurred, as previously returned from
                               NestedInterruptRaiseTPL().

  @param SystemContext         A pointer to the system context when the
                               interrupt occurred.

  @param IsrState              A pointer to the state shared between all
                               invocations of the nested interrupt handler.
**/
VOID
EFIAPI
NestedInterruptRestoreTPL (
  IN EFI_TPL                     InterruptedTPL,
  IN OUT EFI_SYSTEM_CONTEXT      SystemContext,
  IN OUT NESTED_INTERRUPT_STATE  *IsrState
  )
{
  EFI_TPL  SavedInProgressRestoreTPL;
  BOOLEAN  DeferredRestoreTPL;

  //
  // If the TPL at which this interrupt occurred is equal to that of
  // the in-progress RestoreTPL() for an outer instance of the same
  // interrupt handler, then that outer handler's call to RestoreTPL()
  // must have finished dispatching all event notifications.  This
  // interrupt must therefore have occurred at the point that the
  // outer handler's call to RestoreTPL() had finished and was about
  // to return to the outer handler.
  //
  // If we were to call RestoreTPL() at this point, then we would open
  // up the possibility for unlimited stack consumption in the event
  // of an interrupt storm.  We therefore cannot safely call
  // RestoreTPL() from within this stack frame (i.e. from within this
  // instance of the interrupt handler).
  //
  // Instead, we arrange to return from this interrupt with the TPL
  // still at TPL_HIGH_LEVEL and with interrupts disabled, and to
  // defer our call to RestoreTPL() to the in-progress outer instance
  // of the same interrupt handler.
  //
  ASSERT (GetInterruptState () == FALSE);
  if (InterruptedTPL == IsrState->InProgressRestoreTPL) {
    //
    // Trigger outer instance of this interrupt handler to perform the
    // RestoreTPL() call that we cannot issue at this point without
    // risking stack exhaustion.
    //
    ASSERT (IsrState->DeferredRestoreTPL == FALSE);
    IsrState->DeferredRestoreTPL = TRUE;

    //
    // DEFERRAL INVOCATION POINT
    //
    // Return from this interrupt handler with interrupts still
    // disabled (by clearing the "interrupts-enabled" bit in the CPU
    // flags that will be restored by the IRET or equivalent
    // instruction).
    //
    // This ensures that no further interrupts may occur before
    // control reaches the outer interrupt handler's RestoreTPL() loop
    // at the point marked "DEFERRAL RETURN POINT" (see below).
    //
    DisableInterruptsOnIret (SystemContext);
    return;
  }

  //
  // If the TPL at which this interrupt occurred is higher than that
  // of the in-progress RestoreTPL() for an outer instance of the same
  // interrupt handler, then that outer handler's call to RestoreTPL()
  // must still be dispatching event notifications.
  //
  // We must therefore call RestoreTPL() at this point to allow more
  // event notifications to be dispatched, since those event
  // notification callback functions may themselves be waiting upon
  // other events.
  //
  // We cannot avoid creating a new stack frame for this call to
  // RestoreTPL(), but the total number of such stack frames is
  // intrinsically limited by the number of distinct TPLs.
  //
  // We may need to issue the call to RestoreTPL() more than once, if
  // an inner instance of the same interrupt handler needs to defer
  // its RestoreTPL() call to be performed from within this stack
  // frame (see above).
  //
  while (TRUE) {
    //
    // Check shared state loop invariants.
    //
    ASSERT (GetInterruptState () == FALSE);
    ASSERT (IsrState->InProgressRestoreTPL < InterruptedTPL);
    ASSERT (IsrState->DeferredRestoreTPL == FALSE);

    //
    // Record the in-progress RestoreTPL() value in the shared state
    // where it will be visible to an inner instance of the same
    // interrupt handler, in case a nested interrupt occurs during our
    // call to RestoreTPL().
    //
    SavedInProgressRestoreTPL      = IsrState->InProgressRestoreTPL;
    IsrState->InProgressRestoreTPL = InterruptedTPL;

    //
    // Call RestoreTPL() to allow event notifications to be
    // dispatched.  This will implicitly re-enable interrupts (if
    // InterruptedTPL is below TPL_HIGH_LEVEL), even though we are
    // still inside the interrupt handler.
    //
    gBS->RestoreTPL (InterruptedTPL);

    //
    // Re-disable interrupts after the call to RestoreTPL() to ensure
    // that we have exclusive access to the shared state.  Interrupts
    // will be re-enabled by the IRET or equivalent instruction when
    // we subsequently return from the interrupt handler.
    //
    DisableInterrupts ();

    //
    // DEFERRAL RETURN POINT
    //
    // An inner instance of the same interrupt handler may have chosen
    // to defer its RestoreTPL() call to be performed from within this
    // stack frame.  If so, it is guaranteed that no further event
    // notifications or interrupts have been processed between the
    // DEFERRAL INVOCATION POINT (see above) and this DEFERRAL RETURN
    // POINT.
    //

    //
    // Restore the locally saved in-progress RestoreTPL() value in the
    // shared state, now that our call to RestoreTPL() has returned
    // and is therefore no longer in progress.
    //
    ASSERT (IsrState->InProgressRestoreTPL == InterruptedTPL);
    IsrState->InProgressRestoreTPL = SavedInProgressRestoreTPL;

    //
    // Check (and clear) the shared state to see if an inner instance
    // of the same interrupt handler deferred its call to
    // RestoreTPL().
    //
    DeferredRestoreTPL           = IsrState->DeferredRestoreTPL;
    IsrState->DeferredRestoreTPL = FALSE;

    //
    // If no inner interrupt handler deferred its call to
    // RestoreTPL(), then the TPL has been successfully restored and
    // we may return from the interrupt handler.
    //
    if (DeferredRestoreTPL == FALSE) {
      return;
    }
  }
}
