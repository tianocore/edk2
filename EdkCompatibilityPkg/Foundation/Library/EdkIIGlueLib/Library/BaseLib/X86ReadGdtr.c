/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  x86ReadGdtr.c
  
Abstract: 

  IA-32/x64 specific functions.

--*/

#include "BaseLibInternals.h"

/**
  Reads the current Global Descriptor Table Register(GDTR) descriptor.

  Reads and returns the current GDTR descriptor and returns it in Gdtr. This
  function is only available on IA-32 and X64.

  If Gdtr is NULL, then ASSERT().

  @param  Gdtr  Pointer to a GDTR descriptor.

**/
VOID
EFIAPI
AsmReadGdtr (
  OUT     IA32_DESCRIPTOR           *Gdtr
  )
{
  ASSERT (Gdtr != NULL);
  InternalX86ReadGdtr (Gdtr);
}
