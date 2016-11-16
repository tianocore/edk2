/*++

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  EdbDisasmSupport.h

Abstract:


--*/

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

VOID
EdbSetOffset (
  IN UINTN InstructionNameOffset,
  IN UINTN InstructionContentOffset
  );

CHAR16 *
EdbPreInstructionString (
  VOID
  );

CHAR16 *
EdbPostInstructionString (
  VOID
  );

UINTN
EdbPrintInstructionName (
  IN CHAR16                 *Name
  );

BOOLEAN
EdbGetNaturalIndex16 (
  IN  UINT16  Data16,
  OUT UINTN   *NaturalUnits,
  OUT UINTN   *ConstantUnits
  );

BOOLEAN
EdbGetNaturalIndex32 (
  IN  UINT32  Data32,
  OUT UINTN   *NaturalUnits,
  OUT UINTN   *ConstantUnits
  );

BOOLEAN
EdbGetNaturalIndex64 (
  IN  UINT64  Data64,
  OUT UINT64  *NaturalUnits,
  OUT UINT64  *ConstantUnits
  );

UINTN
EdbPrintRawIndexData16 (
  IN UINT16                 Data16
  );

UINTN
EdbPrintRawIndexData32 (
  IN UINT32                 Data32
  );

UINTN
EdbPrintRawIndexData64 (
  IN UINT64                 Data64
  );

UINTN
EdbPrintRegister1 (
  IN UINT8                  Operands
  );

UINTN
EdbPrintRegister2 (
  IN UINT8                  Operands
  );

UINTN
EdbPrintDedicatedRegister1 (
  IN UINT8                  Operands
  );

UINTN
EdbPrintDedicatedRegister2 (
  IN UINT8                  Operands
  );

UINTN
EdbPrintIndexData (
  IN BOOLEAN                Sign,
  IN UINTN                  NaturalUnits,
  IN UINTN                  ConstantUnits
  );

UINTN
EdbPrintIndexData64 (
  IN BOOLEAN                Sign,
  IN UINT64                 NaturalUnits,
  IN UINT64                 ConstantUnits
  );

UINTN
EdbPrintImmData8 (
  IN UINT8                  Data
  );

UINTN
EdbPrintImmData16 (
  IN UINT16                 Data
  );

UINTN
EdbPrintImmData32 (
  IN UINT32                 Data
  );

UINTN
EdbPrintImmData64 (
  IN UINT64                 Data
  );

UINTN
EdbPrintImmDatan (
  IN UINTN                  Data
  );

UINTN
EdbPrintImmData64n (
  IN UINT64                 Data64
  );

UINTN
EdbPrintData8 (
  IN UINT8                  Data8
  );

UINTN
EdbPrintData16 (
  IN UINT16                 Data16
  );

UINTN
EdbPrintData32 (
  IN UINT32                 Data32
  );

UINTN
EdbPrintData64 (
  IN UINT64                 Data64
  );

UINTN
EdbPrintDatan (
  IN UINTN                  Data
  );

UINTN
EdbPrintData64n (
  IN UINT64                 Data64
  );

UINTN
EdbPrintData8s (
  IN UINT8                  Data8
  );

UINTN
EdbPrintData16s (
  IN UINT16                 Data16
  );

UINTN
EdbPrintData32s (
  IN UINT32                 Data32
  );

UINTN
EdbPrintData64s (
  IN UINT64                 Data64
  );

UINTN
EdbPrintComma (
  VOID
  );

UINTN
EdbFindAndPrintSymbol (
  IN UINTN                  Address
  );

VOID
EdbPrintRaw (
  IN EFI_PHYSICAL_ADDRESS   InstructionAddress,
  IN UINTN                  InstructionNumber
  );

EFI_STATUS
EdbShowDisasm (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_SYSTEM_CONTEXT        SystemContext
  );

UINT64
GetRegisterValue (
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  IN     UINT8                     Index
  );

#endif
