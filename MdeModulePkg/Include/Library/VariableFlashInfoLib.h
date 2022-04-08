/** @file
  Variable Flash Information Library

Copyright (c) Microsoft Corporation<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef VARIABLE_FLASH_INFO_LIB_H_
#define VARIABLE_FLASH_INFO_LIB_H_

/**
  Get the base address and size for the NV storage area used for UEFI variable storage.

  @param[out] BaseAddress    The NV storage base address.
  @param[out] Length         The NV storage length in bytes.

  @retval EFI_SUCCESS             NV storage information was found successfully.
  @retval EFI_INVALID_PARAMETER   A required pointer parameter is NULL.
  @retval EFI_NOT_FOUND           NV storage information could not be found.

**/
EFI_STATUS
EFIAPI
GetVariableFlashNvStorageInfo (
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                *Length
  );

/**
  Get the base address and size for the fault tolerant write (FTW) spare
  area used for UEFI variable storage.

  @param[out] BaseAddress    The FTW spare base address.
  @param[out] Length         The FTW spare length in bytes.

  @retval EFI_SUCCESS             FTW spare information was found successfully.
  @retval EFI_INVALID_PARAMETER   A required pointer parameter is NULL.
  @retval EFI_NOT_FOUND           FTW spare information could not be found.

**/
EFI_STATUS
EFIAPI
GetVariableFlashFtwSpareInfo (
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                *Length
  );

/**
  Get the base address and size for the fault tolerant write (FTW) working
  area used for UEFI variable storage.

  @param[out] BaseAddress    The FTW working area base address.
  @param[out] Length         The FTW working area length in bytes.

  @retval EFI_SUCCESS             FTW working information was found successfully.
  @retval EFI_INVALID_PARAMETER   A required pointer parameter is NULL.
  @retval EFI_NOT_FOUND           FTW working information could not be found.

**/
EFI_STATUS
EFIAPI
GetVariableFlashFtwWorkingInfo (
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINT64                *Length
  );

#endif
