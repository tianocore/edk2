/**@file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  CmdLineArgs.c

Abstract:
  Command line handling.

**/

#include "SecMain.h"

extern SIM_CMD_LINE_ARGS_PPI mSimCmdLineArgsPpi;

EFI_STATUS
SimInitCmdLineArgsPpi (
  IN  INTN  Argc,
  IN  CHAR8 **Argv
  )
{
  INTN i;
  UINTN ArgsSize;
  CHAR8 **ArgsArray;

  //
  // Set the number of arguments.
  //

  mSimCmdLineArgsPpi.Argc = Argc;

  //
  // Allocate memory for the array of pointers.
  //

  ArgsArray = malloc (Argc * sizeof (CHAR8 *));

  //
  // Allocate memory and copy each argument string.
  //

  for (i = 0; i < Argc; i++) {

    ArgsSize = AsciiStrLen (Argv[i]) + 1;

    ArgsArray[i] = malloc (ArgsSize);

    ZeroMem (ArgsArray[i], ArgsSize);

    AsciiStrCpy (ArgsArray[i], Argv[i]);

  }

  //
  // Finally, assign the PPI memory to the args.
  //

  mSimCmdLineArgsPpi.Argv = ArgsArray;

  return EFI_SUCCESS;

} // SimInitCmdLineArgsPpi
