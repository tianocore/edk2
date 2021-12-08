/** @file
  This file abstract internal interfaces of which implementation differs per library instance.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/Tpm2DeviceLib.h>
#include <Library/PcdLib.h>

#include "Tpm2DeviceLibDTpm.h"

/**
  Return cached PTP CRB interface IdleByPass state.

  @return Cached PTP CRB interface IdleByPass state.
**/
UINT8
GetCachedIdleByPass (
  VOID
  )
{
  return PcdGet8 (PcdCRBIdleByPass);
}

/**
  Return cached PTP interface type.

  @return Cached PTP interface type.
**/
TPM2_PTP_INTERFACE_TYPE
GetCachedPtpInterface (
  VOID
  )
{
  return PcdGet8 (PcdActiveTpmInterfaceType);
}

/**
  The common function cache current active TpmInterfaceType when needed.

  @retval EFI_SUCCESS   DTPM2.0 instance is registered, or system does not support register DTPM2.0 instance
**/
EFI_STATUS
InternalTpm2DeviceLibDTpmCommonConstructor (
  VOID
  )
{
  TPM2_PTP_INTERFACE_TYPE  PtpInterface;
  UINT8                    IdleByPass;

  //
  // Cache current active TpmInterfaceType only when needed
  //
  if (PcdGet8 (PcdActiveTpmInterfaceType) == 0xFF) {
    PtpInterface = Tpm2GetPtpInterface ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));
    PcdSet8S (PcdActiveTpmInterfaceType, PtpInterface);
  }

  if ((PcdGet8 (PcdActiveTpmInterfaceType) == Tpm2PtpInterfaceCrb) && (PcdGet8 (PcdCRBIdleByPass) == 0xFF)) {
    IdleByPass = Tpm2GetIdleByPass ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));
    PcdSet8S (PcdCRBIdleByPass, IdleByPass);
  }

  return EFI_SUCCESS;
}
