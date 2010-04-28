/*++

Copyright (c) 2005, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  CpuSaveState.h

Abstract:

  Define data structures used by EFI_SMM_CPU_SAVE_STATE protocol.

Revision History

++*/

#ifndef _CPUSAVESTATE_H_
#define _CPUSAVESTATE_H_

typedef unsigned char               ASM_UINT8;
typedef ASM_UINT8                   ASM_BOOL;
typedef unsigned short              ASM_UINT16;
typedef unsigned long               ASM_UINT32;

#ifdef _H2INC
typedef double                      ASM_UINT64;
#else
typedef UINT64                      ASM_UINT64;
#endif

#ifndef __GNUC__
#pragma pack (push)
#pragma pack (1)
#endif

typedef struct _EFI_SMM_CPU_STATE32 {
  ASM_UINT8                         Reserved1[0xf8];        // fe00h
  ASM_UINT32                        SMBASE;                 // fef8h
  ASM_UINT32                        SMMRevId;               // fefch
  ASM_UINT16                        IORestart;              // ff00h
  ASM_UINT16                        AutoHALTRestart;        // ff02h
  ASM_UINT32                        IEDBASE;                // ff04h
  ASM_UINT8                         Reserved2[0x98];        // ff08h
  ASM_UINT32                        IOMemAddr;              // ffa0h
  ASM_UINT32                        IOMisc;                 // ffa4h
  ASM_UINT32                        _ES;
  ASM_UINT32                        _CS;
  ASM_UINT32                        _SS;
  ASM_UINT32                        _DS;
  ASM_UINT32                        _FS;
  ASM_UINT32                        _GS;
  ASM_UINT32                        _LDTBase;
  ASM_UINT32                        _TR;
  ASM_UINT32                        _DR7;
  ASM_UINT32                        _DR6;
  ASM_UINT32                        _EAX;
  ASM_UINT32                        _ECX;
  ASM_UINT32                        _EDX;
  ASM_UINT32                        _EBX;
  ASM_UINT32                        _ESP;
  ASM_UINT32                        _EBP;
  ASM_UINT32                        _ESI;
  ASM_UINT32                        _EDI;
  ASM_UINT32                        _EIP;
  ASM_UINT32                        _EFLAGS;
  ASM_UINT32                        _CR3;
  ASM_UINT32                        _CR0;
} EFI_SMM_CPU_STATE32;

typedef struct _EFI_SMM_CPU_STATE64 {
  ASM_UINT8                         Reserved1[0x1d0];       // fc00h
  ASM_UINT32                        GdtBaseHiDword;         // fdd0h
  ASM_UINT32                        LdtBaseHiDword;         // fdd4h
  ASM_UINT32                        IdtBaseHiDword;         // fdd8h
  ASM_UINT8                         Reserved2[0xc];         // fddch
  ASM_UINT64                        IO_EIP;                 // fde8h
  ASM_UINT8                         Reserved3[0x50];        // fdf0h
  ASM_UINT32                        _CR4;                   // fe40h
  ASM_UINT8                         Reserved4[0x48];        // fe44h
  ASM_UINT32                        GdtBaseLoDword;         // fe8ch
  ASM_UINT32                        GdtLimit;               // fe90h
  ASM_UINT32                        IdtBaseLoDword;         // fe94h
  ASM_UINT32                        IdtLimit;               // fe98h
  ASM_UINT32                        LdtBaseLoDword;         // fe9ch
  ASM_UINT32                        LdtLimit;               // fea0h
  ASM_UINT32                        LdtInfo;                // fea4h
  ASM_UINT8                         Reserved5[0x50];        // fea8h
  ASM_UINT32                        SMBASE;                 // fef8h
  ASM_UINT32                        SMMRevId;               // fefch
  ASM_UINT16                        IORestart;              // ff00h
  ASM_UINT16                        AutoHALTRestart;        // ff02h
  ASM_UINT32                        IEDBASE;                // ff04h
  ASM_UINT8                         Reserved6[0x14];        // ff08h
  ASM_UINT64                        _R15;                   // ff1ch
  ASM_UINT64                        _R14;
  ASM_UINT64                        _R13;
  ASM_UINT64                        _R12;
  ASM_UINT64                        _R11;
  ASM_UINT64                        _R10;
  ASM_UINT64                        _R9;
  ASM_UINT64                        _R8;
  ASM_UINT64                        _RAX;                   // ff5ch
  ASM_UINT64                        _RCX;
  ASM_UINT64                        _RDX;
  ASM_UINT64                        _RBX;
  ASM_UINT64                        _RSP;
  ASM_UINT64                        _RBP;
  ASM_UINT64                        _RSI;
  ASM_UINT64                        _RDI;
  ASM_UINT64                        IOMemAddr;              // ff9ch
  ASM_UINT32                        IOMisc;                 // ffa4h
  ASM_UINT32                        _ES;                    // ffa8h
  ASM_UINT32                        _CS;
  ASM_UINT32                        _SS;
  ASM_UINT32                        _DS;
  ASM_UINT32                        _FS;
  ASM_UINT32                        _GS;
  ASM_UINT32                        _LDTR;                  // ffc0h
  ASM_UINT32                        _TR;
  ASM_UINT64                        _DR7;                   // ffc8h
  ASM_UINT64                        _DR6;
  ASM_UINT64                        _RIP;                   // ffd8h
  ASM_UINT64                        IA32_EFER;              // ffe0h
  ASM_UINT64                        _RFLAGS;                // ffe8h
  ASM_UINT64                        _CR3;                   // fff0h
  ASM_UINT64                        _CR0;                   // fff8h
} EFI_SMM_CPU_STATE64;

#ifndef __GNUC__
#pragma warning (push)
#pragma warning (disable: 4201)
#endif


typedef union _EFI_SMM_CPU_STATE {
  struct {
    ASM_UINT8                       Reserved[0x200];
    EFI_SMM_CPU_STATE32             x86;
  };
  EFI_SMM_CPU_STATE64               x64;
} EFI_SMM_CPU_STATE;

#ifndef __GNUC__
#pragma warning (pop)
#pragma pack (pop)
#endif

#define EFI_SMM_MIN_REV_ID_x64      0x30006

#endif
