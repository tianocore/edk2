/** @file
  IA32/x64 architecture specific defintions needed by debug transfer protocol.It is only
  intended to be used by Debug related module implementation.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PROCESSOR_CONTEXT_H__
#define __PROCESSOR_CONTEXT_H__

//
//  IA-32/x64 processor register index table
//
#define SOFT_DEBUGGER_REGISTER_DR0     0x00
#define SOFT_DEBUGGER_REGISTER_DR1     0x01
#define SOFT_DEBUGGER_REGISTER_DR2     0x02
#define SOFT_DEBUGGER_REGISTER_DR3     0x03
#define SOFT_DEBUGGER_REGISTER_DR6     0x04
#define SOFT_DEBUGGER_REGISTER_DR7     0x05
#define SOFT_DEBUGGER_REGISTER_EFLAGS  0x06
#define SOFT_DEBUGGER_REGISTER_LDTR    0x07
#define SOFT_DEBUGGER_REGISTER_TR      0x08
#define SOFT_DEBUGGER_REGISTER_GDTR0   0x09 // the low 32bit of GDTR
#define SOFT_DEBUGGER_REGISTER_GDTR1   0x0A // the high 32bit of GDTR
#define SOFT_DEBUGGER_REGISTER_IDTR0   0x0B // the low 32bit of IDTR
#define SOFT_DEBUGGER_REGISTER_IDTR1   0x0C // the high 32bot of IDTR
#define SOFT_DEBUGGER_REGISTER_EIP     0x0D
#define SOFT_DEBUGGER_REGISTER_GS      0x0E
#define SOFT_DEBUGGER_REGISTER_FS      0x0F
#define SOFT_DEBUGGER_REGISTER_ES      0x10
#define SOFT_DEBUGGER_REGISTER_DS      0x11
#define SOFT_DEBUGGER_REGISTER_CS      0x12
#define SOFT_DEBUGGER_REGISTER_SS      0x13
#define SOFT_DEBUGGER_REGISTER_CR0     0x14
#define SOFT_DEBUGGER_REGISTER_CR1     0x15
#define SOFT_DEBUGGER_REGISTER_CR2     0x16
#define SOFT_DEBUGGER_REGISTER_CR3     0x17
#define SOFT_DEBUGGER_REGISTER_CR4     0x18

#define SOFT_DEBUGGER_REGISTER_DI      0x19
#define SOFT_DEBUGGER_REGISTER_SI      0x1A
#define SOFT_DEBUGGER_REGISTER_BP      0x1B
#define SOFT_DEBUGGER_REGISTER_SP      0x1C
#define SOFT_DEBUGGER_REGISTER_DX      0x1D
#define SOFT_DEBUGGER_REGISTER_CX      0x1E
#define SOFT_DEBUGGER_REGISTER_BX      0x1F
#define SOFT_DEBUGGER_REGISTER_AX      0x20

//
// This below registers are only available for x64 (not valid for Ia32 mode)
//
#define SOFT_DEBUGGER_REGISTER_CR8     0x21
#define SOFT_DEBUGGER_REGISTER_R8      0x22
#define SOFT_DEBUGGER_REGISTER_R9      0x23
#define SOFT_DEBUGGER_REGISTER_R10     0x24
#define SOFT_DEBUGGER_REGISTER_R11     0x25
#define SOFT_DEBUGGER_REGISTER_R12     0x26
#define SOFT_DEBUGGER_REGISTER_R13     0x27
#define SOFT_DEBUGGER_REGISTER_R14     0x28
#define SOFT_DEBUGGER_REGISTER_R15     0x29

//
// This below registers are FP / MMX / XMM registers
//
#define SOFT_DEBUGGER_REGISTER_FP_BASE            0x30

#define SOFT_DEBUGGER_REGISTER_FP_FCW          (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x00)
#define SOFT_DEBUGGER_REGISTER_FP_FSW          (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x01)
#define SOFT_DEBUGGER_REGISTER_FP_FTW          (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x02)
#define SOFT_DEBUGGER_REGISTER_FP_OPCODE       (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x03)
#define SOFT_DEBUGGER_REGISTER_FP_EIP          (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x04)
#define SOFT_DEBUGGER_REGISTER_FP_CS           (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x05)
#define SOFT_DEBUGGER_REGISTER_FP_DATAOFFSET   (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x06)
#define SOFT_DEBUGGER_REGISTER_FP_DS           (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x07)
#define SOFT_DEBUGGER_REGISTER_FP_MXCSR        (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x08)
#define SOFT_DEBUGGER_REGISTER_FP_MXCSR_MASK   (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x09)
#define SOFT_DEBUGGER_REGISTER_ST0             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x0A)
#define SOFT_DEBUGGER_REGISTER_ST1             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x0B)
#define SOFT_DEBUGGER_REGISTER_ST2             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x0C)
#define SOFT_DEBUGGER_REGISTER_ST3             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x0D)
#define SOFT_DEBUGGER_REGISTER_ST4             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x0E)
#define SOFT_DEBUGGER_REGISTER_ST5             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x0F)
#define SOFT_DEBUGGER_REGISTER_ST6             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x10)
#define SOFT_DEBUGGER_REGISTER_ST7             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x11)
#define SOFT_DEBUGGER_REGISTER_XMM0            (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x12)
#define SOFT_DEBUGGER_REGISTER_XMM1            (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x13)
#define SOFT_DEBUGGER_REGISTER_XMM2            (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x14)
#define SOFT_DEBUGGER_REGISTER_XMM3            (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x15)
#define SOFT_DEBUGGER_REGISTER_XMM4            (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x16)
#define SOFT_DEBUGGER_REGISTER_XMM5            (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x17)
#define SOFT_DEBUGGER_REGISTER_XMM6            (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x18)
#define SOFT_DEBUGGER_REGISTER_XMM7            (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x19)
#define SOFT_DEBUGGER_REGISTER_XMM8            (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x1A)
#define SOFT_DEBUGGER_REGISTER_XMM9            (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x1B)
#define SOFT_DEBUGGER_REGISTER_XMM10           (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x1C)
#define SOFT_DEBUGGER_REGISTER_XMM11           (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x1D)
#define SOFT_DEBUGGER_REGISTER_XMM12           (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x1E)
#define SOFT_DEBUGGER_REGISTER_XMM13           (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x1F)
#define SOFT_DEBUGGER_REGISTER_XMM14           (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x20)
#define SOFT_DEBUGGER_REGISTER_XMM15           (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x21)
#define SOFT_DEBUGGER_REGISTER_MM0             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x22)
#define SOFT_DEBUGGER_REGISTER_MM1             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x23)
#define SOFT_DEBUGGER_REGISTER_MM2             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x24)
#define SOFT_DEBUGGER_REGISTER_MM3             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x25)
#define SOFT_DEBUGGER_REGISTER_MM4             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x26)
#define SOFT_DEBUGGER_REGISTER_MM5             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x27)
#define SOFT_DEBUGGER_REGISTER_MM6             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x28)
#define SOFT_DEBUGGER_REGISTER_MM7             (SOFT_DEBUGGER_REGISTER_FP_BASE + 0x29)

#define SOFT_DEBUGGER_REGISTER_MAX             SOFT_DEBUGGER_REGISTER_MM7

#define SOFT_DEBUGGER_MSR_EFER                 (0xC0000080)

#pragma pack(1)

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
  UINT32                         ExceptionData;
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
  UINT64                         ExceptionData;
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
  UINT64                         Cr1;  ///< Reserved
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

#pragma pack()

#endif

