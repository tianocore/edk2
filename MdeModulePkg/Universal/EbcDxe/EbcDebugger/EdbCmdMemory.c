/*++

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  EdbCmdMemory.c

Abstract:


--*/

#include "Edb.h"

UINTN
EdbDisplayMemoryUnit (
  IN UINTN           Address,
  IN EDB_DATA_WIDTH  Width
  )
/*++

Routine Description:

  Display memory unit

Arguments:

  Address         - Memory Address
  Width           - Memory Width

Returns:

  Length of the memory unit

--*/
{
  UINT8  Data8;
  UINT16 Data16;
  UINT32 Data32;
  UINT64 Data64;

  //
  // Print accroding to width
  //
  switch (Width) {
  case EdbWidthUint8:
    CopyMem (&Data8, (VOID *)Address, sizeof(UINT8));
    EDBPrint (L"%02x ", Data8);
    return sizeof(UINT8);
  case EdbWidthUint16:
    CopyMem (&Data16, (VOID *)Address, sizeof(UINT16));
    EDBPrint (L"%04x ", Data16);
    return sizeof(UINT16);
  case EdbWidthUint32:
    CopyMem (&Data32, (VOID *)Address, sizeof(UINT32));
    EDBPrint (L"%08x ", Data32);
    return sizeof(UINT32);
  case EdbWidthUint64:
    CopyMem (&Data64, (VOID *)Address, sizeof(UINT64));
    EDBPrint (L"%016lx ", Data64);
    return sizeof(UINT64);
  default:
    ASSERT (FALSE);
    break;
  }

  //
  // something wrong
  //
  return 0;
}

VOID
EdbDisplayMemory (
  IN UINTN          Address,
  IN UINTN          Count,
  IN EDB_DATA_WIDTH Width
  )
/*++

Routine Description:

  Display memory

Arguments:

  Address         - Memory Address
  Count           - Memory Count
  Width           - Memory Width

Returns:

  None

--*/
{
  UINTN  LineNumber;
  UINTN  ByteNumber;
  UINTN  LineIndex;
  UINTN  ByteIndex;
  UINTN  NumberInLine;

  if (Count == 0) {
    return ;
  }

  //
  // Get line number and byte number
  //
  switch (Width) {
  case EdbWidthUint8:
    NumberInLine = 16;
    break;
  case EdbWidthUint16:
    NumberInLine = 8;
    break;
  case EdbWidthUint32:
    NumberInLine = 4;
    break;
  case EdbWidthUint64:
    NumberInLine = 2;
    break;
  default:
    return;
  }

  LineNumber = Count / NumberInLine;
  ByteNumber = Count % NumberInLine;
  if (ByteNumber == 0) {
    LineNumber -= 1;
    ByteNumber  = NumberInLine;
  }

  //
  // Print each line
  //
  for (LineIndex = 0; LineIndex < LineNumber; LineIndex++) {

    //
    // Break check
    //
    if (((LineIndex % EFI_DEBUGGER_LINE_NUMBER_IN_PAGE) == 0) &&
        (LineIndex != 0)) {
      if (SetPageBreak ()) {
        break;
      }
    }

    EDBPrint (EDB_PRINT_ADDRESS_FORMAT, (UINTN)Address);
    for (ByteIndex = 0; ByteIndex < NumberInLine; ByteIndex++) {
      Address += EdbDisplayMemoryUnit (Address, Width);
    }
    EDBPrint (L"\n");
  }

  //
  // Break check
  //
  if (((LineIndex % EFI_DEBUGGER_LINE_NUMBER_IN_PAGE) == 0) &&
      (LineIndex != 0)) {
    if (SetPageBreak ()) {
      return;
    }
  }

  //
  // Print last line
  //
  EDBPrint (EDB_PRINT_ADDRESS_FORMAT, (UINTN)Address);
  for (ByteIndex = 0; ByteIndex < ByteNumber; ByteIndex++) {
    Address += EdbDisplayMemoryUnit (Address, Width);
  }

  return ;
}

VOID
EdbEnterMemory (
  IN UINTN          Address,
  IN VOID           *Value,
  IN EDB_DATA_WIDTH Width
  )
/*++

Routine Description:

  Entry memory

Arguments:

  Address         - Memory Address
  Value           - Memory Value
  Width           - Memory Width

Returns:

  None

--*/
{
  switch (Width) {
  case EdbWidthUint8:
    CopyMem ((VOID *)Address, Value, sizeof(UINT8));
    break;
  case EdbWidthUint16:
    CopyMem ((VOID *)Address, Value, sizeof(UINT16));
    break;
  case EdbWidthUint32:
    CopyMem ((VOID *)Address, Value, sizeof(UINT32));
    break;
  case EdbWidthUint64:
    CopyMem ((VOID *)Address, Value, sizeof(UINT64));
    break;
  default:
    break;
  }

  return ;
}

EFI_STATUS
EdbGetMemoryAddressCount (
  IN CHAR16    *CommandArg,
  IN UINTN     *Address,
  IN UINTN     *Count
  )
/*++

Routine Description:

  Get memory address and count

Arguments:

  CommandArg      - The argument for this command
  Address         - Memory Address
  Count           - Memory Count

Returns:

  EFI_SUCCESS           - memory address and count are got
  EFI_INVALID_PARAMETER - something wrong

--*/
{
  CHAR16       *CommandStr;
  UINTN        MemAddress;
  EFI_STATUS   Status;

  //
  // Get Address
  //
  CommandStr = CommandArg;
  if (CommandStr == NULL) {
    EDBPrint (L"Memory: Address error!\n");
    return EFI_INVALID_PARAMETER;
  }
  Status = Symboltoi (CommandStr, &MemAddress);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      MemAddress = Xtoi(CommandStr);
    } else {
      //
      // Something wrong, let Symboltoi print error info.
      //
      EDBPrint (L"Command Argument error!\n");
      return EFI_INVALID_PARAMETER;
    }
  }
  *Address = MemAddress;

  //
  // Get Count
  //
  CommandStr = StrGetNextTokenLine (L" ");
  if (CommandStr == NULL) {
    *Count = 1;
  } else {
    *Count = Xtoi(CommandStr);
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

EFI_STATUS
EdbGetMemoryAddressValue (
  IN CHAR16    *CommandArg,
  IN UINTN     *Address,
  IN UINT64    *Value
  )
/*++

Routine Description:

  Get memory address and value

Arguments:

  CommandArg      - The argument for this command
  Address         - Memory Address
  Value           - Memory Value

Returns:

  EFI_SUCCESS           - memory address and value are got
  EFI_INVALID_PARAMETER - something wrong

--*/
{
  CHAR16       *CommandStr;
  UINTN        MemAddress;
  EFI_STATUS   Status;

  //
  // Get Address
  //
  CommandStr = CommandArg;
  if (CommandStr == NULL) {
    EDBPrint (L"Memory: Address error!\n");
    return EFI_INVALID_PARAMETER;
  }
  Status = Symboltoi (CommandStr, &MemAddress);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      MemAddress = Xtoi(CommandStr);
    } else {
      //
      // Something wrong, let Symboltoi print error info.
      //
      EDBPrint (L"Command Argument error!\n");
      return EFI_INVALID_PARAMETER;
    }
  }
  *Address = MemAddress;

  //
  // Get Value
  //
  CommandStr = StrGetNextTokenLine (L" ");
  if (CommandStr == NULL) {
    EDBPrint (L"Memory: Value error!\n");
    return EFI_INVALID_PARAMETER;
  }
  *Value = LXtoi(CommandStr);

  //
  // Done
  //
  return EFI_SUCCESS;
}

EFI_DEBUG_STATUS
DebuggerMemoryDisplay (
  IN     CHAR16                    *CommandArg,
  IN     EDB_DATA_WIDTH            Width
  )
/*++

Routine Description:

  Display memory

Arguments:

  CommandArg      - The argument for this command
  Width           - Memory Width

Returns:

  EFI_DEBUG_RETURN   - formal return value

--*/
{
  EFI_STATUS Status;
  UINTN      Address;
  UINTN      Count;

  //
  // Get memory address and count
  //
  Status = EdbGetMemoryAddressCount (CommandArg, &Address, &Count);
  if (EFI_ERROR(Status)) {
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Display memory
  //
  EdbDisplayMemory (Address, Count, Width);

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

EFI_DEBUG_STATUS
DebuggerMemoryEnter (
  IN     CHAR16                    *CommandArg,
  IN     EDB_DATA_WIDTH            Width
  )
/*++

Routine Description:

  Enter memory

Arguments:

  CommandArg      - The argument for this command
  Width           - Memory Width

Returns:

  EFI_DEBUG_RETURN   - formal return value

--*/
{
  EFI_STATUS Status;
  UINTN      Address;
  UINT64     Value;

  //
  // Get memory address and value
  //
  Status = EdbGetMemoryAddressValue (CommandArg, &Address, &Value);
  if (EFI_ERROR(Status)) {
    return EFI_DEBUG_CONTINUE;
  }

  //
  // Enter memory
  //
  EdbEnterMemory (Address, &Value, Width);

  //
  // Done
  //
  return EFI_DEBUG_CONTINUE;
}

EFI_DEBUG_STATUS
DebuggerMemoryDB (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - DB

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_RETURN   - formal return value

--*/
{
  return DebuggerMemoryDisplay (CommandArg, EdbWidthUint8);
}

EFI_DEBUG_STATUS
DebuggerMemoryDW (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - DW

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_RETURN   - formal return value

--*/
{
  return DebuggerMemoryDisplay (CommandArg, EdbWidthUint16);
}

EFI_DEBUG_STATUS
DebuggerMemoryDD (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - DD

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_RETURN   - formal return value

--*/
{
  return DebuggerMemoryDisplay (CommandArg, EdbWidthUint32);
}

EFI_DEBUG_STATUS
DebuggerMemoryDQ (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - DQ

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_RETURN   - formal return value

--*/
{
  return DebuggerMemoryDisplay (CommandArg, EdbWidthUint64);
}

EFI_DEBUG_STATUS
DebuggerMemoryEB (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - EB

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_RETURN   - formal return value

--*/
{
  return DebuggerMemoryEnter (CommandArg, EdbWidthUint8);
}

EFI_DEBUG_STATUS
DebuggerMemoryEW (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - EW

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_RETURN   - formal return value

--*/
{
  return DebuggerMemoryEnter (CommandArg, EdbWidthUint16);
}

EFI_DEBUG_STATUS
DebuggerMemoryED (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - ED

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_RETURN   - formal return value

--*/
{
  return DebuggerMemoryEnter (CommandArg, EdbWidthUint32);
}

EFI_DEBUG_STATUS
DebuggerMemoryEQ (
  IN     CHAR16                    *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  DebuggerCommand - EQ

Arguments:

  CommandArg      - The argument for this command
  DebuggerPrivate - EBC Debugger private data structure
  InterruptType   - Interrupt type.
  SystemContext   - EBC system context.

Returns:

  EFI_DEBUG_RETURN   - formal return value

--*/
{
  return DebuggerMemoryEnter (CommandArg, EdbWidthUint64);
}
