/** @file
  This file abstract internal interfaces of which implementation differs per library instance.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/Tpm2DeviceLib.h>

#include "Tpm2DeviceLibDTpm.h"

TPM2_PTP_INTERFACE_TYPE  mActiveTpmInterfaceType;
UINT8                    mCRBIdleByPass;

/**
  Return cached PTP CRB interface IdleByPass state.

  @return Cached PTP CRB interface IdleByPass state.
**/
UINT8
GetCachedIdleByPass (
  VOID
  )
{
  return mCRBIdleByPass;
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
  return mActiveTpmInterfaceType;
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
  mActiveTpmInterfaceType = 0xFF;
  mCRBIdleByPass          = 0xFF;

  //
  // Always cache current active TpmInterfaceType for StandaloneMm implementation
  //
  mActiveTpmInterfaceType = Tpm2GetPtpInterface ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));

  if (mActiveTpmInterfaceType == Tpm2PtpInterfaceCrb) {
    mCRBIdleByPass = Tpm2GetIdleByPass ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));
  }

  return EFI_SUCCESS;
}
