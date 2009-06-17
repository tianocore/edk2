/** @file
  This is THE shell (application)

  Copyright (c) 2009, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/DebugLib.h>

INTN 
EFIAPI 
ShellAppMain (
  IN INTN Argc, 
  IN CHAR16 **Argv
  )
{
  INTN Index;

  Print(L"ShellCTestApp.c:ShellAppMain called with %d parameters\n", Argc);
  for (Index = 0; Index < Argc; Index++) {
    Print(L"Argv[%d]: %s\n", Index, Argv[Index]);
  }

  return 0;
}