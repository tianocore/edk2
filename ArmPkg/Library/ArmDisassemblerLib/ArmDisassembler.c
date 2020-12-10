/** @file
  Default exception handler

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/ArmDisassemblerLib.h>

CHAR8 *gCondition[] = {
  "EQ",
  "NE",
  "CS",
  "CC",
  "MI",
  "PL",
  "VS",
  "VC",
  "HI",
  "LS",
  "GE",
  "LT",
  "GT",
  "LE",
  "",
  "2"
};

#define COND(_a)  gCondition[((_a) >> 28)]

CHAR8 *gReg[] = {
  "r0",
  "r1",
  "r2",
  "r3",
  "r4",
  "r5",
  "r6",
  "r7",
  "r8",
  "r9",
  "r10",
  "r11",
  "r12",
  "sp",
  "lr",
  "pc"
};

CHAR8 *gLdmAdr[] = {
  "DA",
  "IA",
  "DB",
  "IB"
};

CHAR8 *gLdmStack[] = {
  "FA",
  "FD",
  "EA",
  "ED"
};

#define LDM_EXT(_reg, _off) ((_reg == 13) ? gLdmStack[(_off)] : gLdmAdr[(_off)])


#define SIGN(_U)  ((_U) ? "" : "-")
#define WRITE(_W) ((_W) ? "!" : "")
#define BYTE(_B)  ((_B) ? "B":"")
#define USER(_B)  ((_B) ? "^" : "")

CHAR8 mMregListStr[4*15 + 1];

CHAR8 *
MRegList (
  UINT32  OpCode
  )
{
  UINTN     Index, Start, End;
  BOOLEAN   First;

  mMregListStr[0] = '\0';
  AsciiStrCatS (mMregListStr, sizeof mMregListStr, "{");
  for (Index = 0, First = TRUE; Index <= 15; Index++) {
    if ((OpCode & (1 << Index)) != 0) {
      Start = End = Index;
      for (Index++; ((OpCode & (1 << Index)) != 0) && Index <= 15; Index++) {
        End = Index;
      }

      if (!First) {
        AsciiStrCatS (mMregListStr, sizeof mMregListStr, ",");
      } else {
        First = FALSE;
      }

      if (Start == End) {
        AsciiStrCatS (mMregListStr, sizeof mMregListStr, gReg[Start]);
        AsciiStrCatS (mMregListStr, sizeof mMregListStr, ", ");
      } else {
        AsciiStrCatS (mMregListStr, sizeof mMregListStr, gReg[Start]);
        AsciiStrCatS (mMregListStr, sizeof mMregListStr, "-");
        AsciiStrCatS (mMregListStr, sizeof mMregListStr, gReg[End]);
      }
    }
  }
  if (First) {
    AsciiStrCatS (mMregListStr, sizeof mMregListStr, "ERROR");
  }
  AsciiStrCatS (mMregListStr, sizeof mMregListStr, "}");

  // BugBug: Make caller pass in buffer it is cleaner
  return mMregListStr;
}

CHAR8 *
FieldMask (
  IN  UINT32  Mask
  )
{
  return "";
}

UINT32
RotateRight (
  IN UINT32 Op,
  IN UINT32 Shift
  )
{
  return (Op >> Shift) | (Op << (32 - Shift));
}


/**
  Place a disassembly of **OpCodePtr into buffer, and update OpCodePtr to
  point to next instruction.

  We cheat and only decode instructions that access
  memory. If the instruction is not found we dump the instruction in hex.

  @param  OpCodePtr   Pointer to pointer of ARM instruction to disassemble.
  @param  Buf         Buffer to sprintf disassembly into.
  @param  Size        Size of Buf in bytes.
  @param  Extended    TRUE dump hex for instruction too.

**/
VOID
DisassembleArmInstruction (
  IN  UINT32    **OpCodePtr,
  OUT CHAR8     *Buf,
  OUT UINTN     Size,
  IN  BOOLEAN   Extended
  )
{
  UINT32    OpCode;
  CHAR8     *Type, *Root;
  BOOLEAN   I, P, U, B, W, L, S, H;
  UINT32    Rn, Rd, Rm;
  UINT32    imode, offset_8, offset_12;
  UINT32    Index;
  UINT32    shift_imm, shift;

  OpCode = **OpCodePtr;

  I = (OpCode & BIT25) == BIT25;
  P = (OpCode & BIT24) == BIT24;
  U = (OpCode & BIT23) == BIT23;
  B = (OpCode & BIT22) == BIT22;  // Also called S
  W = (OpCode & BIT21) == BIT21;
  L = (OpCode & BIT20) == BIT20;
  S = (OpCode & BIT6) == BIT6;
  H = (OpCode & BIT5) == BIT5;
  Rn = (OpCode >> 16) & 0xf;
  Rd = (OpCode >> 12) & 0xf;
  Rm = (OpCode & 0xf);


  if (Extended) {
    Index = AsciiSPrint (Buf, Size, "0x%08x   ", OpCode);
    Buf += Index;
    Size -= Index;
  }

  // LDREX, STREX
  if ((OpCode  & 0x0fe000f0) == 0x01800090) {
    if (L) {
      // A4.1.27  LDREX{<cond>} <Rd>, [<Rn>]
      AsciiSPrint (Buf, Size, "LDREX%a %a, [%a]", COND (OpCode), gReg[Rd], gReg[Rn]);
    } else {
     // A4.1.103  STREX{<cond>} <Rd>, <Rm>, [<Rn>]
      AsciiSPrint (Buf, Size, "STREX%a %a, %a, [%a]", COND (OpCode), gReg[Rd], gReg[Rn], gReg[Rn]);
    }
    return;
  }

  // LDM/STM
  if ((OpCode  & 0x0e000000) == 0x08000000) {
    if (L) {
      // A4.1.20 LDM{<cond>}<addressing_mode> <Rn>{!}, <registers>
      // A4.1.21 LDM{<cond>}<addressing_mode> <Rn>, <registers_without_pc>^
      // A4.1.22 LDM{<cond>}<addressing_mode> <Rn>{!}, <registers_and_pc>^
      AsciiSPrint (Buf, Size, "LDM%a%a, %a%a, %a", COND (OpCode), LDM_EXT (Rn ,(OpCode >> 23) & 3), gReg[Rn], WRITE (W), MRegList (OpCode), USER (B));
    } else {
      // A4.1.97 STM{<cond>}<addressing_mode> <Rn>{!}, <registers>
      // A4.1.98 STM{<cond>}<addressing_mode> <Rn>, <registers>^
      AsciiSPrint (Buf, Size, "STM%a%a, %a%a, %a", COND (OpCode), LDM_EXT (Rn ,(OpCode >> 23) & 3), gReg[Rn], WRITE (W), MRegList (OpCode), USER (B));
    }
    return;
  }

  // LDR/STR Address Mode 2
  if ( ((OpCode  & 0x0c000000) == 0x04000000) || ((OpCode & 0xfd70f000 ) == 0xf550f000) ) {
    offset_12 = OpCode & 0xfff;
    if ((OpCode & 0xfd70f000 ) == 0xf550f000) {
      Index = AsciiSPrint (Buf, Size, "PLD");
    } else {
      Index = AsciiSPrint (Buf, Size, "%a%a%a%a %a, ", L ? "LDR" : "STR", COND (OpCode), BYTE (B), (!(P) && W) ? "T":"", gReg[Rd]);
    }
    if (P) {
      if (!I) {
        // A5.2.2 [<Rn>, #+/-<offset_12>]
        // A5.2.5 [<Rn>, #+/-<offset_12>]
        AsciiSPrint (&Buf[Index], Size - Index, "[%a, #%a0x%x]%a", gReg[Rn], SIGN (U), offset_12, WRITE (W));
      } else if ((OpCode & 0x03000ff0) == 0x03000000) {
        // A5.2.3 [<Rn>, +/-<Rm>]
        // A5.2.6 [<Rn>, +/-<Rm>]!
        AsciiSPrint (&Buf[Index], Size - Index, "[%a, #%a%a]%a", gReg[Rn], SIGN (U), WRITE (W));
      } else {
        // A5.2.4 [<Rn>, +/-<Rm>, LSL #<shift_imm>]
        // A5.2.7 [<Rn>, +/-<Rm>, LSL #<shift_imm>]!
        shift_imm = (OpCode >> 7) & 0x1f;
        shift = (OpCode >> 5) & 0x3;
        if (shift == 0x0) {
          Type = "LSL";
        } else if (shift == 0x1) {
          Type = "LSR";
          if (shift_imm == 0) {
            shift_imm = 32;
          }
        } else if (shift == 0x2) {
          Type = "ASR";
        } else if (shift_imm == 0) {
          AsciiSPrint (&Buf[Index], Size - Index, "[%a, #%a%a, %a, RRX]%a", gReg[Rn], SIGN (U), gReg[Rm], WRITE (W));
          return;
        } else {
          Type = "ROR";
        }

        AsciiSPrint (&Buf[Index], Size - Index, "[%a, #%a%a, %a, #%d]%a", gReg[Rn], SIGN (U), gReg[Rm], Type, shift_imm, WRITE (W));
      }
    } else {  // !P
      if (!I) {
        // A5.2.8  [<Rn>], #+/-<offset_12>
        AsciiSPrint (&Buf[Index], Size - Index, "[%a], #%a0x%x", gReg[Rn], SIGN (U), offset_12);
      } else if ((OpCode & 0x03000ff0) == 0x03000000) {
        // A5.2.9  [<Rn>], +/-<Rm>
        AsciiSPrint (&Buf[Index], Size - Index, "[%a], #%a%a", gReg[Rn], SIGN (U), gReg[Rm]);
      } else {
        // A5.2.10 [<Rn>], +/-<Rm>, LSL #<shift_imm>
        shift_imm = (OpCode >> 7) & 0x1f;
        shift = (OpCode >> 5) & 0x3;

        if (shift == 0x0) {
          Type = "LSL";
        } else if (shift == 0x1) {
          Type = "LSR";
          if (shift_imm == 0) {
            shift_imm = 32;
          }
        } else if (shift == 0x2) {
          Type = "ASR";
        } else if (shift_imm == 0) {
          AsciiSPrint (&Buf[Index], Size - Index, "[%a], #%a%a, %a, RRX", gReg[Rn], SIGN (U), gReg[Rm]);
          // FIx me
          return;
        } else {
          Type = "ROR";
        }

        AsciiSPrint (&Buf[Index], Size - Index, "[%a], #%a%a, %a, #%d", gReg[Rn], SIGN (U), gReg[Rm], Type, shift_imm);
      }
    }
    return;
  }

  if ((OpCode  & 0x0e000000) == 0x00000000) {
    // LDR/STR address mode 3
    // LDR|STR{<cond>}H|SH|SB|D <Rd>, <addressing_mode>
    if (L) {
      if (!S) {
        Root = "LDR%aH %a, ";
      } else if (!H) {
        Root = "LDR%aSB %a, ";
      } else {
        Root = "LDR%aSH %a, ";
      }
    } else {
      if (!S) {
        Root = "STR%aH %a ";
      } else if (!H) {
        Root = "LDR%aD %a ";
      } else {
        Root = "STR%aD %a ";
      }
    }

    Index = AsciiSPrint (Buf, Size, Root, COND (OpCode), gReg[Rd]);

    S = (OpCode & BIT6) == BIT6;
    H = (OpCode & BIT5) == BIT5;
    offset_8 = ((OpCode >> 4) | (OpCode * 0xf)) & 0xff;
    if (P & !W) {
      // Immediate offset/index
      if (B) {
        // A5.3.2  [<Rn>, #+/-<offset_8>]
        // A5.3.4  [<Rn>, #+/-<offset_8>]!
        AsciiSPrint  (&Buf[Index], Size - Index, "[%a, #%a%d]%a", gReg[Rn], SIGN (U), offset_8, WRITE (W));
      } else {
        // A5.3.3  [<Rn>, +/-<Rm>]
        // A5.3.5  [<Rn>, +/-<Rm>]!
        AsciiSPrint  (&Buf[Index], Size - Index, "[%a, #%a%]a", gReg[Rn], SIGN (U), gReg[Rm], WRITE (W));
      }
    } else {
      // Register offset/index
      if (B) {
        // A5.3.6 [<Rn>], #+/-<offset_8>
        AsciiSPrint  (&Buf[Index], Size - Index, "[%a], #%a%d", gReg[Rn], SIGN (U), offset_8);
      } else {
        // A5.3.7 [<Rn>], +/-<Rm>
        AsciiSPrint  (&Buf[Index], Size - Index, "[%a], #%a%a", gReg[Rn], SIGN (U), gReg[Rm]);
      }
    }
    return;
  }

  if ((OpCode  & 0x0fb000f0) == 0x01000050) {
    // A4.1.108  SWP   SWP{<cond>}B <Rd>, <Rm>, [<Rn>]
    // A4.1.109  SWPB  SWP{<cond>}B <Rd>, <Rm>, [<Rn>]
    AsciiSPrint (Buf, Size, "SWP%a%a %a, %a, [%a]", COND (OpCode), BYTE (B), gReg[Rd], gReg[Rm], gReg[Rn]);
    return;
  }

  if ((OpCode  & 0xfe5f0f00) == 0xf84d0500) {
    // A4.1.90 SRS SRS<addressing_mode> #<mode>{!}
    AsciiSPrint (Buf, Size, "SRS%a #0x%x%a", gLdmStack[(OpCode >> 23) & 3], OpCode & 0x1f, WRITE (W));
    return;
  }

  if ((OpCode  & 0xfe500f00) == 0xf8100500) {
    // A4.1.59 RFE<addressing_mode> <Rn>{!}
    AsciiSPrint (Buf, Size, "RFE%a %a", gLdmStack[(OpCode >> 23) & 3], gReg[Rn], WRITE (W));
    return;
  }

  if ((OpCode  & 0xfff000f0) == 0xe1200070) {
    // A4.1.7 BKPT <immed_16>
    AsciiSPrint (Buf, Size, "BKPT %x", ((OpCode >> 8) | (OpCode & 0xf)) & 0xffff);
    return;
  }

  if ((OpCode  & 0xfff10020) == 0xf1000000) {
    // A4.1.16 CPS<effect> <iflags> {, #<mode>}
    if (((OpCode >> 6) & 0x7) == 0) {
      AsciiSPrint (Buf, Size, "CPS #0x%x", (OpCode & 0x2f));
    } else {
      imode = (OpCode >> 18) & 0x3;
      Index = AsciiSPrint (Buf, Size, "CPS%a %a%a%a",
                      (imode == 3) ? "ID":"IE",
                      ((OpCode & BIT8) != 0) ? "A":"",
                      ((OpCode & BIT7) != 0) ? "I":"",
                      ((OpCode & BIT6) != 0) ? "F":"");
      if ((OpCode & BIT17) != 0) {
        AsciiSPrint (&Buf[Index], Size - Index, ", #0x%x", OpCode & 0x1f);
      }
    }
    return;
  }

  if ((OpCode  & 0x0f000000) == 0x0f000000) {
    // A4.1.107 SWI{<cond>} <immed_24>
    AsciiSPrint (Buf, Size, "SWI%a %x", COND (OpCode), OpCode & 0x00ffffff);
    return;
  }

  if ((OpCode  & 0x0fb00000) == 0x01000000) {
    // A4.1.38 MRS{<cond>} <Rd>, CPSR  MRS{<cond>} <Rd>, SPSR
    AsciiSPrint (Buf, Size, "MRS%a %a, %a", COND (OpCode), gReg[Rd], B ? "SPSR" : "CPSR");
    return;
  }


  if ((OpCode  & 0x0db00000) == 0x01200000) {
    // A4.1.38 MSR{<cond>} CPSR_<fields>, #<immediate> MSR{<cond>} CPSR_<fields>, <Rm>
    if (I) {
      // MSR{<cond>} CPSR_<fields>, #<immediate>
      AsciiSPrint (Buf, Size, "MRS%a %a_%a, #0x%x", COND (OpCode),  B ? "SPSR" : "CPSR", FieldMask ((OpCode >> 16) & 0xf), RotateRight (OpCode & 0xf, ((OpCode >> 8) & 0xf) *2));
    } else {
      // MSR{<cond>} CPSR_<fields>, <Rm>
      AsciiSPrint (Buf, Size, "MRS%a %a_%a, %a", COND (OpCode), B ? "SPSR" : "CPSR", gReg[Rd]);
    }
    return;
  }

  if ((OpCode  & 0xff000010) == 0xfe000000) {
    // A4.1.13 CDP{<cond>} <coproc>, <opcode_1>, <CRd>, <CRn>, <CRm>, <opcode_2>
    AsciiSPrint (Buf, Size, "CDP%a 0x%x, 0x%x, CR%d, CR%d, CR%d, 0x%x", COND (OpCode), (OpCode >> 8) & 0xf, (OpCode >> 20) & 0xf, Rn, Rd, Rm, (OpCode >> 5) &0x7);
    return;
  }

  if ((OpCode  & 0x0e000000) == 0x0c000000) {
    // A4.1.19 LDC and A4.1.96 SDC
    if ((OpCode & 0xf0000000) == 0xf0000000) {
      Index = AsciiSPrint (Buf, Size, "%a2 0x%x, CR%d, ", L ? "LDC":"SDC", (OpCode >> 8) & 0xf, Rd);
    } else {
      Index = AsciiSPrint (Buf, Size, "%a%a 0x%x, CR%d, ",  L ? "LDC":"SDC", COND (OpCode), (OpCode >> 8) & 0xf, Rd);
    }

    if (!P) {
      if (!W) {
        // A5.5.5.5 [<Rn>], <option>
      AsciiSPrint (&Buf[Index], Size - Index, "[%a], {0x%x}", gReg[Rn], OpCode & 0xff);
      } else {
        // A.5.5.4  [<Rn>], #+/-<offset_8>*4
      AsciiSPrint (&Buf[Index], Size - Index, "[%a], #%a0x%x*4", gReg[Rn], SIGN (U), OpCode & 0xff);
      }
    } else {
      // A5.5.5.2 [<Rn>, #+/-<offset_8>*4 ]!
      AsciiSPrint (&Buf[Index], Size - Index, "[%a, #%a0x%x*4]%a", gReg[Rn], SIGN (U), OpCode & 0xff, WRITE (W));
    }

  }

  if ((OpCode  & 0x0f000010) == 0x0e000010) {
    // A4.1.32 MRC2, MCR2
    AsciiSPrint (Buf, Size, "%a%a 0x%x, 0x%x, %a, CR%d, CR%d, 0x%x", L ? "MRC":"MCR", COND (OpCode), (OpCode >> 8) & 0xf, (OpCode >> 20) & 0xf, gReg[Rd], Rn, Rm, (OpCode >> 5) &0x7);
    return;
  }

  if ((OpCode  & 0x0ff00000) == 0x0c400000) {
    // A4.1.33 MRRC2, MCRR2
    AsciiSPrint (Buf, Size, "%a%a 0x%x, 0x%x, %a, %a, CR%d", L ? "MRRC":"MCRR", COND (OpCode), (OpCode >> 4) & 0xf, (OpCode >> 20) & 0xf, gReg[Rd], gReg[Rn], Rm);
    return;
  }

  AsciiSPrint (Buf, Size, "Faulting OpCode 0x%08x", OpCode);

  *OpCodePtr += 1;
  return;
}

