/** @file
  Implementation for EFI iSCSI Initiator Name Protocol.

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IScsiImpl.h"

EFI_ISCSI_INITIATOR_NAME_PROTOCOL gIScsiInitiatorName = {
  IScsiGetInitiatorName,
  IScsiSetInitiatorName
};


/**
  Retrieves the current set value of iSCSI Initiator Name.

  @param[in]       This          Pointer to the EFI_ISCSI_INITIATOR_NAME_PROTOCOL
                                 instance.
  @param[in, out]  BufferSize    Size of the buffer in bytes pointed to by Buffer /
                                 Actual size of the variable data buffer.
  @param[out]      Buffer        Pointer to the buffer for data to be read.
                                 The data is a null-terminated UTF-8 encoded string.
                                 The maximum length is 223 characters, including the null-terminator.

  @retval EFI_SUCCESS            Data was successfully retrieved into the provided
                                 buffer and the BufferSize was sufficient to handle
                                 the iSCSI initiator name.
  @retval EFI_BUFFER_TOO_SMALL   BufferSize is too small for the result. BufferSize
                                 will be updated with the size required to complete
                                 the request. Buffer will not be affected.
  @retval EFI_INVALID_PARAMETER  BufferSize is NULL. BufferSize and Buffer will not
                                 be affected.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL. BufferSize and Buffer will not be
                                 affected.
  @retval EFI_DEVICE_ERROR       The iSCSI initiator name could not be retrieved
                                 due to a hardware error.

**/
EFI_STATUS
EFIAPI
IScsiGetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  OUT    VOID                               *Buffer
  )
{
  EFI_STATUS  Status;

  if ((BufferSize == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gRT->GetVariable (
                  ISCSI_INITIATOR_NAME_VAR_NAME,
                  &gEfiIScsiInitiatorNameProtocolGuid,
                  NULL,
                  BufferSize,
                  Buffer
                  );

  return Status;
}


/**
  Sets the iSSI Initiator Name.

  @param[in]       This          Pointer to the EFI_ISCSI_INITIATOR_NAME_PROTOCOL
                                 instance.
  @param[in, out]  BufferSize    Size of the buffer in bytes pointed to by Buffer.
  @param[in]       Buffer        Pointer to the buffer for data to be written.
                                 The data is a null-terminated UTF-8 encoded string.
                                 The maximum length is 223 characters, including the null-terminator.

  @retval EFI_SUCCESS            Data was successfully stored by the protocol.
  @retval EFI_UNSUPPORTED        Platform policies do not allow for data to be
                                 written.
  @retval EFI_INVALID_PARAMETER  BufferSize exceeds the maximum allowed limit.
                                 BufferSize will be updated with the maximum size
                                 required to complete the request.
  @retval EFI_INVALID_PARAMETER  Buffersize is NULL. BufferSize and Buffer will not
                                 be affected.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL. BufferSize and Buffer will not be
                                 affected.
  @retval EFI_DEVICE_ERROR       The data could not be stored due to a hardware
                                 error.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the data
  @retval EFI_PROTOCOL_ERROR     Input iSCSI initiator name does not adhere to RFC
                                 3720

**/
EFI_STATUS
EFIAPI
IScsiSetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  IN     VOID                               *Buffer
  )
{
  EFI_STATUS  Status;

  if ((BufferSize == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*BufferSize > ISCSI_NAME_MAX_SIZE) {
    *BufferSize = ISCSI_NAME_MAX_SIZE;
    return EFI_INVALID_PARAMETER;
  }
  //
  // Only support iqn iSCSI names.
  //
  Status = IScsiNormalizeName ((CHAR8 *) Buffer, *BufferSize - 1);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
                  ISCSI_INITIATOR_NAME_VAR_NAME,
                  &gEfiIScsiInitiatorNameProtocolGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  *BufferSize,
                  Buffer
                  );

  return Status;
}
