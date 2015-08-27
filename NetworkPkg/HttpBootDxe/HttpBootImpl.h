/** @file
  The declaration of UEFI HTTP boot function.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef __EFI_HTTP_BOOT_IMPL_H__
#define __EFI_HTTP_BOOT_IMPL_H__

/**
  Attempt to complete a DHCPv4 D.O.R.A sequence to retrieve the boot resource information.

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              Boot info was successfully retrieved.
  @retval EFI_INVALID_PARAMETER    Private is NULL.
  @retval EFI_NOT_STARTED          The driver is in stopped state.
  @retval EFI_DEVICE_ERROR         An unexpected network error occurred.
  @retval Others                   Other errors as indicated.
  
**/
EFI_STATUS
HttpBootDhcp (
  IN HTTP_BOOT_PRIVATE_DATA           *Private
  );

/**
  Disable the use of UEFI HTTP boot function.

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              HTTP boot was successfully disabled.
  @retval EFI_NOT_STARTED          The driver is already in stopped state.
  @retval EFI_INVALID_PARAMETER    Private is NULL.
  @retval Others                   Unexpected error when stop the function.
  
**/
EFI_STATUS
HttpBootStop (
  IN HTTP_BOOT_PRIVATE_DATA           *Private
  );

#endif
