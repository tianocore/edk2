/** @file
  %CommandName% for EBL (Embedded Boot Loader)

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  CmdTemplate.c

  Search/Replace %CommandName% with the name of your new command

**/

#include "Ebl.h"


/**
  Fill Me In

  Argv[0] - "%CommandName%"

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
Ebl%CommandName%Cmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  return EFI_SUCCESS;
}


GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mCmd%CommandName%Template[] =
{
  {
    "%CommandName%",
    " [show args] ; explain args and command",
    NULL,
    Ebl%CommandName%Cmd
  }
};


/**
  Initialize the commands in this file
**/
VOID
EblInitialize%CommandName%Cmd (
  VOID
  )
{
  EblAddCommands (mCmd%CommandName%Template, sizeof (mCmd%CommandName%Template)/sizeof (EBL_COMMAND_TABLE));
}

