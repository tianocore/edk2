/*++

Copyright (c)  2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  IScsiInitiatorName.c

Abstract:

  Implementation for EFI iSCSI Initiator Name Protocol.

--*/

#include "IScsiImpl.h"

EFI_ISCSI_INITIATOR_NAME_PROTOCOL gIScsiInitiatorName = {
  IScsiGetInitiatorName,
  IScsiSetInitiatorName
};

EFI_STATUS
EFIAPI
IScsiGetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  OUT    VOID                               *Buffer
  )
/*++

Routine Description:

  Retrieves the current set value of iSCSI Initiator Name. 

Arguments:

  This       - Pointer to the EFI_ISCSI_INITIATOR_NAME_PROTOCOL instance.
  BufferSize - Size of the buffer in bytes pointed to by Buffer / Actual size of
               the variable data buffer.
  Buffer     - Pointer to the buffer for data to be read.

Returns:

  EFI_SUCCESS           - Data was successfully retrieved into the provided 
                          buffer and the BufferSize was sufficient to handle the
                          iSCSI initiator name.
  EFI_BUFFER_TOO_SMALL  - BufferSize is too small for the result. BufferSize will
                          be updated with the size required to complete the request.
                          Buffer will not be affected.
  EFI_INVALID_PARAMETER - BufferSize is NULL. BufferSize and Buffer will not be
                          affected.
  EFI_INVALID_PARAMETER - Buffer is NULL. BufferSize and Buffer will not be
                          affected.
  EFI_DEVICE_ERROR      - The iSCSI initiator name could not be retrieved due to
                          a hardware error.

--*/
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

EFI_STATUS
EFIAPI
IScsiSetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  OUT    VOID                               *Buffer
  )
/*++

Routine Description:

  Sets the iSSI Initiator Name. 

Arguments:

  This       - Pointer to the EFI_ISCSI_INITIATOR_NAME_PROTOCOL instance.
  BufferSize - Size of the buffer in bytes pointed to by Buffer.
  Buffer     - Pointer to the buffer for data to be written.

Returns:

  EFI_SUCCESS           - Data was successfully stored by the protocol.
  EFI_UNSUPPORTED       - Platform policies do not allow for data to be written.
  EFI_INVALID_PARAMETER - BufferSize exceeds the maximum allowed limit.
                          BufferSize will be updated with the maximum size
                          required to complete the request.
  EFI_INVALID_PARAMETER - Buffersize is NULL. BufferSize and Buffer will not be
                          affected.
  EFI_INVALID_PARAMETER - Buffer is NULL. BufferSize and Buffer will not be affected.
  EFI_DEVICE_ERROR      - The data could not be stored due to a hardware error.
  EFI_OUT_OF_RESOURCES  - Not enough storage is available to hold the data
  EFI_PROTOCOL_ERROR    - Input iSCSI initiator name does not adhere to RFC 3720

--*/
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
  // only support iqn iSCSI names.
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
