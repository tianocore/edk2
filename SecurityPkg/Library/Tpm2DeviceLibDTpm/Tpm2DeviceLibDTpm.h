/** @file
  This header file includes common internal fuction prototypes.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TPM2_DEVICE_LIB_DTPM_H_
#define _TPM2_DEVICE_LIB_DTPM_H_

/**
  Return PTP interface type.

  @param[in] Register                Pointer to PTP register.

  @return PTP interface type.
**/
TPM2_PTP_INTERFACE_TYPE
Tpm2GetPtpInterface (
  IN VOID  *Register
  );

/**
  Return PTP CRB interface IdleByPass state.

  @param[in] Register                Pointer to PTP register.

  @return PTP CRB interface IdleByPass state.
**/
UINT8
Tpm2GetIdleByPass (
  IN VOID  *Register
  );

/**
  Return cached PTP interface type.

  @return Cached PTP interface type.
**/
TPM2_PTP_INTERFACE_TYPE
GetCachedPtpInterface (
  VOID
  );

/**
  Return cached PTP CRB interface IdleByPass state.

  @return Cached PTP CRB interface IdleByPass state.
**/
UINT8
GetCachedIdleByPass (
  VOID
  );

/**
  The common function cache current active TpmInterfaceType when needed.

  @retval EFI_SUCCESS   DTPM2.0 instance is registered, or system does not support register DTPM2.0 instance
**/
EFI_STATUS
InternalTpm2DeviceLibDTpmCommonConstructor (
  VOID
  );

/**
  Probe the SVSM vTPM for TPM_SEND_COMMAND support. The
  TPM_SEND_COMMAND platform command can be used to execute a
  TPM command and get the result.

  @retval TRUE    TPM_SEND_COMMAND is supported.
  @retval FALSE   TPM_SEND_COMMAND is not supported.

**/
BOOLEAN
Tpm2SvsmQueryTpmSendCmd (
  VOID
  );

/**
  Send a TPM command to the SVSM vTPM and return the TPM response.

  @param[in]      BufferIn      It should contain the marshaled
                                TPM command.
  @param[in]      SizeIn        Size of the TPM command.
  @param[out]     BufferOut     It will contain the marshaled
                                TPM response.
  @param[in, out] SizeOut       Size of the BufferOut; it will also
                                be used to return the size of the
                                TPM response

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER Buffer not provided.
  @retval EFI_BUFFER_TOO_SMALL  Response data buffer is too small.
  @retval EFI_DEVICE_ERROR      Unexpected device behavior.
  @retval EFI_UNSUPPORTED       Unsupported TPM version

**/
EFI_STATUS
Tpm2SvsmTpmSendCommand (
  IN     UINT8   *BufferIn,
  IN     UINT32  SizeIn,
     OUT UINT8   *BufferOut,
  IN OUT UINT32  *SizeOut
  );

#endif // _TPM2_DEVICE_LIB_DTPM_H_
