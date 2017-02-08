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

extern EDB_DISASM_INSTRUCTION mEdbDisasmInstructionTable[];

typedef struct {
  CHAR16    Name[EDB_INSTRUCTION_NAME_MAX_LENGTH];
  CHAR16    Content[EDB_INSTRUCTION_CONTENT_MAX_LENGTH];
  CHAR16    Tail;
} EDB_INSTRUCTION_STRING;

EDB_INSTRUCTION_STRING mInstructionString;
UINTN                  mInstructionNameOffset;
UINTN                  mInstructionContentOffset;

/**

  Set offset for Instruction name and content.

  @param  InstructionNameOffset     - Instruction name offset
  @param  InstructionContentOffset  - Instruction content offset

**/
VOID
EdbSetOffset (
  IN UINTN InstructionNameOffset,
  IN UINTN InstructionContentOffset
  )
{
  mInstructionNameOffset = InstructionNameOffset;
  mInstructionContentOffset = InstructionContentOffset;

  return ;
}

/**

  Pre instruction string construction.

  @return Instruction string

**/
CHAR16 *
EdbPreInstructionString (
  VOID
  )
{
  ZeroMem (&mInstructionString, sizeof(mInstructionString));
  mInstructionNameOffset    = 0;
  mInstructionContentOffset = 0;

  return (CHAR16 *)&mInstructionString;
}

/**

  Post instruction string construction.

  @return Instruction string

**/
CHAR16 *
EdbPostInstructionString (
  VOID
  )
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

/**

  Get Sign, NaturalUnits, and ConstantUnits of the WORD data.

  @param  Data16        - WORD data
  @param  NaturalUnits  - Natural Units of the WORD
  @param  ConstantUnits - Constant Units of the WORD

  @return Sign value of WORD

**/
BOOLEAN
EdbGetNaturalIndex16 (
  IN  UINT16  Data16,
  OUT UINTN   *NaturalUnits,
  OUT UINTN   *ConstantUnits
  )
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

/**

  Get Sign, NaturalUnits, and ConstantUnits of the DWORD data.

  @param  Data32        - DWORD data
  @param  NaturalUnits  - Natural Units of the DWORD
  @param  ConstantUnits - Constant Units of the DWORD

  @return Sign value of DWORD

**/
BOOLEAN
EdbGetNaturalIndex32 (
  IN  UINT32  Data32,
  OUT UINTN   *NaturalUnits,
  OUT UINTN   *ConstantUnits
  )
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

/**

  Get Sign, NaturalUnits, and ConstantUnits of the QWORD data.

  @param  Data64        - QWORD data
  @param  NaturalUnits  - Natural Units of the QWORD
  @param  ConstantUnits - Constant Units of the QWORD

  @return Sign value of QWORD

**/
BOOLEAN
EdbGetNaturalIndex64 (
  IN  UINT64  Data64,
  OUT UINT64  *NaturalUnits,
  OUT UINT64  *ConstantUnits
  )
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

/**

  Get Bit Width of the value.

  @param  Value - data

  @return Bit width

**/
UINT8
EdbGetBitWidth (
  IN UINT64  Value
  )
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

/**

  Print the instruction name.

  @param  Name - instruction name

  @return Instruction name offset

**/
UINTN
EdbPrintInstructionName (
  IN CHAR16                 *Name
  )
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

/**

  Print register 1 in operands.

  @param  Operands - instruction operands

  @return Instruction content offset

**/
UINTN
EdbPrintRegister1 (
  IN UINT8                  Operands
  )
{
  if ((Operands & OPERAND_M_INDIRECT1) != 0) {
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

/**

  Print register 2 in operands.

  @param  Operands - instruction operands

  @return Instruction content offset

**/
UINTN
EdbPrintRegister2 (
  IN UINT8                  Operands
  )
{
  if ((Operands & OPERAND_M_INDIRECT2) != 0) {
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

/**

  Print dedicated register 1 in operands.

  @param Operands - instruction operands

  @return Instruction content offset

**/
UINTN
EdbPrintDedicatedRegister1 (
  IN UINT8                  Operands
  )
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

/**

  Print dedicated register 2 in operands.

  @param  Operands - instruction operands

  @return Instruction content offset

**/
UINTN
EdbPrintDedicatedRegister2 (
  IN UINT8                  Operands
  )
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

/**

  Print the hexical UINTN index data to instruction content.

  @param  Sign          - Signed bit of UINTN data
  @param  NaturalUnits  - natural units of UINTN data
  @param  ConstantUnits - natural units of UINTN data

  @return Instruction content offset

**/
UINTN
EdbPrintIndexData (
  IN BOOLEAN                Sign,
  IN UINTN                  NaturalUnits,
  IN UINTN                  ConstantUnits
  )
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

/**

  Print the hexical QWORD index data to instruction content.

  @param  Sign          - Signed bit of QWORD data
  @param  NaturalUnits  - natural units of QWORD data
  @param  ConstantUnits - natural units of QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintIndexData64 (
  IN BOOLEAN                Sign,
  IN UINT64                 NaturalUnits,
  IN UINT64                 ConstantUnits
  )
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

/**

  Print the hexical WORD raw index data to instruction content.

  @param  Data16 - WORD data

  @return Instruction content offset

**/
UINTN
EdbPrintRawIndexData16 (
  IN UINT16                 Data16
  )
{
  BOOLEAN Sign;
  UINTN   NaturalUnits;
  UINTN   ConstantUnits;
  UINTN   Offset;

  Sign = EdbGetNaturalIndex16 (Data16, &NaturalUnits, &ConstantUnits);
  Offset = EdbPrintIndexData (Sign, NaturalUnits, ConstantUnits);

  return Offset;
}

/**

  Print the hexical DWORD raw index data to instruction content.

  @param  Data32 - DWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintRawIndexData32 (
  IN UINT32                 Data32
  )
{
  BOOLEAN Sign;
  UINTN   NaturalUnits;
  UINTN   ConstantUnits;
  UINTN   Offset;

  Sign = EdbGetNaturalIndex32 (Data32, &NaturalUnits, &ConstantUnits);
  Offset = EdbPrintIndexData (Sign, NaturalUnits, ConstantUnits);

  return Offset;
}

/**

  Print the hexical QWORD raw index data to instruction content.

  @param  Data64 - QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintRawIndexData64 (
  IN UINT64                 Data64
  )
{
  BOOLEAN Sign;
  UINT64  NaturalUnits;
  UINT64  ConstantUnits;
  UINTN   Offset;

  Sign = EdbGetNaturalIndex64 (Data64, &NaturalUnits, &ConstantUnits);
  Offset = EdbPrintIndexData64 (Sign, NaturalUnits, ConstantUnits);

  return Offset;
}

/**

  Print the hexical BYTE immediate data to instruction content.

  @param  Data - BYTE data

  @return Instruction content offset

**/
UINTN
EdbPrintImmData8 (
  IN UINT8                  Data
  )
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

/**

  Print the hexical WORD immediate data to instruction content.

  @param  Data - WORD data

  @return Instruction content offset

**/
UINTN
EdbPrintImmData16 (
  IN UINT16                 Data
  )
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

/**

  Print the hexical DWORD immediate data to instruction content.

  @param  Data - DWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintImmData32 (
  IN UINT32                 Data
  )
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

/**

  Print the hexical QWORD immediate data to instruction content.

  @param  Data - QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintImmData64 (
  IN UINT64                 Data
  )
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

/**

  Print the decimal UINTN immediate data to instruction content.

  @param  Data - UINTN data

  @return Instruction content offset

**/
UINTN
EdbPrintImmDatan (
  IN UINTN                  Data
  )
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

/**

  Print the decimal QWORD immediate data to instruction content.

  @param  Data64 - QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintImmData64n (
  IN UINT64                 Data64
  )
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

/**

  Print the hexical BYTE to instruction content.

  @param  Data8 - BYTE data

  @return Instruction content offset

**/
UINTN
EdbPrintData8 (
  IN UINT8                  Data8
  )
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

/**

  Print the hexical WORD to instruction content.

  @param  Data16 - WORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData16 (
  IN UINT16                 Data16
  )
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

/**

  Print the hexical DWORD to instruction content.

  @param  Data32 - DWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData32 (
  IN UINT32                 Data32
  )
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

/**

  Print the hexical QWORD to instruction content.

  @param  Data64 - QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData64 (
  IN UINT64                 Data64
  )
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

/**

  Print the decimal unsigned UINTN to instruction content.

  @param  Data - unsigned UINTN data

  @return Instruction content offset

**/
UINTN
EdbPrintDatan (
  IN UINTN                  Data
  )
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

/**

  Print the decimal unsigned QWORD to instruction content.

  @param  Data64 - unsigned QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData64n (
  IN UINT64                 Data64
  )
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

/**

  Print the decimal signed BYTE to instruction content.

  @param  Data8 - signed BYTE data

  @return Instruction content offset

**/
UINTN
EdbPrintData8s (
  IN UINT8                  Data8
  )
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

/**

  Print the decimal signed WORD to instruction content.

  @param  Data16 - signed WORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData16s (
  IN UINT16                 Data16
  )
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

/**

  Print the decimal signed DWORD to instruction content.

  @param  Data32 - signed DWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData32s (
  IN UINT32                 Data32
  )
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

/**

  Print the decimal signed QWORD to instruction content.

  @param  Data64 - signed QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData64s (
  IN UINT64                 Data64
  )
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

/**

  Print the comma to instruction content.

  @return Instruction content offset

**/
UINTN
EdbPrintComma (
  VOID
  )
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

/**

  Find the symbol string according to address, then print it.

  @param  Address - instruction address

  @retval 1 - symbol string is found and printed
  @retval 0 - symbol string not found

**/
UINTN
EdbFindAndPrintSymbol (
  IN UINTN                  Address
  )
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

/**

  Print the EBC byte code.

  @param  InstructionAddress - instruction address
  @param  InstructionNumber  - instruction number

**/
VOID
EdbPrintRaw (
  IN EFI_PHYSICAL_ADDRESS   InstructionAddress,
  IN UINTN                  InstructionNumber
  )
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

/**

  Print the EBC asm code.

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  SystemContext   - EBC system context.

  @retval EFI_SUCCESS - show disasm successfully

**/
EFI_STATUS
EdbShowDisasm (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_SYSTEM_CONTEXT        SystemContext
  )
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

/**

  Get register value according to the system context, and register index.

  @param  SystemContext   - EBC system context.
  @param  Index           - EBC register index

  @return register value

**/
UINT64
GetRegisterValue (
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  IN     UINT8                     Index
  )
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
