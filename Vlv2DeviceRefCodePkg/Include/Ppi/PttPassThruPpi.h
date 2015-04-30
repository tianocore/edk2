/*++

  Copyright (c) 2004  - 2015, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   
--*/

#ifndef _EFI_PTT_PASS_THRU_PPI_H
#define _EFI_PTT_PASS_THRU_PPI_H

#define PTT_PASS_THRU_PPI_GUID \
  { \
    0xc5068bac, 0xa7dc, 0x42f1, 0xae, 0x80, 0xca, 0xa2, 0x4b, 0xb4, 0x90, 0x4b   \
  }
// {C5068BAC-A7DC-42f1-AE80-CAA24BB4904B}
//static const GUID <<name>> = 
//{ 0xc5068bac, 0xa7dc, 0x42f1, { 0xae, 0x80, 0xca, 0xa2, 0x4b, 0xb4, 0x90, 0x4b } };



//#define EFI_PTT_PROTOCOL_GUID  HECI_PROTOCOL_GUID

typedef struct _PTT_PASS_THRU_PPI PTT_PASS_THRU_PPI;

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
typedef
EFI_STATUS
(EFIAPI *TPM2_SUBMIT_COMMAND) (
  IN PTT_PASS_THRU_PPI *This,
  IN UINT32                  InputParameterBlockSize,
  IN UINT8                   *InputParameterBlock,
  IN OUT UINT32              *OutputParameterBlockSize,
  IN UINT8                   *OutputParameterBlock
  );

/**
  This service requests use TPM2.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
typedef
EFI_STATUS
(EFIAPI *TPM2_REQUEST_USE_TPM) (
  IN PTT_PASS_THRU_PPI *This
  );

typedef struct {
  EFI_GUID                           ProviderGuid;
  TPM2_SUBMIT_COMMAND                Tpm2SubmitCommand;
  TPM2_REQUEST_USE_TPM               Tpm2RequestUseTpm;
} PTT_TPM2_DEVICE_INTERFACE;


/**
  This service register TPM2 device.

  @param Tpm2Device  TPM2 device

  @retval EFI_SUCCESS          This TPM2 device is registered successfully.
  @retval EFI_UNSUPPORTED      System does not support register this TPM2 device.
  @retval EFI_ALREADY_STARTED  System already register this TPM2 device.
**/
typedef
EFI_STATUS
(EFIAPI *TPM2_REGISTER_TPM2_DEVICE_LIB) (
  IN PTT_PASS_THRU_PPI  *This,
  IN PTT_TPM2_DEVICE_INTERFACE   *Tpm2Device
  );
  
typedef struct _PTT_PASS_THRU_PPI {
  TPM2_SUBMIT_COMMAND             Tpm2SubmitCommand;
  TPM2_REQUEST_USE_TPM            Tpm2RequestUseTpm;
  TPM2_REGISTER_TPM2_DEVICE_LIB   Tpm2RegisterTpm2DeviceLib;
} PTT_PASS_THRU_PPI;

extern EFI_GUID gPttPassThruPpiGuid;

#endif // _EFI_HECI_H
