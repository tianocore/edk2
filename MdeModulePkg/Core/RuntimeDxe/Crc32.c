/** @file
  This file implements CalculateCrc32 Boot Services as defined in
  Platform Initialization specification 1.0 VOLUME 2 DXE Core Interface.

  This Boot Services is in the Runtime Driver because this service is
  also required by SetVirtualAddressMap() when the EFI System Table and
  EFI Runtime Services Table are converted from physical address to
  virtual addresses.  This requires that the 32-bit CRC be recomputed.

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Uefi.h>
#include <Library/BaseLib.h>

/**
  Calculate CRC32 for target data.

  @param  Data                  The target data.
  @param  DataSize              The target data size.
  @param  CrcOut                The CRC32 for target data.

  @retval EFI_SUCCESS           The CRC32 for target data is calculated successfully.
  @retval EFI_INVALID_PARAMETER Some parameter is not valid, so the CRC32 is not
                                calculated.

**/
EFI_STATUS
EFIAPI
RuntimeDriverCalculateCrc32 (
  IN  VOID    *Data,
  IN  UINTN   DataSize,
  OUT UINT32  *CrcOut
  )
{
  if (Data == NULL || DataSize == 0 || CrcOut == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *CrcOut = CalculateCrc32 (Data, DataSize);
  return EFI_SUCCESS;
}
