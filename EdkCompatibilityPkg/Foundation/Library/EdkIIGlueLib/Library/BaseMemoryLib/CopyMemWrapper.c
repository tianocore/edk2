/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

Module Name:

  CopyMemWrapper.c
  
Abstract: 

  CopyMem() implementation.

--*/

#include "BaseMemoryLibInternal.h"

/**
  Copies a source buffer to a destination buffer, and returns the destination buffer.

  This function copies Length bytes from SourceBuffer to DestinationBuffer, and returns
  DestinationBuffer.  The implementation must be reentrant, and it must handle the case
  where SourceBuffer overlaps DestinationBuffer.
  If Length is greater than (MAX_ADDRESS - DestinationBuffer + 1), then ASSERT(). 
  If Length is greater than (MAX_ADDRESS - SourceBuffer + 1), then ASSERT(). 

  @param  DestinationBuffer   Pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        Pointer to the source buffer of the memory copy.
  @param  Length              Number of bytes to copy from SourceBuffer to DestinationBuffer.

  @return DestinationBuffer.

**/
VOID *
EFIAPI
GlueCopyMem (
  OUT VOID       *DestinationBuffer,
  IN CONST VOID  *SourceBuffer,
  IN UINTN       Length
  )
{
  if (Length == 0) {
    return DestinationBuffer;
  }
  ASSERT ((Length - 1) <= (MAX_ADDRESS - (UINTN)DestinationBuffer));
  ASSERT ((Length - 1) <= (MAX_ADDRESS - (UINTN)SourceBuffer));

  if (DestinationBuffer == SourceBuffer) {
    return DestinationBuffer;
  }
  return InternalMemCopyMem (DestinationBuffer, SourceBuffer, Length);
}
