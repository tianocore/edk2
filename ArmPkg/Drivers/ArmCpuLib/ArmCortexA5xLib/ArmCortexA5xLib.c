/** @file

  Copyright (c) 2011-2014, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/ArmCpuLib.h>
#include <Library/ArmGenericTimerCounterLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <Chipset/ArmCortexA5x.h>

VOID
ArmCpuSetup (
  IN  UINTN         MpId
  )
{
  // Check if Architectural Timer frequency is valid number (should not be 0)
  ASSERT (PcdGet32 (PcdArmArchTimerFreqInHz));
  ASSERT (ArmIsArchTimerImplemented () != 0);

  // Note: System Counter frequency can only be set in Secure privileged mode,
  // if security extensions are implemented.
  ArmGenericTimerSetTimerFreq (PcdGet32 (PcdArmArchTimerFreqInHz));

  if (ArmIsMpCore ()) {
    // Turn on SMP coherency
    ArmSetCpuExCrBit (A5X_FEATURE_SMP);
  }

  //
  // If CPU is CortexA57 r0p0 apply Errata: 806969
  //
  if ((ArmReadMidr () & ((ARM_CPU_TYPE_MASK << 4) | ARM_CPU_REV_MASK)) ==
                         ((ARM_CPU_TYPE_A57 << 4) | ARM_CPU_REV(0,0))) {
    // DisableLoadStoreWB
    ArmSetCpuActlrBit (1ULL << 49);
  }
}

VOID
ArmCpuSetupSmpNonSecure (
  IN  UINTN         MpId
  )
{
}

VOID
EFIAPI
ArmSetCpuExCrBit (
  IN  UINT64    Bits
  )
{
  UINT64 Value;
  Value =  ArmReadCpuExCr ();
  Value |= Bits;
  ArmWriteCpuExCr (Value);
}

VOID
EFIAPI
ArmUnsetCpuExCrBit (
  IN  UINT64    Bits
  )
{
  UINT64 Value;
  Value = ArmReadCpuExCr ();
  Value &= ~Bits;
  ArmWriteCpuExCr (Value);
}
