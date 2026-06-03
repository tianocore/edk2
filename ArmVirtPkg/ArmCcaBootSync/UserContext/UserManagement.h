/** @file
  User Data Management interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef USER_MANAGEMENT_H_
#define USER_MANAGEMENT_H_

/**
  Get the UEFI Variable Data.

  @param[in]   SecChannel          Pointer to the secure channel.
  @param[out]  Data                UEFI Variable data.
  @param[out]  DataSize            UEFI Variable data size.

  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncGetVariableData (
  IN  SECURE_CHANNEL  *SecChannel,
  OUT UINT8           **Data,
  OUT UINTN           *DataSize
  );

/**
  Get the Secret Data.

  @param[in]   SecChannel          Pointer to the secure channel.
  @param[out]  Data                Secret data.
  @param[out]  DataSize            Secret data size.

  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncGetSecretData (
  IN SECURE_CHANNEL  *SecChannel,
  OUT UINT8          **Data,
  OUT UINTN          *DataSize
  );

#endif // USER_MANAGEMENT_H_
