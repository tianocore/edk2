/** @file
SMRAM Save State Map Definitions.

SMRAM Save State Map definitions based on contents of the
Intel(R) 64 and IA-32 Architectures Software Developer's Manual
  Volume 3C, Section 34.4 SMRAM
  Volume 3C, Section 34.5 SMI Handler Execution Environment
  Volume 3C, Section 34.7 Managing Synchronous and Asynchronous SMIs

and the AMD64 Architecture Programmer's Manual
  Volume 2, Section 10.2 SMM Resources

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2015, Red Hat, Inc.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __QEMU_SMRAM_SAVE_STATE_MAP_H__
#define __QEMU_SMRAM_SAVE_STATE_MAP_H__

#pragma pack (1)

///
/// 32-bit SMRAM Save State Map
///
typedef struct {
  UINT8     Reserved0[0x200]; // 7c00h
  UINT8     Reserved1[0xf8];  // 7e00h
  UINT32    SMBASE;           // 7ef8h
  UINT32    SMMRevId;         // 7efch
  UINT16    IORestart;        // 7f00h
  UINT16    AutoHALTRestart;  // 7f02h
  UINT8     Reserved2[0x9C];  // 7f08h
  UINT32    IOMemAddr;        // 7fa0h
  UINT32    IOMisc;           // 7fa4h
  UINT32    _ES;              // 7fa8h
  UINT32    _CS;              // 7fach
  UINT32    _SS;              // 7fb0h
  UINT32    _DS;              // 7fb4h
  UINT32    _FS;              // 7fb8h
  UINT32    _GS;              // 7fbch
  UINT32    Reserved3;        // 7fc0h
  UINT32    _TR;              // 7fc4h
  UINT32    _DR7;             // 7fc8h
  UINT32    _DR6;             // 7fcch
  UINT32    _EAX;             // 7fd0h
  UINT32    _ECX;             // 7fd4h
  UINT32    _EDX;             // 7fd8h
  UINT32    _EBX;             // 7fdch
  UINT32    _ESP;             // 7fe0h
  UINT32    _EBP;             // 7fe4h
  UINT32    _ESI;             // 7fe8h
  UINT32    _EDI;             // 7fech
  UINT32    _EIP;             // 7ff0h
  UINT32    _EFLAGS;          // 7ff4h
  UINT32    _CR3;             // 7ff8h
  UINT32    _CR0;             // 7ffch
} QEMU_SMRAM_SAVE_STATE_MAP32;

///
/// 64-bit SMRAM Save State Map
///
typedef struct {
  UINT8     Reserved0[0x200]; // 7c00h

  UINT16    _ES;             // 7e00h
  UINT16    _ESAccessRights; // 7e02h
  UINT32    _ESLimit;        // 7e04h
  UINT64    _ESBase;         // 7e08h

  UINT16    _CS;             // 7e10h
  UINT16    _CSAccessRights; // 7e12h
  UINT32    _CSLimit;        // 7e14h
  UINT64    _CSBase;         // 7e18h

  UINT16    _SS;             // 7e20h
  UINT16    _SSAccessRights; // 7e22h
  UINT32    _SSLimit;        // 7e24h
  UINT64    _SSBase;         // 7e28h

  UINT16    _DS;             // 7e30h
  UINT16    _DSAccessRights; // 7e32h
  UINT32    _DSLimit;        // 7e34h
  UINT64    _DSBase;         // 7e38h

  UINT16    _FS;             // 7e40h
  UINT16    _FSAccessRights; // 7e42h
  UINT32    _FSLimit;        // 7e44h
  UINT64    _FSBase;         // 7e48h

  UINT16    _GS;             // 7e50h
  UINT16    _GSAccessRights; // 7e52h
  UINT32    _GSLimit;        // 7e54h
  UINT64    _GSBase;         // 7e58h

  UINT32    _GDTRReserved1;  // 7e60h
  UINT16    _GDTRLimit;      // 7e64h
  UINT16    _GDTRReserved2;  // 7e66h
  UINT64    _GDTRBase;       // 7e68h

  UINT16    _LDTR;             // 7e70h
  UINT16    _LDTRAccessRights; // 7e72h
  UINT32    _LDTRLimit;        // 7e74h
  UINT64    _LDTRBase;         // 7e78h

  UINT32    _IDTRReserved1;  // 7e80h
  UINT16    _IDTRLimit;      // 7e84h
  UINT16    _IDTRReserved2;  // 7e86h
  UINT64    _IDTRBase;       // 7e88h

  UINT16    _TR;             // 7e90h
  UINT16    _TRAccessRights; // 7e92h
  UINT32    _TRLimit;        // 7e94h
  UINT64    _TRBase;         // 7e98h

  UINT64    IO_RIP;          // 7ea0h
  UINT64    IO_RCX;          // 7ea8h
  UINT64    IO_RSI;          // 7eb0h
  UINT64    IO_RDI;          // 7eb8h
  UINT32    IO_DWord;        // 7ec0h
  UINT8     Reserved1[0x04]; // 7ec4h
  UINT8     IORestart;       // 7ec8h
  UINT8     AutoHALTRestart; // 7ec9h
  UINT8     Reserved2[0x06]; // 7ecah

  UINT64    IA32_EFER;       // 7ed0h
  UINT64    SVM_Guest;       // 7ed8h
  UINT64    SVM_GuestVMCB;   // 7ee0h
  UINT64    SVM_GuestVIntr;  // 7ee8h
  UINT8     Reserved3[0x0c]; // 7ef0h

  UINT32    SMMRevId;        // 7efch
  UINT32    SMBASE;          // 7f00h

  UINT8     Reserved4[0x1c];   // 7f04h
  UINT64    SVM_GuestPAT;      // 7f20h
  UINT64    SVM_HostIA32_EFER; // 7f28h
  UINT64    SVM_HostCR4;       // 7f30h
  UINT64    SVM_HostCR3;       // 7f38h
  UINT64    SVM_HostCR0;       // 7f40h

  UINT64    _CR4;            // 7f48h
  UINT64    _CR3;            // 7f50h
  UINT64    _CR0;            // 7f58h
  UINT64    _DR7;            // 7f60h
  UINT64    _DR6;            // 7f68h
  UINT64    _RFLAGS;         // 7f70h
  UINT64    _RIP;            // 7f78h
  UINT64    _R15;            // 7f80h
  UINT64    _R14;            // 7f88h
  UINT64    _R13;            // 7f90h
  UINT64    _R12;            // 7f98h
  UINT64    _R11;            // 7fa0h
  UINT64    _R10;            // 7fa8h
  UINT64    _R9;             // 7fb0h
  UINT64    _R8;             // 7fb8h
  UINT64    _RDI;            // 7fc0h
  UINT64    _RSI;            // 7fc8h
  UINT64    _RBP;            // 7fd0h
  UINT64    _RSP;            // 7fd8h
  UINT64    _RBX;            // 7fe0h
  UINT64    _RDX;            // 7fe8h
  UINT64    _RCX;            // 7ff0h
  UINT64    _RAX;            // 7ff8h
} QEMU_SMRAM_SAVE_STATE_MAP64;

///
/// Union of 32-bit and 64-bit SMRAM Save State Maps
///
typedef union  {
  QEMU_SMRAM_SAVE_STATE_MAP32    x86;
  QEMU_SMRAM_SAVE_STATE_MAP64    x64;
} QEMU_SMRAM_SAVE_STATE_MAP;

#pragma pack ()

#endif
