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

CHAR16 *mBranchTypeStr[] = {
  L"(CALL)",
  L"(CALLEX)",
  L"(RET)",
  L"(JMP)",
  L"(JMP8)",
};

/**

  Comvert Branch Type to string.

  @param Type        Branch Type

  @retval String     string of Branch Type.

**/
CHAR16 *
EdbBranchTypeToStr (
  IN EFI_DEBUGGER_BRANCH_TYPE  Type
  )
{
  if (Type < 0 || Type >= EfiDebuggerBranchTypeEbcMax) {
    return L"(Unknown Type)";
  }

  return mBranchTypeStr [Type];
}

/**

  DebuggerCommand - CallStack.

  @param  CommandArg         The argument for this command
  @param  DebuggerPrivate    EBC Debugger private data structure
  @param  ExceptionType      Exception type.
  @param  SystemContext      EBC system context.

  @retval EFI_DEBUG_CONTINUE   formal return value

**/
EFI_DEBUG_STATUS
DebuggerCallStack (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  INTN                           Index;
  UINTN                          SubIndex;
  CHAR8                          *FuncName;
  EFI_DEBUGGER_CALLSTACK_CONTEXT *CallStackEntry;
  BOOLEAN                        ShowParameter;
  UINTN                          ParameterNumber;

  ShowParameter = FALSE;
  ParameterNumber = EFI_DEBUGGER_CALL_DEFAULT_PARAMETER;

  //
  // Check argument
  //
  if (CommandArg != NULL) {
    if (StriCmp (CommandArg, L"c") == 0) {
      //
      // Clear Call-Stack
      //
      DebuggerPrivate->CallStackEntryCount = 0;
      ZeroMem (DebuggerPrivate->CallStackEntry, sizeof(DebuggerPrivate->CallStackEntry));
      EDBPrint (L"Call-Stack is cleared\n");
      return EFI_DEBUG_CONTINUE;
    } else if (StriCmp (CommandArg, L"p") == 0) {
      //
      // Print Call-Stack with parameter
      //
      ShowParameter = TRUE;
      CommandArg = StrGetNextTokenLine (L" ");
      if (CommandArg != NULL) {
        //
        // Try to get the parameter number
        //
        ParameterNumber = Atoi (CommandArg);
        if (ParameterNumber > 16) {
          EDBPrint (L"Call-Stack argument Invalid\n");
          return EFI_DEBUG_CONTINUE;
        }
      }
    } else {
      EDBPrint (L"Call-Stack argument Invalid\n");
      return EFI_DEBUG_CONTINUE;
    }
  }

  //
  // Check CallStack Entry Count
  //
  if (DebuggerPrivate->CallStackEntryCount == 0) {
    EDBPrint (L"No Call-Stack\n");
    return EFI_DEBUG_CONTINUE;
  } else if (DebuggerPrivate->CallStackEntryCount > EFI_DEBUGGER_CALLSTACK_MAX) {
    EDBPrint (L"Call-Stack Crash, re-initialize!\n");
    DebuggerPrivate->CallStackEntryCount = 0;
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Go through each CallStack entry and print
  //
  EDBPrint (L"Call-Stack (TOP):\n");
  EDBPrint (L"         Caller             Callee        Name\n");
  EDBPrint (L"  ================== ================== ========\n");
//EDBPrint (L"  0x00000000FFFFFFFF 0xFFFFFFFF00000000 EfiMain\n");
  for (Index = (INTN)(DebuggerPrivate->CallStackEntryCount - 1); Index >= 0; Index--) {
    //
    // Get CallStack and print
    //
    CallStackEntry = &DebuggerPrivate->CallStackEntry[Index];
    EDBPrint (
      L"  0x%016lx 0x%016lx",
      CallStackEntry->SourceAddress,
      CallStackEntry->DestAddress
      );
    FuncName = FindSymbolStr ((UINTN)CallStackEntry->DestAddress);
    if (FuncName != NULL) {
      EDBPrint (L" %a()", FuncName);
    }
    EDBPrint (L"\n");

    if (ShowParameter) {
      //
      // Print parameter
      //
      if (sizeof(UINTN) == sizeof(UINT64)) {
        EDBPrint (
          L"    Parameter Address (0x%016lx) (\n",
          CallStackEntry->ParameterAddr
          );
        if (ParameterNumber == 0) {
          EDBPrint (L"        )\n");
          continue;
        }
        //
        // Print each parameter
        //
        for (SubIndex = 0; SubIndex < ParameterNumber - 1; SubIndex++) {
          if (SubIndex % 2 == 0) {
            EDBPrint (L"        ");
          }
          EDBPrint (
            L"0x%016lx, ",
            CallStackEntry->Parameter[SubIndex]
            );
          if (SubIndex % 2 == 1) {
            EDBPrint (L"\n");
          }
        }
        if (SubIndex % 2 == 0) {
          EDBPrint (L"        ");
        }
        EDBPrint (
          L"0x%016lx\n",
          CallStackEntry->Parameter[SubIndex]
          );
        EDBPrint (L"        )\n");
        //
        // break only for parameter
        //
        if ((((DebuggerPrivate->CallStackEntryCount - Index) % (16 / ParameterNumber)) == 0) &&
            (Index != 0)) {
          if (SetPageBreak ()) {
            break;
          }
        }
      } else {
        EDBPrint (
          L"    Parameter Address (0x%08x) (\n",
          CallStackEntry->ParameterAddr
          );
        if (ParameterNumber == 0) {
          EDBPrint (L"        )\n");
          continue;
        }
        //
        // Print each parameter
        //
        for (SubIndex = 0; SubIndex < ParameterNumber - 1; SubIndex++) {
          if (SubIndex % 4 == 0) {
            EDBPrint (L"        ");
          }
          EDBPrint (
            L"0x%08x, ",
            CallStackEntry->Parameter[SubIndex]
            );
          if (SubIndex % 4 == 3) {
            EDBPrint (L"\n");
          }
        }
        if (SubIndex % 4 == 0) {
          EDBPrint (L"        ");
        }
        EDBPrint (
          L"0x%08x\n",
          CallStackEntry->Parameter[SubIndex]
          );
        EDBPrint (L"        )\n");
        //
        // break only for parameter
        //
        if ((((DebuggerPrivate->CallStackEntryCount - Index) % (32 / ParameterNumber)) == 0) &&
            (Index != 0)) {
          if (SetPageBreak ()) {
            break;
          }
        }
      }
    }
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - InstructionBranch.

  @param  CommandArg             The argument for this command
  @param  DebuggerPrivate        EBC Debugger private data structure
  @param  ExceptionType          Exception type.
  @param  SystemContext          EBC system context.

  @retval  EFI_DEBUG_CONTINUE    formal return value

**/
EFI_DEBUG_STATUS
DebuggerInstructionBranch (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  UINTN  Index;

  //
  // Check argument
  //
  if (CommandArg != NULL) {
    if (StriCmp (CommandArg, L"c") == 0) {
      //
      // Clear Trace
      //
      DebuggerPrivate->TraceEntryCount = 0;
      ZeroMem (DebuggerPrivate->TraceEntry, sizeof(DebuggerPrivate->TraceEntry));
      EDBPrint (L"Instruction Trace is cleared\n");
    } else {
      EDBPrint (L"Trace argument Invalid\n");
    }
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Check Trace Entry Count
  //
  if (DebuggerPrivate->TraceEntryCount == 0) {
    EDBPrint (L"No Instruction Trace\n");
    return EFI_DEBUG_CONTINUE;
  } else if (DebuggerPrivate->TraceEntryCount > EFI_DEBUGGER_TRACE_MAX) {
    EDBPrint (L"Instruction Trace Crash, re-initialize!\n");
    DebuggerPrivate->TraceEntryCount = 0;
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Go through each Trace entry and print
  //
  EDBPrint (L"Instruction Trace (->Latest):\n");
  EDBPrint (L"    Source Addr        Destination Addr   Type\n");
  EDBPrint (L"  ================== ================== ========\n");
//EDBPrint (L"  0x00000000FFFFFFFF 0xFFFFFFFF00000000  (CALLEX)\n");
  for (Index = 0; Index < DebuggerPrivate->TraceEntryCount; Index++) {
    EDBPrint (
      L"  0x%016lx 0x%016lx  %s\n",
      DebuggerPrivate->TraceEntry[Index].SourceAddress,
      DebuggerPrivate->TraceEntry[Index].DestAddress,
      EdbBranchTypeToStr (DebuggerPrivate->TraceEntry[Index].Type)
      );
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}
