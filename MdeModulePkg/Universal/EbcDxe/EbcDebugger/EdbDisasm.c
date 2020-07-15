/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "Edb.h"

//
// Debugger Disasm definition
//
#define EDB_DISASM_DEFINE(func) \
UINTN \
func ( \
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress, \
  IN     EFI_SYSTEM_CONTEXT        SystemContext, \
  OUT    CHAR16                    **DisasmString \
  )

EDB_DISASM_DEFINE (EdbDisasmBREAK);
EDB_DISASM_DEFINE (EdbDisasmJMP);
EDB_DISASM_DEFINE (EdbDisasmJMP8);
EDB_DISASM_DEFINE (EdbDisasmCALL);
EDB_DISASM_DEFINE (EdbDisasmRET);
EDB_DISASM_DEFINE (EdbDisasmCMP);
EDB_DISASM_DEFINE (EdbDisasmUnsignedDataManip);
EDB_DISASM_DEFINE (EdbDisasmSignedDataManip);
EDB_DISASM_DEFINE (EdbDisasmMOVxx);
EDB_DISASM_DEFINE (EdbDisasmMOVsnw);
EDB_DISASM_DEFINE (EdbDisasmMOVsnd);
EDB_DISASM_DEFINE (EdbDisasmLOADSP);
EDB_DISASM_DEFINE (EdbDisasmSTORESP);
EDB_DISASM_DEFINE (EdbDisasmPUSH);
EDB_DISASM_DEFINE (EdbDisasmPOP);
EDB_DISASM_DEFINE (EdbDisasmCMPI);
EDB_DISASM_DEFINE (EdbDisasmPUSHn);
EDB_DISASM_DEFINE (EdbDisasmPOPn);
EDB_DISASM_DEFINE (EdbDisasmMOVI);
EDB_DISASM_DEFINE (EdbDisasmMOVIn);
EDB_DISASM_DEFINE (EdbDisasmMOVREL);

//
// Debugger Disasm Table
//
EDB_DISASM_INSTRUCTION mEdbDisasmInstructionTable[] = {
  EdbDisasmBREAK,             // opcode 0x00 BREAK
  EdbDisasmJMP,               // opcode 0x01 JMP
  EdbDisasmJMP8,              // opcode 0x02 JMP8
  EdbDisasmCALL,              // opcode 0x03 CALL
  EdbDisasmRET,               // opcode 0x04 RET
  EdbDisasmCMP,               // opcode 0x05 CMPEQ
  EdbDisasmCMP,               // opcode 0x06 CMPLTE
  EdbDisasmCMP,               // opcode 0x07 CMPGTE
  EdbDisasmCMP,               // opcode 0x08 CMPULTE
  EdbDisasmCMP,               // opcode 0x09 CMPUGTE
  EdbDisasmUnsignedDataManip, // opcode 0x0A NOT
  EdbDisasmSignedDataManip,   // opcode 0x0B NEG
  EdbDisasmSignedDataManip,   // opcode 0x0C ADD
  EdbDisasmSignedDataManip,   // opcode 0x0D SUB
  EdbDisasmSignedDataManip,   // opcode 0x0E MUL
  EdbDisasmUnsignedDataManip, // opcode 0x0F MULU
  EdbDisasmSignedDataManip,   // opcode 0x10 DIV
  EdbDisasmUnsignedDataManip, // opcode 0x11 DIVU
  EdbDisasmSignedDataManip,   // opcode 0x12 MOD
  EdbDisasmUnsignedDataManip, // opcode 0x13 MODU
  EdbDisasmUnsignedDataManip, // opcode 0x14 AND
  EdbDisasmUnsignedDataManip, // opcode 0x15 OR
  EdbDisasmUnsignedDataManip, // opcode 0x16 XOR
  EdbDisasmUnsignedDataManip, // opcode 0x17 SHL
  EdbDisasmUnsignedDataManip, // opcode 0x18 SHR
  EdbDisasmSignedDataManip,   // opcode 0x19 ASHR
  EdbDisasmUnsignedDataManip, // opcode 0x1A EXTNDB
  EdbDisasmUnsignedDataManip, // opcode 0x1B EXTNDW
  EdbDisasmUnsignedDataManip, // opcode 0x1C EXTNDD
  EdbDisasmMOVxx,             // opcode 0x1D MOVBW
  EdbDisasmMOVxx,             // opcode 0x1E MOVWW
  EdbDisasmMOVxx,             // opcode 0x1F MOVDW
  EdbDisasmMOVxx,             // opcode 0x20 MOVQW
  EdbDisasmMOVxx,             // opcode 0x21 MOVBD
  EdbDisasmMOVxx,             // opcode 0x22 MOVWD
  EdbDisasmMOVxx,             // opcode 0x23 MOVDD
  EdbDisasmMOVxx,             // opcode 0x24 MOVQD
  EdbDisasmMOVsnw,            // opcode 0x25 MOVSNW
  EdbDisasmMOVsnd,            // opcode 0x26 MOVSND
  NULL,                       // opcode 0x27
  EdbDisasmMOVxx,             // opcode 0x28 MOVQQ
  EdbDisasmLOADSP,            // opcode 0x29 LOADSP
  EdbDisasmSTORESP,           // opcode 0x2A STORESP
  EdbDisasmPUSH,              // opcode 0x2B PUSH
  EdbDisasmPOP,               // opcode 0x2C POP
  EdbDisasmCMPI,              // opcode 0x2D CMPIEQ
  EdbDisasmCMPI,              // opcode 0x2E CMPILTE
  EdbDisasmCMPI,              // opcode 0x2F CMPIGTE
  EdbDisasmCMPI,              // opcode 0x30 CMPIULTE
  EdbDisasmCMPI,              // opcode 0x31 CMPIUGTE
  EdbDisasmMOVxx,             // opcode 0x32 MOVNW
  EdbDisasmMOVxx,             // opcode 0x33 MOVND
  NULL,                       // opcode 0x34
  EdbDisasmPUSHn,             // opcode 0x35 PUSHN
  EdbDisasmPOPn,              // opcode 0x36 POPN
  EdbDisasmMOVI,              // opcode 0x37 MOVI
  EdbDisasmMOVIn,             // opcode 0x38 MOVIN
  EdbDisasmMOVREL,            // opcode 0x39 MOVREL
};

/**

  Disasm instruction - BREAK.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmBREAK (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_BREAK);

  if (*(UINT8 *)(UINTN)(InstructionAddress + 1) > 6) {
    return 0;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"BREAK");
    EdbPrintDatan (*(UINT8 *)(UINTN)(InstructionAddress + 1));

    EdbPostInstructionString ();
  }

  return 2;
}

extern CONST UINT8                    mJMPLen[];

/**

  Disasm instruction - JMP.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmJMP (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8   Modifiers;
  UINT8   Operands;
  UINTN   Size;
  UINT32  Data32;
  UINT64  Data64;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_JMP);

  Modifiers  = GET_MODIFIERS (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);
  Size = (UINTN)mJMPLen[(Modifiers >> 6) & 0x03];

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"JMP");
//    if (Modifiers & OPCODE_M_IMMDATA64) {
//      EdbPrintInstructionName (L"64");
//    } else {
//      EdbPrintInstructionName (L"32");
//    }
    if ((Modifiers & CONDITION_M_CONDITIONAL) != 0) {
      if ((Modifiers & JMP_M_CS) != 0) {
        EdbPrintInstructionName (L"cs");
      } else {
        EdbPrintInstructionName (L"cc");
      }
    }

    InstructionAddress += 2;
    if ((Modifiers & OPCODE_M_IMMDATA64) != 0) {
      CopyMem (&Data64, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT64));
      if ((Modifiers & OPCODE_M_IMMDATA) != 0) {
        EdbPrintData64 (Data64);
      } else {
        return 0;
      }
    } else {
      CopyMem (&Data32, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT32));
      EdbPrintRegister1 (Operands);

      if ((Operands & OPERAND_M_INDIRECT1) == 0) {
        if ((Modifiers & OPCODE_M_IMMDATA) == 0) {
          Data32 = 0;
        }
        EdbPrintImmDatan (Data32);
      } else {
        EdbPrintRawIndexData32 (Data32);
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - JMP8.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmJMP8 (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8   Modifiers;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_JMP8);
  Modifiers  = GET_MODIFIERS (InstructionAddress);

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"JMP8");
    if ((Modifiers & CONDITION_M_CONDITIONAL) != 0) {
      if ((Modifiers & JMP_M_CS) != 0) {
        EdbPrintInstructionName (L"cs");
      } else {
        EdbPrintInstructionName (L"cc");
      }
    }

    EdbPrintData8 (*(UINT8 *)(UINTN)(InstructionAddress + 1));

    EdbPostInstructionString ();
  }

  return 2;
}

/**

  Disasm instruction - CALL.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmCALL (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8   Modifiers;
  UINT8   Operands;
  UINTN   Size;
  UINT32  Data32;
  UINT64  Data64;
  UINT64  Ip;
  UINTN   Result;
  EFI_PHYSICAL_ADDRESS      SavedInstructionAddress;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_CALL);
  SavedInstructionAddress = InstructionAddress;

  Modifiers  = GET_MODIFIERS (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);
  Size = (UINTN)mJMPLen[(Modifiers >> 6) & 0x03];

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"CALL");
//    if (Modifiers & OPCODE_M_IMMDATA64) {
//      EdbPrintInstructionName (L"64");
//    } else {
//      EdbPrintInstructionName (L"32");
//    }
    if ((Operands & OPERAND_M_NATIVE_CALL) != 0) {
      EdbPrintInstructionName (L"EX");
    }
//    if ((Operands & OPERAND_M_RELATIVE_ADDR) == 0) {
//      EdbPrintInstructionName (L"a");
//    }

    InstructionAddress += 2;
    if ((Modifiers & OPCODE_M_IMMDATA64) != 0) {
      CopyMem (&Data64, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT64));
      Ip = Data64;
      if ((Modifiers & OPCODE_M_IMMDATA) != 0) {
        Result = EdbFindAndPrintSymbol ((UINTN)Ip);
        if (Result == 0) {
          EdbPrintData64 (Data64);
        }
      } else {
        return 0;
      }
    } else {
      if ((Modifiers & OPCODE_M_IMMDATA) != 0) {
        CopyMem (&Data32, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT32));
      } else {
        Data32 = 0;
      }

      if ((Operands & OPERAND_M_OP1) == 0) {
        Ip = (UINT64)Data32;
      } else {
        Ip = GetRegisterValue (SystemContext, (Operands & OPERAND_M_OP1));
      }

      if ((Operands & OPERAND_M_INDIRECT1) == 0) {
        if ((Operands & OPERAND_M_RELATIVE_ADDR) != 0) {
          Result = EdbFindAndPrintSymbol ((UINTN)(SavedInstructionAddress + Ip + Size));
        } else {
          Result = EdbFindAndPrintSymbol ((UINTN)Ip);
        }
        if (Result == 0) {
          EdbPrintRegister1 (Operands);
          if ((Modifiers & OPCODE_M_IMMDATA) != 0) {
            EdbPrintImmData32 (Data32);
          }
        }
      } else {
        EdbPrintRegister1 (Operands);
        if ((Modifiers & OPCODE_M_IMMDATA) != 0) {
          EdbPrintRawIndexData32 (Data32);
        }
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - RET.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmRET (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_RET);

  if (*(UINT8 *)(UINTN)(InstructionAddress + 1) != 0) {
    return 0;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"RET");

    EdbPostInstructionString ();
  }

  return 2;
}

/**

  Disasm instruction - CMP.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmCMP (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Opcode;
  UINT8  Modifiers;
  UINT8  Operands;
  UINT16 Data16;
  UINTN  Size;

  ASSERT (
    (GET_OPCODE(InstructionAddress) == OPCODE_CMPEQ)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_CMPLTE)  ||
    (GET_OPCODE(InstructionAddress) == OPCODE_CMPGTE)  ||
    (GET_OPCODE(InstructionAddress) == OPCODE_CMPULTE) ||
    (GET_OPCODE(InstructionAddress) == OPCODE_CMPUGTE)
    );

  Opcode     = GET_OPCODE (InstructionAddress);
  Modifiers  = GET_MODIFIERS (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);
  if ((Modifiers & OPCODE_M_IMMDATA) != 0) {
    Size = 4;
  } else {
    Size = 2;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"CMP");
//    if (Modifiers & OPCODE_M_64BIT) {
//      EdbPrintInstructionName (L"64");
//    } else {
//      EdbPrintInstructionName (L"32");
//    }
    switch (Opcode) {
    case OPCODE_CMPEQ:
      EdbPrintInstructionName (L"eq");
      break;
    case OPCODE_CMPLTE:
      EdbPrintInstructionName (L"lte");
      break;
    case OPCODE_CMPGTE:
      EdbPrintInstructionName (L"gte");
      break;
    case OPCODE_CMPULTE:
      EdbPrintInstructionName (L"ulte");
      break;
    case OPCODE_CMPUGTE:
      EdbPrintInstructionName (L"ugte");
      break;
    }

    EdbPrintRegister1 (Operands);
    InstructionAddress += 2;

    EdbPrintComma ();
    EdbPrintRegister2 (Operands);

    if ((Modifiers & OPCODE_M_IMMDATA) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      if ((Operands & OPERAND_M_INDIRECT2) != 0) {
        EdbPrintRawIndexData16 (Data16);
      } else {
        EdbPrintImmDatan (Data16);
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - Unsigned Data Manipulate.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmUnsignedDataManip (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Modifiers;
  UINT8  Opcode;
  UINT8  Operands;
  UINTN  Size;
  UINT16 Data16;

  ASSERT (
    (GET_OPCODE(InstructionAddress) == OPCODE_NOT)    ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MULU)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_DIVU)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MODU)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_AND)    ||
    (GET_OPCODE(InstructionAddress) == OPCODE_OR)     ||
    (GET_OPCODE(InstructionAddress) == OPCODE_XOR)    ||
    (GET_OPCODE(InstructionAddress) == OPCODE_SHL)    ||
    (GET_OPCODE(InstructionAddress) == OPCODE_SHR)    ||
    (GET_OPCODE(InstructionAddress) == OPCODE_EXTNDB) ||
    (GET_OPCODE(InstructionAddress) == OPCODE_EXTNDW) ||
    (GET_OPCODE(InstructionAddress) == OPCODE_EXTNDD)
    );

  Opcode     = GET_OPCODE (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);
  Modifiers  = GET_MODIFIERS (InstructionAddress);
  if ((Modifiers & DATAMANIP_M_IMMDATA) != 0) {
    Size = 4;
  } else {
    Size = 2;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    switch (Opcode) {
    case OPCODE_NOT:
      EdbPrintInstructionName (L"NOT");
      break;
    case OPCODE_MULU:
      EdbPrintInstructionName (L"MULU");
      break;
    case OPCODE_DIVU:
      EdbPrintInstructionName (L"DIVU");
      break;
    case OPCODE_MODU:
      EdbPrintInstructionName (L"MODU");
      break;
    case OPCODE_AND:
      EdbPrintInstructionName (L"AND");
      break;
    case OPCODE_OR:
      EdbPrintInstructionName (L"OR");
      break;
    case OPCODE_XOR:
      EdbPrintInstructionName (L"XOR");
      break;
    case OPCODE_SHL:
      EdbPrintInstructionName (L"SHL");
      break;
    case OPCODE_SHR:
      EdbPrintInstructionName (L"SHR");
      break;
    case OPCODE_EXTNDB:
      EdbPrintInstructionName (L"EXTNDB");
      break;
    case OPCODE_EXTNDW:
      EdbPrintInstructionName (L"EXTNDW");
      break;
    case OPCODE_EXTNDD:
      EdbPrintInstructionName (L"EXTNDD");
      break;
    }
//    if (Modifiers & DATAMANIP_M_64) {
//      EdbPrintInstructionName (L"64");
//    } else {
//      EdbPrintInstructionName (L"32");
//    }

    EdbPrintRegister1 (Operands);
    EdbPrintComma ();
    EdbPrintRegister2 (Operands);

    InstructionAddress += 2;
    if ((Modifiers & DATAMANIP_M_IMMDATA) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      if ((Operands & OPERAND_M_INDIRECT2) != 0) {
        EdbPrintRawIndexData16 (Data16);
      } else {
        EdbPrintImmDatan (Data16);
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - Signed Data Manipulate,

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmSignedDataManip (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Modifiers;
  UINT8  Opcode;
  UINT8  Operands;
  UINTN  Size;
  UINT16 Data16;

  ASSERT (
    (GET_OPCODE(InstructionAddress) == OPCODE_NEG)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_ADD)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_SUB)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MUL)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_DIV)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MOD)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_ASHR)
    );

  Opcode     = GET_OPCODE (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);
  Modifiers  = GET_MODIFIERS (InstructionAddress);
  if ((Modifiers & DATAMANIP_M_IMMDATA) != 0) {
    Size = 4;
  } else {
    Size = 2;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    switch (Opcode) {
    case OPCODE_NEG:
      EdbPrintInstructionName (L"NEG");
      break;
    case OPCODE_ADD:
      EdbPrintInstructionName (L"ADD");
      break;
    case OPCODE_SUB:
      EdbPrintInstructionName (L"SUB");
      break;
    case OPCODE_MUL:
      EdbPrintInstructionName (L"MUL");
      break;
    case OPCODE_DIV:
      EdbPrintInstructionName (L"DIV");
      break;
    case OPCODE_MOD:
      EdbPrintInstructionName (L"MOD");
      break;
    case OPCODE_ASHR:
      EdbPrintInstructionName (L"ASHR");
      break;
    }
//    if (Modifiers & DATAMANIP_M_64) {
//      EdbPrintInstructionName (L"64");
//    } else {
//      EdbPrintInstructionName (L"32");
//    }

    EdbPrintRegister1 (Operands);
    EdbPrintComma ();
    EdbPrintRegister2 (Operands);

    InstructionAddress += 2;
    if ((Modifiers & DATAMANIP_M_IMMDATA) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      if ((Operands & OPERAND_M_INDIRECT2) != 0) {
        EdbPrintRawIndexData16 (Data16);
      } else {
        EdbPrintImmDatan (Data16);
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - MOVxx.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmMOVxx (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8   Modifiers;
  UINT8   Opcode;
  UINT8   Operands;
  UINTN   Size;
  UINT16  Data16;
  UINT32  Data32;
  UINT64  Data64;

  ASSERT (
    (GET_OPCODE(InstructionAddress) == OPCODE_MOVBW)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MOVWW)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MOVDW)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MOVQW)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MOVBD)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MOVWD)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MOVDD)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MOVQD)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MOVQQ)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MOVNW)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_MOVND)
    );

  Opcode     = GET_OPCODE (InstructionAddress);
  Modifiers  = GET_MODIFIERS (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);
  Size = 2;
  if ((Modifiers & (OPCODE_M_IMMED_OP1 | OPCODE_M_IMMED_OP2)) != 0) {
    if ((Opcode <= OPCODE_MOVQW) || (Opcode == OPCODE_MOVNW)) {
      if ((Modifiers & OPCODE_M_IMMED_OP1) != 0) {
        Size += 2;
      }
      if ((Modifiers & OPCODE_M_IMMED_OP2) != 0) {
        Size += 2;
      }
    } else if (((Opcode <= OPCODE_MOVQD) || (Opcode == OPCODE_MOVND)) != 0) {
      if ((Modifiers & OPCODE_M_IMMED_OP1) != 0) {
        Size += 4;
      }
      if ((Modifiers & OPCODE_M_IMMED_OP2) != 0) {
        Size += 4;
      }
    } else if (Opcode == OPCODE_MOVQQ) {
      if ((Modifiers & OPCODE_M_IMMED_OP1) != 0) {
        Size += 8;
      }
      if ((Modifiers & OPCODE_M_IMMED_OP2) != 0) {
        Size += 8;
      }
    }
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"MOV");
    switch (Opcode) {
    case OPCODE_MOVBW:
      EdbPrintInstructionName (L"bw");
      break;
    case OPCODE_MOVWW:
      EdbPrintInstructionName (L"ww");
      break;
    case OPCODE_MOVDW:
      EdbPrintInstructionName (L"dw");
      break;
    case OPCODE_MOVQW:
      EdbPrintInstructionName (L"qw");
      break;
    case OPCODE_MOVBD:
      EdbPrintInstructionName (L"bd");
      break;
    case OPCODE_MOVWD:
      EdbPrintInstructionName (L"wd");
      break;
    case OPCODE_MOVDD:
      EdbPrintInstructionName (L"dd");
      break;
    case OPCODE_MOVQD:
      EdbPrintInstructionName (L"qd");
      break;
    case OPCODE_MOVQQ:
      EdbPrintInstructionName (L"qq");
      break;
    case OPCODE_MOVNW:
      EdbPrintInstructionName (L"nw");
      break;
    case OPCODE_MOVND:
      EdbPrintInstructionName (L"nd");
      break;
    }

    EdbPrintRegister1 (Operands);

    InstructionAddress += 2;
    if ((Modifiers & OPCODE_M_IMMED_OP1) != 0) {
      if ((Opcode <= OPCODE_MOVQW) || (Opcode == OPCODE_MOVNW)) {
        CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
        InstructionAddress += 2;
        EdbPrintRawIndexData16 (Data16);
      } else if ((Opcode <= OPCODE_MOVQD) || (Opcode == OPCODE_MOVND)) {
        CopyMem (&Data32, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT32));
        InstructionAddress += 4;
        EdbPrintRawIndexData32 (Data32);
      } else if (Opcode == OPCODE_MOVQQ) {
        CopyMem (&Data64, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT64));
        InstructionAddress += 8;
        EdbPrintRawIndexData64 (Data64);
      }
    }

    EdbPrintComma ();
    EdbPrintRegister2 (Operands);

    if ((Modifiers & OPCODE_M_IMMED_OP2) != 0) {
      if ((Opcode <= OPCODE_MOVQW) || (Opcode == OPCODE_MOVNW)) {
        CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
        EdbPrintRawIndexData16 (Data16);
      } else if ((Opcode <= OPCODE_MOVQD) || (Opcode == OPCODE_MOVND)) {
        CopyMem (&Data32, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT32));
        EdbPrintRawIndexData32 (Data32);
      } else if (Opcode == OPCODE_MOVQQ) {
        CopyMem (&Data64, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT64));
        EdbPrintRawIndexData64 (Data64);
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - MOVsnw.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmMOVsnw (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Modifiers;
  UINT8  Operands;
  UINTN  Size;
  UINT16 Data16;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_MOVSNW);

  Modifiers  = GET_MODIFIERS (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);
  Size = 2;
  if ((Modifiers & OPCODE_M_IMMED_OP1) != 0) {
    Size += 2;
  }
  if ((Modifiers & OPCODE_M_IMMED_OP2) != 0) {
    Size += 2;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"MOVsnw");

    EdbPrintRegister1 (Operands);

    InstructionAddress += 2;
    if ((Modifiers & OPCODE_M_IMMED_OP1) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      InstructionAddress += 2;
      EdbPrintRawIndexData16 (Data16);
    }

    EdbPrintComma ();
    EdbPrintRegister2 (Operands);

    if ((Modifiers & OPCODE_M_IMMED_OP2) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      if ((Operands & OPERAND_M_INDIRECT2) != 0) {
        EdbPrintRawIndexData16 (Data16);
      } else {
        EdbPrintImmDatan (Data16);
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - MOVsnd.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmMOVsnd (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Modifiers;
  UINT8  Operands;
  UINTN  Size;
  UINT32 Data32;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_MOVSND);

  Modifiers  = GET_MODIFIERS (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);
  Size = 2;
  if ((Modifiers & OPCODE_M_IMMED_OP1) != 0) {
    Size += 4;
  }
  if ((Modifiers & OPCODE_M_IMMED_OP2) != 0) {
    Size += 4;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"MOVsnd");

    EdbPrintRegister1 (Operands);

    InstructionAddress += 2;
    if ((Modifiers & OPCODE_M_IMMED_OP1) != 0) {
      CopyMem (&Data32, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT32));
      InstructionAddress += 4;
      EdbPrintRawIndexData32 (Data32);
    }

    EdbPrintComma ();
    EdbPrintRegister2 (Operands);

    if ((Modifiers & OPCODE_M_IMMED_OP2) != 0) {
      CopyMem (&Data32, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT32));
      if ((Operands & OPERAND_M_INDIRECT2) != 0) {
        EdbPrintRawIndexData32 (Data32);
      } else {
        EdbPrintImmDatan (Data32);
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - LOADSP.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmLOADSP (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Operands;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_LOADSP);

  Operands   = GET_OPERANDS (InstructionAddress);

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"LOADSP");

    EdbPrintDedicatedRegister1 (Operands);

    EdbPrintRegister2 (Operands);

    EdbPostInstructionString ();
  }

  return 2;
}

/**

  Disasm instruction - STORESP.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmSTORESP (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Operands;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_STORESP);

  Operands   = GET_OPERANDS (InstructionAddress);

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"STORESP");

    EdbPrintRegister1 (Operands);

    EdbPrintDedicatedRegister2 (Operands);

    EdbPostInstructionString ();
  }

  return 2;
}


/**

  Disasm instruction - PUSH.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmPUSH (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Modifiers;
  UINT8  Operands;
  UINTN  Size;
  UINT16 Data16;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_PUSH);

  Operands   = GET_OPERANDS (InstructionAddress);
  Modifiers  = GET_MODIFIERS (InstructionAddress);
  if ((Modifiers & PUSHPOP_M_IMMDATA) != 0) {
    Size = 4;
  } else {
    Size = 2;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"PUSH");
//    if (Modifiers & PUSHPOP_M_64) {
//      EdbPrintInstructionName (L"64");
//    } else {
//      EdbPrintInstructionName (L"32");
//    }

    EdbPrintRegister1 (Operands);

    InstructionAddress += 2;
    if ((Modifiers & PUSHPOP_M_IMMDATA) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      if ((Operands & OPERAND_M_INDIRECT1) != 0) {
        EdbPrintRawIndexData16 (Data16);
      } else {
        EdbPrintImmDatan (Data16);
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - POP.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmPOP (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Modifiers;
  UINT8  Operands;
  UINTN  Size;
  UINT16 Data16;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_POP);

  Operands   = GET_OPERANDS (InstructionAddress);
  Modifiers  = GET_MODIFIERS (InstructionAddress);
  if ((Modifiers & PUSHPOP_M_IMMDATA) != 0) {
    Size = 4;
  } else {
    Size = 2;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"POP");
//    if (Modifiers & PUSHPOP_M_64) {
//      EdbPrintInstructionName (L"64");
//    } else {
//      EdbPrintInstructionName (L"32");
//    }

    EdbPrintRegister1 (Operands);

    InstructionAddress += 2;
    if ((Modifiers & PUSHPOP_M_IMMDATA) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      if ((Operands & OPERAND_M_INDIRECT1) != 0) {
        EdbPrintRawIndexData16 (Data16);
      } else {
        EdbPrintImmDatan (Data16);
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - CMPI.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmCMPI (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Modifiers;
  UINT8  Opcode;
  UINT8  Operands;
  UINT16 Data16;
  UINT32 Data32;
  UINTN  Size;

  ASSERT (
    (GET_OPCODE(InstructionAddress) == OPCODE_CMPIEQ)   ||
    (GET_OPCODE(InstructionAddress) == OPCODE_CMPILTE)  ||
    (GET_OPCODE(InstructionAddress) == OPCODE_CMPIGTE)  ||
    (GET_OPCODE(InstructionAddress) == OPCODE_CMPIULTE) ||
    (GET_OPCODE(InstructionAddress) == OPCODE_CMPIUGTE)
    );

  Modifiers  = GET_MODIFIERS (InstructionAddress);
  Opcode     = GET_OPCODE (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);

  if ((Operands & 0xE0) != 0) {
    return 0;
  }

  Size = 2;
  if ((Operands & OPERAND_M_CMPI_INDEX) != 0) {
    Size += 2;
  }
  if ((Modifiers & OPCODE_M_CMPI32_DATA) != 0) {
    Size += 4;
  } else {
    Size += 2;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"CMPI");
//    if (Modifiers & OPCODE_M_CMPI64) {
//      EdbPrintInstructionName (L"64");
//    } else {
//      EdbPrintInstructionName (L"32");
//    }
    if ((Modifiers & OPCODE_M_CMPI32_DATA) != 0) {
      EdbPrintInstructionName (L"d");
    } else {
      EdbPrintInstructionName (L"w");
    }
    switch (Opcode) {
    case OPCODE_CMPIEQ:
      EdbPrintInstructionName (L"eq");
      break;
    case OPCODE_CMPILTE:
      EdbPrintInstructionName (L"lte");
      break;
    case OPCODE_CMPIGTE:
      EdbPrintInstructionName (L"gte");
      break;
    case OPCODE_CMPIULTE:
      EdbPrintInstructionName (L"ulte");
      break;
    case OPCODE_CMPIUGTE:
      EdbPrintInstructionName (L"ugte");
      break;
    }

    EdbPrintRegister1 (Operands);

    InstructionAddress += 2;
    if ((Operands & OPERAND_M_CMPI_INDEX) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      InstructionAddress += 2;
      EdbPrintRawIndexData16 (Data16);
    }

    EdbPrintComma ();

    if ((Modifiers & OPCODE_M_CMPI32_DATA) != 0) {
      CopyMem (&Data32, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT32));
      EdbPrintDatan (Data32);
    } else {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      EdbPrintDatan (Data16);
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - PUSHn.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmPUSHn (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Modifiers;
  UINT8  Operands;
  UINTN  Size;
  UINT16 Data16;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_PUSHN);

  Operands   = GET_OPERANDS (InstructionAddress);
  Modifiers  = GET_MODIFIERS (InstructionAddress);
  if ((Modifiers & PUSHPOP_M_IMMDATA) != 0) {
    Size = 4;
  } else {
    Size = 2;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"PUSHn");

    EdbPrintRegister1 (Operands);

    InstructionAddress += 2;
    if ((Modifiers & PUSHPOP_M_IMMDATA) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      if ((Operands & OPERAND_M_INDIRECT1) != 0) {
        EdbPrintRawIndexData16 (Data16);
      } else {
        EdbPrintImmDatan (Data16);
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - POPn.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmPOPn (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Modifiers;
  UINT8  Operands;
  UINTN  Size;
  UINT16 Data16;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_POPN);

  Operands   = GET_OPERANDS (InstructionAddress);
  Modifiers  = GET_MODIFIERS (InstructionAddress);
  if ((Modifiers & PUSHPOP_M_IMMDATA) != 0) {
    Size = 4;
  } else {
    Size = 2;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"POPn");

    EdbPrintRegister1 (Operands);

    InstructionAddress += 2;
    if ((Modifiers & PUSHPOP_M_IMMDATA) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      if ((Operands & OPERAND_M_INDIRECT1) != 0) {
        EdbPrintRawIndexData16 (Data16);
      } else {
        EdbPrintImmDatan (Data16);
      }
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - MOVI.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmMOVI (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Modifiers;
  UINT8  Operands;
  UINTN  Size;
  UINT16 Data16;
  UINT32 Data32;
  UINT64 Data64;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_MOVI);

  Modifiers  = GET_MODIFIERS (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);

  if ((Operands & MOVI_M_IMMDATA) != 0) {
    Size    = 4;
  } else {
    Size    = 2;
  }
  if ((Modifiers & MOVI_M_DATAWIDTH) == MOVI_DATAWIDTH16) {
    Size += 2;
  } else if ((Modifiers & MOVI_M_DATAWIDTH) == MOVI_DATAWIDTH32) {
    Size += 4;
  } else if ((Modifiers & MOVI_M_DATAWIDTH) == MOVI_DATAWIDTH64) {
    Size += 8;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"MOVI");
    switch (Operands & MOVI_M_MOVEWIDTH) {
    case MOVI_MOVEWIDTH8:
      EdbPrintInstructionName (L"b");
      break;
    case MOVI_MOVEWIDTH16:
      EdbPrintInstructionName (L"w");
      break;
    case MOVI_MOVEWIDTH32:
      EdbPrintInstructionName (L"d");
      break;
    case MOVI_MOVEWIDTH64:
      EdbPrintInstructionName (L"q");
      break;
    }
    switch (Modifiers & MOVI_M_DATAWIDTH) {
    case MOVI_DATAWIDTH16:
      EdbPrintInstructionName (L"w");
      break;
    case MOVI_DATAWIDTH32:
      EdbPrintInstructionName (L"d");
      break;
    case MOVI_DATAWIDTH64:
      EdbPrintInstructionName (L"q");
      break;
    }

    EdbPrintRegister1 (Operands);

    InstructionAddress += 2;
    if ((Operands & MOVI_M_IMMDATA) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      InstructionAddress += 2;
      EdbPrintRawIndexData16 (Data16);
    }

    EdbPrintComma ();

    switch (Modifiers & MOVI_M_DATAWIDTH) {
    case MOVI_DATAWIDTH16:
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      EdbPrintDatan (Data16);
      break;
    case MOVI_DATAWIDTH32:
      CopyMem (&Data32, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT32));
      EdbPrintDatan (Data32);
      break;
    case MOVI_DATAWIDTH64:
      CopyMem (&Data64, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT64));
      EdbPrintData64n (Data64);
      break;
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - MOVIn.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmMOVIn (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8  Modifiers;
  UINT8  Operands;
  UINTN  Size;
  UINT16 Data16;
  UINT32 Data32;
  UINT64 Data64;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_MOVIN);

  Modifiers  = GET_MODIFIERS (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);

  if ((Operands & MOVI_M_IMMDATA) != 0) {
    Size    = 4;
  } else {
    Size    = 2;
  }
  if ((Modifiers & MOVI_M_DATAWIDTH) == MOVI_DATAWIDTH16) {
    Size += 2;
  } else if ((Modifiers & MOVI_M_DATAWIDTH) == MOVI_DATAWIDTH32) {
    Size += 4;
  } else if ((Modifiers & MOVI_M_DATAWIDTH) == MOVI_DATAWIDTH64) {
    Size += 8;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"MOVIn");
    switch (Modifiers & MOVI_M_DATAWIDTH) {
    case MOVI_DATAWIDTH16:
      EdbPrintInstructionName (L"w");
      break;
    case MOVI_DATAWIDTH32:
      EdbPrintInstructionName (L"d");
      break;
    case MOVI_DATAWIDTH64:
      EdbPrintInstructionName (L"q");
      break;
    }

    EdbPrintRegister1 (Operands);

    InstructionAddress += 2;
    if ((Operands & MOVI_M_IMMDATA) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      InstructionAddress += 2;
      EdbPrintRawIndexData16 (Data16);
    }

    EdbPrintComma ();

    switch (Modifiers & MOVI_M_DATAWIDTH) {
    case MOVI_DATAWIDTH16:
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      EdbPrintRawIndexData16 (Data16);
      break;
    case MOVI_DATAWIDTH32:
      CopyMem (&Data32, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT32));
      EdbPrintRawIndexData32 (Data32);
      break;
    case MOVI_DATAWIDTH64:
      CopyMem (&Data64, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT64));
      EdbPrintRawIndexData64 (Data64);
      break;
    }

    EdbPostInstructionString ();
  }

  return Size;
}

/**

  Disasm instruction - MOVREL.

  @param  InstructionAddress - The instruction address
  @param  SystemContext      - EBC system context.
  @param  DisasmString       - The instruction string

  @return Instruction length

**/
UINTN
EdbDisasmMOVREL (
  IN     EFI_PHYSICAL_ADDRESS      InstructionAddress,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  OUT    CHAR16                    **DisasmString
  )
{
  UINT8   Modifiers;
  UINT8   Operands;
  UINTN   Size;
  UINT16  Data16;
  UINT32  Data32;
  UINT64  Data64;
  UINTN   Result;
  EFI_PHYSICAL_ADDRESS      SavedInstructionAddress;

  ASSERT (GET_OPCODE(InstructionAddress) == OPCODE_MOVREL);
  SavedInstructionAddress = InstructionAddress;

  Modifiers  = GET_MODIFIERS (InstructionAddress);
  Operands   = GET_OPERANDS (InstructionAddress);

  if ((Operands & MOVI_M_IMMDATA) != 0) {
    Size    = 4;
  } else {
    Size    = 2;
  }
  if ((Modifiers & MOVI_M_DATAWIDTH) == MOVI_DATAWIDTH16) {
    Size += 2;
  } else if ((Modifiers & MOVI_M_DATAWIDTH) == MOVI_DATAWIDTH32) {
    Size += 4;
  } else if ((Modifiers & MOVI_M_DATAWIDTH) == MOVI_DATAWIDTH64) {
    Size += 8;
  } else {
    return 0;
  }

  //
  // Construct Disasm String
  //
  if (DisasmString != NULL) {
    *DisasmString = EdbPreInstructionString ();

    EdbPrintInstructionName (L"MOVrel");
    switch (Modifiers & MOVI_M_DATAWIDTH) {
    case MOVI_DATAWIDTH16:
      EdbPrintInstructionName (L"w");
      break;
    case MOVI_DATAWIDTH32:
      EdbPrintInstructionName (L"d");
      break;
    case MOVI_DATAWIDTH64:
      EdbPrintInstructionName (L"q");
      break;
    }

    EdbPrintRegister1 (Operands);

    InstructionAddress += 2;
    if ((Operands & MOVI_M_IMMDATA) != 0) {
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      InstructionAddress += 2;
      EdbPrintRawIndexData16 (Data16);
    }

    EdbPrintComma ();

    switch (Modifiers & MOVI_M_DATAWIDTH) {
    case MOVI_DATAWIDTH16:
      CopyMem (&Data16, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT16));
      Result = EdbFindAndPrintSymbol ((UINTN)(SavedInstructionAddress + Size + (INT16)Data16));
      if (Result == 0) {
        EdbPrintData16 (Data16);
      }
      break;
    case MOVI_DATAWIDTH32:
      CopyMem (&Data32, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT32));
      Result = EdbFindAndPrintSymbol ((UINTN)(SavedInstructionAddress + Size + (INT32)Data32));
      if (Result == 0) {
        EdbPrintData32 (Data32);
      }
      break;
    case MOVI_DATAWIDTH64:
      CopyMem (&Data64, (VOID *)(UINTN)(InstructionAddress), sizeof(UINT64));
      if (sizeof(UINTN) == sizeof(UINT64)) {
        Result = EdbFindAndPrintSymbol ((UINTN)(SavedInstructionAddress + Size + (INT64)Data64));
      } else {
        Result = 0;
      }
      if (Result == 0) {
        EdbPrintData64 (Data64);
      }
      break;
    }

    EdbPostInstructionString ();
  }

  return Size;
}
