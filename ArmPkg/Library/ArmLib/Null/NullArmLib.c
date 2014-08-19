/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>

VOID
EFIAPI
ArmCleanInvalidateDataCache (
  VOID
  )
{
  // Do not run code using the Null cache library.
  ASSERT(FALSE);
}

VOID
EFIAPI
ArmCleanDataCache (
  VOID
  )
{
  // Do not run code using the Null cache library.
  ASSERT(FALSE);
}

VOID
EFIAPI
ArmInvalidateInstructionCache (
  VOID
  )
{
  // Do not run code using the Null cache library.
  ASSERT(FALSE);
}

VOID
EFIAPI
ArmInvalidateDataCacheEntryByMVA (
  IN  UINTN Address
  )
{
  // Do not run code using the Null cache library.
  ASSERT(FALSE);
}

VOID
EFIAPI
ArmCleanDataCacheEntryByMVA (
  IN  UINTN Address
  )
{
  // Do not run code using the Null cache library.
  ASSERT(FALSE);
}

VOID
EFIAPI
ArmCleanInvalidateDataCacheEntryByMVA (
  IN  UINTN Address
  )
{
  // Do not run code using the Null cache library.
  ASSERT(FALSE);
}

VOID
EFIAPI
ArmEnableDataCache (
  VOID
  )
{
  // Do not run code using the Null cache library.
  ASSERT(FALSE);
}

VOID
EFIAPI
ArmDisableDataCache (
  VOID
  )
{
  // Do not run code using the Null cache library.
  ASSERT(FALSE);
}

VOID
EFIAPI
ArmEnableInstructionCache (
  VOID
  )
{
  // Do not run code using the Null cache library.
  ASSERT(FALSE);
}

VOID
EFIAPI
ArmDisableInstructionCache (
  VOID
  )
{
  // Do not run code using the Null cache library.
  ASSERT(FALSE);
}
