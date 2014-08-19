/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/ArmLib.h>
#include "ArmLibPrivate.h"

ARM_CACHE_TYPE
EFIAPI
ArmCacheType (
  VOID
  )
{
  return ARM_CACHE_TYPE_UNKNOWN;
}

ARM_CACHE_ARCHITECTURE
EFIAPI
ArmCacheArchitecture (
  VOID
  )
{
  return ARM_CACHE_ARCHITECTURE_UNKNOWN;
}

BOOLEAN
EFIAPI
ArmDataCachePresent (
  VOID
  )
{
  return FALSE;
}

UINTN
EFIAPI
ArmDataCacheSize (
  VOID
  )
{
  return 0;
}

UINTN
EFIAPI
ArmDataCacheAssociativity (
  VOID
  )
{
  return 0;
}

UINTN
EFIAPI
ArmDataCacheLineLength (
  VOID
  )
{
  return 0;
}

BOOLEAN
EFIAPI
ArmInstructionCachePresent (
  VOID
  )
{
  return FALSE;
}

UINTN
EFIAPI
ArmInstructionCacheSize (
  VOID
  )
{
  return 0;
}

UINTN
EFIAPI
ArmInstructionCacheAssociativity (
  VOID
  )
{
  return 0;
}

UINTN
EFIAPI
ArmInstructionCacheLineLength (
  VOID
  )
{
  return 0;
}
