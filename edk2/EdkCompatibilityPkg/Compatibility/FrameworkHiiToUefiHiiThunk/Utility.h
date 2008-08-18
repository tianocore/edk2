/**@file

  This file contains utility functions by HII Thunk Modules.
  
Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_UTILITY_H
#define _HII_THUNK_UTILITY_H

EFI_GUID *
GetGuidOfFirstFormset (
  CONST EFI_HII_FORM_PACKAGE * FormPackage
);

/**
  Find the UefiHiiHandle based on a Framework HII Handle returned by
  the HII Thunk to Framework HII code.

  @param Private                        The pointer to the private data of Hii Thunk.
  @param FwHiiHandle     Framework HII Handle returned by  the HII Thunk to Framework HII code.

  @retval  NULL                           If Framework HII Handle passed in does not have matching UEFI HII handle.
  @retval  !NULL                         If the match is found.
  
**/
EFI_HII_HANDLE
FwHiiHandleToUefiHiiHandle (
  IN CONST HII_THUNK_PRIVATE_DATA *Private,
  IN FRAMEWORK_EFI_HII_HANDLE          FwHiiHandle
  )
;

HII_THUNK_CONTEXT *
FwHiiHandleToThunkContext (
  IN CONST HII_THUNK_PRIVATE_DATA *Private,
  IN FRAMEWORK_EFI_HII_HANDLE          FwHiiHandle
  )
;

HII_THUNK_CONTEXT *
UefiHiiHandleToThunkContext (
  IN CONST HII_THUNK_PRIVATE_DATA *Private,
  IN EFI_HII_HANDLE                   UefiHiiHandle
  )
;

EFI_HII_HANDLE *
TagGuidToUefiHiiHandle (
  IN CONST HII_THUNK_PRIVATE_DATA *Private,
  IN CONST EFI_GUID                   *Guid
  )
;

EFI_STATUS
AssignFrameworkHiiHandle (
  IN OUT HII_THUNK_PRIVATE_DATA *Private,
  IN     BOOLEAN                    FromFwHiiNewPack,
  OUT    FRAMEWORK_EFI_HII_HANDLE   *Handle
  )
;

HII_THUNK_CONTEXT *
CreateThunkContextForUefiHiiHandle (
  IN  HII_THUNK_PRIVATE_DATA     *Private,
  IN  EFI_HII_HANDLE             UefiHiiHandle
 )
;

EFI_STATUS
DestroyThunkContextForUefiHiiHandle (
  IN HII_THUNK_PRIVATE_DATA     *Private,
  IN EFI_HII_HANDLE             UefiHiiHandle
 )
;

UINTN
GetPackageCountByType (
  IN CONST EFI_HII_PACKAGE_LIST_HEADER     *PackageListHeader,
  IN       UINT8                           PackageType
  )
;

EFI_STATUS
CreateQuestionIdMap (
  IN    OUT HII_THUNK_CONTEXT  *ThunkContext
  )
;

LIST_ENTRY *
GetMapEntryListHead (
  IN CONST HII_THUNK_CONTEXT  *ThunkContext,
  IN       UINT16             VarStoreId
  )
;

#endif
