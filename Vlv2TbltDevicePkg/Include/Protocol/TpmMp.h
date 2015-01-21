/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  Tpm.h

Abstract:


--*/

#ifndef __EFI_TPM_MP_DRIVER_PROTOCOL_H__
#define __EFI_TPM_MP_DRIVER_PROTOCOL_H__


#define EFI_TPM_MP_DRIVER_PROTOCOL_GUID \
  { 0xde161cfe, 0x1e60, 0x42a1, 0x8c, 0xc3, 0xee, 0x7e, 0xf0, 0x73, 0x52, 0x12 }


EFI_FORWARD_DECLARATION (EFI_TPM_MP_DRIVER_PROTOCOL);

#define TPM_DRIVER_STATUS         0
#define TPM_DEVICE_STATUS         1

#define TPM_DRIVER_OK             0
#define TPM_DRIVER_FAILED         1
#define TPM_DRIVER_NOT_OPENED     2
#define TPM_DEVICE_OK             0
#define TPM_DEVICE_UNRECOVERABLE  1
#define TPM_DEVICE_RECOVERABLE    2
#define TPM_DEVICE_NOT_FOUND      3

//
// Prototypes for the TPM MP Driver Protocol
//

/**
  This service Open the TPM interface

  @param[in] This             A pointer to the EFI_TPM_MP_DRIVER_PROTOCOL.

  @retval  EFI_SUCCESS        Operation completed successfully
  @retval  EFI_DEVICE_ERROR   The command was unsuccessful
  @retval  EFI_NOT_FOUND      The component was not running

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TPM_MP_INIT) (
  IN EFI_TPM_MP_DRIVER_PROTOCOL   *This
  );

/**
  This service close the TPM interface and deactivate TPM

  @param[in] This            A pointer to the EFI_TPM_MP_DRIVER_PROTOCOL.

  @retval EFI_SUCCESS        Operation completed successfully
  @retval EFI_DEVICE_ERROR   The command was unsuccessful
  @retval EFI_NOT_FOUND      The component was not running

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TPM_MP_CLOSE) (
  IN EFI_TPM_MP_DRIVER_PROTOCOL   *This
  );

/**
  This service get the current status infomation of TPM

  @param[in]  This                  A pointer to the EFI_TPM_MP_DRIVER_PROTOCOL.
  @param[in]  ReqStatusType	        Requested type of status information, driver or device.
  @param[in]  Status	              Pointer to the returned status.

  @retval  EFI_SUCCESS              Operation completed successfully
  @retval  EFI_DEVICE_ERROR         The command was unsuccessful
  @retval  EFI_INVALID_PARAMETER    One or more of the parameters are incorrect
  @retval  EFI_BUFFER_TOO_SMALL     The receive buffer is too small
  @retval  EFI_NOT_FOUND            The component was not running

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TPM_MP_GET_STATUS_INFO) (
  IN EFI_TPM_MP_DRIVER_PROTOCOL   *This,
  IN UINT32				                ReqStatusType,
  OUT UINT32				              *Status
  );

/**
  This service transmit data to the TPM and get response from TPM

  @param[in] This                  A pointer to the EFI_TPM_MP_DRIVER_PROTOCOL.
  @param[in] TransmitBuf	         Pointer to a buffer containing TPM transmit data.
  @param[in] TransmitBufLen	       Sizeof TPM input buffer in bytes.
  @param[in] ReceiveBuf	           Pointer to a buffer containing TPM receive data.
  @param[in]  ReceiveBufLen	       On input, size of TPM receive buffer in bytes.
                                   On output, number of bytes written.

  @retval  EFI_SUCCESS             Operation completed successfully
  @retval  EFI_DEVICE_ERROR        The command was unsuccessful
  @retval  EFI_INVALID_PARAMETER   One or more of the parameters are incorrect
  @retval  EFI_BUFFER_TOO_SMALL    The receive buffer is too small
  @retval  EFI_NOT_FOUND           The component was not running

**/
typedef
EFI_STATUS
(EFIAPI *EFI_TPM_MP_TRANSMIT) (
  IN EFI_TPM_MP_DRIVER_PROTOCOL   *This,
  IN UINT8				  	            *TransmitBuffer,
  IN UINT32			  	              TransmitBufferLen,
  OUT UINT8				  	            *ReceiveBuf,
  IN OUT UINT32			  	          *ReceiveBufLen
  );



typedef struct _EFI_TPM_MP_DRIVER_PROTOCOL {
  EFI_TPM_MP_INIT			              Init;
  EFI_TPM_MP_CLOSE			            Close;
  EFI_TPM_MP_GET_STATUS_INFO 		    GetStatusInfo;
  EFI_TPM_MP_TRANSMIT		            Transmit;
} EFI_TPM_MP_DRIVER_PROTOCOL;

extern EFI_GUID gEfiTpmMpDriverProtocolGuid;

#endif
