/** @file
  Guest-Hypervisor Communication Block (GHCB) Definition.

  Provides data types allowing an SEV-ES guest to interact with the hypervisor
  using the GHCB protocol.

  Copyright (C) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
  SEV-ES Guest-Hypervisor Communication Block Standardization

**/

#ifndef __GHCB_H__
#define __GHCB_H__

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#define UD_EXCEPTION  6
#define GP_EXCEPTION 13
#define VC_EXCEPTION 29

#define GHCB_VERSION_MIN     1
#define GHCB_VERSION_MAX     1

#define GHCB_STANDARD_USAGE  0

//
// SVM Exit Codes
//
#define SVM_EXIT_DR7_READ       0x27ULL
#define SVM_EXIT_DR7_WRITE      0x37ULL
#define SVM_EXIT_RDTSC          0x6EULL
#define SVM_EXIT_RDPMC          0x6FULL
#define SVM_EXIT_CPUID          0x72ULL
#define SVM_EXIT_INVD           0x76ULL
#define SVM_EXIT_IOIO_PROT      0x7BULL
#define SVM_EXIT_MSR            0x7CULL
#define SVM_EXIT_VMMCALL        0x81ULL
#define SVM_EXIT_RDTSCP         0x87ULL
#define SVM_EXIT_WBINVD         0x89ULL
#define SVM_EXIT_MONITOR        0x8AULL
#define SVM_EXIT_MWAIT          0x8BULL
#define SVM_EXIT_NPF            0x400ULL

//
// VMG Special Exit Codes
//
#define SVM_EXIT_MMIO_READ      0x80000001ULL
#define SVM_EXIT_MMIO_WRITE     0x80000002ULL
#define SVM_EXIT_NMI_COMPLETE   0x80000003ULL
#define SVM_EXIT_AP_RESET_HOLD  0x80000004ULL
#define SVM_EXIT_AP_JUMP_TABLE  0x80000005ULL
#define SVM_EXIT_UNSUPPORTED    0x8000FFFFULL

//
// IOIO Exit Information
//
#define IOIO_TYPE_STR       BIT2
#define IOIO_TYPE_IN        1
#define IOIO_TYPE_INS       (IOIO_TYPE_IN | IOIO_TYPE_STR)
#define IOIO_TYPE_OUT       0
#define IOIO_TYPE_OUTS      (IOIO_TYPE_OUT | IOIO_TYPE_STR)

#define IOIO_REP            BIT3

#define IOIO_ADDR_64        BIT9
#define IOIO_ADDR_32        BIT8
#define IOIO_ADDR_16        BIT7

#define IOIO_DATA_32        BIT6
#define IOIO_DATA_16        BIT5
#define IOIO_DATA_8         BIT4
#define IOIO_DATA_MASK      (BIT6 | BIT5 | BIT4)
#define IOIO_DATA_OFFSET    4
#define IOIO_DATA_BYTES(x)  (((x) & IOIO_DATA_MASK) >> IOIO_DATA_OFFSET)

#define IOIO_SEG_ES         0
#define IOIO_SEG_DS         (BIT11 | BIT10)


typedef PACKED struct {
  UINT8                  Reserved1[203];
  UINT8                  Cpl;
  UINT8                  Reserved8[300];
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

#define GHCB_SAVE_AREA_QWORD_OFFSET(RegisterField) \
  (OFFSET_OF (GHCB, SaveArea.RegisterField) / sizeof (UINT64))

typedef enum {
  GhcbCpl          = GHCB_SAVE_AREA_QWORD_OFFSET (Cpl),
  GhcbRax          = GHCB_SAVE_AREA_QWORD_OFFSET (Rax),
  GhcbRbx          = GHCB_SAVE_AREA_QWORD_OFFSET (Rbx),
  GhcbRcx          = GHCB_SAVE_AREA_QWORD_OFFSET (Rcx),
  GhcbRdx          = GHCB_SAVE_AREA_QWORD_OFFSET (Rdx),
  GhcbXCr0         = GHCB_SAVE_AREA_QWORD_OFFSET (XCr0),
  GhcbSwExitCode   = GHCB_SAVE_AREA_QWORD_OFFSET (SwExitCode),
  GhcbSwExitInfo1  = GHCB_SAVE_AREA_QWORD_OFFSET (SwExitInfo1),
  GhcbSwExitInfo2  = GHCB_SAVE_AREA_QWORD_OFFSET (SwExitInfo2),
  GhcbSwScratch    = GHCB_SAVE_AREA_QWORD_OFFSET (SwScratch),
} GHCB_REGISTER;

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
