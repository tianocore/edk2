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

#endif // _TPM2_DEVICE_LIB_DTPM_H_
