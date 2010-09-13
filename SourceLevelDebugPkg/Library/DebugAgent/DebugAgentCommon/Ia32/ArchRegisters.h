/** @file
  IA32 register defintions needed by debug transfer protocol.

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
/// FXSAVE_STATE
/// FP / MMX / XMM registers (see fxrstor instruction definition)
///
typedef struct {
  UINT16  Fcw;
  UINT16  Fsw;
  UINT16  Ftw;
  UINT16  Opcode;
  UINT32  Eip;
  UINT16  Cs;
  UINT16  Reserved1;
  UINT32  DataOffset;
  UINT16  Ds;
  UINT8   Reserved2[2];
  UINT32  Mxcsr;
  UINT32  Mxcsr_Mask;
  UINT8   St0Mm0[10];
  UINT8   Reserved3[6];
  UINT8   St1Mm1[10];
  UINT8   Reserved4[6];
  UINT8   St2Mm2[10];
  UINT8   Reserved5[6];
  UINT8   St3Mm3[10];
  UINT8   Reserved6[6];
  UINT8   St4Mm4[10];
  UINT8   Reserved7[6];
  UINT8   St5Mm5[10];
  UINT8   Reserved8[6];
  UINT8   St6Mm6[10];
  UINT8   Reserved9[6];
  UINT8   St7Mm7[10];
  UINT8   Reserved10[6];
  UINT8   Xmm0[16];
  UINT8   Xmm1[16];
  UINT8   Xmm2[16];
  UINT8   Xmm3[16];
  UINT8   Xmm4[16];
  UINT8   Xmm5[16];
  UINT8   Xmm6[16];
  UINT8   Xmm7[16];
  UINT8   Reserved11[14 * 16];
} DEBUG_DATA_IA32_FX_SAVE_STATE;

///
///  IA-32 processor context definition
///
typedef struct {
  DEBUG_DATA_IA32_FX_SAVE_STATE  FxSaveState;
  UINT32                         Dr0;
  UINT32                         Dr1;
  UINT32                         Dr2;
  UINT32                         Dr3;
  UINT32                         Dr6;
  UINT32                         Dr7;
  UINT32                         Eflags;
  UINT32                         Ldtr;
  UINT32                         Tr;
  UINT32                         Gdtr[2];
  UINT32                         Idtr[2];
  UINT32                         Eip;
  UINT32                         Gs;
  UINT32                         Fs;
  UINT32                         Es;
  UINT32                         Ds;
  UINT32                         Cs;
  UINT32                         Ss;
  UINT32                         Cr0;
  UINT32                         Cr1;  ///< Reserved
  UINT32                         Cr2;
  UINT32                         Cr3;
  UINT32                         Cr4;
  UINT32                         Edi;
  UINT32                         Esi;
  UINT32                         Ebp;
  UINT32                         Esp;
  UINT32                         Edx;
  UINT32                         Ecx;
  UINT32                         Ebx;
  UINT32                         Eax;
} DEBUG_DATA_IA32_SYSTEM_CONTEXT;

///
///  IA32 GROUP register
///
typedef struct {
  UINT16         Cs;
  UINT16         Ds;
  UINT16         Es;
  UINT16         Fs;
  UINT16         Gs;
  UINT16         Ss;
  UINT32         Eflags;
  UINT32         Ebp;
  UINT32         Eip;
  UINT32         Esp;
  UINT32         Eax;
  UINT32         Ebx;
  UINT32         Ecx;
  UINT32         Edx;
  UINT32         Esi;
  UINT32         Edi;
  UINT32         Dr0;
  UINT32         Dr1;
  UINT32         Dr2;
  UINT32         Dr3;
  UINT32         Dr6;
  UINT32         Dr7;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_IA32;

///
///  IA32 Segment Limit GROUP register
///
typedef struct {
  UINT32         CsLim;
  UINT32         SsLim;
  UINT32         GsLim;
  UINT32         FsLim;
  UINT32         EsLim;
  UINT32         DsLim;
  UINT32         LdtLim;
  UINT32         TssLim;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGLIM_IA32;

///
///  IA32 Segment Base GROUP register
///
typedef struct {
  UINT32         CsBas;
  UINT32         SsBas;
  UINT32         GsBas;
  UINT32         FsBas;
  UINT32         EsBas;
  UINT32         DsBas;
  UINT32         LdtBas;
  UINT32         TssBas;
} DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGBASE_IA32;

#endif
