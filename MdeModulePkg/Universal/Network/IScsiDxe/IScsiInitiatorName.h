/** @file
  The header file for EFI iSCSI Initiator Name Protocol.

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISCSI_INITIATOR_NAME_H_
#define _ISCSI_INITIATOR_NAME_H_

#include <Protocol/IScsiInitiatorName.h>

extern EFI_ISCSI_INITIATOR_NAME_PROTOCOL  gIScsiInitiatorName;

//
// EFI iSCSI Initiator Name Protocol for IScsi driver.
//

/**
  Retrieves the current set value of iSCSI Initiator Name.

  @param[in]       This       Pointer to the EFI_ISCSI_INITIATOR_NAME_PROTOCOL instance.
  @param[in, out]  BufferSize Size of the buffer in bytes pointed to by Buffer / Actual size of the
                              variable data buffer.
  @param[out]      Buffer     Pointer to the buffer for data to be read.

  @retval EFI_SUCCESS           Data was successfully retrieved into the provided buffer and the
                                BufferSize was sufficient to handle the iSCSI initiator name.
  @retval EFI_BUFFER_TOO_SMALL  BufferSize is too small for the result.
  @retval EFI_INVALID_PARAMETER BufferSize or Buffer is NULL.
  @retval EFI_DEVICE_ERROR      The iSCSI initiator name could not be retrieved due to a hardware error.
  @retval Others                Other errors as indicated.
**/
EFI_STATUS
EFIAPI
IScsiGetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  OUT    VOID                               *Buffer
  );

/**
  Sets the iSCSI Initiator Name.

  @param[in]       This       Pointer to the EFI_ISCSI_INITIATOR_NAME_PROTOCOL instance.
  @param[in, out]  BufferSize Size of the buffer in bytes pointed to by Buffer.
  @param[in]       Buffer     Pointer to the buffer for data to be written.

  @retval EFI_SUCCESS           Data was successfully stored by the protocol.
  @retval EFI_UNSUPPORTED       Platform policies do not allow for data to be written.
                                Currently not implemented.
  @retval EFI_INVALID_PARAMETER BufferSize or Buffer is NULL, or BufferSize exceeds the maximum allowed limit.
  @retval EFI_DEVICE_ERROR      The data could not be stored due to a hardware error.
  @retval EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the data.
  @retval EFI_PROTOCOL_ERROR    Input iSCSI initiator name does not adhere to RFC 3720
                                (and other related protocols).
  @retval Others                Other errors as indicated.
**/
EFI_STATUS
EFIAPI
IScsiSetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  IN     VOID                               *Buffer
  );

#endif
