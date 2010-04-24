/** @file
  This file contains functions related to Config Access Protocols installed by
  by HII Thunk Modules which is used to thunk UEFI Config Access Callback to 
  Framework HII Callback.
  
Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_CONFIG_ACCESS_H_
#define _HII_THUNK_CONFIG_ACCESS_H_

/**
  This function installs a EFI_CONFIG_ACCESS_PROTOCOL instance for a form package registered
  by a module using Framework HII Protocol Interfaces.

  UEFI HII require EFI_HII_CONFIG_ACCESS_PROTOCOL to be installed on a EFI_HANDLE, so
  that Setup Utility can load the Buffer Storage using this protocol.
   
  @param Packages             The Package List.
  @param ThunkContext         The Thunk Context.
   
  @retval  EFI_SUCCESS        The Config Access Protocol is installed successfully.
  @retval  EFI_OUT_RESOURCE   There is not enough memory.
   
**/
EFI_STATUS
InstallDefaultConfigAccessProtocol (
  IN  CONST EFI_HII_PACKAGES                    *Packages,
  IN  OUT   HII_THUNK_CONTEXT                   *ThunkContext
  );

/**
  This function un-installs the EFI_CONFIG_ACCESS_PROTOCOL instance for a form package registered
  by a module using Framework HII Protocol Interfaces.

  ASSERT if no Config Access is found for such pakcage list or failed to uninstall the protocol.

  @param ThunkContext         The Thunk Context.
   
**/
VOID
UninstallDefaultConfigAccessProtocol (
  IN  HII_THUNK_CONTEXT                   *ThunkContext
  );

/**

  This function implement the EFI_HII_CONFIG_ACCESS_PROTOCOL.ExtractConfig
  so that data can be read from the data storage such as UEFI Variable or module's
  customized storage exposed by EFI_FRAMEWORK_CALLBACK.

   @param This        Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL
   @param Request     A null-terminated Unicode string in <ConfigRequest> format. Note that this
                      includes the routing information as well as the configurable name / value pairs. It is
                      invalid for this string to be in <MultiConfigRequest> format.

   @param Progress    On return, points to a character in the Request string. Points to the string's null
                      terminator if request was successful. Points to the most recent '&' before the first
                      failing name / value pair (or the beginning of the string if the failure is in the first
                      name / value pair) if the request was not successful
   @param Results     A null-terminated Unicode string in <ConfigAltResp> format which has all
                      values filled in for the names in the Request string. String to be allocated by the called
                      function.
   
   @retval EFI_INVALID_PARAMETER   If there is no Buffer Storage for this Config Access instance.
   @retval EFI_SUCCESS             The setting is retrived successfully.
   @retval !EFI_SUCCESS            The error returned by UEFI Get Variable or Framework Form Callback Nvread.
 **/
EFI_STATUS
EFIAPI
ThunkExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  );


/**
  This function implement the EFI_HII_CONFIG_ACCESS_PROTOCOL.RouteConfig
  so that data can be written to the data storage such as UEFI Variable or module's
  customized storage exposed by EFI_FRAMEWORK_CALLBACK.
   
   @param This             Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL
   @param Configuration    A null-terminated Unicode string in <ConfigResp> format.
   @param Progress         A pointer to a string filled in with the offset of the most recent '&' before the first
                           failing name / value pair (or the beginning of the string if the failure is in the first
                           name / value pair) or the terminating NULL if all was successful.
   
   @retval EFI_INVALID_PARAMETER   If there is no Buffer Storage for this Config Access instance.
   @retval EFI_SUCCESS             The setting is saved successfully.
   @retval !EFI_SUCCESS            The error returned by UEFI Set Variable or Framework Form Callback Nvwrite.
**/   
EFI_STATUS
EFIAPI
ThunkRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  );

/**
  Wrap the EFI_HII_CONFIG_ACCESS_PROTOCOL.CallBack to EFI_FORM_CALLBACK_PROTOCOL.Callback. Therefor,
  the framework HII module willl do no porting and work with a UEFI HII SetupBrowser.
   
   @param This                      Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
   @param Action                    Specifies the type of action taken by the browser. See EFI_BROWSER_ACTION_x.
   @param QuestionId                A unique value which is sent to the original exporting driver so that it can identify the
                                    type of data to expect. The format of the data tends to vary based on the opcode that
                                    generated the callback.
   @param Type                      The type of value for the question. See EFI_IFR_TYPE_x in
                                    EFI_IFR_ONE_OF_OPTION.
   @param Value                     A pointer to the data being sent to the original exporting driver. The type is specified
                                    by Type. Type EFI_IFR_TYPE_VALUE is defined in
                                    EFI_IFR_ONE_OF_OPTION.
   @param ActionRequest             On return, points to the action requested by the callback function. Type
                                    EFI_BROWSER_ACTION_REQUEST is specified in SendForm() in the Form
                                    Browser Protocol.
   
   @retval EFI_UNSUPPORTED        If the Framework HII module does not register Callback although it specify the opcode under
                                  focuse to be INTERRACTIVE.
   @retval EFI_SUCCESS            The callback complete successfully.
   @retval !EFI_SUCCESS           The error code returned by EFI_FORM_CALLBACK_PROTOCOL.Callback.
   
 **/
EFI_STATUS
EFIAPI
ThunkCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  );

#endif

