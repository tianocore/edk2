/** @file

Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "Edb.h"

/**

  Check whether current IP is EBC BREAK3 instruction.

  @param  Address    EBC IP address.

  @retval TRUE       Current IP is EBC BREAK3 instruction
  @retval FALSE      Current IP is not EBC BREAK3 instruction

**/
BOOLEAN
IsEBCBREAK3 (
  IN UINTN            Address
  )
{
  if (GET_OPCODE(Address) != OPCODE_BREAK) {
    return FALSE;
  }

  if (GET_OPERANDS (Address) != 3) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**

  Check whether the Address is already set in breakpoint.

  @param  DebuggerPrivate   EBC Debugger private data structure
  @param  Address           Breakpoint Address

  @retval TRUE              breakpoint is found
  @retval FALSE             breakpoint is not found

**/
BOOLEAN
DebuggerBreakpointIsDuplicated (
  IN EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN UINTN                     Address
  )
{
  UINTN  Index;

  //
  // Go through each breakpoint context
  //
  for (Index = 0; Index < DebuggerPrivate->DebuggerBreakpointCount; Index++) {
    if (DebuggerPrivate->DebuggerBreakpointContext[Index].BreakpointAddress == Address) {
      //
      // Found it
      //
      return TRUE;
    }
  }

  //
  // Not found
  //
  return FALSE;
}

/**

  Add this breakpoint.

  @param  DebuggerPrivate   EBC Debugger private data structure
  @param  Address           Breakpoint Address

  @retval EFI_SUCCESS            breakpoint added successfully
  @retval EFI_ALREADY_STARTED    breakpoint is already added
  @retval EFI_OUT_OF_RESOURCES   all the breakpoint entries are used

**/
EFI_STATUS
DebuggerBreakpointAdd (
  IN EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN UINTN                     Address
  )
{
  //
  // Check duplicated breakpoint
  //
  if (DebuggerBreakpointIsDuplicated (DebuggerPrivate, Address)) {
    EDBPrint (L"Breakpoint duplicated!\n");
    return EFI_ALREADY_STARTED;
  }

  //
  // Check whether the address is a breakpoint 3 instruction
  //
  if (IsEBCBREAK3 (Address)) {
    EDBPrint (L"Breakpoint can not be set on BREAK 3 instruction!\n");
    return EFI_ALREADY_STARTED;
  }

  if (DebuggerPrivate->DebuggerBreakpointCount >= EFI_DEBUGGER_BREAKPOINT_MAX) {
    EDBPrint (L"Breakpoint out of resource!\n");
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Set the breakpoint
  //
  DebuggerPrivate->DebuggerBreakpointContext[DebuggerPrivate->DebuggerBreakpointCount].BreakpointAddress = Address;
  DebuggerPrivate->DebuggerBreakpointContext[DebuggerPrivate->DebuggerBreakpointCount].State = TRUE;
  DebuggerPrivate->DebuggerBreakpointContext[DebuggerPrivate->DebuggerBreakpointCount].OldInstruction = 0;
  CopyMem (
    &DebuggerPrivate->DebuggerBreakpointContext[DebuggerPrivate->DebuggerBreakpointCount].OldInstruction,
    (VOID *)Address,
    sizeof(UINT16)
    );

  DebuggerPrivate->DebuggerBreakpointCount ++;

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**

  Delete this breakpoint.

  @param  DebuggerPrivate   EBC Debugger private data structure
  @param  Index             Breakpoint Index

  @retval EFI_SUCCESS     breakpoint deleted successfully
  @retval EFI_NOT_FOUND   breakpoint not found

**/
EFI_STATUS
DebuggerBreakpointDel (
  IN EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN UINTN                     Index
  )
{
  UINTN    BpIndex;

  if ((Index >= EFI_DEBUGGER_BREAKPOINT_MAX) ||
      (Index >= DebuggerPrivate->DebuggerBreakpointCount)) {
    return EFI_NOT_FOUND;
  }

  //
  // Delete this breakpoint
  //
  for (BpIndex = Index; BpIndex < DebuggerPrivate->DebuggerBreakpointCount - 1; BpIndex++) {
    DebuggerPrivate->DebuggerBreakpointContext[BpIndex] = DebuggerPrivate->DebuggerBreakpointContext[BpIndex + 1];
  }
  ZeroMem (
    &DebuggerPrivate->DebuggerBreakpointContext[BpIndex],
    sizeof(DebuggerPrivate->DebuggerBreakpointContext[BpIndex])
    );

  DebuggerPrivate->DebuggerBreakpointCount --;

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**

  Disable this breakpoint.

  @param  DebuggerPrivate   EBC Debugger private data structure
  @param  Index             Breakpoint Index

  @retval EFI_SUCCESS     breakpoint disabled successfully
  @retval EFI_NOT_FOUND   breakpoint not found

**/
EFI_STATUS
DebuggerBreakpointDis (
  IN EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN UINTN                     Index
  )
{
  if ((Index >= EFI_DEBUGGER_BREAKPOINT_MAX) ||
      (Index >= DebuggerPrivate->DebuggerBreakpointCount)) {
    return EFI_NOT_FOUND;
  }

  //
  // Disable this breakpoint
  //
  DebuggerPrivate->DebuggerBreakpointContext[Index].State = FALSE;

  return EFI_SUCCESS;
}

/**

  Enable this breakpoint.

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  Index           - Breakpoint Index

  @retval EFI_SUCCESS   - breakpoint enabled successfully
  @retval EFI_NOT_FOUND - breakpoint not found

**/
EFI_STATUS
DebuggerBreakpointEn (
  IN EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN UINTN                     Index
  )
{
  if ((Index >= EFI_DEBUGGER_BREAKPOINT_MAX) ||
      (Index >= DebuggerPrivate->DebuggerBreakpointCount)) {
    return EFI_NOT_FOUND;
  }

  //
  // Enable this breakpoint
  //
  DebuggerPrivate->DebuggerBreakpointContext[Index].State = TRUE;

  return EFI_SUCCESS;
}

/**

  DebuggerCommand - BreakpointList.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerBreakpointList (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  UINTN Index;

  //
  // Check breakpoint cound
  //
  if (DebuggerPrivate->DebuggerBreakpointCount == 0) {
    EDBPrint (L"No Breakpoint\n");
    return EFI_DEBUG_CONTINUE;
  } else if (DebuggerPrivate->DebuggerBreakpointCount > EFI_DEBUGGER_BREAKPOINT_MAX) {
    EDBPrint (L"Breakpoint too many!\n");
    DebuggerPrivate->DebuggerBreakpointCount = 0;
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Go through each breakpoint
  //
  EDBPrint (L"Breakpoint :\n");
  EDBPrint (L" Index   Address            Status\n");
  EDBPrint (L"======= ================== ========\n");
//EDBPrint (L"   1    0xFFFFFFFF00000000    *\n");
//EDBPrint (L"  12    0x00000000FFFFFFFF\n");
  for (Index = 0; Index < DebuggerPrivate->DebuggerBreakpointCount; Index++) {
    //
    // Print the breakpoint
    //
    EDBPrint (L"  %2d    0x%016lx", Index, DebuggerPrivate->DebuggerBreakpointContext[Index].BreakpointAddress);
    if (DebuggerPrivate->DebuggerBreakpointContext[Index].State) {
      EDBPrint (L"    *\n");
    } else {
      EDBPrint (L"\n");
    }
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - BreakpointSet.

  @param  CommandArg        The argument for this command
  @param  DebuggerPrivate   EBC Debugger private data structure
  @param  ExceptionType     Exception type.
  @param  SystemContext     EBC system context.

  @retval EFI_DEBUG_CONTINUE - formal return value

**/
EFI_DEBUG_STATUS
DebuggerBreakpointSet (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  UINTN      Address;
  EFI_STATUS Status;

  if (CommandArg == NULL) {
    EDBPrint (L"BreakpointSet Argument error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Get breakpoint address
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

  //
  // Add breakpoint
  //
  Status = DebuggerBreakpointAdd (DebuggerPrivate, Address);
  if (EFI_ERROR(Status)) {
    EDBPrint (L"BreakpointSet error!\n");
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - BreakpointClear

  @param  CommandArg        The argument for this command
  @param  DebuggerPrivate   EBC Debugger private data structure
  @param  ExceptionType     Exception type.
  @param  SystemContext     EBC system context.

  @retval EFI_DEBUG_CONTINUE   formal return value

**/
EFI_DEBUG_STATUS
DebuggerBreakpointClear (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  UINTN      Index;
  EFI_STATUS Status;

  if (CommandArg == NULL) {
    EDBPrint (L"BreakpointClear Argument error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  if (StriCmp (CommandArg, L"*") == 0) {
    //
    // delete all breakpoint
    //
    DebuggerPrivate->DebuggerBreakpointCount = 0;
    ZeroMem (DebuggerPrivate->DebuggerBreakpointContext, sizeof(DebuggerPrivate->DebuggerBreakpointContext));
    EDBPrint (L"All the Breakpoint is cleared\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Get breakpoint index
  //
  Index = Atoi(CommandArg);
  if (Index == (UINTN) -1) {
    EDBPrint (L"BreakpointClear Argument error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  if ((Index >= EFI_DEBUGGER_BREAKPOINT_MAX) ||
      (Index >= DebuggerPrivate->DebuggerBreakpointCount)) {
    EDBPrint (L"BreakpointClear error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Delete breakpoint
  //
  Status = DebuggerBreakpointDel (DebuggerPrivate, Index);
  if (EFI_ERROR(Status)) {
    EDBPrint (L"BreakpointClear error!\n");
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**

  DebuggerCommand - BreakpointDisable

  @param  CommandArg        The argument for this command
  @param  DebuggerPrivate   EBC Debugger private data structure
  @param  ExceptionType     Exception type.
  @param  SystemContext     EBC system context.

  @retval EFI_DEBUG_CONTINUE   formal return value

**/
EFI_DEBUG_STATUS
DebuggerBreakpointDisable (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  UINTN      Index;
  EFI_STATUS Status;

  if (CommandArg == NULL) {
    EDBPrint (L"BreakpointDisable Argument error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  if (StriCmp (CommandArg, L"*") == 0) {
    //
    // disable all breakpoint
    //
    for (Index = 0; Index < DebuggerPrivate->DebuggerBreakpointCount; Index++) {
      Status = DebuggerBreakpointDis (DebuggerPrivate, Index);
    }
    EDBPrint (L"All the Breakpoint is disabled\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Get breakpoint index
  //
  Index = Atoi(CommandArg);
  if (Index == (UINTN) -1) {
    EDBPrint (L"BreakpointDisable Argument error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Disable breakpoint
  //
  Status = DebuggerBreakpointDis (DebuggerPrivate, Index);
  if (EFI_ERROR(Status)) {
    EDBPrint (L"BreakpointDisable error!\n");
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

/**
  DebuggerCommand - BreakpointEnable.

  @param  CommandArg        The argument for this command
  @param  DebuggerPrivate   EBC Debugger private data structure
  @param  ExceptionType     Exception type.
  @param  SystemContext     EBC system context.

  @retval EFI_DEBUG_CONTINUE   formal return value

**/
EFI_DEBUG_STATUS
DebuggerBreakpointEnable (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  UINTN      Index;
  EFI_STATUS Status;

  if (CommandArg == NULL) {
    EDBPrint (L"BreakpointEnable Argument error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  if (StriCmp (CommandArg, L"*") == 0) {
    //
    // enable all breakpoint
    //
    for (Index = 0; Index < DebuggerPrivate->DebuggerBreakpointCount; Index++) {
      Status = DebuggerBreakpointEn (DebuggerPrivate, Index);
    }
    EDBPrint (L"All the Breakpoint is enabled\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Get breakpoint index
  //
  Index = Atoi(CommandArg);
  if (Index == (UINTN) -1) {
    EDBPrint (L"BreakpointEnable Argument error!\n");
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Enable breakpoint
  //
  Status = DebuggerBreakpointEn (DebuggerPrivate, Index);
  if (EFI_ERROR(Status)) {
    EDBPrint (L"BreakpointEnable error!\n");
  }

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}
