/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  EdbCmdQuit.c

Abstract:


**/

#include "Edb.h"

/**

  DebuggerCommand - Quit

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_RETURN   - formal return value

**/
EFI_DEBUG_STATUS
DebuggerQuit (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  return EFI_DEBUG_RETURN;
}
