/*++

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  EdbCmdStep.c

Abstract:


--*/

#include "Edb.h"

BOOLEAN
IsEBCCALL (
  IN UINTN            Address
  )
/*++

Routine Description:

  Check whether current IP is EBC CALL instruction (NOTE: CALLEX is exclusive)

Arguments:

  Address   - EBC IP address.

Returns:

  TRUE  - Current IP is EBC CALL instruction
  FALSE - Current IP is not EBC CALL instruction

--*/
{
  if (GET_OPCODE(Address) != OPCODE_CALL) {
    return FALSE;
  }

  if (GET_OPERANDS (Address) & OPERAND_M_NATIVE_CALL) {
    return FALSE;
  } else {
    return TRUE;
  }
}

BOOLEAN
IsEBCRET (
  IN UINTN            Address
  )
/*++

Routine Description:

  Check whether current IP is EBC RET instruction

Arguments:

  Address   - EBC IP address.

Returns:

  TRUE  - Current IP is EBC RET instruction
  FALSE - Current IP is not EBC RET instruction

--*/
{
  if (GET_OPCODE(Address) != OPCODE_RET) {
    return FALSE;
  }

  if (GET_OPERANDS (Address) != 0) {
    return FALSE;
  } else {
    return TRUE;
  }
}

EFI_DEBUG_STATUS
DebuggerStepInto (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - StepInto

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_CONTINUE - formal return value

--*/
{
  SystemContext.SystemContextEbc->Flags |= VMFLAGS_STEP;

  return EFI_DEBUG_BREAK;
}

EFI_DEBUG_STATUS
DebuggerStepOver (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - StepOver

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_CONTINUE - formal return value

--*/
{
  if (IsEBCCALL((UINTN)SystemContext.SystemContextEbc->Ip)) {
    //
    // Check CALL (NOTE: CALLEX is exclusive)
    //
    DebuggerPrivate->FeatureFlags |= EFI_DEBUG_FLAG_EBC_STEPOVER;
  } else {
    //
    // Other instruction including CALLEX
    //
    SystemContext.SystemContextEbc->Flags |= VMFLAGS_STEP;
  }

  return EFI_DEBUG_BREAK;
}

EFI_DEBUG_STATUS
DebuggerStepOut (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - StepOut

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_CONTINUE - formal return value

--*/
{
  if (IsEBCRET((UINTN)SystemContext.SystemContextEbc->Ip)) {
    //
    // Check RET
    //
    SystemContext.SystemContextEbc->Flags |= VMFLAGS_STEP;
  } else {
    //
    // Other instruction
    //
    DebuggerPrivate->FeatureFlags |= EFI_DEBUG_FLAG_EBC_STEPOUT;
  }

  return EFI_DEBUG_BREAK;
}
