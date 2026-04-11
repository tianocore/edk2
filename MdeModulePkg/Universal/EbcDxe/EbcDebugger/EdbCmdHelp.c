/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "Edb.h"

/**

  DebuggerCommand - Help.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Interrupt type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerHelp (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  UINTN  Index;

  //
  // if no argument, print all the command title
  //
  if (CommandArg == NULL) {
    for (Index = 0; DebuggerPrivate->DebuggerCommandSet[Index].CommandName != NULL; Index++) {
      EDBPrint (DebuggerPrivate->DebuggerCommandSet[Index].ClassName);
      if (StrCmp (DebuggerPrivate->DebuggerCommandSet[Index].CommandTitle, L"") != 0) {
        EDBPrint (L"  ");
        EDBPrint (DebuggerPrivate->DebuggerCommandSet[Index].CommandTitle);
      }
    }

    return EFI_DEBUG_CONTINUE;
  }

  //
  // If there is argument, the argument should be command name.
  // Find the command and print the detail information.
  //
  for (Index = 0; DebuggerPrivate->DebuggerCommandSet[Index].CommandName != NULL; Index++) {
    if (StriCmp (CommandArg, DebuggerPrivate->DebuggerCommandSet[Index].CommandName) == 0) {
      EDBPrint (DebuggerPrivate->DebuggerCommandSet[Index].CommandHelp);
      EDBPrint (DebuggerPrivate->DebuggerCommandSet[Index].CommandSyntax);
      return EFI_DEBUG_CONTINUE;
    }
  }

  //
  // Command not found.
  //
  EDBPrint (L"No help info for this command\n");

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}
