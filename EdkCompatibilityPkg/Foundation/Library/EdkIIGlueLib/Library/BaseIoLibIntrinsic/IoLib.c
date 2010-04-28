/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  IoLib.c
  
Abstract: 

  Common I/O Library routines.

--*/

#include "EdkIIGlueBase.h"

/**
  Reads a 64-bit I/O port.

  Reads the 64-bit I/O port specified by Port. The 64-bit read value is returned.
  This function must guarantee that all I/O read and write operations are
  serialized.

  If 64-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to read.

  @return The value read.

**/
UINT64
EFIAPI
IoRead64 (
  IN      UINTN                     Port
  )
{
  ASSERT (FALSE);
  return 0;
}

/**
  Writes a 64-bit I/O port.

  Writes the 64-bit I/O port specified by Port with the value specified by Value
  and returns Value. This function must guarantee that all I/O read and write
  operations are serialized.

  If 64-bit I/O port operations are not supported, then ASSERT().

  @param  Port  The I/O port to write.
  @param  Value The value to write to the I/O port.

  @return The value written the I/O port.

**/
UINT64
EFIAPI
IoWrite64 (
  IN      UINTN                     Port,
  IN      UINT64                    Value
  )
{
  ASSERT (FALSE);
  return 0;
}

