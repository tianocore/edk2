/** @file

  This file contains utility functions by HII Thunk Modules.
  
Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_UTILITY_H_
#define _HII_THUNK_UTILITY_H_

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
  Find the corressponding UEFI HII Handle from a Framework HII Handle given.

  @param Private      The HII Thunk Module Private context.
  @param FwHiiHandle  The Framemwork HII Handle.

  @return NULL        If Framework HII Handle is invalid.
  @return The corresponding UEFI HII Handle.
**/
EFI_HII_HANDLE
FwHiiHandleToUefiHiiHandle (
  IN CONST HII_THUNK_PRIVATE_DATA      *Private,
  IN FRAMEWORK_EFI_HII_HANDLE          FwHiiHandle
  );

/**
  Find the corressponding HII Thunk Context from a Framework HII Handle given.

  @param Private      The HII Thunk Module Private context.
  @param FwHiiHandle  The Framemwork HII Handle.

  @return NULL        If Framework HII Handle is invalid.
  @return The corresponding HII Thunk Context.
**/
HII_THUNK_CONTEXT *
FwHiiHandleToThunkContext (
  IN CONST HII_THUNK_PRIVATE_DATA      *Private,
  IN FRAMEWORK_EFI_HII_HANDLE          FwHiiHandle
  );

/**
  Find the corressponding HII Thunk Context from a UEFI HII Handle given.

  @param Private        The HII Thunk Module Private context.
  @param UefiHiiHandle  The UEFI HII Handle.

  @return NULL        If UEFI HII Handle is invalid.
  @return The corresponding HII Thunk Context.
**/
HII_THUNK_CONTEXT *
UefiHiiHandleToThunkContext (
  IN CONST HII_THUNK_PRIVATE_DATA     *Private,
  IN EFI_HII_HANDLE                   UefiHiiHandle
  );

/**
  Find the corressponding HII Thunk Context from a Tag GUID.

  @param Private      The HII Thunk Module Private context.
  @param Guid         The Tag GUID.

  @return NULL        No HII Thunk Context matched the Tag GUID.
  @return The corresponding HII Thunk Context.
**/
HII_THUNK_CONTEXT *
TagGuidToIfrPackThunkContext (
  IN CONST HII_THUNK_PRIVATE_DATA *Private,
  IN CONST EFI_GUID                   *Guid
  );

/**
  This function create a HII_THUNK_CONTEXT for the input UEFI HiiHandle
  that is created when a package list registered by a module calling 
  EFI_HII_DATABASE_PROTOCOL.NewPackageList. 
  This function records the PackageListGuid of EFI_HII_PACKAGE_LIST_HEADER 
  into the TagGuid of the created HII_THUNK_CONTEXT.

  @param UefiHiiHandle  The UEFI HII Handle.
  
  @return the new created Hii thunk context.

**/
HII_THUNK_CONTEXT *
CreateThunkContextForUefiHiiHandle (
  IN  EFI_HII_HANDLE             UefiHiiHandle
  );

/**
  Clean up the HII Thunk Context for a UEFI HII Handle.

  @param Private        The HII Thunk Module Private context.
  @param UefiHiiHandle  The UEFI HII Handle.

**/
VOID
DestroyThunkContextForUefiHiiHandle (
  IN HII_THUNK_PRIVATE_DATA     *Private,
  IN EFI_HII_HANDLE             UefiHiiHandle
  );

/**
  Get the number of HII Package for a Package type.

  @param PackageListHeader      The Package List.
  @param PackageType            The Package Type.

  @return The number of Package for given type.
**/
UINTN
GetPackageCountByType (
  IN CONST EFI_HII_PACKAGE_LIST_HEADER     *PackageListHeader,
  IN       UINT8                           PackageType
  );

/**
  Creat a Thunk Context.

  ASSERT if no FormSet Opcode is found.

  @param Private             The HII Thunk Private Context.
  @param StringPackageCount  The String package count.
  @param IfrPackageCount     The IFR Package count.

  @return  A newly created Thunk Context.
  @retval  NULL  No resource to create a new Thunk Context.
**/
HII_THUNK_CONTEXT *
CreateThunkContext (
  IN  HII_THUNK_PRIVATE_DATA      *Private,
  IN  UINTN                       StringPackageCount,
  IN  UINTN                       IfrPackageCount
  );

/**
  Destroy the Thunk Context and free up all resource.

  @param ThunkContext        The HII Thunk Private Context to be freed.

**/
VOID
DestroyThunkContext (
  IN HII_THUNK_CONTEXT          *ThunkContext
  );

/**
  Get FormSet GUID.

  ASSERT if no FormSet Opcode is found.

  @param Packages             Form Framework Package.
  @param FormSetGuid          Return the FormSet Guid.

**/
VOID
GetFormSetGuid (
  IN  EFI_HII_PACKAGE_HEADER  *Package,
  OUT EFI_GUID                *FormSetGuid
  );

/**
  Get the Form Package from a Framework Package List.

  @param Packages               Framework Package List.

  @return The Form Package Header found.
**/
EFI_HII_PACKAGE_HEADER *
GetIfrPackage (
  IN CONST EFI_HII_PACKAGES               *Packages
  );

/**
  Parse the Form Package and build a FORM_BROWSER_FORMSET structure.

  @param  UefiHiiHandle          PackageList Handle

  @return A pointer to FORM_BROWSER_FORMSET.

**/
FORM_BROWSER_FORMSET *
ParseFormSet (
  IN EFI_HII_HANDLE   UefiHiiHandle
  );

#endif
