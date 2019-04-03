/*++

Copyright (c)  1999  - 2015, Intel Corporation. All rights reserved
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent
                                                                                   

--*/

#include <Uefi.h>
#include <PiPei.h>
#include <Ppi/PttPassThruPpi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>






PTT_PASS_THRU_PPI  *SecPttPassThruPpi = NULL;

/**
  The constructor function caches the pointer to PEI services.

  The constructor function caches the pointer to PEI services.
  It will always return EFI_SUCCESS.

  @param  FfsHeader   Pointer to FFS header the loaded driver.
  @param  PeiServices Pointer to the PEI services.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
Tpm2DeviceLibConstructor (
  VOID
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  
  Status = PeiServicesLocatePpi (&gPttPassThruPpiGuid, 0, NULL, (VOID **) &SecPttPassThruPpi);
  if (EFI_ERROR (Status)) {
     // Locate the PPI failed
     SecPttPassThruPpi = NULL;
  }
  return Status;
}

/**
  This service enables the sending of commands to the TPM2.

  @param[in]  InputParameterBlockSize  Size of the TPM2 input parameter block.
  @param[in]  InputParameterBlock      Pointer to the TPM2 input parameter block.
  @param[in]  OutputParameterBlockSize Size of the TPM2 output parameter block.
  @param[in]  OutputParameterBlock     Pointer to the TPM2 output parameter block.

  @retval EFI_SUCCESS            The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR       The command was not successfully sent to the device or a response was not successfully received from the device.
  @retval EFI_BUFFER_TOO_SMALL   The output parameter block is too small.
**/
EFI_STATUS
EFIAPI
Tpm2SubmitCommand (
  IN UINT32            InputParameterBlockSize,
  IN UINT8             *InputParameterBlock,
  IN OUT UINT32        *OutputParameterBlockSize,
  IN UINT8             *OutputParameterBlock
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  if(NULL == InputParameterBlock || NULL == OutputParameterBlock || 0 == InputParameterBlockSize) {
    DEBUG ((EFI_D_ERROR, "Buffer == NULL or InputParameterBlockSize == 0\n"));
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  if (NULL == SecPttPassThruPpi) {
    // Don't locate PPI by calling Tpm2DeviceLibConstructor() function??
    Status = EFI_DEVICE_ERROR;
    return Status;
  }

  Status = SecPttPassThruPpi->Tpm2SubmitCommand (
             SecPttPassThruPpi, 
             InputParameterBlockSize, 
             InputParameterBlock, 
             OutputParameterBlockSize, 
             OutputParameterBlock
           );
  
  return Status;
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
  EFI_STATUS  Status = EFI_SUCCESS;

  if (NULL == SecPttPassThruPpi) {
    // Don't locate PPI by calling Tpm2DeviceLibConstructor() function??
    Status = EFI_DEVICE_ERROR;
    return Status;
  }

  Status = SecPttPassThruPpi->Tpm2RequestUseTpm (SecPttPassThruPpi);
  
  return Status;
}

/**
  This service register TPM2 device.

  @Param Tpm2Device  TPM2 device

  @retval EFI_SUCCESS          This TPM2 device is registered successfully.
  @retval EFI_UNSUPPORTED      System does not support register this TPM2 device.
  @retval EFI_ALREADY_STARTED  System already register this TPM2 device.
**/
EFI_STATUS
EFIAPI
Tpm2RegisterTpm2DeviceLib (
  IN PTT_TPM2_DEVICE_INTERFACE   *Tpm2Device
  )
{
  return EFI_UNSUPPORTED;
}


