/*++

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  EdbCmdExtIo.c

Abstract:


--*/

#include "Edb.h"

EFI_DEBUG_STATUS
DebuggerExtIoIB (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - IB

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_CONTINUE - formal return value

--*/
{
  EDBPrint (L"Unsupported\n");
  //
  // TBD
  //
  return EFI_DEBUG_CONTINUE;
}

EFI_DEBUG_STATUS
DebuggerExtIoIW (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - IW

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_CONTINUE - formal return value

--*/
{
  EDBPrint (L"Unsupported\n");
  //
  // TBD
  //
  return EFI_DEBUG_CONTINUE;
}

EFI_DEBUG_STATUS
DebuggerExtIoID (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - ID

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_CONTINUE - formal return value

--*/
{
  EDBPrint (L"Unsupported\n");
  //
  // TBD
  //
  return EFI_DEBUG_CONTINUE;
}

EFI_DEBUG_STATUS
DebuggerExtIoOB (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - OB

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_CONTINUE - formal return value

--*/
{
  EDBPrint (L"Unsupported\n");
  //
  // TBD
  //
  return EFI_DEBUG_CONTINUE;
}

EFI_DEBUG_STATUS
DebuggerExtIoOW (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - OW

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_CONTINUE - formal return value

--*/
{
  EDBPrint (L"Unsupported\n");
  //
  // TBD
  //
  return EFI_DEBUG_CONTINUE;
}

EFI_DEBUG_STATUS
DebuggerExtIoOD (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - OD

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_CONTINUE - formal return value

--*/
{
  EDBPrint (L"Unsupported\n");
  //
  // TBD
  //
  return EFI_DEBUG_CONTINUE;
}
