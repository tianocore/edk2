/** @file
  GCC inline implementation of BaseLib processor specific functions.
  
  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BaseLibInternals.h"

/**
  Requests CPU to pause for a short period of time.

  Requests CPU to pause for a short period of time. Typically used in MP
  systems to prevent memory starvation while waiting for a spin lock.

**/
VOID
EFIAPI
CpuPause (
  VOID
  )
{
  __asm__ __volatile__ (
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    );
}

/**
  Transfers control to a function starting with a new stack.

  This internal worker function transfers control to the function
  specified by EntryPoint using the new stack specified by NewStack
  and passing in the parameters specified by Context1 and Context2.
  Context1 and Context2 are optional and may be NULL.
  The function EntryPoint must never return.

  @param EntryPoint   The pointer to the function to enter.
  @param Context1     The first parameter to pass in.
  @param Context2     The second Parameter to pass in
  @param NewStack     The new Location of the stack

**/
VOID
EFIAPI
InternalSwitchStackAsm (
  SWITCH_STACK_ENTRY_POINT EntryPoint,
  VOID  *Context1,
  VOID  *Context2,
  VOID  *NewStack
  )
{
  __asm__ __volatile__ (
    "mov  lr, %0\n\t"
    "mov  sp, %3\n\t"
    "mov  %r0, %1\n\t"
    "mov  %r1, %2\n\t"
    "bx   lr\n\t"
    : /* no output operand */
    : "r" (EntryPoint),
      "r" (Context1),
      "r" (Context2),
      "r" (NewStack)
    );
}
