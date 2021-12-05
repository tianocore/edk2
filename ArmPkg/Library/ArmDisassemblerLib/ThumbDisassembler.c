/** @file
  Thumb Disassembler. Still a work in progress.

  Wrong output is a bug, so please fix it.
  Hex output means there is not yet an entry or a decode bug.
  gOpThumb[] are Thumb 16-bit, and gOpThumb2[] work on the 32-bit
  16-bit stream of Thumb2 instruction. Then there are big case
  statements to print everything out. If you are adding instructions
  try to reuse existing case entries if possible.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>

extern CHAR8  *gCondition[];

extern CHAR8  *gReg[];

// Thumb address modes
#define LOAD_STORE_FORMAT1           1
#define LOAD_STORE_FORMAT1_H         101
#define LOAD_STORE_FORMAT1_B         111
#define LOAD_STORE_FORMAT2           2
#define LOAD_STORE_FORMAT3           3
#define LOAD_STORE_FORMAT4           4
#define LOAD_STORE_MULTIPLE_FORMAT1  5
#define PUSH_FORMAT                  6
#define POP_FORMAT                   106
#define IMMED_8                      7
#define CONDITIONAL_BRANCH           8
#define UNCONDITIONAL_BRANCH         9
#define UNCONDITIONAL_BRANCH_SHORT   109
#define BRANCH_EXCHANGE              10
#define DATA_FORMAT1                 11
#define DATA_FORMAT2                 12
#define DATA_FORMAT3                 13
#define DATA_FORMAT4                 14
#define DATA_FORMAT5                 15
#define DATA_FORMAT6_SP              16
#define DATA_FORMAT6_PC              116
#define DATA_FORMAT7                 17
#define DATA_FORMAT8                 19
#define CPS_FORMAT                   20
#define ENDIAN_FORMAT                21
#define DATA_CBZ                     22
#define ADR_FORMAT                   23
#define IT_BLOCK                     24

// Thumb2 address modes
#define B_T3                  200
#define B_T4                  201
#define BL_T2                 202
#define POP_T2                203
#define POP_T3                204
#define STM_FORMAT            205
#define LDM_REG_IMM12_SIGNED  206
#define LDM_REG_IMM12_LSL     207
#define LDM_REG_IMM8          208
#define LDM_REG_IMM12         209
#define LDM_REG_INDIRECT_LSL  210
#define LDM_REG_IMM8_SIGNED   211
#define LDRD_REG_IMM8         212
#define LDREXB                213
#define LDREXD                214
#define SRS_FORMAT            215
#define RFE_FORMAT            216
#define LDRD_REG_IMM8_SIGNED  217
#define ADD_IMM12             218
#define ADD_IMM5              219
#define ADR_THUMB2            220
#define CMN_THUMB2            221
#define ASR_IMM5              222
#define ASR_3REG              223
#define BFC_THUMB2            224
#define CDP_THUMB2            225
#define THUMB2_NO_ARGS        226
#define THUMB2_2REGS          227
#define ADD_IMM5_2REG         228
#define CPD_THUMB2            229
#define THUMB2_4REGS          230
#define ADD_IMM12_1REG        231
#define THUMB2_IMM16          232
#define MRC_THUMB2            233
#define MRRC_THUMB2           234
#define THUMB2_MRS            235
#define THUMB2_MSR            236

typedef struct {
  CHAR8     *Start;
  UINT32    OpCode;
  UINT32    Mask;
  UINT32    AddressMode;
} THUMB_INSTRUCTIONS;

THUMB_INSTRUCTIONS  gOpThumb[] = {
  // Thumb 16-bit instructions
  //          Op       Mask   Format
  { "ADC",    0x4140, 0xffc0, DATA_FORMAT5                }, // ADC <Rndn>, <Rm>
  { "ADR",    0xa000, 0xf800, ADR_FORMAT                  }, // ADR <Rd>, <label>
  { "ADD",    0x1c00, 0xfe00, DATA_FORMAT2                },
  { "ADD",    0x3000, 0xf800, DATA_FORMAT3                },
  { "ADD",    0x1800, 0xfe00, DATA_FORMAT1                },
  { "ADD",    0x4400, 0xff00, DATA_FORMAT8                }, // A8.6.9
  { "ADD",    0xa000, 0xf100, DATA_FORMAT6_PC             },
  { "ADD",    0xa800, 0xf800, DATA_FORMAT6_SP             },
  { "ADD",    0xb000, 0xff80, DATA_FORMAT7                },

  { "AND",    0x4000, 0xffc0, DATA_FORMAT5                },

  { "ASR",    0x1000, 0xf800, DATA_FORMAT4                },
  { "ASR",    0x4100, 0xffc0, DATA_FORMAT5                },

  { "B",      0xd000, 0xf000, CONDITIONAL_BRANCH          },
  { "B",      0xe000, 0xf800, UNCONDITIONAL_BRANCH_SHORT  },
  { "BLX",    0x4780, 0xff80, BRANCH_EXCHANGE             },
  { "BX",     0x4700, 0xff87, BRANCH_EXCHANGE             },

  { "BIC",    0x4380, 0xffc0, DATA_FORMAT5                },
  { "BKPT",   0xdf00, 0xff00, IMMED_8                     },
  { "CBZ",    0xb100, 0xfd00, DATA_CBZ                    },
  { "CBNZ",   0xb900, 0xfd00, DATA_CBZ                    },
  { "CMN",    0x42c0, 0xffc0, DATA_FORMAT5                },

  { "CMP",    0x2800, 0xf800, DATA_FORMAT3                },
  { "CMP",    0x4280, 0xffc0, DATA_FORMAT5                },
  { "CMP",    0x4500, 0xff00, DATA_FORMAT8                },

  { "CPS",    0xb660, 0xffe8, CPS_FORMAT                  },
  { "MOV",    0x4600, 0xff00, DATA_FORMAT8                },
  { "EOR",    0x4040, 0xffc0, DATA_FORMAT5                },

  { "LDMIA",  0xc800, 0xf800, LOAD_STORE_MULTIPLE_FORMAT1 },
  { "LDR",    0x6800, 0xf800, LOAD_STORE_FORMAT1          }, // LDR <Rt>, [<Rn> {,#<imm>}]
  { "LDR",    0x5800, 0xfe00, LOAD_STORE_FORMAT2          }, // STR <Rt>, [<Rn>, <Rm>]
  { "LDR",    0x4800, 0xf800, LOAD_STORE_FORMAT3          },
  { "LDR",    0x9800, 0xf800, LOAD_STORE_FORMAT4          }, // LDR <Rt>, [SP, #<imm>]
  { "LDRB",   0x7800, 0xf800, LOAD_STORE_FORMAT1_B        },
  { "LDRB",   0x5c00, 0xfe00, LOAD_STORE_FORMAT2          }, // STR <Rt>, [<Rn>, <Rm>]
  { "LDRH",   0x8800, 0xf800, LOAD_STORE_FORMAT1_H        },
  { "LDRH",   0x7a00, 0xfe00, LOAD_STORE_FORMAT2          },
  { "LDRSB",  0x5600, 0xfe00, LOAD_STORE_FORMAT2          }, // STR <Rt>, [<Rn>, <Rm>]
  { "LDRSH",  0x5e00, 0xfe00, LOAD_STORE_FORMAT2          },

  { "MOVS",   0x0000, 0xffc0, DATA_FORMAT5                }, // LSL with imm5 == 0 is a MOVS, so this must go before LSL
  { "LSL",    0x0000, 0xf800, DATA_FORMAT4                },
  { "LSL",    0x4080, 0xffc0, DATA_FORMAT5                },
  { "LSR",    0x0001, 0xf800, DATA_FORMAT4                },
  { "LSR",    0x40c0, 0xffc0, DATA_FORMAT5                },
  { "LSRS",   0x0800, 0xf800, DATA_FORMAT4                }, // LSRS <Rd>, <Rm>, #<imm5>

  { "MOVS",   0x2000, 0xf800, DATA_FORMAT3                },
  { "MOV",    0x1c00, 0xffc0, DATA_FORMAT3                },
  { "MOV",    0x4600, 0xff00, DATA_FORMAT8                },

  { "MUL",    0x4340, 0xffc0, DATA_FORMAT5                },
  { "MVN",    0x41c0, 0xffc0, DATA_FORMAT5                },
  { "NEG",    0x4240, 0xffc0, DATA_FORMAT5                },
  { "ORR",    0x4300, 0xffc0, DATA_FORMAT5                },
  { "POP",    0xbc00, 0xfe00, POP_FORMAT                  },
  { "PUSH",   0xb400, 0xfe00, PUSH_FORMAT                 },

  { "REV",    0xba00, 0xffc0, DATA_FORMAT5                },
  { "REV16",  0xba40, 0xffc0, DATA_FORMAT5                },
  { "REVSH",  0xbac0, 0xffc0, DATA_FORMAT5                },

  { "ROR",    0x41c0, 0xffc0, DATA_FORMAT5                },
  { "SBC",    0x4180, 0xffc0, DATA_FORMAT5                },
  { "SETEND", 0xb650, 0xfff0, ENDIAN_FORMAT               },

  { "STMIA",  0xc000, 0xf800, LOAD_STORE_MULTIPLE_FORMAT1 },
  { "STR",    0x6000, 0xf800, LOAD_STORE_FORMAT1          }, // STR  <Rt>, [<Rn> {,#<imm>}]
  { "STR",    0x5000, 0xfe00, LOAD_STORE_FORMAT2          }, // STR  <Rt>, [<Rn>, <Rm>]
  { "STR",    0x9000, 0xf800, LOAD_STORE_FORMAT4          }, // STR  <Rt>, [SP, #<imm>]
  { "STRB",   0x7000, 0xf800, LOAD_STORE_FORMAT1_B        }, // STRB <Rt>, [<Rn>, #<imm5>]
  { "STRB",   0x5400, 0xfe00, LOAD_STORE_FORMAT2          }, // STRB <Rt>, [<Rn>, <Rm>]
  { "STRH",   0x8000, 0xf800, LOAD_STORE_FORMAT1_H        }, // STRH <Rt>, [<Rn>{,#<imm>}]
  { "STRH",   0x5200, 0xfe00, LOAD_STORE_FORMAT2          }, // STRH <Rt>, [<Rn>, <Rm>]

  { "SUB",    0x1e00, 0xfe00, DATA_FORMAT2                },
  { "SUB",    0x3800, 0xf800, DATA_FORMAT3                },
  { "SUB",    0x1a00, 0xfe00, DATA_FORMAT1                },
  { "SUB",    0xb080, 0xff80, DATA_FORMAT7                },

  { "SBC",    0x4180, 0xffc0, DATA_FORMAT5                },

  { "SWI",    0xdf00, 0xff00, IMMED_8                     },
  { "SXTB",   0xb240, 0xffc0, DATA_FORMAT5                },
  { "SXTH",   0xb200, 0xffc0, DATA_FORMAT5                },
  { "TST",    0x4200, 0xffc0, DATA_FORMAT5                },
  { "UXTB",   0xb2c0, 0xffc0, DATA_FORMAT5                },
  { "UXTH",   0xb280, 0xffc0, DATA_FORMAT5                },

  { "IT",     0xbf00, 0xff00, IT_BLOCK                    }
};

THUMB_INSTRUCTIONS  gOpThumb2[] = {
  // Instruct  OpCode      OpCode Mask  Addressig Mode

  { "ADR",    0xf2af0000, 0xfbff8000, ADR_THUMB2           }, // ADDR <Rd>, <label> ;Needs to go before ADDW
  { "CMN",    0xf1100f00, 0xfff08f00, CMN_THUMB2           }, // CMN <Rn>, #<const> ;Needs to go before ADD
  { "CMN",    0xeb100f00, 0xfff08f00, ADD_IMM5_2REG        }, // CMN <Rn>, <Rm> {,<shift> #<const>}
  { "CMP",    0xf1a00f00, 0xfff08f00, CMN_THUMB2           }, // CMP <Rn>, #<const>
  { "TEQ",    0xf0900f00, 0xfff08f00, CMN_THUMB2           }, // CMP <Rn>, #<const>
  { "TEQ",    0xea900f00, 0xfff08f00, ADD_IMM5_2REG        }, // CMN <Rn>, <Rm> {,<shift> #<const>}
  { "TST",    0xf0100f00, 0xfff08f00, CMN_THUMB2           }, // CMP <Rn>, #<const>
  { "TST",    0xea100f00, 0xfff08f00, ADD_IMM5_2REG        }, // TST <Rn>, <Rm> {,<shift> #<const>}

  { "MOV",    0xf04f0000, 0xfbef8000, ADD_IMM12_1REG       }, // MOV  <Rd>, #<const>
  { "MOVW",   0xf2400000, 0xfbe08000, THUMB2_IMM16         }, // MOVW <Rd>, #<const>
  { "MOVT",   0xf2c00000, 0xfbe08000, THUMB2_IMM16         }, // MOVT <Rd>, #<const>

  { "ADC",    0xf1400000, 0xfbe08000, ADD_IMM12            }, // ADC{S}  <Rd>, <Rn>, #<const>
  { "ADC",    0xeb400000, 0xffe08000, ADD_IMM5             }, // ADC{S}  <Rd>, <Rn>, <Rm> {,<shift> #<const>}
  { "ADD",    0xf1000000, 0xfbe08000, ADD_IMM12            }, // ADD{S}  <Rd>, <Rn>, #<const>
  { "ADD",    0xeb000000, 0xffe08000, ADD_IMM5             }, // ADD{S}  <Rd>, <Rn>, <Rm> {,<shift> #<const>}
  { "ADDW",   0xf2000000, 0xfbe08000, ADD_IMM12            }, // ADDW{S} <Rd>, <Rn>, #<const>
  { "AND",    0xf0000000, 0xfbe08000, ADD_IMM12            }, // AND{S}  <Rd>, <Rn>, #<const>
  { "AND",    0xea000000, 0xffe08000, ADD_IMM5             }, // AND{S}  <Rd>, <Rn>, <Rm> {,<shift> #<const>}
  { "BIC",    0xf0200000, 0xfbe08000, ADD_IMM12            }, // BIC{S}  <Rd>, <Rn>, #<const>
  { "BIC",    0xea200000, 0xffe08000, ADD_IMM5             }, // BIC{S}  <Rd>, <Rn>, <Rm> {,<shift> #<const>}
  { "EOR",    0xf0800000, 0xfbe08000, ADD_IMM12            }, // EOR{S}  <Rd>, <Rn>, #<const>
  { "EOR",    0xea800000, 0xffe08000, ADD_IMM5             }, // EOR{S}  <Rd>, <Rn>, <Rm> {,<shift> #<const>}
  { "ORN",    0xf0600000, 0xfbe08000, ADD_IMM12            }, // ORN{S}  <Rd>, <Rn>, #<const>
  { "ORN",    0xea600000, 0xffe08000, ADD_IMM5             }, // ORN{S}  <Rd>, <Rn>, <Rm> {,<shift> #<const>}
  { "ORR",    0xf0400000, 0xfbe08000, ADD_IMM12            }, // ORR{S}  <Rd>, <Rn>, #<const>
  { "ORR",    0xea400000, 0xffe08000, ADD_IMM5             }, // ORR{S}  <Rd>, <Rn>, <Rm> {,<shift> #<const>}
  { "RSB",    0xf1c00000, 0xfbe08000, ADD_IMM12            }, // RSB{S}  <Rd>, <Rn>, #<const>
  { "RSB",    0xebc00000, 0xffe08000, ADD_IMM5             }, // RSB{S}  <Rd>, <Rn>, <Rm> {,<shift> #<const>}
  { "SBC",    0xf1600000, 0xfbe08000, ADD_IMM12            }, // SBC{S}  <Rd>, <Rn>, #<const>
  { "SBC",    0xeb600000, 0xffe08000, ADD_IMM5             }, // SBC{S}  <Rd>, <Rn>, <Rm> {,<shift> #<const>}
  { "SUB",    0xf1a00000, 0xfbe08000, ADD_IMM12            }, // SUB{S}  <Rd>, <Rn>, #<const>
  { "SUB",    0xeba00000, 0xffe08000, ADD_IMM5             }, // SUB{S}  <Rd>, <Rn>, <Rm> {,<shift> #<const>}

  { "ASR",    0xea4f0020, 0xffef8030, ASR_IMM5             }, // ARS  <Rd>, <Rm> #<const>} imm3:imm2
  { "ASR",    0xfa40f000, 0xffe0f0f0, ASR_3REG             }, // ARS  <Rd>, <Rn>, <Rm>
  { "LSR",    0xea4f0010, 0xffef8030, ASR_IMM5             }, // LSR  <Rd>, <Rm> #<const>} imm3:imm2
  { "LSR",    0xfa20f000, 0xffe0f0f0, ASR_3REG             }, // LSR  <Rd>, <Rn>, <Rm>
  { "ROR",    0xea4f0030, 0xffef8030, ASR_IMM5             }, // ROR  <Rd>, <Rm> #<const>} imm3:imm2
  { "ROR",    0xfa60f000, 0xffe0f0f0, ASR_3REG             }, // ROR  <Rd>, <Rn>, <Rm>

  { "BFC",    0xf36f0000, 0xffff8010, BFC_THUMB2           }, // BFC  <Rd>, #<lsb>, #<width>
  { "BIC",    0xf3600000, 0xfff08010, BFC_THUMB2           }, // BIC  <Rn>, <Rd>, #<lsb>, #<width>
  { "SBFX",   0xf3400000, 0xfff08010, BFC_THUMB2           }, // SBFX <Rn>, <Rd>, #<lsb>, #<width>
  { "UBFX",   0xf3c00000, 0xfff08010, BFC_THUMB2           }, // UBFX <Rn>, <Rd>, #<lsb>, #<width>

  { "CPD",    0xee000000, 0xff000010, CPD_THUMB2           }, // CPD <coproc>,<opc1>,<CRd>,<CRn>,<CRm>,<opc2>
  { "CPD2",   0xfe000000, 0xff000010, CPD_THUMB2           }, // CPD <coproc>,<opc1>,<CRd>,<CRn>,<CRm>,<opc2>

  { "MRC",    0xee100000, 0xff100000, MRC_THUMB2           }, // MRC  <coproc>,<opc1>,<Rt>,<CRn>,<CRm>,<opc2>
  { "MRC2",   0xfe100000, 0xff100000, MRC_THUMB2           }, // MRC2 <coproc>,<opc1>,<Rt>,<CRn>,<CRm>,<opc2>
  { "MRRC",   0xec500000, 0xfff00000, MRRC_THUMB2          }, // MRRC <coproc>,<opc1>,<Rt>,<Rt2>,<CRm>
  { "MRRC2",  0xfc500000, 0xfff00000, MRRC_THUMB2          }, // MRR2 <coproc>,<opc1>,<Rt>,<Rt2>,<CRm>

  { "MRS",    0xf3ef8000, 0xfffff0ff, THUMB2_MRS           }, // MRS  <Rd>, CPSR
  { "MSR",    0xf3808000, 0xfff0fcff, THUMB2_MSR           }, // MSR  CPSR_fs, <Rn>

  { "CLREX",  0xf3bf8f2f, 0xfffffff,  THUMB2_NO_ARGS       }, // CLREX

  { "CLZ",    0xfab0f080, 0xfff0f0f0, THUMB2_2REGS         }, // CLZ    <Rd>,<Rm>
  { "MOV",    0xec4f0000, 0xfff0f0f0, THUMB2_2REGS         }, // MOV    <Rd>,<Rm>
  { "MOVS",   0xec5f0000, 0xfff0f0f0, THUMB2_2REGS         }, // MOVS   <Rd>,<Rm>
  { "RBIT",   0xfb90f0a0, 0xfff0f0f0, THUMB2_2REGS         }, // RBIT   <Rd>,<Rm>
  { "REV",    0xfb90f080, 0xfff0f0f0, THUMB2_2REGS         }, // REV    <Rd>,<Rm>
  { "REV16",  0xfa90f090, 0xfff0f0f0, THUMB2_2REGS         }, // REV16  <Rd>,<Rm>
  { "REVSH",  0xfa90f0b0, 0xfff0f0f0, THUMB2_2REGS         }, // REVSH  <Rd>,<Rm>
  { "RRX",    0xea4f0030, 0xfffff0f0, THUMB2_2REGS         }, // RRX    <Rd>,<Rm>
  { "RRXS",   0xea5f0030, 0xfffff0f0, THUMB2_2REGS         }, // RRXS   <Rd>,<Rm>

  { "MLA",    0xfb000000, 0xfff000f0, THUMB2_4REGS         }, // MLA <Rd>, <Rn>, <Rm>, <Ra>
  { "MLS",    0xfb000010, 0xfff000f0, THUMB2_4REGS         }, // MLA <Rd>, <Rn>, <Rm>, <Ra>

  { "SMLABB", 0xfb100000, 0xfff000f0, THUMB2_4REGS         }, // SMLABB   <Rd>, <Rn>, <Rm>, <Ra>
  { "SMLABT", 0xfb100010, 0xfff000f0, THUMB2_4REGS         }, // SMLABT   <Rd>, <Rn>, <Rm>, <Ra>
  { "SMLABB", 0xfb100020, 0xfff000f0, THUMB2_4REGS         }, // SMLATB   <Rd>, <Rn>, <Rm>, <Ra>
  { "SMLATT", 0xfb100030, 0xfff000f0, THUMB2_4REGS         }, // SMLATT   <Rd>, <Rn>, <Rm>, <Ra>
  { "SMLAWB", 0xfb300000, 0xfff000f0, THUMB2_4REGS         }, // SMLAWB   <Rd>, <Rn>, <Rm>, <Ra>
  { "SMLAWT", 0xfb300010, 0xfff000f0, THUMB2_4REGS         }, // SMLAWT   <Rd>, <Rn>, <Rm>, <Ra>
  { "SMLSD",  0xfb400000, 0xfff000f0, THUMB2_4REGS         }, // SMLSD    <Rd>, <Rn>, <Rm>, <Ra>
  { "SMLSDX", 0xfb400010, 0xfff000f0, THUMB2_4REGS         }, // SMLSDX   <Rd>, <Rn>, <Rm>, <Ra>
  { "SMMLA",  0xfb500000, 0xfff000f0, THUMB2_4REGS         }, // SMMLA    <Rd>, <Rn>, <Rm>, <Ra>
  { "SMMLAR", 0xfb500010, 0xfff000f0, THUMB2_4REGS         }, // SMMLAR   <Rd>, <Rn>, <Rm>, <Ra>
  { "SMMLS",  0xfb600000, 0xfff000f0, THUMB2_4REGS         }, // SMMLS    <Rd>, <Rn>, <Rm>, <Ra>
  { "SMMLSR", 0xfb600010, 0xfff000f0, THUMB2_4REGS         }, // SMMLSR   <Rd>, <Rn>, <Rm>, <Ra>
  { "USADA8", 0xfb700000, 0xfff000f0, THUMB2_4REGS         }, // USADA8   <Rd>, <Rn>, <Rm>, <Ra>
  { "SMLAD",  0xfb200000, 0xfff000f0, THUMB2_4REGS         }, // SMLAD    <Rd>, <Rn>, <Rm>, <Ra>
  { "SMLADX", 0xfb200010, 0xfff000f0, THUMB2_4REGS         }, // SMLADX   <Rd>, <Rn>, <Rm>, <Ra>

  { "B",      0xf0008000, 0xf800d000, B_T3                 }, // B<c> <label>
  { "B",      0xf0009000, 0xf800d000, B_T4                 }, // B<c> <label>
  { "BL",     0xf000d000, 0xf800d000, B_T4                 }, // BL<c> <label>
  { "BLX",    0xf000c000, 0xf800d000, BL_T2                }, // BLX<c> <label>

  { "POP",    0xe8bd0000, 0xffff2000, POP_T2               }, // POP <registers>
  { "POP",    0xf85d0b04, 0xffff0fff, POP_T3               }, // POP <register>
  { "PUSH",   0xe8ad0000, 0xffffa000, POP_T2               }, // PUSH <registers>
  { "PUSH",   0xf84d0d04, 0xffff0fff, POP_T3               }, // PUSH <register>
  { "STM",    0xe8800000, 0xffd0a000, STM_FORMAT           }, // STM <Rn>{!},<registers>
  { "STMDB",  0xe9800000, 0xffd0a000, STM_FORMAT           }, // STMDB <Rn>{!},<registers>
  { "LDM",    0xe8900000, 0xffd02000, STM_FORMAT           }, // LDM <Rn>{!},<registers>
  { "LDMDB",  0xe9100000, 0xffd02000, STM_FORMAT           }, // LDMDB <Rn>{!},<registers>

  { "LDR",    0xf8d00000, 0xfff00000, LDM_REG_IMM12        },   // LDR   <rt>, [<rn>, {, #<imm12>]}
  { "LDRB",   0xf8900000, 0xfff00000, LDM_REG_IMM12        },   // LDRB  <rt>, [<rn>, {, #<imm12>]}
  { "LDRH",   0xf8b00000, 0xfff00000, LDM_REG_IMM12        },   // LDRH  <rt>, [<rn>, {, #<imm12>]}
  { "LDRSB",  0xf9900000, 0xfff00000, LDM_REG_IMM12        },   // LDRSB <rt>, [<rn>, {, #<imm12>]}
  { "LDRSH",  0xf9b00000, 0xfff00000, LDM_REG_IMM12        },   // LDRSH <rt>, [<rn>, {, #<imm12>]}

  { "LDR",    0xf85f0000, 0xff7f0000, LDM_REG_IMM12_SIGNED },   // LDR   <Rt>, <label>
  { "LDRB",   0xf81f0000, 0xff7f0000, LDM_REG_IMM12_SIGNED },   // LDRB  <Rt>, <label>
  { "LDRH",   0xf83f0000, 0xff7f0000, LDM_REG_IMM12_SIGNED },   // LDRH  <Rt>, <label>
  { "LDRSB",  0xf91f0000, 0xff7f0000, LDM_REG_IMM12_SIGNED },   // LDRSB <Rt>, <label>
  { "LDRSH",  0xf93f0000, 0xff7f0000, LDM_REG_IMM12_SIGNED },   // LDRSB <Rt>, <label>

  { "LDR",    0xf8500000, 0xfff00fc0, LDM_REG_INDIRECT_LSL },   // LDR   <rt>, [<rn>, <rm> {, LSL #<imm2>]}
  { "LDRB",   0xf8100000, 0xfff00fc0, LDM_REG_INDIRECT_LSL },   // LDRB  <rt>, [<rn>, <rm> {, LSL #<imm2>]}
  { "LDRH",   0xf8300000, 0xfff00fc0, LDM_REG_INDIRECT_LSL },   // LDRH  <rt>, [<rn>, <rm> {, LSL #<imm2>]}
  { "LDRSB",  0xf9100000, 0xfff00fc0, LDM_REG_INDIRECT_LSL },   // LDRSB <rt>, [<rn>, <rm> {, LSL #<imm2>]}
  { "LDRSH",  0xf9300000, 0xfff00fc0, LDM_REG_INDIRECT_LSL },   // LDRSH <rt>, [<rn>, <rm> {, LSL #<imm2>]}

  { "LDR",    0xf8500800, 0xfff00800, LDM_REG_IMM8         },   // LDR    <rt>, [<rn>, {, #<imm8>]}
  { "LDRBT",  0xf8100e00, 0xfff00f00, LDM_REG_IMM8         },   // LDRBT  <rt>, [<rn>, {, #<imm8>]}
  { "LDRHT",  0xf8300e00, 0xfff00f00, LDM_REG_IMM8         },   // LDRHT  <rt>, [<rn>, {, #<imm8>]}
  { "LDRSB",  0xf9100800, 0xfff00800, LDM_REG_IMM8         },   // LDRHT  <rt>, [<rn>, {, #<imm8>]}  {!} form?
  { "LDRSBT", 0xf9100e00, 0xfff00f00, LDM_REG_IMM8         },   // LDRHBT <rt>, [<rn>, {, #<imm8>]}  {!} form?
  { "LDRSH",  0xf9300800, 0xfff00800, LDM_REG_IMM8         },   // LDRSH  <rt>, [<rn>, {, #<imm8>]}
  { "LDRSHT", 0xf9300e00, 0xfff00f00, LDM_REG_IMM8         },   // LDRSHT <rt>, [<rn>, {, #<imm8>]}
  { "LDRT",   0xf8500e00, 0xfff00f00, LDM_REG_IMM8         },   // LDRT   <rt>, [<rn>, {, #<imm8>]}

  { "LDRD",   0xe8500000, 0xfe500000, LDRD_REG_IMM8_SIGNED },   // LDRD <rt>, <rt2>, [<rn>, {, #<imm8>]}{!}
  { "LDRD",   0xe8500000, 0xfe500000, LDRD_REG_IMM8        },   // LDRD <rt>, <rt2>, <label>

  { "LDREX",  0xe8500f00, 0xfff00f00, LDM_REG_IMM8         },    // LDREX <Rt>, [Rn, {#imm8}]]
  { "LDREXB", 0xe8d00f4f, 0xfff00fff, LDREXB               },    // LDREXB <Rt>, [<Rn>]
  { "LDREXH", 0xe8d00f5f, 0xfff00fff, LDREXB               },    // LDREXH <Rt>, [<Rn>]

  { "LDREXD", 0xe8d00f4f, 0xfff00fff, LDREXD               },    // LDREXD <Rt>, <Rt2>, [<Rn>]

  { "STR",    0xf8c00000, 0xfff00000, LDM_REG_IMM12        },   // STR   <rt>, [<rn>, {, #<imm12>]}
  { "STRB",   0xf8800000, 0xfff00000, LDM_REG_IMM12        },   // STRB  <rt>, [<rn>, {, #<imm12>]}
  { "STRH",   0xf8a00000, 0xfff00000, LDM_REG_IMM12        },   // STRH  <rt>, [<rn>, {, #<imm12>]}

  { "STR",    0xf8400000, 0xfff00fc0, LDM_REG_INDIRECT_LSL },   // STR   <rt>, [<rn>, <rm> {, LSL #<imm2>]}
  { "STRB",   0xf8000000, 0xfff00fc0, LDM_REG_INDIRECT_LSL },   // STRB  <rt>, [<rn>, <rm> {, LSL #<imm2>]}
  { "STRH",   0xf8200000, 0xfff00fc0, LDM_REG_INDIRECT_LSL },   // STRH  <rt>, [<rn>, <rm> {, LSL #<imm2>]}

  { "STR",    0xf8400800, 0xfff00800, LDM_REG_IMM8         },   // STR    <rt>, [<rn>, {, #<imm8>]}
  { "STRH",   0xf8200800, 0xfff00800, LDM_REG_IMM8         },   // STRH   <rt>, [<rn>, {, #<imm8>]}
  { "STRBT",  0xf8000e00, 0xfff00f00, LDM_REG_IMM8         },   // STRBT  <rt>, [<rn>, {, #<imm8>]}
  { "STRHT",  0xf8200e00, 0xfff00f00, LDM_REG_IMM8         },   // STRHT  <rt>, [<rn>, {, #<imm8>]}
  { "STRT",   0xf8400e00, 0xfff00f00, LDM_REG_IMM8         },   // STRT   <rt>, [<rn>, {, #<imm8>]}

  { "STRD",   0xe8400000, 0xfe500000, LDRD_REG_IMM8_SIGNED },    // STRD <rt>, <rt2>, [<rn>, {, #<imm8>]}{!}

  { "STREX",  0xe8400f00, 0xfff00f00, LDM_REG_IMM8         },    // STREX <Rt>, [Rn, {#imm8}]]
  { "STREXB", 0xe8c00f4f, 0xfff00fff, LDREXB               },    // STREXB <Rd>, <Rt>, [<Rn>]
  { "STREXH", 0xe8c00f5f, 0xfff00fff, LDREXB               },    // STREXH <Rd>, <Rt>, [<Rn>]

  { "STREXD", 0xe8d00f4f, 0xfff00fff, LDREXD               },    // STREXD <Rd>, <Rt>, <Rt2>, [<Rn>]

  { "SRSDB",  0xe80dc000, 0xffdffff0, SRS_FORMAT           }, // SRSDB<c> SP{!},#<mode>
  { "SRS",    0xe98dc000, 0xffdffff0, SRS_FORMAT           }, // SRS{IA}<c> SP{!},#<mode>
  { "RFEDB",  0xe810c000, 0xffd0ffff, RFE_FORMAT           }, // RFEDB<c> <Rn>{!}
  { "RFE",    0xe990c000, 0xffd0ffff, RFE_FORMAT           } // RFE{IA}<c> <Rn>{!}
};

CHAR8  *gShiftType[] = {
  "LSL",
  "LSR",
  "ASR",
  "ROR"
};

CHAR8  mThumbMregListStr[4*15 + 1];

CHAR8 *
ThumbMRegList (
  UINT32  RegBitMask
  )
{
  UINTN    Index, Start, End;
  BOOLEAN  First;

  mThumbMregListStr[0] = '\0';
  AsciiStrCatS (mThumbMregListStr, sizeof mThumbMregListStr, "{");

  for (Index = 0, First = TRUE; Index <= 15; Index++) {
    if ((RegBitMask & (1 << Index)) != 0) {
      Start = End = Index;
      for (Index++; ((RegBitMask & (1 << Index)) != 0) && (Index <= 9); Index++) {
        End = Index;
      }

      if (!First) {
        AsciiStrCatS (mThumbMregListStr, sizeof mThumbMregListStr, ",");
      } else {
        First = FALSE;
      }

      if (Start == End) {
        AsciiStrCatS (mThumbMregListStr, sizeof mThumbMregListStr, gReg[Start]);
      } else {
        AsciiStrCatS (mThumbMregListStr, sizeof mThumbMregListStr, gReg[Start]);
        AsciiStrCatS (mThumbMregListStr, sizeof mThumbMregListStr, "-");
        AsciiStrCatS (mThumbMregListStr, sizeof mThumbMregListStr, gReg[End]);
      }
    }
  }

  if (First) {
    AsciiStrCatS (mThumbMregListStr, sizeof mThumbMregListStr, "ERROR");
  }

  AsciiStrCatS (mThumbMregListStr, sizeof mThumbMregListStr, "}");

  // BugBug: Make caller pass in buffer it is cleaner
  return mThumbMregListStr;
}

UINT32
SignExtend32 (
  IN  UINT32  Data,
  IN  UINT32  TopBit
  )
{
  if (((Data & TopBit) == 0) || (TopBit == BIT31)) {
    return Data;
  }

  do {
    TopBit <<= 1;
    Data    |= TopBit;
  } while ((TopBit & BIT31) != BIT31);

  return Data;
}

//
// Some instructions specify the PC is always considered aligned
// The PC is after the instruction that is executing. So you pass
// in the instruction address and you get back the aligned answer
//
UINT32
PcAlign4 (
  IN  UINT32  Data
  )
{
  return (Data + 4) & 0xfffffffc;
}

/**
  Place a disassembly of **OpCodePtr into buffer, and update OpCodePtr to
  point to next instruction.

  We cheat and only decode instructions that access
  memory. If the instruction is not found we dump the instruction in hex.

  @param  OpCodePtrPtr  Pointer to pointer of ARM Thumb instruction to disassemble.
  @param  Buf           Buffer to sprintf disassembly into.
  @param  Size          Size of Buf in bytes.
  @param  Extended    TRUE dump hex for instruction too.

**/
VOID
DisassembleThumbInstruction (
  IN  UINT16   **OpCodePtrPtr,
  OUT CHAR8    *Buf,
  OUT UINTN    Size,
  OUT UINT32   *ItBlock,
  IN  BOOLEAN  Extended
  )
{
  UINT16   *OpCodePtr;
  UINT16   OpCode;
  UINT32   OpCode32;
  UINT32   Index;
  UINT32   Offset;
  UINT16   Rd, Rn, Rm, Rt, Rt2;
  BOOLEAN  H1Bit; // H1
  BOOLEAN  H2Bit; // H2
  BOOLEAN  IMod;  // imod
  // BOOLEAN ItFlag;
  UINT32   Pc, Target, MsBit, LsBit;
  CHAR8    *Cond;
  BOOLEAN  Sign;      // S
  BOOLEAN  J1Bit;     // J1
  BOOLEAN  J2Bit;     // J2
  BOOLEAN  Pre;       // P
  BOOLEAN  UAdd;      // U
  BOOLEAN  WriteBack; // W
  UINT32   Coproc, Opc1, Opc2, CRd, CRn, CRm;
  UINT32   Mask;

  OpCodePtr = *OpCodePtrPtr;
  OpCode    = **OpCodePtrPtr;

  // Thumb2 is a stream of 16-bit instructions not a 32-bit instruction.
  OpCode32 = (((UINT32)OpCode) << 16) | *(OpCodePtr + 1);

  // These register names match branch form, but not others
  Rd    = OpCode & 0x7;
  Rn    = (OpCode >> 3) & 0x7;
  Rm    = (OpCode >> 6) & 0x7;
  H1Bit = (OpCode & BIT7) != 0;
  H2Bit = (OpCode & BIT6) != 0;
  IMod  = (OpCode & BIT4) != 0;
  Pc    = (UINT32)(UINTN)OpCodePtr;

  // Increment by the minimum instruction size, Thumb2 could be bigger
  *OpCodePtrPtr += 1;

  // Manage IT Block ItFlag TRUE means we are in an IT block

  /*if (*ItBlock != 0) {
    ItFlag = TRUE;
    *ItBlock -= 1;
  } else {
    ItFlag = FALSE;
  }*/

  for (Index = 0; Index < sizeof (gOpThumb)/sizeof (THUMB_INSTRUCTIONS); Index++) {
    if ((OpCode & gOpThumb[Index].Mask) == gOpThumb[Index].OpCode) {
      if (Extended) {
        Offset = AsciiSPrint (Buf, Size, "0x%04x       %-6a", OpCode, gOpThumb[Index].Start);
      } else {
        Offset = AsciiSPrint (Buf, Size, "%-6a", gOpThumb[Index].Start);
      }

      switch (gOpThumb[Index].AddressMode) {
        case LOAD_STORE_FORMAT1:
          // A6.5.1  <Rd>, [<Rn>, #<5_bit_offset>]
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, [r%d #0x%x]", Rd, Rn, (OpCode >> 4) & 0x7c);
          return;
        case LOAD_STORE_FORMAT1_H:
          // A6.5.1  <Rd>, [<Rn>, #<5_bit_offset>]
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, [r%d #0x%x]", Rd, Rn, (OpCode >> 5) & 0x3e);
          return;
        case LOAD_STORE_FORMAT1_B:
          // A6.5.1  <Rd>, [<Rn>, #<5_bit_offset>]
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, [r%d #0x%x]", Rd, Rn, (OpCode >> 6) & 0x1f);
          return;

        case LOAD_STORE_FORMAT2:
          // A6.5.1  <Rd>, [<Rn>, <Rm>]
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, [r%d, r%d]", Rd, Rn, Rm);
          return;
        case LOAD_STORE_FORMAT3:
          // A6.5.1 <Rd>, [PC, #<8_bit_offset>]
          Target = (OpCode & 0xff) << 2;
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, [pc, #0x%x] ;0x%08x", (OpCode >> 8) & 7, Target, PcAlign4 (Pc) + Target);
          return;
        case LOAD_STORE_FORMAT4:
          // Rt, [SP, #imm8]
          Target = (OpCode & 0xff) << 2;
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, [sp, #0x%x]", (OpCode >> 8) & 7, Target);
          return;

        case LOAD_STORE_MULTIPLE_FORMAT1:
          // <Rn>!, {r0-r7}
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d!, %a", (OpCode >> 8) & 7, ThumbMRegList (OpCode & 0xff));
          return;

        case POP_FORMAT:
          // POP {r0-r7,pc}
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a", ThumbMRegList ((OpCode & 0xff) | ((OpCode & BIT8) == BIT8 ? BIT15 : 0)));
          return;

        case PUSH_FORMAT:
          // PUSH {r0-r7,lr}
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a", ThumbMRegList ((OpCode & 0xff) | ((OpCode & BIT8) == BIT8 ? BIT14 : 0)));
          return;

        case IMMED_8:
          // A6.7 <immed_8>
          AsciiSPrint (&Buf[Offset], Size - Offset, " 0x%x", OpCode & 0xff);
          return;

        case CONDITIONAL_BRANCH:
          // A6.3.1 B<cond> <target_address>
          // Patch in the condition code. A little hack but based on "%-6a"
          Cond          = gCondition[(OpCode >> 8) & 0xf];
          Buf[Offset-5] = *Cond++;
          Buf[Offset-4] = *Cond;
          AsciiSPrint (&Buf[Offset], Size - Offset, " 0x%04x", Pc + 4 + SignExtend32 ((OpCode & 0xff) << 1, BIT8));
          return;
        case UNCONDITIONAL_BRANCH_SHORT:
          // A6.3.2 B  <target_address>
          AsciiSPrint (&Buf[Offset], Size - Offset, " 0x%04x", Pc + 4 + SignExtend32 ((OpCode & 0x3ff) << 1, BIT11));
          return;

        case BRANCH_EXCHANGE:
          // A6.3.3 BX|BLX <Rm>
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a", gReg[Rn | (H2Bit ? 8 : 0)]);
          return;

        case DATA_FORMAT1:
          // A6.4.3  <Rd>, <Rn>, <Rm>
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, r%d, r%d", Rd, Rn, Rm);
          return;
        case DATA_FORMAT2:
          // A6.4.3  <Rd>, <Rn>, #3_bit_immed
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, r%d, 0x%x", Rd, Rn, Rm);
          return;
        case DATA_FORMAT3:
          // A6.4.3  <Rd>|<Rn>, #imm8
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, #0x%x", (OpCode >> 8) & 7, OpCode & 0xff);
          return;
        case DATA_FORMAT4:
          // A6.4.3  <Rd>|<Rm>, #immed_5
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, r%d, 0x%x", Rn, Rd, (OpCode >> 6) & 0x1f);
          return;
        case DATA_FORMAT5:
          // A6.4.3  <Rd>|<Rm>, <Rm>|<Rs>
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, r%d", Rd, Rn);
          return;
        case DATA_FORMAT6_SP:
          // A6.4.3  <Rd>, <reg>, #<8_Bit_immed>
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, sp, 0x%x", (OpCode >> 8) & 7, (OpCode & 0xff) << 2);
          return;
        case DATA_FORMAT6_PC:
          // A6.4.3  <Rd>, <reg>, #<8_Bit_immed>
          AsciiSPrint (&Buf[Offset], Size - Offset, " r%d, pc, 0x%x", (OpCode >> 8) & 7, (OpCode & 0xff) << 2);
          return;
        case DATA_FORMAT7:
          // A6.4.3  SP, SP, #<7_Bit_immed>
          AsciiSPrint (&Buf[Offset], Size - Offset, " sp, sp, 0x%x", (OpCode & 0x7f)*4);
          return;
        case DATA_FORMAT8:
          // A6.4.3  <Rd>|<Rn>, <Rm>
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a", gReg[Rd | (H1Bit ? 8 : 0)], gReg[Rn | (H2Bit ? 8 : 0)]);
          return;

        case CPS_FORMAT:
          // A7.1.24
          AsciiSPrint (&Buf[Offset], Size - Offset, "%a %a%a%a", IMod ? "ID" : "IE", ((OpCode & BIT2) == 0) ? "" : "a", ((OpCode & BIT1) == 0) ? "" : "i", ((OpCode & BIT0) == 0) ? "" : "f");
          return;

        case ENDIAN_FORMAT:
          // A7.1.24
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a", (OpCode & BIT3) == 0 ? "LE" : "BE");
          return;

        case DATA_CBZ:
          // CB{N}Z <Rn>, <Lable>
          Target = ((OpCode >> 2) & 0x3e) | (((OpCode & BIT9) == BIT9) ? BIT6 : 0);
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %08x", gReg[Rd], Pc + 4 + Target);
          return;

        case ADR_FORMAT:
          // ADR <Rd>, <Label>
          Target = (OpCode & 0xff) << 2;
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %08x", gReg[(OpCode >> 8) & 7], PcAlign4 (Pc) + Target);
          return;

        case IT_BLOCK:
          // ITSTATE = cond:mask   OpCode[7:4]:OpCode[3:0]
          // ITSTATE[7:5] == cond[3:1]
          // ITSTATE[4] == 1st Instruction cond[0]
          // ITSTATE[3] == 2st Instruction cond[0]
          // ITSTATE[2] == 3st Instruction cond[0]
          // ITSTATE[1] == 4st Instruction cond[0]
          // ITSTATE[0] == 1 4 instruction IT block. 0 means 0,1,2 or 3 instructions
          // 1st one  in ITSTATE low bits defines the number of instructions
          Mask = (OpCode & 0xf);
          if ((Mask & 0x1) == 0x1) {
            *ItBlock = 4;
            Offset  +=  AsciiSPrint (&Buf[Offset], Size - Offset, "%a%a%a", (Mask & BIT3) ? "T" : "E", (Mask & BIT2) ? "T" : "E", (Mask & BIT1) ? "T" : "E");
          } else if ((OpCode & 0x3) == 0x2) {
            *ItBlock = 3;
            Offset  +=  AsciiSPrint (&Buf[Offset], Size - Offset, "%a%a", (Mask & BIT3) ? "T" : "E", (Mask & BIT2) ? "T" : "E");
          } else if ((OpCode & 0x7) == 0x4) {
            *ItBlock = 2;
            Offset  +=  AsciiSPrint (&Buf[Offset], Size - Offset, "%a", (Mask & BIT3) ? "T" : "E");
          } else if ((OpCode & 0xf) == 0x8) {
            *ItBlock = 1;
          }

          AsciiSPrint (&Buf[Offset], Size - Offset, " %a", gCondition[(OpCode >> 4) & 0xf]);
          return;
      }
    }
  }

  // Thumb2 are 32-bit instructions
  *OpCodePtrPtr += 1;
  Rt             = (OpCode32 >> 12) & 0xf;
  Rt2            = (OpCode32 >> 8) & 0xf;
  Rd             = (OpCode32 >> 8) & 0xf;
  Rm             = (OpCode32 & 0xf);
  Rn             = (OpCode32 >> 16) & 0xf;
  for (Index = 0; Index < sizeof (gOpThumb2)/sizeof (THUMB_INSTRUCTIONS); Index++) {
    if ((OpCode32 & gOpThumb2[Index].Mask) == gOpThumb2[Index].OpCode) {
      if (Extended) {
        Offset = AsciiSPrint (Buf, Size, "0x%04x   %-6a", OpCode32, gOpThumb2[Index].Start);
      } else {
        Offset = AsciiSPrint (Buf, Size, "   %-6a", gOpThumb2[Index].Start);
      }

      switch (gOpThumb2[Index].AddressMode) {
        case B_T3:
          Cond          = gCondition[(OpCode32 >> 22) & 0xf];
          Buf[Offset-5] = *Cond++;
          Buf[Offset-4] = *Cond;
          // S:J2:J1:imm6:imm11:0
          Target  = ((OpCode32 << 1) & 0xffe) + ((OpCode32 >> 4) & 0x3f000);
          Target |= ((OpCode32 & BIT11) == BIT11) ? BIT19 : 0; // J2
          Target |= ((OpCode32 & BIT13) == BIT13) ? BIT18 : 0; // J1
          Target |= ((OpCode32 & BIT26) == BIT26) ? BIT20 : 0; // S
          Target  = SignExtend32 (Target, BIT20);
          AsciiSPrint (&Buf[Offset], Size - Offset, " 0x%08x", Pc + 4 + Target);
          return;
        case B_T4:
          // S:I1:I2:imm10:imm11:0
          Target  = ((OpCode32 << 1) & 0xffe) + ((OpCode32 >> 4) & 0x3ff000);
          Sign    = (OpCode32 & BIT26) == BIT26;
          J1Bit   = (OpCode32 & BIT13) == BIT13;
          J2Bit   = (OpCode32 & BIT11) == BIT11;
          Target |= (!(J2Bit ^ Sign) ? BIT22 : 0); // I2
          Target |= (!(J1Bit ^ Sign) ? BIT23 : 0); // I1
          Target |= (Sign ? BIT24 : 0);            // S
          Target  = SignExtend32 (Target, BIT24);
          AsciiSPrint (&Buf[Offset], Size - Offset, " 0x%08x", Pc + 4 + Target);
          return;

        case BL_T2:
          // BLX  S:I1:I2:imm10:imm11:0
          Target  = ((OpCode32 << 1) & 0xffc) + ((OpCode32 >> 4) & 0x3ff000);
          Sign    = (OpCode32 & BIT26) == BIT26;
          J1Bit   = (OpCode32 & BIT13) == BIT13;
          J2Bit   = (OpCode32 & BIT11) == BIT11;
          Target |= (!(J2Bit ^ Sign) ? BIT23 : 0); // I2
          Target |= (!(J1Bit ^ Sign) ? BIT24 : 0); // I1
          Target |= (Sign ? BIT25 : 0);            // S
          Target  = SignExtend32 (Target, BIT25);
          AsciiSPrint (&Buf[Offset], Size - Offset, " 0x%08x", PcAlign4 (Pc) + Target);
          return;

        case POP_T2:
          // <reglist>  some must be zero, handled in table
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a", ThumbMRegList (OpCode32 & 0xffff));
          return;

        case POP_T3:
          // <register>
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a", gReg[(OpCode32 >> 12) & 0xf]);
          return;

        case STM_FORMAT:
          // <Rn>{!}, <registers>
          WriteBack = (OpCode32 & BIT21) == BIT21;
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a%a, %a", gReg[(OpCode32 >> 16) & 0xf], WriteBack ? "!" : "", ThumbMRegList (OpCode32 & 0xffff));
          return;

        case LDM_REG_IMM12_SIGNED:
          // <rt>, <label>
          Target = OpCode32 & 0xfff;
          if ((OpCode32 & BIT23) == 0) {
            // U == 0 means subtrack, U == 1 means add
            Target = -Target;
          }

          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a", gReg[(OpCode32 >> 12) & 0xf], PcAlign4 (Pc) + Target);
          return;

        case LDM_REG_INDIRECT_LSL:
          // <rt>, [<rn>, <rm> {, LSL #<imm2>]}
          Offset += AsciiSPrint (&Buf[Offset], Size - Offset, " %a, [%a, %a", gReg[Rt], gReg[Rn], gReg[Rm]);
          if (((OpCode32 >> 4) & 3) == 0) {
            AsciiSPrint (&Buf[Offset], Size - Offset, "]");
          } else {
            AsciiSPrint (&Buf[Offset], Size - Offset, ", LSL #%d]", (OpCode32 >> 4) & 3);
          }

          return;

        case LDM_REG_IMM12:
          // <rt>, [<rn>, {, #<imm12>]}
          Offset += AsciiSPrint (&Buf[Offset], Size - Offset, " %a, [%a", gReg[Rt], gReg[Rn]);
          if ((OpCode32 & 0xfff) == 0) {
            AsciiSPrint (&Buf[Offset], Size - Offset, "]");
          } else {
            AsciiSPrint (&Buf[Offset], Size - Offset, ", #0x%x]", OpCode32 & 0xfff);
          }

          return;

        case LDM_REG_IMM8:
          // <rt>, [<rn>, {, #<imm8>}]{!}
          WriteBack = (OpCode32 & BIT8) == BIT8;
          UAdd      = (OpCode32 & BIT9) == BIT9;
          Pre       = (OpCode32 & BIT10) == BIT10;
          Offset   += AsciiSPrint (&Buf[Offset], Size - Offset, " %a, [%a", gReg[Rt], gReg[Rn]);
          if (Pre) {
            if ((OpCode32 & 0xff) == 0) {
              AsciiSPrint (&Buf[Offset], Size - Offset, "]%a", WriteBack ? "!" : "");
            } else {
              AsciiSPrint (&Buf[Offset], Size - Offset, ", #%a0x%x]%a", UAdd ? "" : "-", OpCode32 & 0xff, WriteBack ? "!" : "");
            }
          } else {
            AsciiSPrint (&Buf[Offset], Size - Offset, "], #%a0x%x", UAdd ? "" : "-", OpCode32 & 0xff);
          }

          return;

        case LDRD_REG_IMM8_SIGNED:
          // LDRD <rt>, <rt2>, [<rn>, {, #<imm8>]}{!}
          Pre       = (OpCode32 & BIT24) == BIT24; // index = P
          UAdd      = (OpCode32 & BIT23) == BIT23;
          WriteBack = (OpCode32 & BIT21) == BIT21;
          Offset   += AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a, [%a", gReg[Rt], gReg[Rt2], gReg[Rn]);
          if (Pre) {
            if ((OpCode32 & 0xff) == 0) {
              AsciiSPrint (&Buf[Offset], Size - Offset, "]");
            } else {
              AsciiSPrint (&Buf[Offset], Size - Offset, ", #%a0x%x]%a", UAdd ? "" : "-", (OpCode32 & 0xff) << 2, WriteBack ? "!" : "");
            }
          } else {
            if ((OpCode32 & 0xff) != 0) {
              AsciiSPrint (&Buf[Offset], Size - Offset, ", #%a0x%x", UAdd ? "" : "-", (OpCode32 & 0xff) << 2);
            }
          }

          return;

        case LDRD_REG_IMM8:
          // LDRD <rt>, <rt2>, <label>
          Target = (OpCode32 & 0xff) << 2;
          if ((OpCode32 & BIT23) == 0) {
            // U == 0 means subtrack, U == 1 means add
            Target = -Target;
          }

          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a, %a", gReg[Rt], gReg[Rt2], Pc + 4 + Target);
          return;

        case LDREXB:
          // LDREXB <Rt>, [Rn]
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, [%a]", gReg[Rt], gReg[Rn]);
          return;

        case LDREXD:
          // LDREXD <Rt>, <Rt2>, [<Rn>]
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, ,%a, [%a]", gReg[Rt], gReg[Rt2], gReg[Rn]);
          return;

        case SRS_FORMAT:
          // SP{!}, #<mode>
          WriteBack = (OpCode32 & BIT21) == BIT21;
          AsciiSPrint (&Buf[Offset], Size - Offset, " SP%a, #0x%x", WriteBack ? "!" : "", OpCode32 & 0x1f);
          return;

        case RFE_FORMAT:
          // <Rn>{!}
          WriteBack = (OpCode32 & BIT21) == BIT21;
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a%a, #0x%x", gReg[Rn], WriteBack ? "!" : "");
          return;

        case ADD_IMM12:
          // ADD{S} <Rd>, <Rn>, #<const>   i:imm3:imm8
          if ((OpCode32 & BIT20) == BIT20) {
            Buf[Offset - 3] = 'S'; // assume %-6a
          }

          Target = (OpCode32 & 0xff) | ((OpCode32 >> 4) & 0x700) | ((OpCode & BIT26) == BIT26 ? BIT11 : 0);
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a, #0x%x", gReg[Rd], gReg[Rn], Target);
          return;

        case ADD_IMM12_1REG:
          // MOV{S} <Rd>, #<const>   i:imm3:imm8
          if ((OpCode32 & BIT20) == BIT20) {
            Buf[Offset - 3] = 'S'; // assume %-6a
          }

          Target = (OpCode32 & 0xff) | ((OpCode32 >> 4) & 0x700) | ((OpCode & BIT26) == BIT26 ? BIT11 : 0);
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, #0x%x", gReg[Rd], Target);
          return;

        case THUMB2_IMM16:
          // MOVW <Rd>, #<const>   i:imm3:imm8
          Target  = (OpCode32 & 0xff) | ((OpCode32 >> 4) & 0x700) | ((OpCode & BIT26) == BIT26 ? BIT11 : 0);
          Target |= ((OpCode32 >> 4) & 0xf0000);
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, #0x%x", gReg[Rd], Target);
          return;

        case ADD_IMM5:
          // ADC{S}  <Rd>, <Rn>, <Rm> {,LSL #<const>} imm3:imm2
          if ((OpCode32 & BIT20) == BIT20) {
            Buf[Offset - 3] = 'S'; // assume %-6a
          }

          Target  = ((OpCode32 >> 6) & 3) | ((OpCode32 >> 10) & 0x1c0);
          Offset += AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a, %a", gReg[Rd], gReg[Rn], gReg[Rm]);
          if (Target != 0) {
            AsciiSPrint (&Buf[Offset], Size - Offset, ", LSL %d", gShiftType[(OpCode >> 5) & 3], Target);
          }

          return;

        case ADD_IMM5_2REG:
          // CMP  <Rn>, <Rm> {,LSL #<const>} imm3:imm2
          Target  = ((OpCode32 >> 6) & 3) | ((OpCode32 >> 10) & 0x1c0);
          Offset += AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a", gReg[Rn], gReg[Rm]);
          if (Target != 0) {
            AsciiSPrint (&Buf[Offset], Size - Offset, ", LSL %d", gShiftType[(OpCode >> 5) & 3], Target);
          }

        case ASR_IMM5:
          // ARS  <Rd>, <Rm> #<const>} imm3:imm2
          if ((OpCode32 & BIT20) == BIT20) {
            Buf[Offset - 3] = 'S'; // assume %-6a
          }

          Target = ((OpCode32 >> 6) & 3) | ((OpCode32 >> 10) & 0x1c0);
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a #%d", gReg[Rd], gReg[Rm], Target);
          return;

        case ASR_3REG:
          // ARS  <Rd>, <Rn>, <Rm>
          if ((OpCode32 & BIT20) == BIT20) {
            Buf[Offset - 3] = 'S'; // assume %-6a
          }

          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a %a", gReg[Rd], gReg[Rn], gReg[Rm]);
          return;

        case ADR_THUMB2:
          // ADDR <Rd>, <label>
          Target = (OpCode32 & 0xff) | ((OpCode32 >> 8) & 0x700) | ((OpCode & BIT26) == BIT26 ? BIT11 : 0);
          if ((OpCode & (BIT23 | BIT21)) == (BIT23 | BIT21)) {
            Target = PcAlign4 (Pc) - Target;
          } else {
            Target = PcAlign4 (Pc) + Target;
          }

          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, 0x%08x", gReg[Rd], Target);
          return;

        case CMN_THUMB2:
          // CMN <Rn>, #<const>}
          Target = (OpCode32 & 0xff) | ((OpCode >> 4) & 0x700) | ((OpCode & BIT26) == BIT26 ? BIT11 : 0);
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, #0x%x", gReg[Rn], Target);
          return;

        case BFC_THUMB2:
          // BFI <Rd>, <Rn>, #<lsb>, #<width>
          MsBit = OpCode32 & 0x1f;
          LsBit = ((OpCode32 >> 6) & 3) | ((OpCode >> 10) &  0x1c);
          if ((Rn == 0xf) & (AsciiStrCmp (gOpThumb2[Index].Start, "BFC") == 0)) {
            // BFC <Rd>, #<lsb>, #<width>
            AsciiSPrint (&Buf[Offset], Size - Offset, " %a, #%d, #%d", gReg[Rd], LsBit, MsBit - LsBit + 1);
          } else if (AsciiStrCmp (gOpThumb2[Index].Start, "BFI") == 0) {
            AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a, #%d, #%d", gReg[Rd], gReg[Rn], LsBit, MsBit - LsBit + 1);
          } else {
            AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a, #%d, #%d", gReg[Rd], gReg[Rn], LsBit, MsBit + 1);
          }

          return;

        case CPD_THUMB2:
          // <coproc>,<opc1>,<CRd>,<CRn>,<CRm>,<opc2>
          Coproc  = (OpCode32 >> 8)  & 0xf;
          Opc1    = (OpCode32 >> 20) & 0xf;
          Opc2    = (OpCode32 >> 5)  & 0x7;
          CRd     = (OpCode32 >> 12) & 0xf;
          CRn     = (OpCode32 >> 16) & 0xf;
          CRm     = OpCode32 & 0xf;
          Offset += AsciiSPrint (&Buf[Offset], Size - Offset, " p%d,#%d,c%d,c%d,c%d", Coproc, Opc1, CRd, CRn, CRm);
          if (Opc2 != 0) {
            AsciiSPrint (&Buf[Offset], Size - Offset, ",#%d,", Opc2);
          }

          return;

        case MRC_THUMB2:
          // MRC  <coproc>,<opc1>,<Rt>,<CRn>,<CRm>,<opc2>
          Coproc  = (OpCode32 >> 8)  & 0xf;
          Opc1    = (OpCode32 >> 20) & 0xf;
          Opc2    = (OpCode32 >> 5)  & 0x7;
          CRn     = (OpCode32 >> 16) & 0xf;
          CRm     = OpCode32 & 0xf;
          Offset += AsciiSPrint (&Buf[Offset], Size - Offset, " p%d,#%d,%a,c%d,c%d", Coproc, Opc1, gReg[Rt], CRn, CRm);
          if (Opc2 != 0) {
            AsciiSPrint (&Buf[Offset], Size - Offset, ",#%d,", Opc2);
          }

          return;

        case MRRC_THUMB2:
          // MRC  <coproc>,<opc1>,<Rt>,<Rt2>,<CRm>,<opc2>
          Coproc  = (OpCode32 >> 8)  & 0xf;
          Opc1    = (OpCode32 >> 20) & 0xf;
          CRn     = (OpCode32 >> 16) & 0xf;
          CRm     = OpCode32 & 0xf;
          Offset += AsciiSPrint (&Buf[Offset], Size - Offset, " p%d,#%d,%a,%a,c%d", Coproc, Opc1, gReg[Rt], gReg[Rt2], CRm);
          return;

        case THUMB2_2REGS:
          // <Rd>, <Rm>
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a", gReg[Rd], gReg[Rm]);
          return;

        case THUMB2_4REGS:
          // <Rd>, <Rn>, <Rm>, <Ra>
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, %a, %a, %a", gReg[Rd], gReg[Rn], gReg[Rm], gReg[Rt]);
          return;

        case THUMB2_MRS:
          // MRS <Rd>, CPSR
          AsciiSPrint (&Buf[Offset], Size - Offset, " %a, CPSR", gReg[Rd]);
          return;

        case THUMB2_MSR:
          // MRS CPSR_<fields>, <Rd>
          Target = (OpCode32 >> 10) & 3;
          AsciiSPrint (&Buf[Offset], Size - Offset, " CPSR_%a%a, %a", (Target & 2) == 0 ? "" : "f", (Target & 1) == 0 ? "" : "s", gReg[Rd]);
          return;

        case THUMB2_NO_ARGS:
        default:
          break;
      }
    }
  }

  AsciiSPrint (Buf, Size, "0x%08x", OpCode32);
}

VOID
DisassembleArmInstruction (
  IN  UINT32   **OpCodePtr,
  OUT CHAR8    *Buf,
  OUT UINTN    Size,
  IN  BOOLEAN  Extended
  );

/**
  Place a disassembly of **OpCodePtr into buffer, and update OpCodePtr to
  point to next instruction.

  We cheat and only decode instructions that access
  memory. If the instruction is not found we dump the instruction in hex.

  @param  OpCodePtrPtr  Pointer to pointer of ARM Thumb instruction to disassemble.
  @param  Thumb         TRUE for Thumb(2), FALSE for ARM instruction stream
  @param  Extended      TRUE dump hex for instruction too.
  @param  ItBlock       Size of IT Block
  @param  Buf           Buffer to sprintf disassembly into.
  @param  Size          Size of Buf in bytes.

**/
VOID
DisassembleInstruction (
  IN  UINT8      **OpCodePtr,
  IN  BOOLEAN    Thumb,
  IN  BOOLEAN    Extended,
  IN OUT UINT32  *ItBlock,
  OUT CHAR8      *Buf,
  OUT UINTN      Size
  )
{
  if (Thumb) {
    DisassembleThumbInstruction ((UINT16 **)OpCodePtr, Buf, Size, ItBlock, Extended);
  } else {
    DisassembleArmInstruction ((UINT32 **)OpCodePtr, Buf, Size, Extended);
  }
}
