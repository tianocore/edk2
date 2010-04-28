/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  x86GetInterruptState.c
  
Abstract: 

  IA-32/x64 specific functions.

--*/

#include "BaseLibInternals.h"

/**
  Retrieves the current CPU interrupt state.

  Retrieves the current CPU interrupt state. Returns TRUE is interrupts are
  currently enabled. Otherwise returns FALSE.

  @retval TRUE  CPU interrupts are enabled.
  @retval FALSE CPU interrupts are disabled.

**/
BOOLEAN
EFIAPI
GlueGetInterruptState (
  VOID
  )
{
  IA32_EFLAGS32                     EFlags;

  EFlags.UintN = AsmReadEflags ();
  return (BOOLEAN)(EFlags.Bits.IF == 1);
}


