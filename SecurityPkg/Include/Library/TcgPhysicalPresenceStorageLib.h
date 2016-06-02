/** @file
  This library is to support TCG PC Client Platform Physical Presence Interface Specification
  Family, >= 96 && <128 storage Specific PPI Operation.
  
  Caution: This function may receive untrusted input.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TCG_PHYSICAL_PRESENCE_STORAGE_LIB_H_
#define _TCG_PHYSICAL_PRESENCE_STORAGE_LIB_H_

//
// UEFI TCG2 library definition bit of the BIOS Storage Management Flags
//
#define TCG_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_ENABLE_BLOCK_SID   BIT1
#define TCG_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_DISABLE_BLOCK_SID  BIT2
#define TCG_BIOS_STORAGE_MANAGEMENT_FLAG_ENABLE_BLOCK_SID                   BIT3

//
// Default value
//
#define TCG_BIOS_STORAGE_MANAGEMENT_FLAG_DEFAULT  (TCG_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_ENABLE_BLOCK_SID |\
                                                   TCG_BIOS_STORAGE_MANAGEMENT_FLAG_PP_REQUIRED_FOR_DISABLE_BLOCK_SID)

/**
  Check and execute the pending TPM request.

  The TPM request may come from OS or BIOS. This API will display request information and wait 
  for user confirmation if TPM request exists. The TPM request will be sent to TPM device after
  the TPM request is confirmed, and one or more reset may be required to make TPM request to 
  take effect.
  
  This API should be invoked after console in and console out are all ready as they are required
  to display request information and get user input to confirm the request.  

  @param[in]  PlatformAuth                   platform auth value. NULL means no platform auth change.
**/
VOID
EFIAPI
TcgPhysicalPresenceStorageLibProcessRequest (
  VOID
  );

/**
  Check if the pending TPM request needs user input to confirm.

  The TPM request may come from OS. This API will check if TPM request exists and need user
  input to confirmation.
  
  @retval    TRUE        TPM needs input to confirm user physical presence.
  @retval    FALSE       TPM doesn't need input to confirm user physical presence.

**/
BOOLEAN
EFIAPI
TcgPhysicalPresenceStorageLibNeedUserConfirm(
  VOID
  );

/**
  The handler for TPM physical presence function:
  Submit TPM Operation Request to Pre-OS Environment and
  Submit TPM Operation Request to Pre-OS Environment 2.

  Caution: This function may receive untrusted input.
  
  @param[in]      OperationRequest TPM physical presence operation request.
  @param[in]      RequestParameter TPM physical presence operation request parameter.

  @return Return Code for Submit TPM Operation Request to Pre-OS Environment and
          Submit TPM Operation Request to Pre-OS Environment 2.
**/
UINT32
EFIAPI
TcgPhysicalPresenceStorageLibSubmitRequestToPreOSFunction (
  IN UINT32                 OperationRequest,
  IN UINT32                 RequestParameter
  );

/**
  The handler for TPM physical presence function:
  Return TPM Operation Response to OS Environment.

  This API should be invoked in OS runtime phase to interface with ACPI method.

  @param[out]     MostRecentRequest Most recent operation request.
  @param[out]     Response          Response to the most recent operation request.

  @return Return Code for Return TPM Operation Response to OS Environment.
**/
UINT32
EFIAPI
TcgPhysicalPresenceStorageLibReturnOperationResponseToOsFunction (
  OUT UINT32                *MostRecentRequest,
  OUT UINT32                *Response
  );

/**
  The handler for TPM physical presence function:
  Return TPM Operation flag variable.

  @return Return Code for Return TPM Operation flag variable.
**/
UINT32
EFIAPI
TcgPhysicalPresenceStorageLibReturnStorageFlags (
  VOID
  );

/**

  Install string package.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  Install string package success.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
TcgPhysicalPresenceStorageLibConstructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  );

/**
  Unloads the library and its installed protocol.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.
  @param[in]  SystemTable       System Table

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
TcgPhysicalPresenceStorageLibDestructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  );

#endif
