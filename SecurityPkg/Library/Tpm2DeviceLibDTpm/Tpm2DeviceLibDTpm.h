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
  The common function to cache the currently active TpmInterfaceType when needed.

  @param[out] PtpInterface  Pointer to store the cached PTP interface type.

  @retval EFI_SUCCESS   DTPM2.0 instance is registered or the system does not support registering a DTPM2.0 instance
**/
EFI_STATUS
InternalTpm2DeviceLibDTpmCommonConstructor (
  TPM2_PTP_INTERFACE_TYPE  *PtpInterface
  );

#endif // _TPM2_DEVICE_LIB_DTPM_H_
