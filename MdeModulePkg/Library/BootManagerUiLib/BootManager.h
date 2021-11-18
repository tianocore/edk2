/** @file
  The boot manager reference implementation

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_BOOT_MANAGER_H_
#define _EFI_BOOT_MANAGER_H_

#include <Guid/MdeModuleHii.h>
#include <Guid/GlobalVariable.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/FormBrowserEx2.h>

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootManagerLib.h>

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;
#pragma pack()

//
// These are defined as the same with vfr file
//
#define BOOT_MANAGER_FORMSET_GUID \
  { \
  0x847bc3fe, 0xb974, 0x446d, {0x94, 0x49, 0x5a, 0xd5, 0x41, 0x2e, 0x99, 0x3b} \
  }

#define BOOT_MANAGER_FORM_ID  0x1000

#define LABEL_BOOT_OPTION      0x00
#define LABEL_BOOT_OPTION_END  0x01
#define MAX_STRING_LEN         200

//
// Variable created with this flag will be "Efi:...."
//
#define VAR_FLAG  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE

//
// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8  BootManagerVfrBin[];

#define BOOT_MANAGER_CALLBACK_DATA_SIGNATURE  SIGNATURE_32 ('B', 'M', 'C', 'B')

typedef struct {
  UINTN                             Signature;

  //
  // HII relative handles
  //
  EFI_HII_HANDLE                    HiiHandle;
  EFI_HANDLE                        DriverHandle;

  //
  // Produced protocols
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
} BOOT_MANAGER_CALLBACK_DATA;

/**
  This call back function is registered with Boot Manager formset.
  When user selects a boot option, this call back function will
  be triggered. The boot option is saved for later processing.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
BootManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         - A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        - On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         - A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is NULL, illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
BootManagerExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  );

/**
  This function processes the results of changes in configuration.


  @param This            - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   - A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        - A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
BootManagerRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  );

#endif
