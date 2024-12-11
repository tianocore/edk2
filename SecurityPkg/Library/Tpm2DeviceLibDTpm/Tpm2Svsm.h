/** @file
  SVSM TPM communication

Copyright (C) 2024 James.Bottomley@HansenPartnership.com
Copyright (C) 2024 IBM Corporation
Copyright (C) 2024 Red Hat

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>

BOOLEAN
Tpm2SvsmQueryTpmSendCmd (
  VOID
  );

EFI_STATUS
Tpm2SvsmTpmSendCommand (
  IN     UINT8   *BufferIn,
  IN     UINT32  SizeIn,
  OUT UINT8      *BufferOut,
  IN OUT UINT32  *SizeOut
  );
