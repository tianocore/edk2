/** @file
  Public include file for the CPU Debug Dump library.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __CPU_DEBUG_DUMP_LIB_H__
#define __CPU_DEBUG_DUMP_LIB_H__

/**
  Dumps the Interrupt Descriptor Table (IDT) to debug output.

**/
VOID
EFIAPI
DumpIdt (
  VOID
  );

/**
  Dumps the Global Descriptor Table (GDT) to debug output.

**/
VOID
EFIAPI
DumpGdt (
  VOID
  );

#endif

