/** @file
  PTP (Platform TPM Profile) CRB (Command Response Buffer) interface used by DTPM2.0 library.

Copyright (c) 2024 Red Hat
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#ifndef TPM2_PTP_H_
#define TPM2_PTP_H_

/**
  Dump PTP register information.

  @param[in] Register                Pointer to PTP register.
**/
VOID
DumpPtpInfo (
  IN VOID  *Register
  );

/**
  This service enables the sending of commands to the TPM2.

  @param[in]      InputParameterBlockSize  Size of the TPM2 input parameter block.
  @param[in]      InputParameterBlock      Pointer to the TPM2 input parameter block.
  @param[in,out]  OutputParameterBlockSize Size of the TPM2 output parameter block.
  @param[in]      OutputParameterBlock     Pointer to the TPM2 output parameter block.

  @retval EFI_SUCCESS            The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR       The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_BUFFER_TOO_SMALL   The output parameter block is too small.
**/
EFI_STATUS
EFIAPI
DTpm2SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  );

/**
  This service requests use TPM2.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
DTpm2RequestUseTpm (
  VOID
  );

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
  @param[in]  CommandCode      Command code for the input block.
**/
VOID
EFIAPI
DumpTpmOutputBlock (
  IN UINT32       OutputBlockSize,
  IN CONST UINT8  *OutputBlock,
  IN UINT32       CommandCode
  );

#endif // TPM2_PTP_H_
