/** @file
  CpuFlushTlb function for Ia32/X64 GCC.

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
  Portions copyright (c) 2008-2009 Apple Inc.<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/



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
  __asm__ __volatile__ (
    "movl %%cr3, %0\n\t"
    "movl %0, %%cr3    "
    : "r"  // %0
    );
}

