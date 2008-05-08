/**@file
  This file contains functions related to Config Access Protocols installed by
  by HII Thunk Modules which is used to thunk UEFI Config Access Callback to 
  Framework HII Callback.
  
Copyright (c) 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_CONFIG_ACCESS_H
#define _HII_THUNK_CONFIG_ACCESS_H


EFI_STATUS
InstallDefaultUefiConfigAccessProtocol (
  IN  CONST EFI_HII_PACKAGES                         *Packages,
  IN  OUT   HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *MapEntry
  )
;

EFI_STATUS
EFIAPI
ThunkExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
;


EFI_STATUS
EFIAPI
ThunkRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
;

EFI_STATUS
EFIAPI
ThunkCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
;

#endif

