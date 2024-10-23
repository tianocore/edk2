/** @file
  Arm CCA Boot Sync Protocol interfaces.

  Copyright (c) 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include "Include/BootSyncSecureChannel.h"

/**
  Establish a secure channel for communication.

  @param[in]  SecChannel  Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
EstablishSecureChannel (
  IN SECURE_CHANNEL  *SecChannel
  );

/**
  Terminate the secure session.

  @param[in]    SecChannel  Pointer to the secure channel.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
TerminateSecureChannel (
  IN SECURE_CHANNEL  *SecChannel
  );

/**
  Send FIN request message.

  @param[in]  SecChannel    Pointer to the secure channel.
  @param[in]  Reason        Reason code for the FIN message.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SendFin (
  IN SECURE_CHANNEL  *SecChannel,
  IN UINT64          Reason
  );

/**
  Send a negetive acknowlegdement (NACK) message.

  @param[in]  SecChannel    Pointer to the secure channel.
  @param[in]  Reason        Reason code for the NACK message.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval EFI_SUCCESS             Success.
**/
EFI_STATUS
EFIAPI
SendNack (
  IN SECURE_CHANNEL  *SecChannel,
  IN UINT64          Reason
  );
