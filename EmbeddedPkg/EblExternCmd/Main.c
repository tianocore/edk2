/** @file
  Example of an external EBL command. It's loaded via EBL start command.
  Argc and Argv are passed in via "" of the EBL command line.

  Start fs0:\EdkExternCmd.efi "Argv[0] Argv[1] 2"

  will launch this command with
    Argv[0] = "Argv[0]"
    Argv[1] = "Argv[2]"
    Argv[2] = "3"

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "Ebl.h"

/**
  Entry point with Argc, Argv. Put your code here.

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblMain (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  UINTN   Index;

  AsciiPrint ("Hello World\n");
  for (Index = 0; Index < Argc; Index++) {
    AsciiPrint ("Argv[%d] = %a\n", Index, Argv[Index]);
  }

  return EFI_SUCCESS;
}

