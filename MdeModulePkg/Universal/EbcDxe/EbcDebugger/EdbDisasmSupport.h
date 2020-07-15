/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _EFI_EDB_DISASM_SUPPORT_H_
#define _EFI_EDB_DISASM_SUPPORT_H_

#include <Uefi.h>

#define EDB_BYTECODE_NUMBER_IN_LINE     5

#ifdef EFI32
#define EDB_PRINT_ADDRESS_FORMAT    L"%08x: "
#else
// To use 012l instead of 016l because space is not enough
#define EDB_PRINT_ADDRESS_FORMAT    L"%012lx: "
#endif

#define OPCODE_MAX 0x40

#define EDB_INSTRUCTION_NAME_MAX_LENGTH     10
#define EDB_INSTRUCTION_NAME_MAX_SIZE       (EDB_INSTRUCTION_NAME_MAX_LENGTH * sizeof(CHAR16))
#define EDB_INSTRUCTION_CONTENT_MAX_LENGTH  30
#define EDB_INSTRUCTION_CONTENT_MAX_SIZE    (EDB_INSTRUCTION_CONTENT_MAX_LENGTH * sizeof(CHAR16))

/**

  Set offset for Instruction name and content.

  @param  InstructionNameOffset     - Instruction name offset
  @param  InstructionContentOffset  - Instruction content offset

**/
VOID
EdbSetOffset (
  IN UINTN InstructionNameOffset,
  IN UINTN InstructionContentOffset
  );

/**

  Pre instruction string construction.

  @return Instruction string

**/
CHAR16 *
EdbPreInstructionString (
  VOID
  );

/**

  Post instruction string construction.

  @return Instruction string

**/
CHAR16 *
EdbPostInstructionString (
  VOID
  );

/**

  Print the instruction name.

  @param  Name - instruction name

  @return Instruction name offset

**/
UINTN
EdbPrintInstructionName (
  IN CHAR16                 *Name
  );

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
  );

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
  );

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
  );

/**

  Print the hexical WORD raw index data to instruction content.

  @param  Data16 - WORD data

  @return Instruction content offset

**/
UINTN
EdbPrintRawIndexData16 (
  IN UINT16                 Data16
  );

/**

  Print the hexical DWORD raw index data to instruction content.

  @param  Data32 - DWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintRawIndexData32 (
  IN UINT32                 Data32
  );

/**

  Print the hexical QWORD raw index data to instruction content.

  @param  Data64 - QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintRawIndexData64 (
  IN UINT64                 Data64
  );

/**

  Print register 1 in operands.

  @param  Operands - instruction operands

  @return Instruction content offset

**/
UINTN
EdbPrintRegister1 (
  IN UINT8                  Operands
  );

/**

  Print register 2 in operands.

  @param  Operands - instruction operands

  @return Instruction content offset

**/
UINTN
EdbPrintRegister2 (
  IN UINT8                  Operands
  );

/**

  Print dedicated register 1 in operands.

  @param Operands - instruction operands

  @return Instruction content offset

**/
UINTN
EdbPrintDedicatedRegister1 (
  IN UINT8                  Operands
  );

/**

  Print dedicated register 2 in operands.

  @param  Operands - instruction operands

  @return Instruction content offset

**/
UINTN
EdbPrintDedicatedRegister2 (
  IN UINT8                  Operands
  );

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
  );

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
  );

/**

  Print the hexical BYTE immediate data to instruction content.

  @param  Data - BYTE data

  @return Instruction content offset

**/
UINTN
EdbPrintImmData8 (
  IN UINT8                  Data
  );

/**

  Print the hexical WORD immediate data to instruction content.

  @param  Data - WORD data

  @return Instruction content offset

**/
UINTN
EdbPrintImmData16 (
  IN UINT16                 Data
  );

/**

  Print the hexical DWORD immediate data to instruction content.

  @param  Data - DWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintImmData32 (
  IN UINT32                 Data
  );

/**

  Print the hexical QWORD immediate data to instruction content.

  @param  Data - QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintImmData64 (
  IN UINT64                 Data
  );

/**

  Print the decimal UINTN immediate data to instruction content.

  @param  Data - UINTN data

  @return Instruction content offset

**/
UINTN
EdbPrintImmDatan (
  IN UINTN                  Data
  );

/**

  Print the decimal QWORD immediate data to instruction content.

  @param  Data64 - QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintImmData64n (
  IN UINT64                 Data64
  );

/**

  Print the hexical BYTE to instruction content.

  @param  Data8 - BYTE data

  @return Instruction content offset

**/
UINTN
EdbPrintData8 (
  IN UINT8                  Data8
  );

/**

  Print the hexical WORD to instruction content.

  @param  Data16 - WORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData16 (
  IN UINT16                 Data16
  );

/**

  Print the hexical DWORD to instruction content.

  @param  Data32 - DWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData32 (
  IN UINT32                 Data32
  );

/**

  Print the hexical QWORD to instruction content.

  @param  Data64 - QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData64 (
  IN UINT64                 Data64
  );

/**

  Print the decimal unsigned UINTN to instruction content.

  @param  Data - unsigned UINTN data

  @return Instruction content offset

**/
UINTN
EdbPrintDatan (
  IN UINTN                  Data
  );

/**

  Print the decimal unsigned QWORD to instruction content.

  @param  Data64 - unsigned QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData64n (
  IN UINT64                 Data64
  );

/**

  Print the decimal signed BYTE to instruction content.

  @param  Data8 - signed BYTE data

  @return Instruction content offset

**/
UINTN
EdbPrintData8s (
  IN UINT8                  Data8
  );

/**

  Print the decimal signed WORD to instruction content.

  @param  Data16 - signed WORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData16s (
  IN UINT16                 Data16
  );

/**

  Print the decimal signed DWORD to instruction content.

  @param  Data32 - signed DWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData32s (
  IN UINT32                 Data32
  );

/**

  Print the decimal signed QWORD to instruction content.

  @param  Data64 - signed QWORD data

  @return Instruction content offset

**/
UINTN
EdbPrintData64s (
  IN UINT64                 Data64
  );

/**

  Print the comma to instruction content.

  @return Instruction content offset

**/
UINTN
EdbPrintComma (
  VOID
  );

/**

  Find the symbol string according to address, then print it.

  @param  Address - instruction address

  @retval 1 - symbol string is found and printed
  @retval 0 - symbol string not found

**/
UINTN
EdbFindAndPrintSymbol (
  IN UINTN                  Address
  );

/**

  Print the EBC byte code.

  @param  InstructionAddress - instruction address
  @param  InstructionNumber  - instruction number

**/
VOID
EdbPrintRaw (
  IN EFI_PHYSICAL_ADDRESS   InstructionAddress,
  IN UINTN                  InstructionNumber
  );

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
  );

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
  );

#endif
