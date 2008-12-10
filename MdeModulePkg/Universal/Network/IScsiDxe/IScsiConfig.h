/** @file
  The header file of IScsiConfig.c.

Copyright (c) 2004 - 2008, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISCSI_CONFIG_H_
#define _ISCSI_CONFIG_H_

#include <Library/HiiLib.h>
#include <Library/ExtendedHiiLib.h>
#include <Library/IfrSupportLib.h>
#include <Library/ExtendedIfrSupportLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/NetLib.h>

extern UINT8  IScsiConfigDxeBin[];
extern UINT8  IScsiDxeStrings[];

#define ISCSI_INITATOR_NAME_VAR_NAME        L"I_NAME"

#define ISCSI_CONFIG_VAR_ATTR               (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE)

#define ISCSI_FORM_CALLBACK_INFO_SIGNATURE  EFI_SIGNATURE_32 ('I', 'f', 'c', 'i')

#define ISCSI_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK(Callback) \
  CR ( \
  Callback, \
  ISCSI_FORM_CALLBACK_INFO, \
  ConfigAccess, \
  ISCSI_FORM_CALLBACK_INFO_SIGNATURE \
  )

#pragma pack(1)

typedef struct _ISCSI_MAC_INFO {
  EFI_MAC_ADDRESS Mac;
  UINT8           Len;
} ISCSI_MAC_INFO;

typedef struct _ISCSI_DEVICE_LIST {
  UINT8           NumDevice;
  ISCSI_MAC_INFO  MacInfo[1];
} ISCSI_DEVICE_LIST;

#pragma pack()

typedef struct _ISCSI_CONFIG_FORM_ENTRY {
  LIST_ENTRY                    Link;
  EFI_HANDLE                    Controller;
  CHAR16                        MacString[95];
  EFI_STRING_ID                 PortTitleToken;
  EFI_STRING_ID                 PortTitleHelpToken;

  ISCSI_SESSION_CONFIG_NVDATA   SessionConfigData;
  ISCSI_CHAP_AUTH_CONFIG_NVDATA AuthConfigData;
} ISCSI_CONFIG_FORM_ENTRY;

typedef struct _ISCSI_FORM_CALLBACK_INFO {
  UINTN                            Signature;
  EFI_HANDLE                       DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;
  EFI_HII_DATABASE_PROTOCOL        *HiiDatabase;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *ConfigRouting;
  UINT16                           *KeyList;
  VOID                             *FormBuffer;
  EFI_HII_HANDLE                   RegisteredHandle;
  ISCSI_CONFIG_FORM_ENTRY          *Current;
} ISCSI_FORM_CALLBACK_INFO;

/**
  Updates the iSCSI configuration form to add/delete an entry for the iSCSI
  device specified by the Controller.

  @param[in]  DriverBindingHandle The driverbinding handle.
  @param[in]  Controller          The controller handle of the iSCSI device.
  @param[in]  AddForm             Whether to add or delete a form entry.

  @retval EFI_SUCCESS             The iSCSI configuration form is updated.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
  @retval Others                  Some unexpected errors happened.
**/
EFI_STATUS
IScsiConfigUpdateForm (
  IN EFI_HANDLE  DriverBindingHandle,
  IN EFI_HANDLE  Controller,
  IN BOOLEAN     AddForm
  );

/**
  Initialize the iSCSI configuration form.

  @param[in]  DriverBindingHandle  The iSCSI driverbinding handle.

  @retval EFI_SUCCESS              The iSCSI configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES     Failed to allocate memory.
  @retval Others                   Some unexpected error happened.
**/
EFI_STATUS
IScsiConfigFormInit (
  IN EFI_HANDLE  DriverBindingHandle
  );

/**
  Unload the iSCSI configuration form, this includes: delete all the iSCSI
  device configuration entries, uninstall the form callback protocol and
  free the resources used.

  @param[in]  DriverBindingHandle The iSCSI driverbinding handle.
  
  @retval EFI_SUCCESS             The iSCSI configuration form is unloaded.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
IScsiConfigFormUnload (
  IN EFI_HANDLE  DriverBindingHandle
  );

#endif
