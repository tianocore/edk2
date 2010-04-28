/*++

Copyright (c) 2005, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:

   IdtDumb.c

Abstract:

--*/

#include "Tiano.h"

UINTN
ReadIdtBase ( 
  VOID 
  ) 
{
  
  return 0;
}


VOID
UpdateIdt (
  UINT32  IdtBase,
  UINT16  IdtLimit 
 )
{
  return;
}

