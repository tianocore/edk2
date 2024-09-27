/** @file
  EFI_FILE_PROTOCOL wrappers for other items (Like Environment Variables,
  StdIn, StdOut, StdErr, etc...).

  Copyright 2016 Dell Inc.
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2013 Hewlett-Packard Development Company, L.P.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/ShellProtocolInteractivityLib.h>

/**
  Prints out each previously typed command in the command list history log.

  When each screen is full it will pause for a key before continuing.

  @param[in] TotalCols    How many columns are on the screen
  @param[in] TotalRows    How many rows are on the screen
  @param[in] StartColumn  which column to start at
**/
VOID
PrintCommandHistory (
  IN CONST UINTN  TotalCols,
  IN CONST UINTN  TotalRows,
  IN CONST UINTN  StartColumn
  )
{
  BUFFER_LIST  *Node;
  UINTN        Index;
  UINTN        LineNumber;
  UINTN        LineCount;

  ShellPrintEx (-1, -1, L"\n");
  Index      = 0;
  LineNumber = 0;
  //
  // go through history list...
  //
  for ( Node = (BUFFER_LIST *)GetFirstNode (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory.Link)
        ; !IsNull (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory.Link, &Node->Link)
        ; Node = (BUFFER_LIST *)GetNextNode (&ShellProtocolInteractivityInfoObject.ViewingSettings.CommandHistory.Link, &Node->Link)
        )
  {
    Index++;
    LineCount = ((StrLen (Node->Buffer) + StartColumn + 1) / TotalCols) + 1;

    if (LineNumber + LineCount >= TotalRows) {
      ShellPromptForResponseHii (
        ShellPromptResponseTypeEnterContinue,
        STRING_TOKEN (STR_SHELL_ENTER_TO_CONT),
        ShellProtocolInteractivityInfoObject.HiiHandle,
        NULL
        );
      LineNumber = 0;
    }

    ShellPrintEx (-1, -1, L"%2d. %s\n", Index, Node->Buffer);
    LineNumber += LineCount;
  }
}
