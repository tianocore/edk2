/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



**/

#include "Edb.h"

/**

  DebuggerCommand - Go.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Interrupt type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_BREAK    - formal return value
  @retval EFI_DEBUG_CONTINUE - something wrong

**/
EFI_DEBUG_STATUS
DebuggerGo (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  UINTN        Address;
  CHAR16       *CommandStr;
  EFI_STATUS   Status;

  //
  // Check argument
  //
  if (CommandArg != NULL) {
    if (StriCmp (CommandArg, L"til") == 0) {
      CommandStr = StrGetNextTokenLine (L" ");
      if (CommandStr != NULL) {
        //
        // Enable GoTil break now
        // set BreakAddress, and set feature flag.
        //
        Status = Symboltoi (CommandStr, &Address);
        if (EFI_ERROR (Status)) {
          if (Status == EFI_NOT_FOUND) {
            Address = Xtoi(CommandStr);
          } else {
            //
            // Something wrong, let Symboltoi print error info.
            //
            EDBPrint (L"Command Argument error!\n");
            return EFI_DEBUG_CONTINUE;
          }
        }
        DebuggerPrivate->GoTilContext.BreakAddress = Address;
        DebuggerPrivate->FeatureFlags |= EFI_DEBUG_FLAG_EBC_GT;
      } else {
        EDBPrint (L"Command Argument error!\n");
        return EFI_DEBUG_CONTINUE;
      }
    } else {
      EDBPrint (L"Command Argument error!\n");
      return EFI_DEBUG_CONTINUE;
    }
  }

  //
  // Done
  //
  return EFI_DEBUG_BREAK;
}
