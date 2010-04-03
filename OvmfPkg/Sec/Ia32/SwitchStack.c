/** @file
  Switch Stack functions.

  Copyright (c) 2006 - 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// Include common header file for this module.
//


#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>

/**
  Transfers control to a function starting with a new stack.

  Transfers control to the function specified by EntryPoint using the new stack
  specified by NewStack and passing in the parameters specified by Context1 and
  Context2. Context1 and Context2 are optional and may be NULL. The function
  EntryPoint must never return.

  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  @param  EntryPoint  A pointer to function to call with the new stack.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function.
  @param  NewBsp      A pointer to the new BSP for the EntryPoint on IPF. It's
                      Reserved on other architectures.

**/
VOID
EFIAPI
PeiSwitchStacks (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *Context3,  OPTIONAL
  IN      VOID                      *OldTopOfStack,
  IN      VOID                      *NewStack
  )
{
  BASE_LIBRARY_JUMP_BUFFER  JumpBuffer;
  
  ASSERT (EntryPoint != NULL);
  ASSERT (NewStack != NULL);

  //
  // Stack should be aligned with CPU_STACK_ALIGNMENT
  //
  ASSERT (((UINTN)NewStack & (CPU_STACK_ALIGNMENT - 1)) == 0);

  JumpBuffer.Eip = (UINTN)EntryPoint;
  JumpBuffer.Esp = (UINTN)NewStack - sizeof (VOID*);
  JumpBuffer.Esp -= sizeof (Context1) + sizeof (Context2) + sizeof(Context3);
  ((VOID**)JumpBuffer.Esp)[1] = Context1;
  ((VOID**)JumpBuffer.Esp)[2] = Context2;
  ((VOID**)JumpBuffer.Esp)[3] = Context3;

  LongJump (&JumpBuffer, (UINTN)-1);

  //
  // InternalSwitchStack () will never return
  //
  ASSERT (FALSE);  
}

/**
  Transfers control to a function starting with a new stack.

  Transfers control to the function specified by EntryPoint using the new stack
  specified by NewStack and passing in the parameters specified by Context1 and
  Context2. Context1 and Context2 are optional and may be NULL. The function
  EntryPoint must never return.

  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  @param  EntryPoint  A pointer to function to call with the new stack.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function.
  @param  NewBsp      A pointer to the new BSP for the EntryPoint on IPF. It's
                      Reserved on other architectures.

**/
VOID
EFIAPI
SecSwitchStack (
  IN UINTN   TemporaryMemoryBase,
  IN UINTN   PermanentMemoryBase,
  IN UINTN   CopySize
  )
{
  BASE_LIBRARY_JUMP_BUFFER  JumpBuffer;
  UINTN                     SetJumpFlag;

  ASSERT ((VOID*)TemporaryMemoryBase != NULL);
  ASSERT ((VOID*)PermanentMemoryBase != NULL);

  SetJumpFlag = SetJump (&JumpBuffer);
  //
  // The initial call to SetJump() must always return 0.
  // Subsequent calls to LongJump() may cause a non-zero value to be returned by SetJump().
  //
  if (SetJumpFlag == 0) {
    DEBUG ((EFI_D_ERROR, "SecSwitchStack+%d: Esp: 0x%xL\n", __LINE__, JumpBuffer.Esp));
    JumpBuffer.Esp =
      (INTN)JumpBuffer.Esp -
      (INTN)TemporaryMemoryBase +
      (INTN)PermanentMemoryBase;
    MemoryFence ();
    CopyMem((VOID*)PermanentMemoryBase, (VOID*)TemporaryMemoryBase, CopySize);
    LongJump (&JumpBuffer, (UINTN)-1);
  }

}

