/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

Module Name:

  SetMemWrapper.c
  
Abstract: 

  SetMem() implementation.

--*/

#include "BaseMemoryLibInternal.h"

/**
  Fills a target buffer with a byte value, and returns the target buffer.

  This function fills Length bytes of Buffer with Value, and returns Buffer.
  If Length is greater than (MAX_ADDRESS ? Buffer + 1), then ASSERT(). 

  @param  Buffer    Memory to set.
  @param  Length    Number of bytes to set.
  @param  Value     Value of the set operation.

  @return Buffer.

**/
VOID *
EFIAPI
GlueSetMem (
  OUT VOID  *Buffer,
  IN UINTN  Length,
  IN UINT8  Value
  )
{
  if (Length == 0) {
    return Buffer;
  }

  ASSERT ((Length - 1) <= (MAX_ADDRESS - (UINTN)Buffer));

  return InternalMemSetMem (Buffer, Length, Value);
}
