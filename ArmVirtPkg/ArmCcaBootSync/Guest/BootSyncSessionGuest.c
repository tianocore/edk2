/** @file
  Arm CCA Boot Sync session interfaces for the guest firmware.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmCcaRhiHostSessionLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/RealmApertureManagementProtocol.h>

#include "Include/BootSyncSecureChannel.h"
#include "Include/BootSyncSession.h"

/** Pointer to the Realm Aperture Management Protocol
*/
STATIC EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL  *mRamp = NULL;

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
  EFI_STATUS                 Status;
  UINT64                     ProtocolVersion;
  UINT64                     AbiSupportBitmap;
  UINT64                     ConnectionModeBitmap;
  ARM_CCA_RHI_SESSION_STATE  SessionState;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Step 1.a: Version
  if (mRamp == NULL) {
    Status = gBS->LocateProtocol (
                    &gEfiRealmApertureManagementProtocolGuid,
                    NULL,
                    (VOID **)&mRamp
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Error: Failed to get Realm Aperture Management protocol."
        " Status = %r\n",
        Status
        ));
      ASSERT (FALSE);
      return Status;
    }
  }

  // Step 1.b: Version
  Status = ArmCcaRhiSessionVersion (
             &ProtocolVersion
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to get RHI Host Session protocol version. Status = %r\n",
      Status
      ));
    ASSERT (FALSE);
    return Status;
  }

  // Step 1.c: Features
  Status = ArmCcaRhiSessionFeatures (
             &AbiSupportBitmap,
             &ConnectionModeBitmap
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Failed to get RHI Host Session protocol features. Status = %r\n",
      Status
      ));
    ASSERT (FALSE);
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "RHI ProtocolVersion = 0x%llx\n",
    ProtocolVersion
    ));

  DEBUG ((
    DEBUG_INFO,
    "RHI AbiSupportBitmap = 0x%llx\n",
    AbiSupportBitmap
    ));

  DEBUG ((
    DEBUG_INFO,
    "RHI ConnectionModeBitmap = 0x%llx\n",
    ConnectionModeBitmap
    ));

  // Step 2: Check that mandatory RHI Session ABIs are supported
  if ((AbiSupportBitmap & ARM_CCA_RHI_SESSION_MANDATORY_ABIS) !=
      ARM_CCA_RHI_SESSION_MANDATORY_ABIS)
  {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Mandatory RHI Host Session ABIs not supported.\n"
      ));
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  // Step 3: Select Connection Mode to use
  if ((ConnectionModeBitmap & ARM_CCA_RHI_SESSION_CONNECTION_MODE_BLOCKING) ==
      ARM_CCA_RHI_SESSION_CONNECTION_MODE_BLOCKING)
  {
    // Use blocking mode
    SecChannel->ConnectionMode = ARM_CCA_RHI_SESSION_CONNECTION_MODE_BLOCKING;
  } else if ((ConnectionModeBitmap &
              ARM_CCA_RHI_SESSION_CONNECTION_MODE_NON_BLOCKING) ==
             ARM_CCA_RHI_SESSION_CONNECTION_MODE_NON_BLOCKING)
  {
    // Use non bocking mode
    SecChannel->ConnectionMode = ARM_CCA_RHI_SESSION_CONNECTION_MODE_NON_BLOCKING;
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "Error: Connection mode not supported, 0x%lx.\n",
      SecChannel->ConnectionMode
      ));
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  do {
    Status = ArmCcaRhiSessionOpen (
               SecChannel->ConnectionMode,
               &SecChannel->SessionId,
               &SessionState
               );
  } while ((!EFI_ERROR (Status)) &&
           (SessionState != RhiSessionStateConnectionEstablished));

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to open RHI Session. Status = %r\n", Status));
    ASSERT (FALSE);
  } else {
    DEBUG ((
      DEBUG_INFO,
      "Info: RHI Session Opened. SessionId = %llx\n",
      SecChannel->SessionId
      ));
  }

  return Status;
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
  EFI_STATUS                 Status;
  ARM_CCA_RHI_SESSION_STATE  SessionState;

  if (SecChannel == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  do {
    Status = ArmCcaRhiSessionClose (
               SecChannel->SessionId,
               &SessionState
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Failed to close RHI Session. Status = %r\n",
        Status
        ));
      ASSERT (FALSE);
      break;
    }
  } while (SessionState == RhiSessionStateConnectionCloseInProgress);

  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "Info: RHI Session Closed. SessionId = %llx\n",
      SecChannel->SessionId
      ));
  }

  return Status;
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
  UINT8              *Data,
  UINTN              DataLen
  )
{
  EFI_STATUS                 Status;
  EFI_STATUS                 Status1;
  UINTN                      Pages;
  UINT8                      *Buffer;
  UINTN                      BytesToSend;
  UINTN                      Offset;
  ARM_CCA_RHI_SESSION_STATE  SessionState;
  EFI_HANDLE                 ApertureRef;

  if ((SecChannel == NULL) ||
      (Data == NULL) ||
      (DataLen == 0) ||
      (mRamp == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Pages  = EFI_SIZE_TO_PAGES (DataLen);
  Buffer = AllocateAlignedPages (
             Pages,
             ARM_CCA_REALM_GRANULE_SIZE
             );
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Open aperture here
  Status = mRamp->OpenAperture (
                    (EFI_PHYSICAL_ADDRESS)Buffer,
                    Pages,
                    &ApertureRef
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RSI Set IPA State failed, Address = %p, Pages = 0x%lx, "
      "Status = %r\n",
      Buffer,
      Pages,
      Status
      ));
    goto error_handler;
  }

  // Copy the data to transmit to the aligned buffer.
  CopyMem (Buffer, Data, DataLen);
  if (EFI_PAGES_TO_SIZE (Pages) > DataLen) {
    // Zero pad the remaining buffer
    ZeroMem ((Buffer + DataLen), (EFI_PAGES_TO_SIZE (Pages) - DataLen));
  }

  BytesToSend = DataLen;
  Offset      = 0;
  do {
    Status = ArmCcaRhiSessionSend (
               SecChannel->SessionId,
               Buffer,
               &BytesToSend,
               Offset,
               &SessionState
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Failed to send data over RHI Session. SessionId = %llx, Status = %r\n",
        SecChannel->SessionId,
        Status
        ));
      ASSERT (FALSE);
      break;
    }

    // NOTE: We do not change the IPA. Changing the IPA will
    // cancel the previous IoInProgress transaction.
    Offset     += BytesToSend;
    BytesToSend = DataLen - Offset;
  } while ((BytesToSend > 0) &&
           (SessionState == RhiSessionStateIoInProgress));

  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "Info: RHI Session Data send success. SessionId = %llx,  Status = %r\n",
      SecChannel->SessionId,
      Status
      ));
  }

  // Close the aperture
  Status1 = mRamp->CloseAperture (ApertureRef, FALSE);
  if (EFI_ERROR (Status1)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to close aperture. Status = %r\n",
      Status1
      ));

    // if ArmCcaRhiSessionSend() did not fail but CloseAperture() failed, then
    // return the error code from CloseAperture(), otherwise return the first
    // error.
    if (!EFI_ERROR (Status)) {
      Status = Status1;
    }
  }

error_handler:
  FreeAlignedPages (Buffer, Pages);

  return Status;
}

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
  )
{
  EFI_STATUS                 Status;
  EFI_STATUS                 Status1;
  UINTN                      Pages;
  UINT8                      *Buffer;
  UINTN                      BytesToRecv;
  UINTN                      Offset;
  ARM_CCA_RHI_SESSION_STATE  SessionState;
  EFI_HANDLE                 ApertureRef;

  if ((SecChannel == NULL) ||
      (Data == NULL) ||
      (DataLen == 0) ||
      (mRamp == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "To Receive DataLen = %d\n", DataLen));
  Pages  = EFI_SIZE_TO_PAGES (DataLen);
  Buffer = AllocateAlignedPages (
             Pages,
             ARM_CCA_REALM_GRANULE_SIZE
             );
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Open aperture here
  Status = mRamp->OpenAperture (
                    (EFI_PHYSICAL_ADDRESS)Buffer,
                    Pages,
                    &ApertureRef
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: RSI Set IPA State failed, Address = %p, Pages = 0x%lx, "
      "Status = %r\n",
      Buffer,
      Pages,
      Status
      ));
    goto error_handler;
  }

  ZeroMem (Buffer, EFI_PAGES_TO_SIZE (Pages));

  BytesToRecv = DataLen;
  Offset      = 0;
  do {
    Status = ArmCcaRhiSessionReceive (
               SecChannel->SessionId,
               Buffer,
               &BytesToRecv,
               Offset,
               &SessionState
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "Failed to recv data over RHI Session. SessionId = %llx, Status = %r\n",
        SecChannel->SessionId,
        Status
        ));
      ASSERT (FALSE);
      break;
    }

    // NOTE: We do not change the IPA. Changing the IPA will
    // cancel the previous IoInProgress transaction.
    Offset     += BytesToRecv;
    BytesToRecv = DataLen - Offset;
  } while ((BytesToRecv > 0) &&
           (SessionState == RhiSessionStateIoInProgress));

  if (!EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "Info: RHI Session Data recv success. SessionId = %llx,  Status = %r\n",
      SecChannel->SessionId,
      Status
      ));
    // Copy the received data.
    CopyMem (Data, Buffer, DataLen);
  }

  // Close the aperture
  Status1 = mRamp->CloseAperture (ApertureRef, FALSE);
  if (EFI_ERROR (Status1)) {
    DEBUG ((
      DEBUG_ERROR,
      "Failed to close aperture. Status = %r\n",
      Status1
      ));

    // if ArmCcaRhiSessionSend() did not fail but CloseAperture() failed, then
    // return the error code from CloseAperture(), otherwise return the first
    // error.
    if (!EFI_ERROR (Status)) {
      Status = Status1;
    }
  }

error_handler:
  FreeAlignedPages (Buffer, Pages);

  return Status;
}
