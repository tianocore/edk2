/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "Edb.h"

/**

  Display memory unit.

  @param  Address         - Memory Address
  @param  Width           - Memory Width

  @return Length of the memory unit

**/
UINTN
EdbDisplayMemoryUnit (
  IN UINTN           Address,
  IN EDB_DATA_WIDTH  Width
  )
{
  UINT8   Data8;
  UINT16  Data16;
  UINT32  Data32;
  UINT64  Data64;

  //
  // Print according to width
  //
  switch (Width) {
    case EdbWidthUint8:
      CopyMem (&Data8, (VOID *)Address, sizeof (UINT8));
      EDBPrint (L"%02x ", Data8);
      return sizeof (UINT8);
    case EdbWidthUint16:
      CopyMem (&Data16, (VOID *)Address, sizeof (UINT16));
      EDBPrint (L"%04x ", Data16);
      return sizeof (UINT16);
    case EdbWidthUint32:
      CopyMem (&Data32, (VOID *)Address, sizeof (UINT32));
      EDBPrint (L"%08x ", Data32);
      return sizeof (UINT32);
    case EdbWidthUint64:
      CopyMem (&Data64, (VOID *)Address, sizeof (UINT64));
      EDBPrint (L"%016lx ", Data64);
      return sizeof (UINT64);
    default:
      ASSERT (FALSE);
      break;
  }

  //
  // something wrong
  //
  return 0;
}

/**

  Display memory.

  @param  Address         - Memory Address
  @param  Count           - Memory Count
  @param  Width           - Memory Width

**/
VOID
EdbDisplayMemory (
  IN UINTN           Address,
  IN UINTN           Count,
  IN EDB_DATA_WIDTH  Width
  )
{
  UINTN  LineNumber;
  UINTN  ByteNumber;
  UINTN  LineIndex;
  UINTN  ByteIndex;
  UINTN  NumberInLine;

  if (Count == 0) {
    return;
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
        (LineIndex != 0))
    {
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
      (LineIndex != 0))
  {
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

  return;
}

/**

  Entry memory.

  @param  Address         - Memory Address
  @param  Value           - Memory Value
  @param  Width           - Memory Width

**/
VOID
EdbEnterMemory (
  IN UINTN           Address,
  IN VOID            *Value,
  IN EDB_DATA_WIDTH  Width
  )
{
  switch (Width) {
    case EdbWidthUint8:
      CopyMem ((VOID *)Address, Value, sizeof (UINT8));
      break;
    case EdbWidthUint16:
      CopyMem ((VOID *)Address, Value, sizeof (UINT16));
      break;
    case EdbWidthUint32:
      CopyMem ((VOID *)Address, Value, sizeof (UINT32));
      break;
    case EdbWidthUint64:
      CopyMem ((VOID *)Address, Value, sizeof (UINT64));
      break;
    default:
      break;
  }

  return;
}

/**

  Get memory address and count.

  @param  CommandArg      - The argument for this command
  @param  Address         - Memory Address
  @param  Count           - Memory Count

  @retval EFI_SUCCESS           - memory address and count are got
  @retval EFI_INVALID_PARAMETER - something wrong

**/
EFI_STATUS
EdbGetMemoryAddressCount (
  IN CHAR16  *CommandArg,
  IN UINTN   *Address,
  IN UINTN   *Count
  )
{
  CHAR16      *CommandStr;
  UINTN       MemAddress;
  EFI_STATUS  Status;

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
      MemAddress = Xtoi (CommandStr);
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
    *Count = Xtoi (CommandStr);
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**

  Get memory address and value.

  @param  CommandArg      - The argument for this command
  @param  Address         - Memory Address
  @param  Value           - Memory Value

  @retval EFI_SUCCESS           - memory address and value are got
  @retval EFI_INVALID_PARAMETER - something wrong

**/
EFI_STATUS
EdbGetMemoryAddressValue (
  IN CHAR16  *CommandArg,
  IN UINTN   *Address,
  IN UINT64  *Value
  )
{
  CHAR16      *CommandStr;
  UINTN       MemAddress;
  EFI_STATUS  Status;

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
      MemAddress = Xtoi (CommandStr);
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

  *Value = LXtoi (CommandStr);

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**

  Display memory.

  @param  CommandArg      - The argument for this command
  @param  Width           - Memory Width

  @retval EFI_DEBUG_RETURN   - formal return value

**/
EFI_DEBUG_STATUS
DebuggerMemoryDisplay (
  IN     CHAR16          *CommandArg,
  IN     EDB_DATA_WIDTH  Width
  )
{
  EFI_STATUS  Status;
  UINTN       Address;
  UINTN       Count;

  //
  // Get memory address and count
  //
  Status = EdbGetMemoryAddressCount (CommandArg, &Address, &Count);
  if (EFI_ERROR (Status)) {
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

/**

  Enter memory.

  @param  CommandArg      - The argument for this command
  @param  Width           - Memory Width

  @retval EFI_DEBUG_RETURN   - formal return value

**/
EFI_DEBUG_STATUS
DebuggerMemoryEnter (
  IN     CHAR16          *CommandArg,
  IN     EDB_DATA_WIDTH  Width
  )
{
  EFI_STATUS  Status;
  UINTN       Address;
  UINT64      Value;

  //
  // Get memory address and value
  //
  Status = EdbGetMemoryAddressValue (CommandArg, &Address, &Value);
  if (EFI_ERROR (Status)) {
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

/**

  DebuggerCommand - DB.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Interrupt type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_RETURN   - formal return value

**/
EFI_DEBUG_STATUS
DebuggerMemoryDB (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  return DebuggerMemoryDisplay (CommandArg, EdbWidthUint8);
}

/**

  DebuggerCommand - DW.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Interrupt type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_RETURN   - formal return value

**/
EFI_DEBUG_STATUS
DebuggerMemoryDW (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  return DebuggerMemoryDisplay (CommandArg, EdbWidthUint16);
}

/**

  DebuggerCommand - DD.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Interrupt type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_RETURN   - formal return value

**/
EFI_DEBUG_STATUS
DebuggerMemoryDD (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  return DebuggerMemoryDisplay (CommandArg, EdbWidthUint32);
}

/**

  DebuggerCommand - DQ.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_RETURN   - formal return value

**/
EFI_DEBUG_STATUS
DebuggerMemoryDQ (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  return DebuggerMemoryDisplay (CommandArg, EdbWidthUint64);
}

/**

  DebuggerCommand - EB.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_RETURN   - formal return value

**/
EFI_DEBUG_STATUS
DebuggerMemoryEB (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  return DebuggerMemoryEnter (CommandArg, EdbWidthUint8);
}

/**

  DebuggerCommand - EW.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Interrupt type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_RETURN   - formal return value

**/
EFI_DEBUG_STATUS
DebuggerMemoryEW (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  return DebuggerMemoryEnter (CommandArg, EdbWidthUint16);
}

/**

  DebuggerCommand - ED.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_RETURN   - formal return value

**/
EFI_DEBUG_STATUS
DebuggerMemoryED (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  return DebuggerMemoryEnter (CommandArg, EdbWidthUint32);
}

/**

  DebuggerCommand - EQ.

  @param  CommandArg      - The argument for this command
  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  ExceptionType   - Exception type.
  @param  SystemContext   - EBC system context.

  @retval EFI_DEBUG_RETURN   - formal return value

**/
EFI_DEBUG_STATUS
DebuggerMemoryEQ (
  IN     CHAR16                     *CommandArg,
  IN     EFI_DEBUGGER_PRIVATE_DATA  *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE         ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  return DebuggerMemoryEnter (CommandArg, EdbWidthUint64);
}
