/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2014, ARM Limited. All rights reserved.
  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

#include <Library/ArmLib.h>
#include <Library/DebugLib.h>

#include <Arm/AArch32.h>

#include "ArmV7Lib.h"
#include "ArmLibPrivate.h"

/**
  Check whether the CPU supports the GIC system register interface (any version)

  @return   Whether GIC System Register Interface is supported

**/
BOOLEAN
EFIAPI
ArmHasGicSystemRegisters (
  VOID
  )
{
  return ((ArmReadIdPfr1 () & ARM_PFR1_GIC) != 0);
}

/**
  Check whether the CPU supports the Security extensions

  @return   Whether the Security extensions are implemented

**/
BOOLEAN
EFIAPI
ArmHasSecurityExtensions (
  VOID
  )
{
  return ((ArmReadIdPfr1 () & ARM_PFR1_SEC) != 0);
}

/** Checks if CCIDX is implemented.

   @retval TRUE  CCIDX is implemented.
   @retval FALSE CCIDX is not implemented.
**/
BOOLEAN
EFIAPI
ArmHasCcidx (
  VOID
  )
{
  UINTN  Mmfr4;

  Mmfr4 = ArmReadIdMmfr4 ();
  return (((Mmfr4 >> 24) & 0xF) == 1) ? TRUE : FALSE;
}
