/** @file
  Default exception handler

  Copyright (c) 2008-2010, Apple Inc. All rights reserved.
  
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>

extern CHAR8 *gReg[];

#define LOAD_STORE_FORMAT1            1
#define LOAD_STORE_FORMAT2            2
#define LOAD_STORE_FORMAT3            3
#define LOAD_STORE_FORMAT4            4
#define LOAD_STORE_MULTIPLE_FORMAT1   5 
#define LOAD_STORE_MULTIPLE_FORMAT2   6 
#define IMMED_8                       7
#define CONDITIONAL_BRANCH            8
#define UNCONDITIONAL_BRANCH          9
#define UNCONDITIONAL_BRANCH_SHORT  109
#define BRANCH_EXCHANGE              10
#define DATA_FORMAT1                 11
#define DATA_FORMAT2                 12
#define DATA_FORMAT3                 13
#define DATA_FORMAT4                 14
#define DATA_FORMAT5                 15
#define DATA_FORMAT6_SP              16
#define DATA_FORMAT6_PC             116
#define DATA_FORMAT7                 17
#define DATA_FORMAT8                 19
#define CPS_FORMAT                   20
#define ENDIAN_FORMAT                21
     

typedef struct {
  CHAR8   *Start;
  UINT32  OpCode;
  UINT32  Mask;
  UINT32  AddressMode;
} THUMB_INSTRUCTIONS;

THUMB_INSTRUCTIONS gOp[] = {
// Thumb 16-bit instrucitons
//         Op      Mask    Format
  { "ADC" , 0x4140, 0xffc0, DATA_FORMAT5 },

  { "ADD" , 0x1c00, 0xfe00, DATA_FORMAT2 },
  { "ADD" , 0x3000, 0xf800, DATA_FORMAT3 },
  { "ADD" , 0x1800, 0xfe00, DATA_FORMAT1 },
  { "ADD" , 0x4400, 0xff00, DATA_FORMAT8 },   // A8.6.9
  { "ADD" , 0xa000, 0xf100, DATA_FORMAT6_PC },
  { "ADD" , 0xa100, 0xf100, DATA_FORMAT6_SP }, 
  { "ADD" , 0xb000, 0xff10, DATA_FORMAT7 },

  { "AND" , 0x4000, 0xffc0, DATA_FORMAT5 },

  { "ASR" , 0x1000, 0xf800, DATA_FORMAT4 },
  { "ASR" , 0x4100, 0xffc0, DATA_FORMAT5 },

  { "B"   , 0xd000, 0xf000, CONDITIONAL_BRANCH },
  { "B"   , 0xe000, 0xf100, UNCONDITIONAL_BRANCH_SHORT },
  { "BL"  , 0xf100, 0xf100, UNCONDITIONAL_BRANCH },
  { "BLX" , 0xe100, 0xf100, UNCONDITIONAL_BRANCH },
  { "BLX" , 0x4780, 0xff80, BRANCH_EXCHANGE },
  { "BX"  , 0x4700, 0xff80, BRANCH_EXCHANGE },

  { "BIC" , 0x4380, 0xffc0, DATA_FORMAT5 },
  { "BKPT", 0xdf00, 0xff00, IMMED_8 },
  { "CMN" , 0x42c0, 0xffc0, DATA_FORMAT5 },

  { "CMP" , 0x2800, 0xf100, DATA_FORMAT3 },
  { "CMP" , 0x4280, 0xffc0, DATA_FORMAT5 },
  { "CMP" , 0x4500, 0xff00, DATA_FORMAT8 },

  { "CPS" , 0xb660, 0xffe8, CPS_FORMAT },
  { "CPY" , 0x4600, 0xff00, DATA_FORMAT8 },
  { "EOR" , 0x4040, 0xffc0, DATA_FORMAT5 },

  { "LDMIA" , 0xc800, 0xf800, LOAD_STORE_MULTIPLE_FORMAT1 },
  { "LDR"   , 0x6800, 0xf800, LOAD_STORE_FORMAT1 },
  { "LDR"   , 0x5800, 0xfe00, LOAD_STORE_FORMAT2 },
  { "LDR"   , 0x4800, 0xf800, LOAD_STORE_FORMAT3 },
  { "LDR"   , 0x9800, 0xf800, LOAD_STORE_FORMAT4 },
  { "LDRB"  , 0x7800, 0xf800, LOAD_STORE_FORMAT1 },
  { "LDRB"  , 0x5c00, 0xfe00, LOAD_STORE_FORMAT2 },
  { "LDRH"  , 0x8800, 0xf800, LOAD_STORE_FORMAT1 },
  { "LDRH"  , 0x7a00, 0xfe00, LOAD_STORE_FORMAT2 },
  { "LDRSB" , 0x5600, 0xfe00, LOAD_STORE_FORMAT2 },
  { "LDRSH" , 0x5e00, 0xfe00, LOAD_STORE_FORMAT2 },
 
  { "LSL" , 0x0000, 0xf800, DATA_FORMAT4 },
  { "LSL" , 0x4080, 0xffc0, DATA_FORMAT5 },
  { "LSR" , 0x0001, 0xf800, DATA_FORMAT4 },
  { "LSR" , 0x40c0, 0xffc0, DATA_FORMAT5 },

  { "MOV" , 0x2000, 0xf800, DATA_FORMAT3 },
  { "MOV" , 0x1c00, 0xffc0, DATA_FORMAT3 },
  { "MOV" , 0x4600, 0xff00, DATA_FORMAT8 },

  { "MUL" , 0x4340, 0xffc0, DATA_FORMAT5 },
  { "MVN" , 0x41c0, 0xffc0, DATA_FORMAT5 },
  { "NEG" , 0x4240, 0xffc0, DATA_FORMAT5 },
  { "ORR" , 0x4180, 0xffc0, DATA_FORMAT5 },
  { "POP" , 0xbc00, 0xfe00, LOAD_STORE_MULTIPLE_FORMAT2 },
  { "POP" , 0xe400, 0xfe00, LOAD_STORE_MULTIPLE_FORMAT2 },
  
  { "REV"   , 0xba00, 0xffc0, DATA_FORMAT5 },
  { "REV16" , 0xba40, 0xffc0, DATA_FORMAT5 },
  { "REVSH" , 0xbac0, 0xffc0, DATA_FORMAT5 },

  { "ROR"  , 0x41c0, 0xffc0, DATA_FORMAT5 },
  { "SBC"  , 0x4180, 0xffc0, DATA_FORMAT5 },
  { "SETEND"  , 0xb650, 0xfff0, ENDIAN_FORMAT },

  { "STMIA" , 0xc000, 0xf800, LOAD_STORE_MULTIPLE_FORMAT1 },
  { "STR"   , 0x6000, 0xf800, LOAD_STORE_FORMAT1 },
  { "STR"   , 0x5000, 0xfe00, LOAD_STORE_FORMAT2 },
  { "STR"   , 0x4000, 0xf800, LOAD_STORE_FORMAT3 },
  { "STR"   , 0x9000, 0xf800, LOAD_STORE_FORMAT4 },
  { "STRB"  , 0x7000, 0xf800, LOAD_STORE_FORMAT1 },
  { "STRB"  , 0x5800, 0xfe00, LOAD_STORE_FORMAT2 },
  { "STRH"  , 0x8000, 0xf800, LOAD_STORE_FORMAT1 },
  { "STRH"  , 0x5200, 0xfe00, LOAD_STORE_FORMAT2 },

  { "SUB" , 0x1e00, 0xfe00, DATA_FORMAT2 },
  { "SUB" , 0x3800, 0xf800, DATA_FORMAT3 },
  { "SUB" , 0x1a00, 0xfe00, DATA_FORMAT1 },
  { "SUB" , 0xb080, 0xff80, DATA_FORMAT7 },

  { "SWI" , 0xdf00, 0xff00, IMMED_8 },
  { "SXTB", 0xb240, 0xffc0, DATA_FORMAT5 },
  { "SXTH", 0xb200, 0xffc0, DATA_FORMAT5 },
  { "TST" , 0x4200, 0xffc0, DATA_FORMAT5 },
  { "UXTB", 0xb2c0, 0xffc0, DATA_FORMAT5 },
  { "UXTH", 0xb280, 0xffc0, DATA_FORMAT5 }
  
#if 0  
  ,
  
  // 32-bit Thumb instructions  op1 01
  
  //  1110 100x x0xx xxxx xxxx xxxx xxxx xxxx Load/store multiple
  { "SRSDB", 0xe80dc000, 0xffdffff0, SRS_FORMAT },       // SRSDB<c> SP{!},#<mode>
  { "SRS"  , 0xe98dc000, 0xffdffff0, SRS_IA_FORMAT },    // SRS{IA}<c> SP{!},#<mode>
  { "RFEDB", 0xe810c000, 0xffd0ffff, RFE_FORMAT },       // RFEDB<c> <Rn>{!}
  { "RFE"  , 0xe990c000, 0xffd0ffff, RFE_IA_FORMAT },    // RFE{IA}<c> <Rn>{!}
  
  { "STM"  , 0xe8800000, 0xffd00000,  STM_FORMAT },      // STM<c>.W <Rn>{!},<registers>
  { "LDM"  , 0xe8900000, 0xffd00000,  STM_FORMAT },      // LDR<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<imm2>}]
  { "POP"  , 0xe8bd0000, 0xffff2000,  REGLIST_FORMAT },  // POP<c>.W <registers> >1 register
  { "POP"  , 0xf85d0b04, 0xffff0fff,  RT_FORMAT },       // POP<c>.W <registers>  1 register

  { "STMDB", 0xe9000000, 0xffd00000,  STM_FORMAT },      // STMDB
  { "PUSH" , 0xe8bd0000, 0xffffa000,  REGLIST_FORMAT },  // PUSH<c>.W <registers>  >1 register
  { "PUSH" , 0xf84d0b04, 0xffff0fff,  RT_FORMAT },       // PUSH<c>.W <registers>   1 register
  { "LDMDB", 0xe9102000, 0xffd02000,  STM_FORMAT },      // LDMDB<c> <Rn>{!},<registers>

  //  1110 100x x1xx xxxx xxxx xxxx xxxx xxxx Load/store dual,
  { "STREX" , 0xe0400000, 0xfff000f0, 3REG_IMM8_FORMAT },  // STREX<c> <Rd>,<Rt>,[<Rn>{,#<imm>}]
  { "STREXB", 0xe8c00f40, 0xfff00ff0, 3REG_FORMAT },       // STREXB<c> <Rd>,<Rt>,[<Rn>]
  { "STREXD", 0xe8c00070, 0xfff000f0, 4REG_FORMAT },       // STREXD<c> <Rd>,<Rt>,<Rt2>,[<Rn>]
  { "STREXH", 0xe8c00f70, 0xfff00ff0, 3REG_FORMAT },       // STREXH<c> <Rd>,<Rt>,[<Rn>]
  { "STRH",   0xf8c00000, 0xfff00000, 2REG_IMM8_FORMAT },  // STRH<c>.W <Rt>,[<Rn>{,#<imm12>}]
  { "STRH",   0xf8200000, 0xfff00000,  },                  // STRH<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<imm2>}]



  //  1110 101x xxxx xxxx xxxx xxxx xxxx xxxx Data-processing
  //  1110 11xx xxxx xxxx xxxx xxxx xxxx xxxx Coprocessor
  
  //  1111 0x0x xxxx xxxx 0xxx xxxx xxxx xxxx Data-processing modified immediate
  //  1111 0x1x xxxx xxxx 0xxx xxxx xxxx xxxx Data-processing plain immediate
  //  1111 0xxx xxxx xxxx 1xxx xxxx xxxx xxxx Branches
  
  //  1111 1000 xxx0 xxxx xxxx xxxx xxxx xxxx Store single data item
  //  1111 1001 xxx0 xxxx xxxx xxxx xxxx xxxx SIMD or load/store
  //  1111 100x x001 xxxx xxxx xxxx xxxx xxxx Load byte, memory hints 
  //  1111 100x x011 xxxx xxxx xxxx xxxx xxxx Load halfword, memory hints
  //  1111 100x x101 xxxx xxxx xxxx xxxx xxxx Load word 

  //  1111 1 010 xxxx xxxx xxxx xxxx xxxx xxxx Data-processing register
  //  1111 1 011 0xxx xxxx xxxx xxxx xxxx xxxx Multiply
  //  1111 1 011 1xxx xxxx xxxx xxxx xxxx xxxx Long Multiply
  //  1111 1 1xx xxxx xxxx xxxx xxxx xxxx xxxx Coprocessor 
#endif
};


CHAR8 mThumbMregListStr[4*15 + 1];

CHAR8 *
ThumbMRegList (
  UINT32  OpCode
  )
{
  UINTN     Index, Start, End;
  CHAR8     *Str;
  BOOLEAN   First;
 
  Str = mThumbMregListStr;
  *Str = '\0';
  AsciiStrCat  (Str, "{");
  // R0 - R7, PC
  for (Index = 0, First = TRUE; Index <= 9; Index++) {
    if ((OpCode & (1 << Index)) != 0) {
      Start = End = Index;
      for (Index++; ((OpCode & (1 << Index)) != 0) && (Index <= 9); Index++) {
        End = Index;
      }
      
      if (!First) {
        AsciiStrCat  (Str, ",");
      } else {
        First = FALSE;
      }
      
      if (Start == End) {
        AsciiStrCat  (Str, gReg[(Start == 9)?15:Start]);
        AsciiStrCat  (Str, ", ");
      } else {
        AsciiStrCat  (Str, gReg[Start]);
        AsciiStrCat  (Str, "-");
        AsciiStrCat  (Str, gReg[(End == 9)?15:End]);
      }
    }
  }
  if (First) {
    AsciiStrCat  (Str, "ERROR");
  }
  AsciiStrCat  (Str, "}");
  
  // BugBug: Make caller pass in buffer it is cleaner
  return mThumbMregListStr;
}

UINT32
SignExtend (
  IN  UINT32  Data
  )
{
  return 0;
}

/**
  DEBUG print the faulting instruction. We cheat and only decode instructions that access 
  memory. If the instruction is not found we dump the instruction in hex.
   
  @param  Insturction   ARM instruction to disassemble.  
  
**/
VOID
DisassembleThumbInstruction (
  IN  UINT16    *OpCodePtr,
  OUT CHAR8     *Buf,
  OUT UINTN     Size
  )
{
  UINT16  OpCode = *OpCodePtr;
  UINT32  Index;
  UINT32  Offset;
  UINT16  Rd, Rn, Rm;
  INT32   target_addr;
  BOOLEAN H1, H2, imod;
  UINT32  PC;

  // These register names match branch form, but not others
  Rd = OpCode & 0x7;
  Rn = (OpCode >> 3) & 0x7;
  Rm = (OpCode >> 6) & 0x7;
  H1 = (OpCode & BIT7) != 0;
  H2 = (OpCode & BIT6) != 0;
  imod = (OpCode & BIT4) != 0;
  PC = (UINT32)(UINTN)*OpCodePtr;

  for (Index = 0; Index < sizeof (gOp)/sizeof (THUMB_INSTRUCTIONS); Index++) {
    if ((OpCode & gOp[Index].Mask) == gOp[Index].OpCode) {
      Offset = AsciiSPrint (Buf, Size, "%a", gOp[Index].Start);   
      switch (gOp[Index].AddressMode) {
      case LOAD_STORE_FORMAT1:
        // A6.5.1  <Rd>, [<Rn>, #<5_bit_offset>]
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, [r%d #0x%x]", Rd, (OpCode >> 7) & 7, (OpCode >> 6) & 0x1f);   
        break;
      case LOAD_STORE_FORMAT2:
        // A6.5.1  <Rd>, [<Rn>, <Rm>]
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, [r%d, r%d]", Rd, (OpCode >> 3) & 7, Rm);   
        break;
      case LOAD_STORE_FORMAT3:
        // A6.5.1 <Rd>, [PC, #<8_bit_offset>]
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, [pc, #0x%x]", (OpCode >> 8) & 7, OpCode & 0xff);   
        break;
      case LOAD_STORE_FORMAT4:
        // FIX ME!!!!!
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, [sp, #0x%x]", (OpCode >> 8) & 7, OpCode & 0xff);   
        break;
      
      case LOAD_STORE_MULTIPLE_FORMAT1:
        // <Rn>!, <registers>  
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d!, %a", (OpCode >> 8) & 7, ThumbMRegList (!BIT8 & OpCode));   
        break;
      case LOAD_STORE_MULTIPLE_FORMAT2:
        // <Rn>!, <registers>  
        // BIT8 is PC 
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d!, %a", (OpCode >> 8) & 7, ThumbMRegList (OpCode));   
        break;
      
      case IMMED_8:
        // A6.7 <immed_8>
        AsciiSPrint (&Buf[Offset], Size - Offset, " 0x%x", OpCode & 0xff);   
        break;

      case CONDITIONAL_BRANCH:
        // A6.3.1 B<cond> <target_address>
        AsciiSPrint (&Buf[Offset], Size - Offset, "%a 0x%04x", PC + 4 + SignExtend ((OpCode & 0xff) << 1));   
        break;
      case UNCONDITIONAL_BRANCH_SHORT:
        // A6.3.2 B  <target_address>
        AsciiSPrint (&Buf[Offset], Size - Offset, " 0x%04x", PC + 4 + SignExtend ((OpCode & 0x3ff) << 1));   
        break;
      case UNCONDITIONAL_BRANCH:
        // A6.3.2 BL|BLX <target_address>  ; Produces two 16-bit instructions 
        target_addr = *(OpCodePtr - 1);
        if ((target_addr & 0xf800) == 0xf000) {
          target_addr = ((target_addr & 0x3ff) << 12) | (OpCode & 0x3ff);
        } else {
          target_addr = OpCode & 0x3ff;
        }
        // PC + 2 +/- target_addr
        AsciiSPrint (&Buf[Offset], Size - Offset, " 0x%04x", PC + 2 + SignExtend (target_addr));   
        break;
      case BRANCH_EXCHANGE:
        // A6.3.3 BX|BLX <Rm>
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d", gReg[Rn | (H2 ? 8:0)]);   
        break;

      case DATA_FORMAT1:
        // A6.4.3  <Rd>, <Rn>, <Rm>
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, r%d, r%d", Rd, Rn, Rm);   
        break;
      case DATA_FORMAT2:
        // A6.4.3  <Rd>, <Rn>, #3_bit_immed
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, r%d, 0x%x", Rd, Rn, Rm);   
        break;
      case DATA_FORMAT3:
        // A6.4.3  <Rd>|<Rn>, #8_bit_immed
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, r%d, 0x%x", (OpCode >> 8) & 0x7, OpCode & 0xff);   
        break;
      case DATA_FORMAT4:
        // A6.4.3  <Rd>|<Rm>, #immed_5
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, r%d, 0x%x", Rn, Rd, (OpCode >> 6) & 0x1f);   
        break;
      case DATA_FORMAT5:
        // A6.4.3  <Rd>|<Rm>, <Rm>|<Rs>
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, r%d", Rd, Rn);   
        break;
      case DATA_FORMAT6_SP:
        // A6.4.3  <Rd>, <reg>, #<8_Bit_immed>
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, sp, 0x%x", (OpCode >> 8) & 7, OpCode & 0xff);   
        break;
      case DATA_FORMAT6_PC:
        // A6.4.3  <Rd>, <reg>, #<8_Bit_immed>
        AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, pc, 0x%x", (OpCode >> 8) & 7, OpCode & 0xff);   
        break;
      case DATA_FORMAT7:
        // A6.4.3  SP, SP, #<7_Bit_immed>
        AsciiSPrint (&Buf[Offset], Size - Offset, " sp, sp 0x%x", (OpCode & 0x7f)*4);   
        break;
      case DATA_FORMAT8:
        // A6.4.3  <Rd>|<Rn>, <Rm>
        AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a", gReg[Rd | (H1 ? 8:0)], gReg[Rn | (H2 ? 8:0)]);   
        break;
      
      case CPS_FORMAT:
        // A7.1.24
        AsciiSPrint (&Buf[Offset], Size - Offset, "%a %a%a%a", imod ? "ID":"IE", ((OpCode & BIT2) == 0) ? "":"a",  ((OpCode & BIT1) == 0) ? "":"i", ((OpCode & BIT0) == 0) ? "":"f");   
        break;

      case ENDIAN_FORMAT:
        // A7.1.24
        AsciiSPrint (&Buf[Offset], Size - Offset, " %a", (OpCode & BIT3) == 0 ? "LE":"BE");   
        break;
      }
    }
  }
      

}

 
