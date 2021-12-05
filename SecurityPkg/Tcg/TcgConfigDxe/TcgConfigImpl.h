/** @file
  The header file of HII Config Access protocol implementation of TCG
  configuration module.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TCG_CONFIG_IMPL_H__
#define __TCG_CONFIG_IMPL_H__

#include <Uefi.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/TcgService.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/Tpm12DeviceLib.h>

#include <Guid/MdeModuleHii.h>

#include "TcgConfigNvData.h"

//
// Tool generated IFR binary data and String package data
//
extern UINT8  TcgConfigBin[];
extern UINT8  TcgConfigDxeStrings[];

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

typedef struct {
  UINTN                             Signature;

  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  EFI_HII_HANDLE                    HiiHandle;
  EFI_HANDLE                        DriverHandle;

  TCG_CONFIGURATION                 *Configuration;
  EFI_TCG_PROTOCOL                  *TcgProtocol;
} TCG_CONFIG_PRIVATE_DATA;

extern TCG_CONFIG_PRIVATE_DATA  mTcgConfigPrivateDateTemplate;

#define TCG_CONFIG_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('T', 'C', 'G', 'D')
#define TCG_CONFIG_PRIVATE_DATA_FROM_THIS(a)  CR (a, TCG_CONFIG_PRIVATE_DATA, ConfigAccess, TCG_CONFIG_PRIVATE_DATA_SIGNATURE)

/**
  This function publish the TCG configuration Form for TPM device.

  @param[in, out]  PrivateData   Points to TCG configuration private data.

  @retval EFI_SUCCESS            HII Form is installed for this network device.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource for HII Form installation.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
InstallTcgConfigForm (
  IN OUT TCG_CONFIG_PRIVATE_DATA  *PrivateData
  );

/**
  This function removes TCG configuration Form.

  @param[in, out]  PrivateData   Points to TCG configuration private data.

**/
VOID
UninstallTcgConfigForm (
  IN OUT TCG_CONFIG_PRIVATE_DATA  *PrivateData
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param[in]   This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]   Request           A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param[out]  Progress          On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param[out]  Results           A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
TcgExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Request,
  OUT EFI_STRING                           *Progress,
  OUT EFI_STRING                           *Results
  );

/**
  This function processes the results of changes in configuration.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration      A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param[out] Progress           A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
TcgRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                           *Progress
  );

/**
  This function processes the results of changes in configuration.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action             Specifies the type of action taken by the browser.
  @param[in]  QuestionId         A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param[in]  Type               The type of value for the question.
  @param[in]  Value              A pointer to the data being sent to the original
                                 exporting driver.
  @param[out] ActionRequest      On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
TcgCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN     EFI_BROWSER_ACTION                Action,
  IN     EFI_QUESTION_ID                   QuestionId,
  IN     UINT8                             Type,
  IN     EFI_IFR_TYPE_VALUE                *Value,
  OUT EFI_BROWSER_ACTION_REQUEST           *ActionRequest
  );

#endif
