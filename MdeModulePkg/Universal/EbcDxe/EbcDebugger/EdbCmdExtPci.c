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

  DebuggerCommand - PCIL.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Interrupt type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerExtPciPCIL (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  EDBPrint (L"Unsupported\n");
  //
  // TBD
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - PCID.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Interrupt type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerExtPciPCID (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  EDBPrint (L"Unsupported\n");
  //
  // TBD
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - CFGB.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Interrupt type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerExtPciCFGB (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  EDBPrint (L"Unsupported\n");
  //
  // TBD
  //
  return EFI_DEBUG_CONTINUE;
}


/**

  DebuggerCommand - CFGW.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Interrupt type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerExtPciCFGW (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  EDBPrint (L"Unsupported\n");
  //
  // TBD
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - CFGD.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Interrupt type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerExtPciCFGD (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  EDBPrint (L"Unsupported\n");
  //
  // TBD
  //
  return EFI_DEBUG_CONTINUE;
}
