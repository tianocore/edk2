/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  memset.c

Abstract:

  The Microsoft compiler inlines memset and we can not stop it.
  These routines allow the code to link!

  There is no *.h definition of these modules as they are well known by the 
  compiler. See Microsoft documentation for more details!

  volatile is used to prevent the compiler from trying to implement these
  C functions as inline functions. 

--*/

#include "Tiano.h"

VOID *
memset (
  OUT VOID    *Dest,
  IN  int     Char,
  IN  UINTN   Count
  )
{
  volatile UINT8  *Ptr;
  
  for (Ptr = Dest; Count > 0; Count--, Ptr++) {
    *Ptr = (UINT8) Char;
  }

  return Dest;
}

