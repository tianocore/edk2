/** @file
  This library will parse the coreboot table in memory and extract those required
  information.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
EFI_STATUS
BuildBlHobs (
  IN  UINTN                       Param1,
  IN  UINTN                       Param2,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  );

/**
  It will build HOBs based on information from bootloaders.

  @param[in]  Param1   Hard ID
  @param[in]  Param2   FDT blob pointer
  @param[out] DxeFv    The pointer to the DXE FV in memory.

  @retval EFI_SUCCESS        If it completed successfully.
**/
EFI_STATUS
BuildBlHobs (
  IN  UINTN                       Param1,
  IN  UINTN                       Param2,
  OUT EFI_FIRMWARE_VOLUME_HEADER  **DxeFv
  )
{
  return EFI_SUCCESS;
}
