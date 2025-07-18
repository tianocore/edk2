/** @file -- Tpm2DumpLib.h

  This lib contains helper functions to perform a detailed debugging of
  TPM transactions as they go to and from the TPM device.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/Tpm2DeviceLib.h>

#ifndef TPM2_DUMP_H_
#define TPM2_DUMP_H_

/**
  This function dumps as much information as possible about
  a command being sent to the TPM for maximum user-readability.
  @param[in]  InputBlockSize  Size of the input buffer.
  @param[in]  InputBlock      Pointer to the input buffer itself.
**/
VOID
EFIAPI
DumpTpmInputBlock (
  IN UINT32       InputBlockSize,
  IN CONST UINT8  *InputBlock
  );

/**
  This function dumps as much information as possible about
  a response from the TPM for maximum user-readability.
  @param[in]  OutputBlockSize  Size of the output buffer.
  @param[in]  OutputBlock      Pointer to the output buffer itself.
**/
VOID
EFIAPI
DumpTpmOutputBlock (
  IN UINT32       OutputBlockSize,
  IN CONST UINT8  *OutputBlock
  );

/**
  Dump PTP register information.

  @param[in] Register                Pointer to PTP register.
  @param[in] PtpInterface            Type of the PTP interface.
**/
VOID
EFIAPI
DumpPtpInfo (
  IN VOID                     *Register,
  IN TPM2_PTP_INTERFACE_TYPE  PtpInterface
  );

#endif // TPM2_DUMP_H_
