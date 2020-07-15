/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "Edb.h"

/**

  DebuggerCommand - Scope.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerScope (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  EFI_STATUS   Status;
  UINTN        Address;

  if (CommandArg == NULL) {
    EDBPrint (L"Scope: invalid Address\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Load new scope
  //
  Status = Symboltoi (CommandArg, &Address);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      Address = Xtoi(CommandArg);
    } else {
      //
      // Something wrong, let Symboltoi print error info.
      //
      EDBPrint (L"Command Argument error!\n");
      return EFI_DEBUG_CONTINUE;
    }
  }
  DebuggerPrivate->InstructionScope = Address;
  EDBPrint (L"Scope: 0x%x\n", DebuggerPrivate->InstructionScope);
  EdbShowDisasm (DebuggerPrivate, SystemContext);

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - List.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerList (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  if (CommandArg == NULL) {
    EdbShowDisasm (DebuggerPrivate, SystemContext);
  } else {
    //
    // Load new list number
    //
    DebuggerPrivate->InstructionNumber = Atoi(CommandArg);
    EDBPrint (L"List Number: %d\n", DebuggerPrivate->InstructionNumber);
    EdbShowDisasm (DebuggerPrivate, SystemContext);
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}
