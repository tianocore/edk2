/** @file
  Hardware IO based debug commands

  Copyright (c) 2007, Intel Corporation<BR>
  Portions copyright (c) 2008-2009, Apple Inc. All rights reserved.

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Commands useful for debugging hardware. IO commands seperated out as not all
  processor architectures support the IO command.

**/

#include "Ebl.h"



/**
  Read from IO space

  Argv[0] - "ioread"
  Argv[1] - Hex IO address
  Argv[2] - IO Width [1|2|4] with a default of 1

  ior 0x3f8 4  ;Do a 32-bit IO Read from 0x3f8
  ior 0x3f8 1  ;Do a  8-bit IO Read from 0x3f8

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line. 
                 Argv[0] is the comamnd name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblIoReadCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  UINTN   Width;
  UINTN   Port;
  UINTN   Data;

  if (Argc < 2) {
    return EFI_INVALID_PARAMETER;
  }

  Port = AsciiStrHexToUintn (Argv[1]);
  Width = (Argc > 2) ? AsciiStrHexToUintn (Argv[2]) : 1;

  if (Width == 1) {
    Data = IoRead8 (Port);
  } else if (Width == 2) {
    Data = IoRead16 (Port);
  } else if (Width == 4) {
    Data = IoRead32 (Port);
  } else {
    return EFI_INVALID_PARAMETER;
  }

  AsciiPrint ("0x%04x = 0x%x", Port, Data);

  return EFI_SUCCESS;
}


/**
  Write to IO space

  Argv[0] - "iowrite"
  Argv[1] - Hex IO address
  Argv[2] - Hex data to write
  Argv[3] - IO Width [1|2|4] with a default of 1

  iow 0x3f8 af 4  ;Do a 32-bit IO write of af to 0x3f8
  iow 0x3f8 af    ;Do an 8-bit IO write of af to 0x3f8

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line. 
                 Argv[0] is the comamnd name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblIoWriteCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  UINTN   Width;
  UINTN   Port;
  UINTN   Data;

  if (Argc < 3) {
    return EFI_INVALID_PARAMETER;
  }

  Port = AsciiStrHexToUintn (Argv[1]);
  Data = AsciiStrHexToUintn (Argv[2]);
  Width = (Argc > 3) ? AsciiStrHexToUintn (Argv[3]) : 1;
 
  if (Width == 1) {
    IoWrite8 (Port, (UINT8)Data);
  } else if (Width == 2) {
    IoWrite16 (Port, (UINT16)Data);
  } else if (Width == 4) {
    IoWrite32 (Port, (UINT32)Data);
  } else {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}


GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mCmdHwIoDebugTemplate[] =
{
  {
    "ioread",
    " Port [1|2|4]; IO read of width[1] byte(s) from Port",
    NULL,
    EblIoReadCmd
  },
  {
    "iowrite",
    " Port Data [1|2|4]; IO write Data of width[1] byte(s) to Port",
    NULL,
    EblIoWriteCmd
  }
};



/**
  Initialize the commands in this in this file
**/
VOID
EblInitializemdHwIoDebugCmds (
  VOID
  )
{
  if (FeaturePcdGet (PcdEmbeddedIoEnable)) {
    EblAddCommands (mCmdHwIoDebugTemplate, sizeof (mCmdHwIoDebugTemplate)/sizeof (EBL_COMMAND_TABLE));
  }
}

