/** @file
  Implement the getpass function.

  Copyright (c) 2011 - 2014, Intel Corporation <BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/ShellLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>

static CHAR8   *ReturnStringAscii = NULL;

char *getpass(const char *Prompt)
{
  BOOLEAN Ascii;
  CHAR16  *ReturnString;

  Ascii = FALSE;

  Print(L"%a", Prompt);

  ReturnString = ShellFileHandleReturnLine (gEfiShellParametersProtocol->StdIn, &Ascii);
  if (ReturnString == NULL) {
    return (NULL);
  }

  ReturnStringAscii = AllocateZeroPool((StrLen(ReturnString)+1)*sizeof(CHAR8));
  if (ReturnStringAscii == NULL) {
    return (NULL);
  }

  UnicodeStrToAsciiStr(ReturnString, ReturnStringAscii);

  FreePool(ReturnString);

  return (ReturnStringAscii);
}

EFI_STATUS
EFIAPI
DestructMePlease (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SHELL_FREE_NON_NULL(ReturnStringAscii);

  return EFI_SUCCESS;
}
