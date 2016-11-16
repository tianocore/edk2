/*++

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  EdbDisasmSupport.c

Abstract:


--*/

#include "Edb.h"

extern EDB_DISASM_INSTRUCTION mEdbDisasmInstructionTable[];

typedef struct {
  CHAR16    Name[EDB_INSTRUCTION_NAME_MAX_LENGTH];
  CHAR16    Content[EDB_INSTRUCTION_CONTENT_MAX_LENGTH];
  CHAR16    Tail;
} EDB_INSTRUCTION_STRING;

EDB_INSTRUCTION_STRING mInstructionString;
UINTN                  mInstructionNameOffset;
UINTN                  mInstructionContentOffset;

VOID
EdbSetOffset (
  IN UINTN InstructionNameOffset,
  IN UINTN InstructionContentOffset
  )
/*++

Routine Description:

  Set offset for Instruction name and content

Arguments:

  InstructionNameOffset     - Instruction name offset
  InstructionContentOffset  - Instruction content offset

Returns:

  None

--*/
{
  mInstructionNameOffset = InstructionNameOffset;
  mInstructionContentOffset = InstructionContentOffset;

  return ;
}

CHAR16 *
EdbPreInstructionString (
  VOID
  )
/*++

Routine Description:

  Pre instruction string construction

Arguments:

  None

Returns:

  Instruction string

--*/
{
  ZeroMem (&mInstructionString, sizeof(mInstructionString));
  mInstructionNameOffset    = 0;
  mInstructionContentOffset = 0;

  return (CHAR16 *)&mInstructionString;
}

CHAR16 *
EdbPostInstructionString (
  VOID
  )
/*++

Routine Description:

  Post instruction string construction

Arguments:

  None

Returns:

  Instruction string

--*/
{
  CHAR16 *Char;

  for (Char = (CHAR16 *)&mInstructionString; Char < &mInstructionString.Tail; Char++) {
    if (*Char == 0) {
      *Char = L' ';
    }
  }
  mInstructionString.Tail = 0;

  mInstructionNameOffset    = 0;
  mInstructionContentOffset = 0;

  return (CHAR16 *)&mInstructionString;
}

BOOLEAN
EdbGetNaturalIndex16 (
  IN  UINT16  Data16,
  OUT UINTN   *NaturalUnits,
  OUT UINTN   *ConstantUnits
  )
/*++

Routine Description:

  Get Sign, NaturalUnits, and ConstantUnits of the WORD data

Arguments:

  Data16        - WORD data
  NaturalUnits  - Natural Units of the WORD
  ConstantUnits - Constant Units of the WORD

Returns:

  Sign value of WORD

--*/
{
  BOOLEAN Sign;
  UINTN   NaturalUnitBit;

  Sign = (BOOLEAN)(Data16 >> 15);
  NaturalUnitBit = (UINTN)((Data16 >> 12) & 0x7);
  NaturalUnitBit *= 2;
  Data16 = Data16 & 0xFFF;
  *NaturalUnits = (UINTN)(Data16 & ((1 << NaturalUnitBit) - 1));
  *ConstantUnits = (UINTN)((Data16 >> NaturalUnitBit) & ((1 << (12 - NaturalUnitBit)) - 1));

  return Sign;
}

BOOLEAN
EdbGetNaturalIndex32 (
  IN  UINT32  Data32,
  OUT UINTN   *NaturalUnits,
  OUT UINTN   *ConstantUnits
  )
/*++

Routine Description:

  Get Sign, NaturalUnits, and ConstantUnits of the DWORD data

Arguments:

  Data32        - DWORD data
  NaturalUnits  - Natural Units of the DWORD
  ConstantUnits - Constant Units of the DWORD

Returns:

  Sign value of DWORD

--*/
{
  BOOLEAN Sign;
  UINTN   NaturalUnitBit;

  Sign = (BOOLEAN)(Data32 >> 31);
  NaturalUnitBit = (UINTN)((Data32 >> 28) & 0x7);
  NaturalUnitBit *= 4;
  Data32 = Data32 & 0xFFFFFFF;
  *NaturalUnits = (UINTN)(Data32 & ((1 << NaturalUnitBit) - 1));
  *ConstantUnits = (UINTN)((Data32 >> NaturalUnitBit) & ((1 << (28 - NaturalUnitBit)) - 1));

  return Sign;
}

BOOLEAN
EdbGetNaturalIndex64 (
  IN  UINT64  Data64,
  OUT UINT64  *NaturalUnits,
  OUT UINT64  *ConstantUnits
  )
/*++

Routine Description:

  Get Sign, NaturalUnits, and ConstantUnits of the QWORD data

Arguments:

  Data64        - QWORD data
  NaturalUnits  - Natural Units of the QWORD
  ConstantUnits - Constant Units of the QWORD

Returns:

  Sign value of QWORD

--*/
{
  BOOLEAN Sign;
  UINTN   NaturalUnitBit;

  Sign = (BOOLEAN)RShiftU64 (Data64, 63);
  NaturalUnitBit = (UINTN)(RShiftU64 (Data64, 60) & 0x7);
  NaturalUnitBit *= 8;
  Data64 = RShiftU64 (LShiftU64 (Data64, 4), 4);
  *NaturalUnits = (UINT64)(Data64 & (LShiftU64 (1, NaturalUnitBit) - 1));
  *ConstantUnits = (UINT64)(RShiftU64 (Data64, NaturalUnitBit) & (LShiftU64 (1, (60 - NaturalUnitBit)) - 1));

  return Sign;
}

UINT8
EdbGetBitWidth (
  IN UINT64  Value
  )
/*++

Routine Description:

  Get Bit Width of the value

Arguments:

  Value - data

Returns:

  Bit width

--*/
{
  if (Value >= 10000000000000) {
    return 14;
  } else if (Value >= 1000000000000) {
    return 13;
  } else if (Value >= 100000000000) {
    return 12;
  } else if (Value >= 10000000000) {
    return 11;
  } else if (Value >= 1000000000) {
    return 10;
  } else if (Value >= 100000000) {
    return 9;
  } else if (Value >= 10000000) {
    return 8;
  } else if (Value >= 1000000) {
    return 7;
  } else if (Value >= 100000) {
    return 6;
  } else if (Value >= 10000) {
    return 5;
  } else if (Value >= 1000) {
    return 4;
  } else if (Value >= 100) {
    return 3;
  } else if (Value >= 10) {
    return 2;
  } else {
    return 1;
  }
}

UINTN
EdbPrintInstructionName (
  IN CHAR16                 *Name
  )
/*++

Routine Description:

  Print the instruction name

Arguments:

  Name - instruction name

Returns:

  Instruction name offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Name,
    EDB_INSTRUCTION_NAME_MAX_SIZE,
    mInstructionNameOffset,
    L"%s",
    Name
    );
  mInstructionNameOffset += StrLen (Name);

  return mInstructionNameOffset;
}

UINTN
EdbPrintRegister1 (
  IN UINT8                  Operands
  )
/*++

Routine Description:

  Print register 1 in operands

Arguments:

  Operands - instruction operands

Returns:

  Instruction content offset

--*/
{
  if (Operands & OPERAND_M_INDIRECT1) {
    EDBSPrintWithOffset (
      mInstructionString.Content,
      EDB_INSTRUCTION_CONTENT_MAX_SIZE,
      mInstructionContentOffset,
      L"@"
      );
    mInstructionContentOffset += 1;
  }
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"R%d",
    (UINTN)(Operands & OPERAND_M_OP1)
    );
  mInstructionContentOffset += 2;

  return mInstructionContentOffset;
}

UINTN
EdbPrintRegister2 (
  IN UINT8                  Operands
  )
/*++

Routine Description:

  Print register 2 in operands

Arguments:

  Operands - instruction operands

Returns:

  Instruction content offset

--*/
{
  if (Operands & OPERAND_M_INDIRECT2) {
    EDBSPrintWithOffset (
      mInstructionString.Content,
      EDB_INSTRUCTION_CONTENT_MAX_SIZE,
      mInstructionContentOffset,
      L"@"
      );
    mInstructionContentOffset += 1;
  }
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"R%d",
    (UINTN)((Operands & OPERAND_M_OP2) >> 4)
    );
  mInstructionContentOffset += 2;

  return mInstructionContentOffset;
}

UINTN
EdbPrintDedicatedRegister1 (
  IN UINT8                  Operands
  )
/*++

Routine Description:

  Print dedicated register 1 in operands

Arguments:

  Operands - instruction operands

Returns:

  Instruction content offset

--*/
{
  switch (Operands & OPERAND_M_OP1) {
  case 0:
    EDBSPrintWithOffset (
      mInstructionString.Content,
      EDB_INSTRUCTION_CONTENT_MAX_SIZE,
      mInstructionContentOffset,
      L"[FLAGS]"
      );
    mInstructionContentOffset += 7;
    break;
  case 1:
    EDBSPrintWithOffset (
      mInstructionString.Content,
      EDB_INSTRUCTION_CONTENT_MAX_SIZE,
      mInstructionContentOffset,
      L"[IP]"
      );
    mInstructionContentOffset += 4;
    break;
  }

  return mInstructionContentOffset;
}

UINTN
EdbPrintDedicatedRegister2 (
  IN UINT8                  Operands
  )
/*++

Routine Description:

  Print dedicated register 2 in operands

Arguments:

  Operands - instruction operands

Returns:

  Instruction content offset

--*/
{
  switch ((Operands & OPERAND_M_OP2) >> 4) {
  case 0:
    EDBSPrintWithOffset (
      mInstructionString.Content,
      EDB_INSTRUCTION_CONTENT_MAX_SIZE,
      mInstructionContentOffset,
      L"[FLAGS]"
      );
    mInstructionContentOffset += 7;
    break;
  case 1:
    EDBSPrintWithOffset (
      mInstructionString.Content,
      EDB_INSTRUCTION_CONTENT_MAX_SIZE,
      mInstructionContentOffset,
      L"[IP]"
      );
    mInstructionContentOffset += 4;
    break;
  }

  return mInstructionContentOffset;
}

UINTN
EdbPrintIndexData (
  IN BOOLEAN                Sign,
  IN UINTN                  NaturalUnits,
  IN UINTN                  ConstantUnits
  )
/*++

Routine Description:

  Print the hexical UINTN index data to instruction content

Arguments:

  Sign          - Signed bit of UINTN data
  NaturalUnits  - natural units of UINTN data
  ConstantUnits - natural units of UINTN data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"(%s%d,%s%d)",
    Sign ? L"-" : L"+",
    NaturalUnits,
    Sign ? L"-" : L"+",
    ConstantUnits
    );
  mInstructionContentOffset  = mInstructionContentOffset + 5 + EdbGetBitWidth (NaturalUnits) + EdbGetBitWidth (ConstantUnits);

  return mInstructionContentOffset;
}

UINTN
EdbPrintIndexData64 (
  IN BOOLEAN                Sign,
  IN UINT64                 NaturalUnits,
  IN UINT64                 ConstantUnits
  )
/*++

Routine Description:

  Print the hexical QWORD index data to instruction content

Arguments:

  Sign          - Signed bit of QWORD data
  NaturalUnits  - natural units of QWORD data
  ConstantUnits - natural units of QWORD data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"(%s%ld,%s%ld)",
    Sign ? L"-" : L"+",
    NaturalUnits,
    Sign ? L"-" : L"+",
    ConstantUnits
    );
  mInstructionContentOffset  = mInstructionContentOffset + 5 + EdbGetBitWidth (NaturalUnits) + EdbGetBitWidth (ConstantUnits);

  return mInstructionContentOffset;
}

UINTN
EdbPrintRawIndexData16 (
  IN UINT16                 Data16
  )
/*++

Routine Description:

  Print the hexical WORD raw index data to instruction content

Arguments:

  Data16 - WORD data

Returns:

  Instruction content offset

--*/
{
  BOOLEAN Sign;
  UINTN   NaturalUnits;
  UINTN   ConstantUnits;
  UINTN   Offset;

  Sign = EdbGetNaturalIndex16 (Data16, &NaturalUnits, &ConstantUnits);
  Offset = EdbPrintIndexData (Sign, NaturalUnits, ConstantUnits);

  return Offset;
}

UINTN
EdbPrintRawIndexData32 (
  IN UINT32                 Data32
  )
/*++

Routine Description:

  Print the hexical DWORD raw index data to instruction content

Arguments:

  Data32 - DWORD data

Returns:

  Instruction content offset

--*/
{
  BOOLEAN Sign;
  UINTN   NaturalUnits;
  UINTN   ConstantUnits;
  UINTN   Offset;

  Sign = EdbGetNaturalIndex32 (Data32, &NaturalUnits, &ConstantUnits);
  Offset = EdbPrintIndexData (Sign, NaturalUnits, ConstantUnits);

  return Offset;
}

UINTN
EdbPrintRawIndexData64 (
  IN UINT64                 Data64
  )
/*++

Routine Description:

  Print the hexical QWORD raw index data to instruction content

Arguments:

  Data64 - QWORD data

Returns:

  Instruction content offset

--*/
{
  BOOLEAN Sign;
  UINT64  NaturalUnits;
  UINT64  ConstantUnits;
  UINTN   Offset;

  Sign = EdbGetNaturalIndex64 (Data64, &NaturalUnits, &ConstantUnits);
  Offset = EdbPrintIndexData64 (Sign, NaturalUnits, ConstantUnits);

  return Offset;
}

UINTN
EdbPrintImmData8 (
  IN UINT8                  Data
  )
/*++

Routine Description:

  Print the hexical BYTE immediate data to instruction content

Arguments:

  Data - BYTE data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"(0x%02x)",
    (UINTN)Data
    );
  mInstructionContentOffset  += 6;

  return mInstructionContentOffset;
}

UINTN
EdbPrintImmData16 (
  IN UINT16                 Data
  )
/*++

Routine Description:

  Print the hexical WORD immediate data to instruction content

Arguments:

  Data - WORD data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"(0x%04x)",
    (UINTN)Data
    );
  mInstructionContentOffset  += 8;

  return mInstructionContentOffset;
}

UINTN
EdbPrintImmData32 (
  IN UINT32                 Data
  )
/*++

Routine Description:

  Print the hexical DWORD immediate data to instruction content

Arguments:

  Data - DWORD data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"(0x%08x)",
    (UINTN)Data
    );
  mInstructionContentOffset  += 12;

  return mInstructionContentOffset;
}

UINTN
EdbPrintImmData64 (
  IN UINT64                 Data
  )
/*++

Routine Description:

  Print the hexical QWORD immediate data to instruction content

Arguments:

  Data - QWORD data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"(0x%016lx)",
    Data
    );
  mInstructionContentOffset  += 20;

  return mInstructionContentOffset;
}

UINTN
EdbPrintImmDatan (
  IN UINTN                  Data
  )
/*++

Routine Description:

  Print the decimal UINTN immediate data to instruction content

Arguments:

  Data - UINTN data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"(%d)",
    (UINTN)Data
    );
  mInstructionContentOffset  = mInstructionContentOffset + 2 + EdbGetBitWidth (Data);

  return mInstructionContentOffset;
}

UINTN
EdbPrintImmData64n (
  IN UINT64                 Data64
  )
/*++

Routine Description:

  Print the decimal QWORD immediate data to instruction content

Arguments:

  Data64 - QWORD data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"(%ld)",
    Data64
    );
  mInstructionContentOffset  = mInstructionContentOffset + 2 + EdbGetBitWidth (Data64);

  return mInstructionContentOffset;
}

UINTN
EdbPrintData8 (
  IN UINT8                  Data8
  )
/*++

Routine Description:

  Print the hexical BYTE to instruction content

Arguments:

  Data8 - BYTE data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"0x%02x",
    (UINTN)Data8
    );
  mInstructionContentOffset += 4;

  return mInstructionContentOffset;
}

UINTN
EdbPrintData16 (
  IN UINT16                 Data16
  )
/*++

Routine Description:

  Print the hexical WORD to instruction content

Arguments:

  Data16 - WORD data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"0x%04x",
    (UINTN)Data16
    );
  mInstructionContentOffset += 6;

  return mInstructionContentOffset;
}

UINTN
EdbPrintData32 (
  IN UINT32                 Data32
  )
/*++

Routine Description:

  Print the hexical DWORD to instruction content

Arguments:

  Data32 - DWORD data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"0x%08x",
    (UINTN)Data32
    );
  mInstructionContentOffset += 10;

  return mInstructionContentOffset;
}

UINTN
EdbPrintData64 (
  IN UINT64                 Data64
  )
/*++

Routine Description:

  Print the hexical QWORD to instruction content

Arguments:

  Data64 - QWORD data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"0x%016lx",
    (UINT64)Data64
    );
  mInstructionContentOffset += 18;

  return mInstructionContentOffset;
}

UINTN
EdbPrintDatan (
  IN UINTN                  Data
  )
/*++

Routine Description:

  Print the decimal unsigned UINTN to instruction content

Arguments:

  Data - unsigned UINTN data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"%d",
    (UINTN)Data
    );
  mInstructionContentOffset = mInstructionContentOffset + EdbGetBitWidth (Data);

  return mInstructionContentOffset;
}

UINTN
EdbPrintData64n (
  IN UINT64                 Data64
  )
/*++

Routine Description:

  Print the decimal unsigned QWORD to instruction content

Arguments:

  Data64 - unsigned QWORD data

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"%ld",
    Data64
    );
  mInstructionContentOffset = mInstructionContentOffset + EdbGetBitWidth (Data64);

  return mInstructionContentOffset;
}

UINTN
EdbPrintData8s (
  IN UINT8                  Data8
  )
/*++

Routine Description:

  Print the decimal signed BYTE to instruction content

Arguments:

  Data8 - signed BYTE data

Returns:

  Instruction content offset

--*/
{
  BOOLEAN Sign;

  Sign = (BOOLEAN)(Data8 >> 7);

  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"%s%d",
    Sign ? L"-" : L"+",
    (UINTN)(Data8 & 0x7F)
    );
  mInstructionContentOffset = mInstructionContentOffset + 1 + EdbGetBitWidth (Data8 & 0x7F);

  return mInstructionContentOffset;
}

UINTN
EdbPrintData16s (
  IN UINT16                 Data16
  )
/*++

Routine Description:

  Print the decimal signed WORD to instruction content

Arguments:

  Data16 - signed WORD data

Returns:

  Instruction content offset

--*/
{
  BOOLEAN Sign;

  Sign = (BOOLEAN)(Data16 >> 15);

  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"%s%d",
    Sign ? L"-" : L"+",
    (UINTN)(Data16 & 0x7FFF)
    );
  mInstructionContentOffset = mInstructionContentOffset + 1 + EdbGetBitWidth (Data16 & 0x7FFF);

  return mInstructionContentOffset;
}

UINTN
EdbPrintData32s (
  IN UINT32                 Data32
  )
/*++

Routine Description:

  Print the decimal signed DWORD to instruction content

Arguments:

  Data32 - signed DWORD data

Returns:

  Instruction content offset

--*/
{
  BOOLEAN Sign;

  Sign = (BOOLEAN)(Data32 >> 31);

  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"%s%d",
    Sign ? L"-" : L"+",
    (UINTN)(Data32 & 0x7FFFFFFF)
    );
  mInstructionContentOffset = mInstructionContentOffset + 1 + EdbGetBitWidth (Data32 & 0x7FFFFFFF);

  return mInstructionContentOffset;
}

UINTN
EdbPrintData64s (
  IN UINT64                 Data64
  )
/*++

Routine Description:

  Print the decimal signed QWORD to instruction content

Arguments:

  Data64 - signed QWORD data

Returns:

  Instruction content offset

--*/
{
  BOOLEAN Sign;
  INT64   Data64s;

  Sign = (BOOLEAN)RShiftU64 (Data64, 63);
  Data64s = (INT64)RShiftU64 (LShiftU64 (Data64, 1), 1);

  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L"%s%ld",
    Sign ? L"-" : L"+",
    (UINT64)Data64s
    );
  mInstructionContentOffset = mInstructionContentOffset + 1 + EdbGetBitWidth (Data64s);

  return mInstructionContentOffset;
}

UINTN
EdbPrintComma (
  VOID
  )
/*++

Routine Description:

  Print the comma to instruction content

Arguments:

  None

Returns:

  Instruction content offset

--*/
{
  EDBSPrintWithOffset (
    mInstructionString.Content,
    EDB_INSTRUCTION_CONTENT_MAX_SIZE,
    mInstructionContentOffset,
    L", "
    );
  mInstructionContentOffset += 2;

  return mInstructionContentOffset;
}

UINTN
EdbFindAndPrintSymbol (
  IN UINTN                  Address
  )
/*++

Routine Description:

  Find the symbol string according to address, then print it

Arguments:

  Address - instruction address

Returns:

  1 - symbol string is found and printed
  0 - symbol string not found

--*/
{
  CHAR8 *SymbolStr;

  SymbolStr = FindSymbolStr (Address);
  if (SymbolStr != NULL) {
    EDBSPrintWithOffset (
      mInstructionString.Content,
      EDB_INSTRUCTION_CONTENT_MAX_SIZE,
      mInstructionContentOffset,
      L"[%a]",
      SymbolStr
      );
    return 1;
  }

  return 0;
}

VOID
EdbPrintRaw (
  IN EFI_PHYSICAL_ADDRESS   InstructionAddress,
  IN UINTN                  InstructionNumber
  )
/*++

Routine Description:

  Print the EBC byte code

Arguments:

  InstructionAddress - instruction address
  InstructionNumber  - instruction number

Returns:

  None

--*/
{
  UINTN  LineNumber;
  UINTN  ByteNumber;
  UINTN  LineIndex;
  UINTN  ByteIndex;
  CHAR8  *SymbolStr;

  if (InstructionNumber == 0) {
    return ;
  }

  LineNumber = InstructionNumber / EDB_BYTECODE_NUMBER_IN_LINE;
  ByteNumber = InstructionNumber % EDB_BYTECODE_NUMBER_IN_LINE;
  if (ByteNumber == 0) {
    LineNumber -= 1;
    ByteNumber  = EDB_BYTECODE_NUMBER_IN_LINE;
  }

  //
  // Print Symbol
  //
  SymbolStr = FindSymbolStr ((UINTN)InstructionAddress);
  if (SymbolStr != NULL) {
    EDBPrint (L"[%a]:\n", SymbolStr);
  }

  for (LineIndex = 0; LineIndex < LineNumber; LineIndex++) {
    EDBPrint (EDB_PRINT_ADDRESS_FORMAT, (UINTN)InstructionAddress);
    for (ByteIndex = 0; ByteIndex < EDB_BYTECODE_NUMBER_IN_LINE; ByteIndex++) {
      EDBPrint (L"%02x ", *(UINT8 *)(UINTN)InstructionAddress);
      InstructionAddress += 1;
    }
    EDBPrint (L"\n");
  }

  EDBPrint (EDB_PRINT_ADDRESS_FORMAT, (UINTN)InstructionAddress);
  for (ByteIndex = 0; ByteIndex < ByteNumber; ByteIndex++) {
    EDBPrint (L"%02x ", *(UINT8 *)(UINTN)InstructionAddress);
    InstructionAddress += 1;
  }
  for (ByteIndex = 0; ByteIndex < EDB_BYTECODE_NUMBER_IN_LINE - ByteNumber; ByteIndex++) {
    EDBPrint (L"   ");
  }

  return ;
}

EFI_STATUS
EdbShowDisasm (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_SYSTEM_CONTEXT        SystemContext
  )
/*++

Routine Description:

  Print the EBC asm code

Arguments:

  DebuggerPrivate - EBC Debugger private data structure
  SystemContext   - EBC system context.

Returns:

  EFI_SUCCESS - show disasm successfully

--*/
{
  EFI_PHYSICAL_ADDRESS    InstructionAddress;
  UINTN                   InstructionNumber;
  UINTN                   InstructionLength;
  UINT8                   Opcode;
  CHAR16                  *InstructionString;
//  UINTN                   Result;

  InstructionAddress = DebuggerPrivate->InstructionScope;
  for (InstructionNumber = 0; InstructionNumber < DebuggerPrivate->InstructionNumber; InstructionNumber++) {

    //
    // Break each 0x10 instruction
    //
    if (((InstructionNumber % EFI_DEBUGGER_LINE_NUMBER_IN_PAGE) == 0) &&
        (InstructionNumber != 0)) {
      if (SetPageBreak ()) {
        break;
      }
    }

    Opcode = GET_OPCODE(InstructionAddress);
    if ((Opcode < OPCODE_MAX) && (mEdbDisasmInstructionTable[Opcode] != NULL)) {
      InstructionLength = mEdbDisasmInstructionTable [Opcode] (InstructionAddress, SystemContext, &InstructionString);
      if (InstructionLength != 0) {

        //
        // Print Source
        //
//        Result = EdbPrintSource ((UINTN)InstructionAddress, FALSE);

        if (!DebuggerPrivate->DebuggerSymbolContext.DisplayCodeOnly) {

          EdbPrintRaw (InstructionAddress, InstructionLength);
          if (InstructionString != NULL) {
            EDBPrint (L"%s\n", InstructionString);
          } else {
            EDBPrint (L"%s\n", L"<Unknown Instruction>");
          }
        }

        EdbPrintSource ((UINTN)InstructionAddress, TRUE);

        InstructionAddress += InstructionLength;
      } else {
        //
        // Something wrong with OPCODE
        //
        EdbPrintRaw (InstructionAddress, EDB_BYTECODE_NUMBER_IN_LINE);
        EDBPrint (L"%s\n", L"<Bad Instruction>");
        break;
      }
    } else {
      //
      // Something wrong with OPCODE
      //
      EdbPrintRaw (InstructionAddress, EDB_BYTECODE_NUMBER_IN_LINE);
      EDBPrint (L"%s\n", L"<Bad Instruction>");
      break;
    }
  }

  return EFI_SUCCESS;
}

UINT64
GetRegisterValue (
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  IN     UINT8                     Index
  )
/*++

Routine Description:

  Get register value accroding to the system context, and register index

Arguments:

  SystemContext   - EBC system context.
  Index           - EBC register index

Returns:

  register value

--*/
{
  switch (Index) {
  case 0:
    return SystemContext.SystemContextEbc->R0;
  case 1:
    return SystemContext.SystemContextEbc->R1;
  case 2:
    return SystemContext.SystemContextEbc->R2;
  case 3:
    return SystemContext.SystemContextEbc->R3;
  case 4:
    return SystemContext.SystemContextEbc->R4;
  case 5:
    return SystemContext.SystemContextEbc->R5;
  case 6:
    return SystemContext.SystemContextEbc->R6;
  case 7:
    return SystemContext.SystemContextEbc->R7;
  default:
    ASSERT (FALSE);
    break;
  }
  return 0;
}
