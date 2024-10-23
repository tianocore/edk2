/** @file
  Arm CCA Boot Sync session interfaces for a User Context.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Uefi/UefiBaseType.h>

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "Include/BootSyncSecureChannel.h"
#include "Include/BootSyncSession.h"

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
  )
{
  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Nothing to do here.
  return EFI_SUCCESS;
}

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
  )
{
  INTN  RetVal;

  if ((SecChannel == NULL) || ((INT64)SecChannel->SessionId < 0)) {
    return EFI_INVALID_PARAMETER;
  }

  RetVal = shutdown (SecChannel->SessionId, SHUT_RDWR);
  if (RetVal != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to shutdown socket!, RetVal = 0x%x\n",
      RetVal
      ));
  }

  RetVal = close (SecChannel->SessionId);
  if (RetVal != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to close socket!, RetVal = 0x%x\n",
      RetVal
      ));
    return EFI_DEVICE_ERROR;
  }

  SecChannel->SessionId = MAX_UINT64;
  return EFI_SUCCESS;
}

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
  IN UINT8           *Data,
  IN UINTN           DataLen
  )
{
  INTN   Retval;
  UINTN  Offset;

  if ((SecChannel == NULL) ||
      ((INT64)SecChannel->SessionId < 0) ||
      (Data == NULL) ||
      (DataLen == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  Offset = 0;
  do {
    Retval = send (SecChannel->SessionId, &Data[Offset], DataLen, 0);
    if (Retval <= 0) {
      DEBUG ((DEBUG_ERROR, "ERROR writing to socket\n"));
      return EFI_ABORTED;
    }

    Offset  += Retval;
    DataLen -= Retval;
  } while (DataLen > 0);

  return EFI_SUCCESS;
}

/**
  Receive data over the Boot Sync session

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
  )
{
  INTN   Retval;
  UINTN  Offset;

  if ((SecChannel == NULL) ||
      ((INT64)SecChannel->SessionId < 0) ||
      (Data == NULL) ||
      (DataLen == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  Offset = 0;
  do {
    Retval = recv (SecChannel->SessionId, &Data[Offset], DataLen, MSG_WAITALL);
    if (Retval < 0) {
      DEBUG ((DEBUG_ERROR, "ERROR reading from socket\n"));
      return EFI_DEVICE_ERROR;
    } else if (Retval == 0) {
      DEBUG ((DEBUG_ERROR, "ERROR socket disconnected\n"));
      return EFI_ABORTED;
    }

    Offset  += Retval;
    DataLen -= Retval;
  } while (DataLen > 0);

  return EFI_SUCCESS;
}
