/** @file
This library instance implements a very limited MMU Lib instance
for the ARM/AARCH64 architectures.  This library shims a common library
interface to the ArmPkg defined ArmMmuLib.ib.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmMmuLib.h>
#include <Library/DebugLib.h>
#include <Library/MmuLib.h>

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
    EFI_STATUS Status;

    Status = EFI_UNSUPPORTED;

    if ((Attributes & EFI_MEMORY_XP) == EFI_MEMORY_XP) {
      Status = ArmSetMemoryRegionNoExec (BaseAddress, Length);
      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a - Failed to set NX.  Status = %r\n", __FUNCTION__, Status));
      }
    }

    ASSERT_EFI_ERROR(Status);
    return Status;
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
    EFI_STATUS Status;

    Status = EFI_UNSUPPORTED;

    if ((Attributes & EFI_MEMORY_XP) == EFI_MEMORY_XP) {
      Status = ArmClearMemoryRegionNoExec (BaseAddress, Length);
      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a - Failed to clear NX.  Status = %r\n", __FUNCTION__, Status));
      }
    }

    if ((Attributes & EFI_MEMORY_RO) == EFI_MEMORY_RO) {
      Status = ArmClearMemoryRegionReadOnly(BaseAddress, Length);
      if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a - Failed to clear RO.  Status = %r\n", __FUNCTION__, Status));
      }
    }

    ASSERT_EFI_ERROR(Status);
    return Status;
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
    EFI_STATUS Status;

    Status = EFI_UNSUPPORTED;

    DEBUG ((DEBUG_ERROR, "%a() API not implemented\n", __FUNCTION__));

    ASSERT_EFI_ERROR(Status);
    return Status;
}
