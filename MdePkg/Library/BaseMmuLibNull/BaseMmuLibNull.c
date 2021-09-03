/** @file
This lib abstracts some of the MMU accesses currently hardcoded against
an Arm lib. It's likely that this will need to be refactored at some point.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>

/**
  Bitwise sets the memory attributes on a range of memory based on an attributes mask.

  @param  BaseAddress           The start of the range for which to set attributes.
  @param  Length                The length of the range.
  @param  Attributes            A bitmask of the attributes to set. See "Physical memory
                                protection attributes" in UefiSpec.h

  @return EFI_SUCCESS
  @return Others

**/
EFI_STATUS
EFIAPI
MmuSetAttributes (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length,
  IN  UINT64                    Attributes
  )
{
  DEBUG ((DEBUG_ERROR, "%a() NULL implementation used!\n", __FUNCTION__));
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}


/**
  Bitwise clears the memory attributes on a range of memory based on an attributes mask.

  @param  BaseAddress           The start of the range for which to clear attributes.
  @param  Length                The length of the range.
  @param  Attributes            A bitmask of the attributes to clear. See "Physical memory
                                protection attributes" in UefiSpec.h

  @return EFI_SUCCESS
  @return Others

**/
EFI_STATUS
EFIAPI
MmuClearAttributes (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length,
  IN  UINT64                    Attributes
  )
{
  DEBUG ((DEBUG_ERROR, "%a() NULL implementation used!\n", __FUNCTION__));
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}


/**
  Returns the memory attributes on a range of memory.

  @param  BaseAddress           The start of the range for which to set attributes.
  @param  Attributes            A return pointer for the attributes.

  @return EFI_SUCCESS
  @return EFI_INVALID_PARAMETER   A return pointer is NULL.
  @return Others

**/
EFI_STATUS
EFIAPI
MmuGetAttributes (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  OUT UINT64                    *Attributes
  )
{
  DEBUG ((DEBUG_ERROR, "%a() NULL implementation used!\n", __FUNCTION__));
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}
