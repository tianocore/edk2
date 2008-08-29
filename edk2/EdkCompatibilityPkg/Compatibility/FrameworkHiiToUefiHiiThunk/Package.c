/**@file
  Implement protocol interface related to package registrations.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"
#include "HiiHandle.h"


STATIC BOOLEAN mInFrameworkHiiNewPack = FALSE;
STATIC BOOLEAN mInFrameworkHiiRemovePack = FALSE;
BOOLEAN mInFrameworkUpdatePakcage = FALSE;


EFI_STATUS
GetPackageCount (
  IN CONST EFI_HII_PACKAGES               *Packages,
  UINTN                                   *IfrPackageCount,
  UINTN                                   *StringPackageCount
  )
{
  UINTN                         Index;
  TIANO_AUTOGEN_PACKAGES_HEADER **TianoAutogenPackageHdrArray;

  ASSERT (Packages != NULL);
  ASSERT (IfrPackageCount != NULL);
  ASSERT (StringPackageCount != NULL);

  *IfrPackageCount = 0;
  *StringPackageCount = 0;

  TianoAutogenPackageHdrArray = (TIANO_AUTOGEN_PACKAGES_HEADER **) (((UINT8 *) &Packages->GuidId) + sizeof (Packages->GuidId));
  
  for (Index = 0; Index < Packages->NumberOfPackages; Index++) {
    //
    // The current UEFI HII build tool generate a binary in the format defined by 
    // TIANO_AUTOGEN_PACKAGES_HEADER. We assume that all packages generated in
    // this binary is with same package type. So the returned IfrPackageCount and StringPackageCount
    // may not be the exact number of valid package number in the binary generated 
    // by HII Build tool.
    //
    switch (TianoAutogenPackageHdrArray[Index]->PackageHeader.Type) {
      case EFI_HII_PACKAGE_FORM:
        *IfrPackageCount += 1;
        break;
      case EFI_HII_PACKAGE_STRINGS:
        *StringPackageCount += 1;
        break;

      case EFI_HII_PACKAGE_SIMPLE_FONTS:
        break;

      //
      // The following fonts are invalid for a module that using Framework to UEFI thunk layer.
      //
      case EFI_HII_PACKAGE_KEYBOARD_LAYOUT:
      case EFI_HII_PACKAGE_FONTS:
      case EFI_HII_PACKAGE_IMAGES:
      default:
        ASSERT (FALSE);
        return EFI_INVALID_PARAMETER;
        break;
    }
  }

  return EFI_SUCCESS;
}

VOID
UpdatePackListWithOnlyIfrPack (
  IN       HII_THUNK_PRIVATE_DATA      *Private,
  IN       HII_THUNK_CONTEXT            *StringPackageThunkContext,
  IN CONST EFI_HII_PACKAGE_LIST_HEADER *StringPackageListHeader
  )
{
  EFI_STATUS                 Status;
  LIST_ENTRY                 *Link;
  HII_THUNK_CONTEXT *ThunkContext;

  Link = GetFirstNode (&Private->ThunkContextListHead);
  while (!IsNull (&Private->ThunkContextListHead, Link)) {

    ThunkContext = HII_THUNK_CONTEXT_FROM_LINK (Link);

    if (StringPackageThunkContext != ThunkContext) {
      //
      // Skip the String Package Thunk Entry itself.
      //
    
      if (CompareGuid (&StringPackageListHeader->PackageListGuid, &ThunkContext->TagGuid)) {

        ASSERT (ThunkContext->StringPackageCount == 0 && ThunkContext->IfrPackageCount == 1);

        ThunkContext->StringPackageCount = GetPackageCountByType (StringPackageListHeader, EFI_HII_PACKAGE_STRINGS);
        
        Status = mHiiDatabase->UpdatePackageList (
                                              mHiiDatabase,
                                              ThunkContext->UefiHiiHandle,
                                              StringPackageListHeader
                                              );
        ASSERT_EFI_ERROR (Status);
        
      }
    }
    
    Link = GetNextNode (&Private->ThunkContextListHead, Link);
  }
  
}


EFI_HII_PACKAGE_LIST_HEADER *
PrepareUefiPackageListFromFrameworkHiiPackages (
  IN CONST EFI_HII_PACKAGES            *Packages,
  IN CONST EFI_GUID                    *PackageListGuid
  )
{
  UINTN                       NumberOfPackages;
  EFI_HII_PACKAGE_LIST_HEADER *PackageListHeader;
  UINT8                       *PackageListData;
  UINT32                      PackageListLength;
  UINT32                      PackageLength;
  EFI_HII_PACKAGE_HEADER      PackageHeader;
  UINTN                       Index;
  TIANO_AUTOGEN_PACKAGES_HEADER **TianoAutogenPackageHdrArray;

  ASSERT (Packages != NULL);
  ASSERT (PackageListGuid != NULL);

  TianoAutogenPackageHdrArray = (TIANO_AUTOGEN_PACKAGES_HEADER **) ((UINT8 *) &Packages->GuidId + sizeof (Packages->GuidId));
  NumberOfPackages = Packages->NumberOfPackages;

  PackageListLength = sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  for (Index = 0; Index < NumberOfPackages; Index++) {
    CopyMem (&PackageLength, &TianoAutogenPackageHdrArray[Index]->BinaryLength, sizeof (UINT32));
    //
    //TIANO_AUTOGEN_PACKAGES_HEADER.BinaryLength include the BinaryLength itself.
    //
    PackageListLength += (PackageLength - sizeof(UINT32)); 
  }

  //
  // Include the lenght of EFI_HII_PACKAGE_END
  //
  PackageListLength += sizeof (EFI_HII_PACKAGE_HEADER);
  PackageListHeader = AllocateZeroPool (PackageListLength);
  ASSERT (PackageListHeader != NULL);

  CopyMem (&PackageListHeader->PackageListGuid, PackageListGuid, sizeof (EFI_GUID));
  PackageListHeader->PackageLength = PackageListLength;

  PackageListData = ((UINT8 *) PackageListHeader) + sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  for (Index = 0; Index < NumberOfPackages; Index++) {
    CopyMem (&PackageLength, &(TianoAutogenPackageHdrArray[Index]->BinaryLength), sizeof (UINT32));
    PackageLength  -= sizeof (UINT32);
    CopyMem (PackageListData, &(TianoAutogenPackageHdrArray[Index]->PackageHeader), PackageLength);
    PackageListData += PackageLength;
  }

  //
  // Append EFI_HII_PACKAGE_END
  //
  PackageHeader.Type = EFI_HII_PACKAGE_END;
  PackageHeader.Length = sizeof (EFI_HII_PACKAGE_HEADER);
  CopyMem (PackageListData, &PackageHeader, PackageHeader.Length);

  return PackageListHeader;  
}

VOID
GenerateRandomGuid (
  OUT           EFI_GUID * Guid
  )
{
  EFI_GUID        GuidBase = { 0x14f95e01, 0xd562, 0x432e, { 0x84, 0x4a, 0x95, 0xa4, 0x39, 0x5, 0x10, 0x7e }};
  static  UINT64  Count = 0;

  CopyGuid (Guid, &GuidBase);

  Count++;  
  *((UINT64 *) Guid) = *((UINT64 *) Guid) + Count;
}

EFI_STATUS
FindStringPackAndUpdatePackListWithOnlyIfrPack (
  IN HII_THUNK_PRIVATE_DATA          *Private,
  IN HII_THUNK_CONTEXT                *IfrThunkContext
  )
{
  EFI_STATUS                      Status;
  LIST_ENTRY                      *Link;
  EFI_HII_PACKAGE_LIST_HEADER     *StringPackageListHeader;
  UINTN                           Size;
  HII_THUNK_CONTEXT                *ThunkContext;

  
  Link = GetFirstNode (&Private->ThunkContextListHead);

  while (!IsNull (&Private->ThunkContextListHead, Link)) {

    ThunkContext = HII_THUNK_CONTEXT_FROM_LINK (Link);

    if (ThunkContext != IfrThunkContext) {
      if (CompareGuid (&IfrThunkContext->TagGuid, &ThunkContext->TagGuid) && (ThunkContext->IfrPackageCount == 0)) {
        Status = HiiLibExportPackageLists (ThunkContext->UefiHiiHandle, &StringPackageListHeader, &Size);
        ASSERT_EFI_ERROR (Status);

        IfrThunkContext->StringPackageCount = GetPackageCountByType (StringPackageListHeader, EFI_HII_PACKAGE_STRINGS);
        //
        // Add Function to only get only String Packages from the Package List
        //
        Status = mHiiDatabase->UpdatePackageList (
                                  mHiiDatabase,
                                  IfrThunkContext->UefiHiiHandle,
                                  StringPackageListHeader
                                  );
        ASSERT_EFI_ERROR (Status);
        
        FreePool (StringPackageListHeader);
        return EFI_SUCCESS;

      }
    }

    Link = GetNextNode (&Private->ThunkContextListHead, Link);
  }

  ASSERT (FALSE);
  return EFI_NOT_FOUND;
  
}


//
// 
//
EFI_STATUS
UefiRegisterPackageList(
  IN  HII_THUNK_PRIVATE_DATA      *Private,
  IN  EFI_HII_PACKAGES            *Packages,
  OUT FRAMEWORK_EFI_HII_HANDLE    *Handle
  )
{
  EFI_STATUS                  Status;
  UINTN                       StringPackageCount;
  UINTN                       IfrPackageCount;
  EFI_HII_PACKAGE_LIST_HEADER *PackageListHeader;
  HII_THUNK_CONTEXT           *ThunkContext;
  EFI_GUID                    GuidId;

  PackageListHeader = NULL;

  Status = GetPackageCount (Packages, &IfrPackageCount, &StringPackageCount);
  ASSERT_EFI_ERROR (Status);
  
  if (IfrPackageCount > 1) {
    //
    // HII Thunk only handle package with 0 or 1 IFR package. 
    //
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  ThunkContext = CreateThunkContext (Private, StringPackageCount, IfrPackageCount);
  if (ThunkContext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  ThunkContext->ByFrameworkHiiNewPack = TRUE;
  
  if (Packages->GuidId == NULL) {
    //
    // UEFI HII Database require Package List GUID must be unique.
    //
    // If Packages->GuidId is NULL, the caller of FramworkHii->NewPack is registering
    // packages with at least 1 StringPack and 1 IfrPack. Therefore, Packages->GuidId is
    // not used as the name of the package list.  A GUID is generated as a Package List
    // GUID.
    //
    ASSERT (StringPackageCount >=1 && IfrPackageCount == 1);
    GenerateRandomGuid (&GuidId);
  } else {
    CopyGuid (&GuidId, Packages->GuidId);
  }

  //
  // Record the Package List GUID, it is used as a name for the package list by Framework HII.
  //
  CopyGuid (&ThunkContext->TagGuid, &GuidId);

  if ((StringPackageCount == 0) && (IfrPackageCount != 0)) {
    //
    // UEFI HII database does not allow two package list with the same GUID.
    // In Framework HII implementation, Packages->GuidId is used as an identifier to associate 
    // a PackageList with only IFR to a Package list the with String package.
    //
    GenerateRandomGuid (&GuidId);
  }

  //
  // UEFI HII require EFI_HII_CONFIG_ACCESS_PROTOCOL to be installed on a EFI_HANDLE, so
  // that Setup Utility can load the Buffer Storage using this protocol.
  //
  if (IfrPackageCount != 0) {
    InstallDefaultConfigAccessProtocol (Packages, ThunkContext);
  }
  PackageListHeader = PrepareUefiPackageListFromFrameworkHiiPackages (Packages, &GuidId);
  Status = mHiiDatabase->NewPackageList (
              mHiiDatabase,
              PackageListHeader,  
              ThunkContext->UefiHiiDriverHandle,
              &ThunkContext->UefiHiiHandle
              );

  //
  // BUGBUG: Remove when development is done
  //
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  if (IfrPackageCount == 0) {
    if (StringPackageCount != 0) {
      //
      // Look for a Package List with only IFR Package with the same GUID name.
      // If found one, add the String Packages to it.
      //
      UpdatePackListWithOnlyIfrPack (
          Private,
          ThunkContext,
          PackageListHeader
      );
    }
  } else {
    CreateQuestionIdMap (ThunkContext);
    
    if (StringPackageCount == 0) {
      //
      // Register the Package List to UEFI HII first.
      //
      Status = FindStringPackAndUpdatePackListWithOnlyIfrPack (
                  Private,
                  ThunkContext
                  );

      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }
  }

Done:
  if (EFI_ERROR (Status)) {
    DestroyThunkContext (ThunkContext);
  } else {
    InsertTailList (&Private->ThunkContextListHead, &ThunkContext->Link);
    *Handle = ThunkContext->FwHiiHandle;
  }

  SafeFreePool (PackageListHeader);
  
  return Status;
}


EFI_STATUS
EFIAPI
HiiNewPack (
  IN  EFI_HII_PROTOCOL               *This,
  IN  EFI_HII_PACKAGES               *Packages,
  OUT FRAMEWORK_EFI_HII_HANDLE       *Handle
  )
/*++

Routine Description:

  Extracts the various packs from a package list.

Arguments:

  This      - Pointer of HII protocol.
  Packages  - Pointer of HII packages.
  Handle    - Handle value to be returned.

Returns:

  EFI_SUCCESS           - Pacakges has added to HII database successfully.
  EFI_INVALID_PARAMETER - Invalid parameter.

--*/
{
  EFI_STATUS                 Status;
  HII_THUNK_PRIVATE_DATA *Private;
  EFI_TPL                    OldTpl;

  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Packages == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  
  //
  // We use a simple Global variable to inform NewPackNotify
  // that the package list registered here is already registered
  // in the HII Thunk Layer. So NewPackNotify does not need to
  // call RegisterUefiHiiHandle () to registered it.
  //
  mInFrameworkHiiNewPack = TRUE;

  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  Status = UefiRegisterPackageList (
              Private,
              Packages,
              Handle
            );

  mInFrameworkHiiNewPack = FALSE;

  gBS->RestoreTPL (OldTpl);

  return Status;
}

EFI_STATUS
EFIAPI
HiiRemovePack (
  IN EFI_HII_PROTOCOL    *This,
  IN FRAMEWORK_EFI_HII_HANDLE       Handle
  )
/*++

Routine Description:
  Removes the various packs from a Handle

Arguments:

Returns:

--*/
{
  EFI_STATUS                 Status;
  HII_THUNK_PRIVATE_DATA *Private;
  HII_THUNK_CONTEXT *ThunkContext;
  EFI_TPL                    OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  mInFrameworkHiiRemovePack = TRUE;

  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  ThunkContext = FwHiiHandleToThunkContext (Private, Handle);

  if (ThunkContext != NULL) {
    Status = mHiiDatabase->RemovePackageList (
                                          mHiiDatabase,
                                          ThunkContext->UefiHiiHandle
                                          );
    ASSERT_EFI_ERROR (Status);

    if (ThunkContext->IfrPackageCount != 0) {
      UninstallDefaultConfigAccessProtocol (ThunkContext);
    }

    RemoveEntryList (&ThunkContext->Link);
    DestroyThunkContext (ThunkContext);
  }else {
    Status = EFI_NOT_FOUND;
  }

  mInFrameworkHiiRemovePack = FALSE;
  gBS->RestoreTPL (OldTpl);

  return Status;
}

EFI_STATUS
EFIAPI
NewOrAddPackNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  EFI_STATUS              Status;
  HII_THUNK_PRIVATE_DATA  *Private;
  HII_THUNK_CONTEXT       *ThunkContext;

  ASSERT (PackageType == EFI_HII_PACKAGE_STRINGS || PackageType == EFI_HII_PACKAGE_FORM);
  ASSERT (NotifyType == EFI_HII_DATABASE_NOTIFY_ADD_PACK || NotifyType == EFI_HII_DATABASE_NOTIFY_NEW_PACK);

  Status  = EFI_SUCCESS;
  Private = mHiiThunkPrivateData;

  if (mInFrameworkHiiNewPack || mInFrameworkUpdatePakcage) {
    return EFI_SUCCESS;
  }

  //
  // We only create a ThunkContext if the Uefi Hii Handle is only already registered
  // by the HII Thunk Layer.
  //
  ThunkContext = UefiHiiHandleToThunkContext (Private, Handle);
  if (ThunkContext == NULL) {
    ThunkContext = CreateThunkContextForUefiHiiHandle (Handle);
    ASSERT (ThunkContext != NULL);

    InsertTailList (&Private->ThunkContextListHead, &ThunkContext->Link);
  } 

  if (PackageType == EFI_HII_PACKAGE_FORM) {
    GetAttributesOfFirstFormSet (ThunkContext);
  }

  return Status;  
}

//
// Framework HII module may cache a GUID as the name of the package list.
// Then search for the Framework HII handle database for the handle matching
// this GUID

EFI_STATUS
EFIAPI
RemovePackNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  EFI_STATUS                  Status;
  HII_THUNK_PRIVATE_DATA      *Private;
  HII_THUNK_CONTEXT           *ThunkContext;
  EFI_HII_PACKAGE_LIST_HEADER *HiiPackageList;
  UINTN                        BufferSize;

  Status = EFI_SUCCESS;

  ASSERT (PackageType == EFI_HII_PACKAGE_STRINGS);
  ASSERT (NotifyType == EFI_HII_DATABASE_NOTIFY_REMOVE_PACK);

  if (mInFrameworkHiiRemovePack || mInFrameworkUpdatePakcage) {
    return EFI_SUCCESS;
  }

  Private = mHiiThunkPrivateData;

  ThunkContext = UefiHiiHandleToThunkContext (Private, Handle);

  if (!ThunkContext->ByFrameworkHiiNewPack) {
    Status = HiiLibExportPackageLists (Handle, &HiiPackageList, &BufferSize);
    ASSERT_EFI_ERROR (Status);

    if (GetPackageCountByType (HiiPackageList, EFI_HII_PACKAGE_STRINGS) == 1) {
      //
      // If the string package will be removed is the last string package
      // in the package list, we will remove the HII Thunk entry from the
      // database.
      //
      DestroyThunkContextForUefiHiiHandle (Private, Handle);
    }

    FreePool (HiiPackageList);
  }

  return Status;
}



