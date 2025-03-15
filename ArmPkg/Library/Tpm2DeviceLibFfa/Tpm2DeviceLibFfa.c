/** @file
  This library provides an implementation of Tpm2DeviceLib
  using ARM64 SMC calls to request TPM service.

  The implementation is only supporting the Command Response Buffer (CRB)
  for sharing data with the TPM.

  Copyright (c), Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/ArmFfaSvc.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <IndustryStandard/ArmStdSmc.h>
#include <IndustryStandard/Tpm20.h>
#include <Library/TimerLib.h>

#include "Tpm2DeviceLibFfa.h"

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
  Send a command to TPM for execution and return response data.
  Used during boot only.

  @retval EFI_SUCCESS     Command was successfully sent to the TPM
                          and the response was copied to the Output buffer.
  @retval Other           Some error occurred in communication with the TPM.
**/
EFI_STATUS
EFIAPI
Tpm2SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  )
{
  return FfaTpm2SubmitCommand (
           InputParameterBlockSize,
           InputParameterBlock,
           OutputParameterBlockSize,
           OutputParameterBlock
           );
}

/**
  This service requests use TPM2.
  Since every communication with the TPM is blocking
  you are always good to start communicating with the TPM.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
**/
EFI_STATUS
EFIAPI
Tpm2RequestUseTpm (
  VOID
  )
{
  return FfaTpm2RequestUseTpm ();
}

/**
  This service register TPM2 device.

  @param Tpm2Device  TPM2 device

  @retval EFI_UNSUPPORTED      System does not support register this TPM2 device.
**/
EFI_STATUS
EFIAPI
Tpm2RegisterTpm2DeviceLib (
  IN TPM2_DEVICE_INTERFACE  *Tpm2Device
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Check the return status from the FF-A call and returns EFI_STATUS

  @param FfaReturnStatus  FF-A return status

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Others         Other corresponding EFI_STATUS.
**/
EFI_STATUS
TranslateFfaReturnStatus (
  UINTN  FfaReturnStatus
  )
{
  EFI_STATUS  Status;

  switch (FfaReturnStatus) {
    case ARM_FFA_RET_SUCCESS:
      Status = EFI_SUCCESS;
      break;

    case ARM_FFA_RET_NOT_SUPPORTED:
      Status = EFI_UNSUPPORTED;
      break;

    case ARM_FFA_RET_INVALID_PARAMETERS:
      Status = EFI_INVALID_PARAMETER;
      break;

    case ARM_FFA_RET_NO_MEMORY:
      Status = EFI_BUFFER_TOO_SMALL;
      break;

    case ARM_FFA_RET_BUSY:
      Status = EFI_WRITE_PROTECTED;
      break;

    case ARM_FFA_RET_INTERRUPTED:
      Status = EFI_MEDIA_CHANGED;
      break;

    case ARM_FFA_RET_DENIED:
      Status = EFI_ACCESS_DENIED;
      break;

    case ARM_FFA_RET_RETRY:
      Status = EFI_LOAD_ERROR;
      break;

    case ARM_FFA_RET_ABORTED:
      Status = EFI_ABORTED;
      break;

    case ARM_FFA_RET_NODATA:
      Status = EFI_NOT_FOUND;
      break;

    case ARM_FFA_RET_NOT_READY:
      Status = EFI_NOT_READY;
      break;

    default:
      Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  Check that we have an address for the CRB

  @retval EFI_SUCCESS      The entry point is executed successfully.
  @retval EFI_NOT_STARTED  The TPM base address is not set up.
  @retval EFI_UNSUPPORTED  The TPM interface type is not supported.
**/
EFI_STATUS
EFIAPI
Tpm2DeviceLibFfaConstructor (
  VOID
  )
{
  EFI_STATUS  Status;

  mActiveTpmInterfaceType = PcdGet8 (PcdActiveTpmInterfaceType);
  mCRBIdleByPass          = 0xFF;

  if (PcdGet64 (PcdTpmBaseAddress) == 0) {
    Status = EFI_NOT_STARTED;
    goto Exit;
  }

  //
  // Start by checking the PCD out of the gate and read from the CRB if it is invalid
  //
  if (mActiveTpmInterfaceType == 0xFF) {
    mActiveTpmInterfaceType = Tpm2GetPtpInterface ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));
    PcdSet8S (PcdActiveTpmInterfaceType, mActiveTpmInterfaceType);
  }

  if (mActiveTpmInterfaceType != Tpm2PtpInterfaceCrb) {
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  DEBUG ((DEBUG_INFO, "Setting Tpm Active Interface Type %d\n", mActiveTpmInterfaceType));
  mCRBIdleByPass = Tpm2GetIdleByPass ((VOID *)(UINTN)PcdGet64 (PcdTpmBaseAddress));

  Status = EFI_SUCCESS;

Exit:
  return Status;
}
