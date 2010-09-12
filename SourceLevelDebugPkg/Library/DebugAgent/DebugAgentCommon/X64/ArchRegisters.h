/** @file
  X64 register defintions needed by debug transfer protocol.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ARCH_REGISTERS_H_
#define _ARCH_REGISTERS_H_

///
/// FXSAVE_STATE (promoted operation)
/// FP / MMX / XMM registers (see fxrstor instruction definition)
///
typedef struct {
  UINT16  Fcw;
  UINT16  Fsw;
  UINT16  Ftw;
  UINT16  Opcode;
  UINT64  Rip;
  UINT64  DataOffset;
  UINT32  Mxcsr;
  UINT32  Mxcsr_Mask;
  UINT8   St0Mm0[10];
  UINT8   Reserved2[6];
  UINT8   St1Mm1[10];
  UINT8   Reserved3[6];
  UINT8   St2Mm2[10];
  UINT8   Reserved4[6];
  UINT8   St3Mm3[10];
  UINT8   Reserved5[6];
  UINT8   St4Mm4[10];
  UINT8   Reserved6[6];
  UINT8   St5Mm5[10];
  UINT8   Reserved7[6];
  UINT8   St6Mm6[10];
  UINT8   Reserved8[6];
  UINT8   St7Mm7[10];
  UINT8   Reserved9[6];
  UINT8   Xmm0[16];
  UINT8   Xmm1[16];
  UINT8   Xmm2[16];
  UINT8   Xmm3[16];
  UINT8   Xmm4[16];
  UINT8   Xmm5[16];
  UINT8   Xmm6[16];
  UINT8   Xmm7[16];
  UINT8   Xmm8[16];
  UINT8   Xmm9[16];
  UINT8   Xmm10[16];
  UINT8   Xmm11[16];
  UINT8   Xmm12[16];
  UINT8   Xmm13[16];
  UINT8   Xmm14[16];
  UINT8   Xmm15[16];
  UINT8   Reserved11[6 * 16];
} DEBUG_DATA_X64_FX_SAVE_STATE;

///
///  x64 processor context definition
///
typedef struct {
  DEBUG_DATA_X64_FX_SAVE_STATE   FxSaveState;
  UINT64                         Dr0;
  UINT64                         Dr1;
  UINT64                         Dr2;
  UINT64                         Dr3;
  UINT64                         Dr6;
  UINT64                         Dr7;
  UINT64                         Eflags;
  UINT64                         Ldtr;
  UINT64                         Tr;
  UINT64                         Gdtr[2];
  UINT64                         Idtr[2];
  UINT64                         Eip;
  UINT64                         Gs;
  UINT64                         Fs;
  UINT64                         Es;
  UINT64                         Ds;
  UINT64                         Cs;
  UINT64                         Ss;
  UINT64                         Cr0;
  UINT64                         Cr1;  /* Reserved */
  UINT64                         Cr2;
  UINT64                         Cr3;
  UINT64                         Cr4;
  UINT64                         Rdi;
  UINT64                         Rsi;
  UINT64                         Rbp;
  UINT64                         Rsp;
  UINT64                         Rdx;
  UINT64                         Rcx;
  UINT64                         Rbx;
  UINT64                         Rax;
  UINT64                         Cr8;
  UINT64                         R8;
  UINT64                         R9;
  UINT64                         R10;
  UINT64                         R11;
  UINT64                         R12;
  UINT64                         R13;
  UINT64                         R14;
  UINT64                         R15;
} DEBUG_DATA_X64_SYSTEM_CONTEXT;


///
///  x64 GROUP register
///
typedef struct {
  UINT16         Cs;
  UINT16         Ds;
  UINT16         Es;
  UINT16         Fs;
  UINT16         Gs;
  UINT16         Ss;
  UINT32         Eflags;
  UINT64         Rbp;
  UINT64         Eip;
  UINT64         Rsp;
  UINT64         Eax;
  UINT64         Rbx;
  UINT64         Rcx;
  UINT64         Rdx;
  UINT64         Rsi;
  UINT64         Rdi;
  UINT64         R8;
  UINT64         R9;
  UINT64         R10;
  UINT64         R11;
  UINT64         R12;
  UINT64         R13;
  UINT64         R14;
  UINT64         R15;
  UINT64         Dr0;
  UINT64         Dr1;
  UINT64         Dr2;
  UINT64         Dr3;
  UINT64         Dr6;
  UINT64         Dr7;
  UINT64         Cr0;
  UINT64         Cr2;
  UINT64         Cr3;
  UINT64         Cr4;
  UINT64         Cr8;
  UINT8          Xmm0[16];
  UINT8          Xmm1[16];
  UINT8          Xmm2[16];
  UINT8          Xmm3[16];
  UINT8          Xmm4[16];
  UINT8          Xmm5[16];
  UINT8          Xmm6[16];
  UINT8          Xmm7[16];
  UINT8          Xmm8[16];
  UINT8          Xmm9[16];
  UINT8          Xmm10[16];
  UINT8          Xmm11[16];
  UINT8          Xmm12[16];
  UINT8          Xmm13[16];
  UINT8          Xmm14[16];
  UINT8          Xmm15[16];
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_X64;

///
///  x64 Segment Limit GROUP register
///
typedef struct {
  UINT64         CsLim;
  UINT64         SsLim;
  UINT64         GsLim;
  UINT64         FsLim;
  UINT64         EsLim;
  UINT64         DsLim;
  UINT64         LdtLim;
  UINT64         TssLim;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGLIM_X64;

///
///  x64 Segment Base GROUP register
///
typedef struct {
  UINT64         CsBas;
  UINT64         SsBas;
  UINT64         GsBas;
  UINT64         FsBas;
  UINT64         EsBas;
  UINT64         DsBas;
  UINT64         LdtBas;
  UINT64         TssBas;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGBASE_X64;

///
///  x64 Segment Base/Limit GROUP register
///
typedef struct {
  UINT64         IdtBas;
  UINT64         IdtLim;
  UINT64         GdtBas;
  UINT64         GdtLim;
  UINT64         CsLim;
  UINT64         SsLim;
  UINT64         GsLim;
  UINT64         FsLim;
  UINT64         EsLim;
  UINT64         DsLim;
  UINT64         LdtLim;
  UINT64         TssLim;
  UINT64         CsBas;
  UINT64         SsBas;
  UINT64         GsBas;
  UINT64         FsBas;
  UINT64         EsBas;
  UINT64         DsBas;
  UINT64         LdtBas;
  UINT64         TssBas;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGMENT_BAS_LIM;

///
///  x64 register GROUP register
///
typedef struct {
  UINT32         Eflags;
  UINT64         Rbp;
  UINT64         Eip;
  UINT64         Rsp;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_GP2;

///
///  x64 general register GROUP register
///
typedef struct {
  UINT64         Eax;
  UINT64         Rbx;
  UINT64         Rcx;
  UINT64         Rdx;
  UINT64         Rsi;
  UINT64         Rdi;
  UINT64         R8;
  UINT64         R9;
  UINT64         R10;
  UINT64         R11;
  UINT64         R12;
  UINT64         R13;
  UINT64         R14;
  UINT64         R15;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_GP;

///
///  x64 Segment GROUP register
///
typedef struct {
  UINT16         Cs;
  UINT16         Ds;
  UINT16         Es;
  UINT16         Fs;
  UINT16         Gs;
  UINT16         Ss;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGMENT;

///
///  x64 Debug Register GROUP register
///
typedef struct {
  UINT64         Dr0;
  UINT64         Dr1;
  UINT64         Dr2;
  UINT64         Dr3;
  UINT64         Dr6;
  UINT64         Dr7;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_DR;

///
///  x64 Control Register GROUP register
///
typedef struct {
  UINT64         Cr0;
  UINT64         Cr2;
  UINT64         Cr3;
  UINT64         Cr4;
  UINT64         Cr8;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_CR;

///
///  x64 XMM Register GROUP register
///
typedef struct {
  UINT8          Xmm0[16];
  UINT8          Xmm1[16];
  UINT8          Xmm2[16];
  UINT8          Xmm3[16];
  UINT8          Xmm4[16];
  UINT8          Xmm5[16];
  UINT8          Xmm6[16];
  UINT8          Xmm7[16];
  UINT8          Xmm8[16];
  UINT8          Xmm9[16];
  UINT8          Xmm10[16];
  UINT8          Xmm11[16];
  UINT8          Xmm12[16];
  UINT8          Xmm13[16];
  UINT8          Xmm14[16];
  UINT8          Xmm15[16];
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_XMM;

///
///  x64 Segment Base GROUP register
///
typedef struct {
  UINT16         Ldtr;
  UINT16         Tr;
  UINT64         Csas;
  UINT64         Ssas;
  UINT64         Gsas;
  UINT64         Fsas;
  UINT64         Esas;
  UINT64         Dsas;
  UINT64         Ldtas;
  UINT64         Tssas;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGMENT_BASES_X64;


#endif
