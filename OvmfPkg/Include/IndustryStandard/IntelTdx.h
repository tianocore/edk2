/** @file
  Defines the defitions used by TDX in OvmfPkg.

  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef OVMF_INTEL_TDX_H_
#define OVMF_INTEL_TDX_H_

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Uefi/UefiSpec.h>
#include <Uefi/UefiBaseType.h>

#define MP_CPU_PROTECTED_MODE_MAILBOX_APICID_INVALID    0xFFFFFFFF
#define MP_CPU_PROTECTED_MODE_MAILBOX_APICID_BROADCAST  0xFFFFFFFE

typedef enum {
  MpProtectedModeWakeupCommandNoop        = 0,
  MpProtectedModeWakeupCommandWakeup      = 1,
  MpProtectedModeWakeupCommandSleep       = 2,
  MpProtectedModeWakeupCommandAcceptPages = 3,
} MP_CPU_PROTECTED_MODE_WAKEUP_CMD;

#pragma pack(1)

//
// Describes the CPU MAILBOX control structure use to
// wakeup cpus spinning in long mode
//
typedef struct {
  UINT16    Command;
  UINT16    Resv;
  UINT32    ApicId;
  UINT64    WakeUpVector;
  UINT8     ResvForOs[2032];
  //
  // Arguments available for wakeup code
  //
  UINT64    WakeUpArgs1;
  UINT64    WakeUpArgs2;
  UINT64    WakeUpArgs3;
  UINT64    WakeUpArgs4;
  UINT8     Pad1[0xe0];
  UINT64    NumCpusArriving;
  UINT8     Pad2[0xf8];
  UINT64    NumCpusExiting;
  UINT32    Tallies[256];
  UINT8     Errors[256];
  UINT8     Pad3[0xf8];
} MP_WAKEUP_MAILBOX;

//
// AP relocation code information including code address and size,
// this structure will be shared be C code and assembly code.
// It is natural aligned by design.
//
typedef struct {
  UINT8    *RelocateApLoopFuncAddress;
  UINTN    RelocateApLoopFuncSize;
} MP_RELOCATION_MAP;

#pragma pack()

#endif
