/** @file
  Base Library CPU Functions for EBC

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

extern
UINT64
_break (
  CHAR8 BreakCode
  );

/**
  Generates a breakpoint on the CPU.

  Generates a breakpoint on the CPU. The breakpoint must be implemented such
  that code can resume normal execution after the breakpoint.

**/
VOID
EFIAPI
CpuBreakpoint (
  VOID
  )
{
  _break (3);
}

/**
  Used to serialize load and store operations.

  All loads and stores that proceed calls to this function are guaranteed to be
  globally visible when this function returns.

**/
VOID
EFIAPI
MemoryFence (
  VOID
  )
{
}

/**
  Disables CPU interrupts.

  Disables CPU interrupts.

**/
VOID
EFIAPI
DisableInterrupts (
  VOID
  )
{
  ASSERT (FALSE);
}

/**
  Enables CPU interrupts.

  Enables CPU interrupts.

**/
VOID
EFIAPI
EnableInterrupts (
  VOID
  )
{
  ASSERT (FALSE);
}

/**
  Retrieves the current CPU interrupt state.

  Retrieves the current CPU interrupt state. Returns TRUE is interrupts are
  currently enabled. Otherwise returns FALSE.

  @retval TRUE  CPU interrupts are enabled.
  @retval FALSE CPU interrupts are disabled.

**/
BOOLEAN
EFIAPI
GetInterruptState (
  VOID
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Enables CPU interrupts for the smallest window required to capture any
  pending interrupts.

  Enables CPU interrupts for the smallest window required to capture any
  pending interrupts.

**/
VOID
EFIAPI
EnableDisableInterrupts (
  VOID
  )
{
  EnableInterrupts ();
  DisableInterrupts ();
}

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
}

/**
  Flushes all the Translation Lookaside Buffers(TLB) entries in a CPU.

  Flushes all the Translation Lookaside Buffers(TLB) entries in a CPU.

**/
VOID
EFIAPI
CpuFlushTlb (
  VOID
  )
{
  ASSERT (FALSE);
}

/**
  Places the CPU in a sleep state until an interrupt is received.

  Places the CPU in a sleep state until an interrupt is received. If interrupts
  are disabled prior to calling this function, then the CPU will be placed in a
  sleep state indefinitely.

**/
VOID
EFIAPI
CpuSleep (
  VOID
  )
{
}
