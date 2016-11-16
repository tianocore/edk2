/*++

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  EdbCmdScope.c

Abstract:


--*/

#include "Edb.h"

EFI_DEBUG_STATUS
DebuggerScope (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - Scope

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_CONTINUE - formal return value

--*/
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

EFI_DEBUG_STATUS
DebuggerList (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - List

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_CONTINUE - formal return value

--*/
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
