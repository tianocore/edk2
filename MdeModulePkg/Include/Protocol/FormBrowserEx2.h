/** @file
  Extension Form Browser Protocol provides the services that can be used to 
  register the different hot keys for the standard Browser actions described in UEFI specification.

Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FORM_BROWSER_EXTENSION2_H__
#define __FORM_BROWSER_EXTENSION2_H__

#include <Protocol/FormBrowserEx.h>

#define EDKII_FORM_BROWSER_EXTENSION2_PROTOCOL_GUID  \
  { 0xa770c357, 0xb693, 0x4e6d, { 0xa6, 0xcf, 0xd2, 0x1c, 0x72, 0x8e, 0x55, 0xb }}

typedef struct _EDKII_FORM_BROWSER_EXTENSION2_PROTOCOL   EDKII_FORM_BROWSER_EXTENSION2_PROTOCOL;

#define BROWSER_EXTENSION2_VERSION_1    0x10000
#define BROWSER_EXTENSION2_VERSION_1_1  0x10001

/**
  Check whether the browser data has been modified.

  @retval TRUE        Browser data is modified.
  @retval FALSE       No browser data is modified.

**/
typedef
BOOLEAN
(EFIAPI *IS_BROWSER_DATA_MODIFIED) (
  VOID
  );

/**
  Execute the action requested by the Action parameter.

  @param[in] Action     Execute the request action.
  @param[in] DefaultId  The default Id info when need to load default value.

  @retval EFI_SUCCESS              Execute the request action succss.

**/
typedef 
EFI_STATUS 
(EFIAPI *EXECUTE_ACTION) (
  IN UINT32        Action,
  IN UINT16        DefaultId
  );

/**
  Check whether required reset when exit the browser

  @retval TRUE      Browser required to reset after exit.
  @retval FALSE     Browser not need to reset after exit.

**/
typedef
BOOLEAN
(EFIAPI *IS_RESET_REQUIRED) (
  VOID
  );

#define FORM_ENTRY_INFO_SIGNATURE    SIGNATURE_32 ('f', 'e', 'i', 's')

typedef struct {
  UINTN           Signature;
  LIST_ENTRY      Link;

  EFI_HII_HANDLE  HiiHandle;
  EFI_GUID        FormSetGuid;
  EFI_FORM_ID     FormId;
  EFI_QUESTION_ID QuestionId;
} FORM_ENTRY_INFO;

#define FORM_ENTRY_INFO_FROM_LINK(a)  CR (a, FORM_ENTRY_INFO, Link, FORM_ENTRY_INFO_SIGNATURE)

#define FORM_QUESTION_ATTRIBUTE_OVERRIDE_SIGNATURE    SIGNATURE_32 ('f', 'q', 'o', 's')

typedef struct {
  UINTN            Signature;
  LIST_ENTRY       Link;

  EFI_QUESTION_ID  QuestionId;           // Find the question
  EFI_FORM_ID      FormId;               // Find the form
  EFI_GUID         FormSetGuid;          // Find the formset.
  EFI_HII_HANDLE   HiiHandle;            // Find the HII handle
  UINT32           Attribute;            // Hide or grayout ... 
} QUESTION_ATTRIBUTE_OVERRIDE;

#define FORM_QUESTION_ATTRIBUTE_OVERRIDE_FROM_LINK(a)  CR (a, QUESTION_ATTRIBUTE_OVERRIDE, Link, FORM_QUESTION_ATTRIBUTE_OVERRIDE_SIGNATURE)

struct _EDKII_FORM_BROWSER_EXTENSION2_PROTOCOL {
  ///
  /// Version for protocol future extension.
  ///
  UINT32                    Version;
  SET_SCOPE                 SetScope;
  REGISTER_HOT_KEY          RegisterHotKey;
  REGISTER_EXIT_HANDLER     RegiserExitHandler;
  IS_BROWSER_DATA_MODIFIED  IsBrowserDataModified;
  EXECUTE_ACTION            ExecuteAction;
  ///
  /// A list of type FORMID_INFO is Browser View Form History List.
  ///
  LIST_ENTRY                FormViewHistoryHead;
  ///
  /// A list of type QUESTION_ATTRIBUTE_OVERRIDE.
  ///
  LIST_ENTRY                OverrideQestListHead;

  IS_RESET_REQUIRED         IsResetRequired;
};

extern EFI_GUID gEdkiiFormBrowserEx2ProtocolGuid;

#endif

