/** @file
  The header file of functions for configuring or getting the parameters
  relating to iSCSI.

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISCSI_CONFIG_H_
#define _ISCSI_CONFIG_H_

#include "IScsiConfigNVDataStruc.h"

typedef struct _ISCSI_FORM_CALLBACK_INFO ISCSI_FORM_CALLBACK_INFO;

extern UINT8                       IScsiConfigVfrBin[];
extern UINT8                       IScsiDxeStrings[];
extern ISCSI_FORM_CALLBACK_INFO    *mCallbackInfo;


#define VAR_OFFSET(Field)    \
  ((UINT16) ((UINTN) &(((ISCSI_CONFIG_IFR_NVDATA *) 0)->Field)))

#define QUESTION_ID(Field)   \
  ((UINT16) (VAR_OFFSET (Field) + CONFIG_OPTION_OFFSET))


#define DYNAMIC_ONE_OF_VAR_OFFSET           VAR_OFFSET  (Enabled)
#define DYNAMIC_ORDERED_LIST_QUESTION_ID    QUESTION_ID (DynamicOrderedList)
#define DYNAMIC_ORDERED_LIST_VAR_OFFSET     VAR_OFFSET  (DynamicOrderedList)
#define ATTEMPT_DEL_QUESTION_ID             QUESTION_ID (DeleteAttemptList)
#define ATTEMPT_DEL_VAR_OFFSET              VAR_OFFSET  (DeleteAttemptList)

//
// sizeof (EFI_MAC_ADDRESS) * 3
//
#define ISCSI_MAX_MAC_STRING_LEN            96

#define ISCSI_INITATOR_NAME_VAR_NAME        L"I_NAME"

#define ISCSI_CONFIG_VAR_ATTR               (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE)

#define ISCSI_FORM_CALLBACK_INFO_SIGNATURE  SIGNATURE_32 ('I', 'f', 'c', 'i')

#define ISCSI_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK(Callback) \
  CR ( \
  Callback, \
  ISCSI_FORM_CALLBACK_INFO, \
  ConfigAccess, \
  ISCSI_FORM_CALLBACK_INFO_SIGNATURE \
  )

#pragma pack(1)
struct _ISCSI_ATTEMPT_CONFIG_NVDATA {
  LIST_ENTRY                       Link;
  UINT8                            NicIndex;
  UINT8                            AttemptConfigIndex;
  BOOLEAN                          DhcpSuccess;
  BOOLEAN                          ValidiBFTPath;
  BOOLEAN                          ValidPath;
  UINT8                            AutoConfigureMode;
  EFI_STRING_ID                    AttemptTitleToken;
  EFI_STRING_ID                    AttemptTitleHelpToken;
  CHAR8                            AttemptName[ATTEMPT_NAME_MAX_SIZE];
  CHAR8                            MacString[ISCSI_MAX_MAC_STRING_LEN];
  EFI_IP_ADDRESS                   PrimaryDns;
  EFI_IP_ADDRESS                   SecondaryDns;
  EFI_IP_ADDRESS                   DhcpServer;
  ISCSI_SESSION_CONFIG_NVDATA      SessionConfigData;
  UINT8                            AuthenticationType;
  union {
    ISCSI_CHAP_AUTH_CONFIG_NVDATA  CHAP;
  } AuthConfigData;

};

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH               VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL         End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

struct _ISCSI_FORM_CALLBACK_INFO {
  UINT32                           Signature;
  EFI_HANDLE                       DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;
  UINT16                           *KeyList;
  VOID                             *FormBuffer;
  EFI_HII_HANDLE                   RegisteredHandle;
  ISCSI_ATTEMPT_CONFIG_NVDATA      *Current;
};

/**
  Initialize the iSCSI configuration form.

  @param[in]  DriverBindingHandle The iSCSI driverbinding handle.

  @retval EFI_SUCCESS             The iSCSI configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.

**/
EFI_STATUS
IScsiConfigFormInit (
  IN EFI_HANDLE  DriverBindingHandle
  );

/**
  Unload the iSCSI configuration form, this includes: delete all the iSCSI
  configuration entries, uninstall the form callback protocol, and
  free the resources used.

  @param[in]  DriverBindingHandle The iSCSI driverbinding handle.

  @retval EFI_SUCCESS             The iSCSI configuration form is unloaded.
  @retval Others                  Failed to unload the form.

**/
EFI_STATUS
IScsiConfigFormUnload (
  IN EFI_HANDLE  DriverBindingHandle
  );

/**
  Update the MAIN form to display the configured attempts.

**/
VOID
IScsiConfigUpdateAttempt (
  VOID
  );

/**
  Get the attempt config data from global structure by the ConfigIndex.

  @param[in]  AttemptConfigIndex     The unique index indicates the attempt.

  @return       Pointer to the attempt config data.
  @retval NULL  The attempt configuration data can not be found.

**/
ISCSI_ATTEMPT_CONFIG_NVDATA *
IScsiConfigGetAttemptByConfigIndex (
  IN UINT8                     AttemptConfigIndex
  );

#endif
