/** @file
  Hardware IO based debug commands

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Commands useful for debugging hardware. IO commands separated out as not all
  processor architectures support the IO command.

**/

#include "Ebl.h"



/**
  Read from IO space

  Argv[0] - "ioread"[.#] # is optional width 1, 2, or 4. Default 1
  Argv[1] - Hex IO address

  ior.4 0x3f8  ;Do a 32-bit IO Read from 0x3f8
  ior   0x3f8  ;Do a  8-bit IO Read from 0x3f8

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

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
  Width = WidthFromCommandName (Argv[0], 1);

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

  Argv[0] - "iowrite"[.#] # is optional width 1, 2, or 4. Default 1
  Argv[1] - Hex IO address
  Argv[2] - Hex data to write

  iow.4 0x3f8 af  ;Do a 32-bit IO write of af to 0x3f8
  iow   0x3f8 af  ;Do an 8-bit IO write of af to 0x3f8

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

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
  Width = WidthFromCommandName (Argv[0], 1);

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
    "[.{1|2|4}] Port ; IO read of width byte(s) from Port",
    NULL,
    EblIoReadCmd
  },
  {
    "iowrite",
    "[.{1|2|4}] Port Data ; IO write Data of width byte(s) to Port",
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

