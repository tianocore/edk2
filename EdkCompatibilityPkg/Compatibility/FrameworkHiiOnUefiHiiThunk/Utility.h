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

/**
  
  This function returns a list of the package handles of the   
  specified type that are currently active in the HII database. The   
  pseudo-type EFI_HII_PACKAGE_TYPE_ALL will cause all package   
  handles to be listed.

  If HandleBufferLength is NULL, then ASSERT.
  If HandleBuffer is NULL, the ASSERT.
  If PackageType is EFI_HII_PACKAGE_TYPE_GUID and PackageGuid is
  NULL, then ASSERT.
  If PackageType is not EFI_HII_PACKAGE_TYPE_GUID and PackageGuid is not
  NULL, then ASSERT.
  
  
  @param PackageType          Specifies the package type of the packages
                              to list or EFI_HII_PACKAGE_TYPE_ALL for
                              all packages to be listed.
  
  @param PackageGuid          If PackageType is
                              EFI_HII_PACKAGE_TYPE_GUID, then this is
                              the pointer to the GUID which must match
                              the Guid field of
                              EFI_HII_PACKAGE_GUID_HEADER. Otherwise, it
                              must be NULL.
  
  @param HandleBufferLength   On output, the length of the handle buffer
                              that is required for the handles found.

  @param HandleBuffer         On output, an array of EFI_HII_HANDLE  instances returned.
                              The caller is responcible to free this pointer allocated.

  @retval EFI_SUCCESS           The matching handles are outputed successfully.
                                HandleBufferLength is updated with the actual length.
  @retval EFI_OUT_OF_RESOURCES  Not enough resource to complete the operation.
  @retval EFI_NOT_FOUND         No matching handle could not be found in database.
**/
EFI_STATUS
EFIAPI
ListPackageLists (
  IN        UINT8                     PackageType,
  IN CONST  EFI_GUID                  *PackageGuid,
  IN OUT    UINTN                     *HandleBufferLength,
  OUT       EFI_HII_HANDLE            **HandleBuffer
  )
;

/**
  Exports the contents of one or all package lists in the HII database into a buffer.

  If Handle is not NULL and not a valid EFI_HII_HANDLE registered in the database, 
  then ASSERT.
  If PackageListHeader is NULL, then ASSERT.
  If PackageListSize is NULL, then ASSERT.

  @param  Handle                 The HII Handle.
  @param  PackageListHeader      A pointer to a buffer that will contain the results of 
                                 the export function.
  @param  PackageListSize        On output, the length of the buffer that is required for the exported data.

  @retval EFI_SUCCESS            Package exported.

  @retval EFI_OUT_OF_RESOURCES   Not enought memory to complete the operations.

**/
EFI_STATUS 
EFIAPI
ExportPackageLists (
  IN EFI_HII_HANDLE                    Handle,
  OUT EFI_HII_PACKAGE_LIST_HEADER      **PackageListHeader,
  OUT UINTN                            *PackageListSize
  )
;

/**
  Extract Hii package list GUID for given HII handle.

  If HiiHandle could not be found in the HII database, then ASSERT.
  If Guid is NULL, then ASSERT.

  @param  Handle              Hii handle
  @param  Guid                Package list GUID

  @retval EFI_SUCCESS            Successfully extract GUID from Hii database.

**/
EFI_STATUS
EFIAPI
ExtractGuidFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  OUT     EFI_GUID            *Guid
  )
;

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
  );

HII_THUNK_CONTEXT *
FwHiiHandleToThunkContext (
  IN CONST HII_THUNK_PRIVATE_DATA *Private,
  IN FRAMEWORK_EFI_HII_HANDLE          FwHiiHandle
  );

HII_THUNK_CONTEXT *
UefiHiiHandleToThunkContext (
  IN CONST HII_THUNK_PRIVATE_DATA *Private,
  IN EFI_HII_HANDLE                   UefiHiiHandle
  );

HII_THUNK_CONTEXT *
TagGuidToIfrPackThunkContext (
  IN CONST HII_THUNK_PRIVATE_DATA *Private,
  IN CONST EFI_GUID                   *Guid
  );

HII_THUNK_CONTEXT *
CreateThunkContextForUefiHiiHandle (
  IN  EFI_HII_HANDLE             UefiHiiHandle
 );

VOID
DestroyThunkContextForUefiHiiHandle (
  IN HII_THUNK_PRIVATE_DATA     *Private,
  IN EFI_HII_HANDLE             UefiHiiHandle
 );

UINTN
GetPackageCountByType (
  IN CONST EFI_HII_PACKAGE_LIST_HEADER     *PackageListHeader,
  IN       UINT8                           PackageType
  );

EFI_STATUS
CreateQuestionIdMap (
  IN    OUT HII_THUNK_CONTEXT  *ThunkContext
  );

VOID
GetAttributesOfFirstFormSet (
  IN    OUT HII_THUNK_CONTEXT  *ThunkContext
  );

LIST_ENTRY *
GetMapEntryListHead (
  IN CONST HII_THUNK_CONTEXT  *ThunkContext,
  IN       UINT16             VarStoreId
  );

HII_THUNK_CONTEXT *
CreateThunkContext (
  IN  HII_THUNK_PRIVATE_DATA      *Private,
  IN  UINTN                       StringPackageCount,
  IN  UINTN                       IfrPackageCount
  );

VOID
DestroyThunkContext (
  IN HII_THUNK_CONTEXT          *ThunkContext
  );

VOID
DestoryOneOfOptionMap (
  IN LIST_ENTRY     *OneOfOptionMapListHead
  );

VOID
GetFormSetGuid (
  IN  EFI_HII_PACKAGE_HEADER  *Package,
  OUT EFI_GUID                *FormSetGuid
  )
;

EFI_HII_PACKAGE_HEADER *
GetIfrPackage (
  IN CONST EFI_HII_PACKAGES               *Packages
  )
;

FORM_BROWSER_FORMSET *
ParseFormSet (
  IN EFI_HII_HANDLE   UefiHiiHandle
  )
;

#endif
