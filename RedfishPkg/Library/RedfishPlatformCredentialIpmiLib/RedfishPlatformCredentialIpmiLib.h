/** @file
  Header file for RedfishPlatformCredentialIpmiLib.

  Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REDFISH_PLATFORM_CREDENTIAL_IPMI_LIB_H_
#define REDFISH_PLATFORM_CREDENTIAL_IPMI_LIB_H_

#include <Uefi.h>
#include <IndustryStandard/Ipmi.h>
#include <IndustryStandard/RedfishHostInterfaceIpmi.h>

#include <Protocol/EdkIIRedfishCredential.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/RedfishCredentialLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#define CREDENTIAL_VARIABLE_NAME  L"Partstooblaitnederc"

///
/// The bootstrap credential keeping in UEFI variable
///
typedef struct {
  CHAR8    Username[USERNAME_MAX_SIZE];
  CHAR8    Password[PASSWORD_MAX_SIZE];
} BOOTSTRAP_CREDENTIALS_VARIABLE;

/**
  Function to retrieve temporary user credentials for the UEFI redfish client. This function can
  also disable bootstrap credential service in BMC.

  @param[in]     DisableBootstrapControl TRUE - Tell the BMC to disable the bootstrap credential
                                                service to ensure no one else gains credentials
                                         FALSE  Allow the bootstrap credential service to continue
  @param[in,out] BootstrapUsername       A pointer to a Ascii encoded string for the credential username
                                         When DisableBootstrapControl is TRUE, this pointer can be NULL
  @param[in]     BootstrapUsernameSize   The size of BootstrapUsername including NULL terminator in bytes.
                                         Per specification, the size is USERNAME_MAX_SIZE.
  @param[in,out] BootstrapPassword       A pointer to a Ascii encoded string for the credential password
                                         When DisableBootstrapControl is TRUE, this pointer can be NULL
  @param[in]     BootstrapPasswordSize   The size of BootstrapPassword including NULL terminator in bytes.
                                         Per specification, the size is PASSWORD_MAX_SIZE.

  @retval  EFI_SUCCESS                Credentials were successfully fetched and returned. When DisableBootstrapControl
                                      is set to TRUE, the bootstrap credential service is disabled successfully.
  @retval  EFI_INVALID_PARAMETER      BootstrapUsername or BootstrapPassword is NULL when DisableBootstrapControl
                                      is set to FALSE. BootstrapUsernameSize or BootstrapPasswordSize is incorrect when
                                      DisableBootstrapControl is set to FALSE.
  @retval  EFI_DEVICE_ERROR           An IPMI failure occurred
**/
EFI_STATUS
GetBootstrapAccountCredentials (
  IN     BOOLEAN DisableBootstrapControl,
  IN OUT CHAR8 *BootstrapUsername, OPTIONAL
  IN     UINTN   BootstrapUsernameSize,
  IN OUT CHAR8   *BootstrapPassword, OPTIONAL
  IN     UINTN   BootstrapPasswordSize
  );

/**
  Function to save temporary user credentials into boot time variable. When DeleteVariable is True,
  this function delete boot time variable.

  @param[in] BootstrapUsername       A pointer to a Ascii encoded string for the credential username.
  @param[in] BootstrapPassword       A pointer to a Ascii encoded string for the credential password.
  @param[in] DeleteVariable          True to remove boot time variable. False otherwise.

  @retval  EFI_SUCCESS                Credentials were successfully saved.
  @retval  EFI_INVALID_PARAMETER      BootstrapUsername or BootstrapPassword is NULL
  @retval  Others                     Error occurs
**/
EFI_STATUS
SetBootstrapAccountCredentialsToVariable (
  IN CHAR8 *BootstrapUsername, OPTIONAL
  IN CHAR8  *BootstrapPassword, OPTIONAL
  IN BOOLEAN DeleteVariable
  );

#endif
