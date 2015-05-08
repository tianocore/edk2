/*++

Copyright (c)  1999  - 2015, Intel Corporation. All rights reserved
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   
--*/

#include <Uefi.h>
#include <Protocol/PttPassThru.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
//#include <Library/Tpm2DeviceLib.h>


PTT_PASS_THRU_PROTOCOL *mPttPassThruProtocol;


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
  EFI_STATUS Status = EFI_SUCCESS;
  
  Status = gBS->LocateProtocol (&gPttPassThruProtocolGuid, NULL, (VOID **) &mPttPassThruProtocol);
  
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
  EFI_STATUS Status;
  
  Status = mPttPassThruProtocol->Tpm2SubmitCommand (
             mPttPassThruProtocol,
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
  EFI_STATUS Status;
  
  Status = mPttPassThruProtocol->Tpm2RequestUseTpm (mPttPassThruProtocol);
           
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


