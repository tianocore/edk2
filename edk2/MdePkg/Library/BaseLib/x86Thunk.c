/** @file
  Real Mode Thunk Functions for IA32 and X64.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  x86Thunk.c

**/

/**
  Invokes 16-bit code in big real mode and returns the updated register set.

  This function transfers control to the 16-bit code specified by CS:EIP using
  the stack specified by SS:ESP in RegisterSet. The updated registers are saved
  on the real mode stack and the starting address of the save area is returned.

  @param  RegisterSet Values of registers before invocation of 16-bit code.
  @param  Patch       Pointer to the area following the 16-bit code.

  @return The pointer to a IA32_REGISTER_SET structure containing the updated
          register values.

**/
IA32_REGISTER_SET *
InternalAsmThunk16 (
  IN      IA32_REGISTER_SET         *RegisterSet,
  IN OUT  VOID                      *Patch
  );

/**
  Prepares all structures a code required to use AsmThunk16().

  Prepares all structures and code required to use AsmThunk16().

  If ThunkContext is NULL, then ASSERT().

  @param  ThunkContext  A pointer to the context structure that describes the
                        16-bit real mode code to call.

**/
VOID
EFIAPI
AsmPrepareThunk16 (
  OUT     THUNK_CONTEXT             *ThunkContext
  )
{
  ASSERT (ThunkContext != NULL);
}

/**
  Transfers control to a 16-bit real mode entry point and returns the results.

  Transfers control to a 16-bit real mode entry point and returns the results.
  AsmPrepareThunk16() must be called with ThunkContext before this function is
  used. This function must be called with interrupts disabled.

  If ThunkContext is NULL, then ASSERT().
  If AsmPrepareThunk16() was not previously called with ThunkContext, then ASSERT().

  @param  ThunkContext  A pointer to the context structure that describes the
                        16-bit real mode code to call.

**/
VOID
EFIAPI
AsmThunk16 (
  IN OUT  THUNK_CONTEXT             *ThunkContext
  )
{
  UINT16                            *Patch;

  ASSERT (ThunkContext != NULL);

  Patch = (UINT16*)(
            (UINTN)ThunkContext->RealModeCode +
            ThunkContext->RealModeCodeSize
            );

  //
  // 0x9a66 is the OpCode of far call with an operand size override.
  //
  *Patch = 0x9a66;

  //
  // CopyMem() here copies the updated register values back to RealModeState
  //
  CopyMem (
    &ThunkContext->RealModeState,
    InternalAsmThunk16 (&ThunkContext->RealModeState, Patch + 1),
    sizeof (ThunkContext->RealModeState)
    );
}

/**
  Prepares all structures and code for a 16-bit real mode thunk, transfers
  control to a 16-bit real mode entry point, and returns the results.

  Prepares all structures and code for a 16-bit real mode thunk, transfers
  control to a 16-bit real mode entry point, and returns the results. If the
  caller only need to perform a single 16-bit real mode thunk, then this
  service should be used. If the caller intends to make more than one 16-bit
  real mode thunk, then it is more efficient if AsmPrepareThunk16() is called
  once and AsmThunk16() can be called for each 16-bit real mode thunk. This
  function must be called with interrupts disabled.

  If ThunkContext is NULL, then ASSERT().

  @param  ThunkContext  A pointer to the context structure that describes the
                        16-bit real mode code to call.

**/
VOID
EFIAPI
AsmPrepareAndThunk16 (
  IN OUT  THUNK_CONTEXT             *ThunkContext
  )
{
  AsmPrepareThunk16 (ThunkContext);
  AsmThunk16 (ThunkContext);
}
