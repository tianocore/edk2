/** @file
  Basic serial IO abstaction for GDB

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/GdbSerialLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <ArmEb/ArmEb.h>

RETURN_STATUS
EFIAPI
GdbSerialLibConstructor (
  VOID
  )
{
  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
GdbSerialInit (
  IN UINT64     BaudRate, 
  IN UINT8      Parity, 
  IN UINT8      DataBits, 
  IN UINT8      StopBits 
  )
{
  return RETURN_SUCCESS;
}

BOOLEAN
EFIAPI
GdbIsCharAvailable (
  VOID
  )  
{
  return FALSE;
}

CHAR8
EFIAPI
GdbGetChar (
  VOID
  )
{
  return (CHAR8)0;
}

VOID
EFIAPI
GdbPutChar (
  IN  CHAR8   Char
  )
{
  return;
}

VOID
GdbPutString (
  IN CHAR8  *String
  )
{
  while (*String != '\0') {
    GdbPutChar (*String);
    String++;
  }
}




