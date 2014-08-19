/** @file
  Glue code that contains the EFI entry point and converts it to an EBL
  ASCII Argc, Argv sytle entry point


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

#define CMD_SEPARATOR     ';'
#define MAX_ARGS          32

EFI_STATUS
EblMain (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  );


///
/// EdkExternCmdEntry() & ParseArguments() convert the standard EFI entry point
/// into Argc, Argv form that calls EblMain().
///


/**
  Parse the CmdLine and break it up into Argc (arg count) and Argv (array of
  pointers to each argument). The Cmd buffer is altered and separators are
  converted to string terminators. This allows Argv to point into CmdLine.
  A CmdLine can support multiple commands. The next command in the command line
  is returned if it exists.

  @param  CmdLine     String to parse for a set of commands
  @param  CmdLineSize Size of CmdLine in bytes
  @param  Argc        Returns the number of arguments in the CmdLine current command
  @param  Argv        Argc pointers to each string in CmdLine

  @return Next Command in the command line or NULL if non exists
**/
VOID
ParseArguments (
  IN  CHAR8  *CmdLine,
  IN  UINTN  CmdLineSize,
  OUT UINTN  *Argc,
  OUT CHAR8  **Argv
  )
{
  UINTN   Arg;
  CHAR8   *Char;
  BOOLEAN LookingForArg;
  BOOLEAN InQuote;
  UINTN   Index;

  *Argc = 0;
  if ((CmdLineSize == 0) || (AsciiStrLen (CmdLine) == 0)) {
    // basic error checking failed on the arguments
    return;
  }

  // Walk a single command line. A CMD_SEPARATOR allows multiple commands on a single line
  InQuote       = FALSE;
  LookingForArg = TRUE;
  for (Char = CmdLine, Arg = 0, Index = 0; *Char != '\0' && *Char != CMD_SEPARATOR; Char++, Index++) {
    // Perform any text conversion here
    if (*Char == '\t') {
      // TAB to space
      *Char = ' ';
    }

    if (LookingForArg) {
      // Look for the beginning of an Argv[] entry
      if (*Char == '"') {
        Argv[Arg++] = ++Char;
        LookingForArg = FALSE;
        InQuote = TRUE;
      } else if (*Char != ' ') {
        Argv[Arg++] = Char;
        LookingForArg = FALSE;
      }
    } else {
      // Looking for the terminator of an Argv[] entry
      if ((InQuote && (*Char == '"')) || (!InQuote && (*Char == ' '))) {
        *Char = '\0';
        LookingForArg = TRUE;
      }
    }

    if ((Arg >= MAX_ARGS) || (Index > CmdLineSize)) {
      // Error check buffer and exit since it does not look valid
      break;
    }
  }

  *Argc = Arg;

  if (*Char == CMD_SEPARATOR) {
    // Replace the command delimiter with null
    *Char = '\0';
  }

  return;
}




/**
  Embedded Boot Loader (EBL) - A simple EFI command line application for embedded
  devices. PcdEmbeddedAutomaticBootCommand is a complied in command line that
  gets executed automatically. The ; separator allows multiple commands
  for each command line.

  @param  ImageHandle   EFI ImageHandle for this application.
  @param  SystemTable   EFI system table

  @return EFI status of the application

**/
EFI_STATUS
EFIAPI
EdkExternCmdEntry (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *ImageInfo;
  UINTN                       Argc;
  CHAR8                       *Argv[MAX_ARGS];

  Status = gBS->HandleProtocol (ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&ImageInfo);
  if (EFI_ERROR (Status)) {
    Argc = 0;
  } else {
    // Looks like valid commands were passed in.
    ParseArguments (ImageInfo->LoadOptions, ImageInfo->LoadOptionsSize, &Argc, Argv);
  }

  return EblMain (Argc, Argv);
}


