/** @file
  This library is TPM2 device router. Platform can register multi TPM2 instance to it
  via PcdTpmInstanceGuid. Platform need make choice that which one will be final one.
  At most one TPM2 instance can be finally registered, and other will return unsupported.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/Tpm2DeviceLib.h>

EFI_GUID  mInternalTpm2DeviceInterfaceGuid = {
  0x349cf818, 0xc0ba, 0x4c43, { 0x92, 0x9a, 0xc8, 0xa1, 0xb1, 0xb3, 0xd2, 0x55 }
};

/**
  This function get TPM2.0 interface.

  @retval TPM2.0 interface.
**/
TPM2_DEVICE_INTERFACE *
InternalGetTpm2DeviceInterface (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *Hob;

  Hob = GetFirstGuidHob (&mInternalTpm2DeviceInterfaceGuid);
  if (Hob == NULL) {
    return NULL;
  }

  return (TPM2_DEVICE_INTERFACE *)(Hob + 1);
}

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
Tpm2SubmitCommand (
  IN UINT32      InputParameterBlockSize,
  IN UINT8       *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN UINT8       *OutputParameterBlock
  )
{
  TPM2_DEVICE_INTERFACE  *Tpm2DeviceInterface;

  Tpm2DeviceInterface = InternalGetTpm2DeviceInterface ();
  if (Tpm2DeviceInterface == NULL) {
    return EFI_UNSUPPORTED;
  }

  return Tpm2DeviceInterface->Tpm2SubmitCommand (
                                InputParameterBlockSize,
                                InputParameterBlock,
                                OutputParameterBlockSize,
                                OutputParameterBlock
                                );
}

/**
  This service requests use TPM2.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2RequestUseTpm (
  VOID
  )
{
  TPM2_DEVICE_INTERFACE  *Tpm2DeviceInterface;

  Tpm2DeviceInterface = InternalGetTpm2DeviceInterface ();
  if (Tpm2DeviceInterface == NULL) {
    return EFI_UNSUPPORTED;
  }

  return Tpm2DeviceInterface->Tpm2RequestUseTpm ();
}

/**
  This service register TPM2 device.

  @param Tpm2Device  TPM2 device

  @retval EFI_SUCCESS          This TPM2 device is registered successfully.
  @retval EFI_UNSUPPORTED      System does not support register this TPM2 device.
  @retval EFI_ALREADY_STARTED  System already register this TPM2 device.
**/
EFI_STATUS
EFIAPI
Tpm2RegisterTpm2DeviceLib (
  IN TPM2_DEVICE_INTERFACE  *Tpm2Device
  )
{
  TPM2_DEVICE_INTERFACE  *Tpm2DeviceInterface;

  if (!CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &Tpm2Device->ProviderGuid)) {
    DEBUG ((DEBUG_WARN, "WARNING: Tpm2RegisterTpm2DeviceLib - does not support %g registration\n", &Tpm2Device->ProviderGuid));
    return EFI_UNSUPPORTED;
  }

  Tpm2DeviceInterface = InternalGetTpm2DeviceInterface ();
  if (Tpm2DeviceInterface != NULL) {
    //
    // In PEI phase, there will be shadow driver dispatched again.
    //
    DEBUG ((DEBUG_INFO, "Tpm2RegisterTpm2DeviceLib - Override\n"));
    CopyMem (Tpm2DeviceInterface, Tpm2Device, sizeof (*Tpm2Device));
    return EFI_SUCCESS;
  } else {
    Tpm2Device = BuildGuidDataHob (&mInternalTpm2DeviceInterfaceGuid, Tpm2Device, sizeof (*Tpm2Device));
    if (Tpm2Device != NULL) {
      return EFI_SUCCESS;
    } else {
      return EFI_OUT_OF_RESOURCES;
    }
  }
}
