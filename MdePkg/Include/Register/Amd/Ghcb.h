/** @file
  Guest-Hypervisor Communication Block (GHCB) Definition.

  Provides data types allowing an SEV-ES guest to interact with the hypervisor
  using the GHCB protocol.

  Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
  SEV-ES Guest-Hypervisor Communication Block Standardization

**/

#ifndef __GHCB_H__
#define __GHCB_H__

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#define UD_EXCEPTION  6
#define GP_EXCEPTION 13

#define GHCB_VERSION_MIN     1
#define GHCB_VERSION_MAX     1

#define GHCB_STANDARD_USAGE  0

typedef enum {
  SvmExitDr7Read       = 0x27,
  SvmExitDr7Write      = 0x37,
  SvmExitRdtsc         = 0x6E,
  SvmExitRdpmc,
  SvmExitCpuid         = 0x72,
  SvmExitInvd          = 0x76,
  SvmExitIoioProt      = 0x7B,
  SvmExitMsr,
  SvmExitVmmCall       = 0x81,
  SvmExitRdtscp        = 0x87,
  SvmExitWbinvd        = 0x89,
  SvmExitMonitor,
  SvmExitMwait,
  SvmExitNpf           = 0x400,

  // VMG special exits
  SvmExitMmioRead      = 0x80000001,
  SvmExitMmioWrite,
  SvmExitNmiComplete,
  SvmExitApResetHold,
  SvmExitApJumpTable,

  SvmExitUnsupported   = 0x8000FFFF,
} SVM_EXITCODE;

typedef enum {
  GhcbCpl              = 25,
  GhcbRflags           = 46,
  GhcbRip,
  GhcbRsp              = 59,
  GhcbRax              = 63,
  GhcbRcx              = 97,
  GhcbRdx,
  GhcbRbx,
  GhcbRbp              = 101,
  GhcbRsi,
  GhcbRdi,
  GhcbR8,
  GhcbR9,
  GhcbR10,
  GhcbR11,
  GhcbR12,
  GhcbR13,
  GhcbR14,
  GhcbR15,
  GhcbXCr0             = 125,
} GHCB_REGISTER;

typedef PACKED struct {
  UINT8                  Reserved1[203];
  UINT8                  Cpl;
  UINT8                  Reserved2[148];
  UINT64                 Dr7;
  UINT8                  Reserved3[144];
  UINT64                 Rax;
  UINT8                  Reserved4[264];
  UINT64                 Rcx;
  UINT64                 Rdx;
  UINT64                 Rbx;
  UINT8                  Reserved5[112];
  UINT64                 SwExitCode;
  UINT64                 SwExitInfo1;
  UINT64                 SwExitInfo2;
  UINT64                 SwScratch;
  UINT8                  Reserved6[56];
  UINT64                 XCr0;
  UINT8                  ValidBitmap[16];
  UINT64                 X87StateGpa;
  UINT8                  Reserved7[1016];
} GHCB_SAVE_AREA;

typedef PACKED struct {
  GHCB_SAVE_AREA         SaveArea;
  UINT8                  SharedBuffer[2032];
  UINT8                  Reserved1[10];
  UINT16                 ProtocolVersion;
  UINT32                 GhcbUsage;
} GHCB;

typedef union {
  struct {
    UINT32  Lower32Bits;
    UINT32  Upper32Bits;
  } Elements;

  UINT64    Uint64;
} GHCB_EXIT_INFO;

typedef union {
  struct {
    UINT32  Vector:8;
    UINT32  Type:3;
    UINT32  ErrorCodeValid:1;
    UINT32  Rsvd:19;
    UINT32  Valid:1;
    UINT32  ErrorCode;
  } Elements;

  UINT64    Uint64;
} GHCB_EVENT_INJECTION;

#define GHCB_EVENT_INJECTION_TYPE_INT        0
#define GHCB_EVENT_INJECTION_TYPE_NMI        2
#define GHCB_EVENT_INJECTION_TYPE_EXCEPTION  3
#define GHCB_EVENT_INJECTION_TYPE_SOFT_INT   4

#endif
