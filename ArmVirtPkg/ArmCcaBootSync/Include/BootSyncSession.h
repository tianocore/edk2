/** @file
  Arm CCA Boot Sync session interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmCcaRhiHostSessionLib.h>

/**
  Initialise and open a Boot Sync session for communication.

  @param[in]  SecChannel  Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_UNSUPPORTED         Connection mode not supported.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncSessionOpen (
  IN SECURE_CHANNEL  *SecChannel
  );

/**
  Close a Boot Sync session.

  @param[in]  SecChannel  Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncSessionClose (
  IN SECURE_CHANNEL  *SecChannel
  );

/**
  Send data over the Boot Sync session.

  @param[in]  SecChannel  Pointer to the secure channel.
  @param[in]  Data        Pointer to the buffer to send.
  @param[in]  DataLen     Length of data to send.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncSessionSendData (
  IN SECURE_CHANNEL  *SecChannel,
  UINT8              *Data,
  UINTN              DataLen
  );

/**
  Receive data over the Boot Sync session.

  @param[in]        SecChannel  Pointer to the secure channel.
  @param[out]       Data        Pointer to the buffer to receive the data.
  @param[in]        DataLen     Length of data to receive.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Out of memory.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
BootSyncSessionReceiveData (
  IN  SECURE_CHANNEL  *SecChannel,
  OUT UINT8           *Data,
  IN  UINTN           DataLen
  );
