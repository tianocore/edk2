/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include "ArmLibPrivate.h"

VOID
EFIAPI
ArmSetAuxCrBit (
  IN  UINT32    Bits
  )
{
  UINT32 val = ArmReadAuxCr();
  val |= Bits;
  ArmWriteAuxCr(val);
}

VOID
EFIAPI
ArmUnsetAuxCrBit (
  IN  UINT32    Bits
  )
{
  UINT32 val = ArmReadAuxCr();
  val &= ~Bits;
  ArmWriteAuxCr(val);
}

//
// Helper functions for accessing CPUACTLR
//

VOID
EFIAPI
ArmSetCpuActlrBit (
  IN  UINTN    Bits
  )
{
  UINTN Value;
  Value =  ArmReadCpuActlr ();
  Value |= Bits;
  ArmWriteCpuActlr (Value);
}

VOID
EFIAPI
ArmUnsetCpuActlrBit (
  IN  UINTN    Bits
  )
{
  UINTN Value;
  Value = ArmReadCpuActlr ();
  Value &= ~Bits;
  ArmWriteCpuActlr (Value);
}

UINTN
EFIAPI
ArmDataCacheLineLength (
  VOID
  )
{
  return 4 << ((ArmCacheInfo () >> 16) & 0xf); // CTR_EL0.DminLine
}

UINTN
EFIAPI
ArmInstructionCacheLineLength (
  VOID
  )
{
  return 4 << (ArmCacheInfo () & 0xf); // CTR_EL0.IminLine
}

UINTN
EFIAPI
ArmCacheWritebackGranule (
  VOID
  )
{
  UINTN   CWG;

  CWG = (ArmCacheInfo () >> 24) & 0xf; // CTR_EL0.CWG

  if (CWG == 0) {
    return SIZE_2KB;
  }

  return 4 << CWG;
}
