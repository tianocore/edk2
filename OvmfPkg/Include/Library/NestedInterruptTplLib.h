/** @file
  Handle raising and lowering TPL from within nested interrupt handlers.

  Allows interrupt handlers to safely raise and lower the TPL to
  dispatch event notifications, correctly allowing for nested
  interrupts to occur without risking stack exhaustion.

  Copyright (C) 2022, Fen Systems Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __NESTED_INTERRUPT_TPL_LIB__
#define __NESTED_INTERRUPT_TPL_LIB__

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Protocol/DebugSupport.h>

///
/// State shared between all invocations of a nested interrupt handler.
///
typedef struct {
  ///
  /// Highest TPL that is currently the target of a call to
  /// RestoreTPL() by an instance of this interrupt handler.
  ///
  EFI_TPL    InProgressRestoreTPL;
  ///
  /// Flag used to defer a call to RestoreTPL() from an inner instance
  /// of the interrupt handler to an outer instance of the same
  /// interrupt handler.
  ///
  BOOLEAN    DeferredRestoreTPL;
} NESTED_INTERRUPT_STATE;

/**
  Raise the task priority level to TPL_HIGH_LEVEL.

  @param  None.

  @return The task priority level at which the interrupt occurred.
**/
EFI_TPL
EFIAPI
NestedInterruptRaiseTPL (
  VOID
  );

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
  );

#endif // __NESTED_INTERRUPT_TPL_LIB__
