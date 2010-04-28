/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  ScanMem16Wrapper.c
  
Abstract: 

  ScanMem16() implementation.

--*/

#include "BaseMemoryLibInternal.h"

/**
  Scans a target buffer for a 16-bit value, and returns a pointer to the matching 16-bit value
  in the target buffer.

  This function searches target the buffer specified by Buffer and Length from the lowest
  address to the highest address for a 16-bit value that matches Value.  If a match is found,
  then a pointer to the matching byte in the target buffer is returned.  If no match is found,
  then NULL is returned.  If Length is 0, then NULL is returned.
  If Length > 0 and Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 16-bit boundary, then ASSERT().
  If Length is not aligned on a 16-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS ? Buffer + 1), then ASSERT(). 

  @param  Buffer      Pointer to the target buffer to scan.
  @param  Length      Number of bytes in Buffer to scan.
  @param  Value       Value to search for in the target buffer.

  @return A pointer to the matching byte in the target buffer or NULL otherwise.

**/
VOID *
EFIAPI
ScanMem16 (
  IN CONST VOID  *Buffer,
  IN UINTN       Length,
  IN UINT16      Value
  )
{
  if (Length == 0) {
    return NULL;
  }

  ASSERT (Buffer != NULL);
  ASSERT (((UINTN)Buffer & (sizeof (Value) - 1)) == 0);
  ASSERT ((Length - 1) <= (MAX_ADDRESS - (UINTN)Buffer));
  ASSERT ((Length & (sizeof (Value) - 1)) == 0);

  return (VOID*)InternalMemScanMem16 (Buffer, Length / sizeof (Value), Value);
}
