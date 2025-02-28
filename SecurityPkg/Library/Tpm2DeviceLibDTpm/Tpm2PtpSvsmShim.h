/** @file
  PTP (Platform TPM Profile) CRB (Command Response Buffer) interface shim that switches between
  SVSM vTPM Ptp and regular Ptp implementations.

  Use TryUseSvsmVTpm () do check for SVSM vTPM presnece and initialzie the shim.

Copyright (c) 2024 Red Hat
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

BOOLEAN
EFIAPI
TryUseSvsmVTpm (
  );

EFI_STATUS
EFIAPI
SvsmDTpm2SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  );

EFI_STATUS
EFIAPI
SvsmDTpm2RequestUseTpm (
  VOID
  );
