/** @file
  Common header file for MP Initialize Library.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _MP_LIB_H_
#define _MP_LIB_H_

#include <PiPei.h>

#include <Register/Cpuid.h>
#include <Register/Msr.h>
#include <Register/LocalApic.h>
#include <Register/Microcode.h>

#include <Library/MpInitLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/LocalApicLib.h>
#include <Library/CpuLib.h>
#include <Library/UefiCpuLib.h>
#include <Library/TimerLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/MtrrLib.h>
#include <Library/HobLib.h>

//
// AP reset code information including code address and size,
// this structure will be shared be C code and assembly code.
// It is natural aligned by design.
//
typedef struct {
  UINT8             *RendezvousFunnelAddress;
  UINTN             ModeEntryOffset;
  UINTN             RendezvousFunnelSize;
  UINT8             *RelocateApLoopFuncAddress;
  UINTN             RelocateApLoopFuncSize;
} MP_ASSEMBLY_ADDRESS_MAP;

#pragma pack(1)

//
// MP CPU exchange information for AP reset code
// This structure is required to be packed because fixed field offsets
// into this structure are used in assembly code in this module
//
typedef struct {
  UINTN                 Lock;
  UINTN                 StackStart;
  UINTN                 StackSize;
  UINTN                 CFunction;
  IA32_DESCRIPTOR       GdtrProfile;
  IA32_DESCRIPTOR       IdtrProfile;
  UINTN                 BufferStart;
  UINTN                 ModeOffset;
  UINTN                 NumApsExecuting;
  UINTN                 CodeSegment;
  UINTN                 DataSegment;
  UINTN                 EnableExecuteDisable;
  UINTN                 Cr3;
} MP_CPU_EXCHANGE_INFO;

#pragma pack()
/**
  Assembly code to place AP into safe loop mode.

  Place AP into targeted C-State if MONITOR is supported, otherwise
  place AP into hlt state.
  Place AP in protected mode if the current is long mode. Due to AP maybe
  wakeup by some hardware event. It could avoid accessing page table that
  may not available during booting to OS.

  @param[in] MwaitSupport    TRUE indicates MONITOR is supported.
                             FALSE indicates MONITOR is not supported.
  @param[in] ApTargetCState  Target C-State value.
  @param[in] PmCodeSegment   Protected mode code segment value.
**/
typedef
VOID
(EFIAPI * ASM_RELOCATE_AP_LOOP) (
  IN BOOLEAN                 MwaitSupport,
  IN UINTN                   ApTargetCState,
  IN UINTN                   PmCodeSegment
  );

/**
  Assembly code to get starting address and size of the rendezvous entry for APs.
  Information for fixing a jump instruction in the code is also returned.

  @param[out] AddressMap  Output buffer for address map information.
**/
VOID
EFIAPI
AsmGetAddressMap (
  OUT MP_ASSEMBLY_ADDRESS_MAP    *AddressMap
  );

#endif

