/** @file
  Script command allows the execution of commands from a text file

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  EfiDevice.c

**/

#include "Ebl.h"


/**
  Execute the passed in file like a series of commands. The ; can be used on
  a single line to indicate multiple commands per line. The Ascii text file
  can contain any number of lines. The following line termination forms are
  supported:
    LF   : Unix, Mac OS X*, BeOS
    CR+LF: MS-DOS*, Microsoft Windows*
    CR   : Commodore, Apple II, and really Mac OS
    LF+CR: for simplicity and completeness

  Argv[0] - "script"
  Argv[1] - Device Name:path for the file to load

  script fv1:\script.txt

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblScriptCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS                    Status;
  EFI_OPEN_FILE                 *File;
  VOID                          *Address;
  UINTN                         Size;
  CHAR8                         *Ptr;
  CHAR8                         *ScanPtr;
  UINTN                         CmdLineSize;



  if (Argc < 2) {
    // file name required
    return EFI_SUCCESS;
  }

  File = EfiOpen (Argv[1], EFI_FILE_MODE_READ, 0);
  if (File == NULL) {
    AsciiPrint ("  %a is not a valid path\n", Argv[1]);
    return EFI_SUCCESS;
  }

  Status = EfiReadAllocatePool (File, &Address, &Size);
  if (!EFI_ERROR (Status)) {
    // Loop through each line in the text file
    for (Ptr = (CHAR8 *)Address; (Ptr < (((CHAR8 *)Address) + Size)) && !EFI_ERROR (Status); Ptr += CmdLineSize) {
      for (CmdLineSize = 0, ScanPtr = Ptr; ; CmdLineSize++, ScanPtr++) {
        // look for the end of the line
        if ((*ScanPtr == EBL_CR) || (*ScanPtr == EBL_LF)) {
          // convert to NULL as this is what input routine would do
          *ScanPtr = 0;
          if ((*(ScanPtr + 1) == EBL_CR) || (*(ScanPtr + 1) == EBL_LF)) {
            // if its a set get the 2nd EOL char
            CmdLineSize++;
            *(ScanPtr + 1) = 0;
          }
          CmdLineSize++;
          break;
        }

      }

      Status = ProcessCmdLine (Ptr, CmdLineSize);
    }

    FreePool (Address);
  }

  EfiClose (File);
  return Status;
}



GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mScriptTemplate[] = {
  {
    "script",
    " device:path; load an ascii file and execute it like commands",
    NULL,
    EblScriptCmd
  }
};


/**
  Initialize the commands in this in this file
**/

VOID
EblInitializeScriptCmd (
  VOID
  )
{
  if (FeaturePcdGet (PcdEmbeddedScriptCmd)) {
    EblAddCommands (mScriptTemplate, sizeof (mScriptTemplate)/sizeof (EBL_COMMAND_TABLE));
  }
}

