/**@file

  This file contains the keyboard processing code to the HII database.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"


BOOLEAN mInFrameworkHiiNewPack = FALSE;
BOOLEAN mInFrameworkHiiRemovePack = FALSE;


EFI_STATUS
GetIfrAndStringPackNum (
  IN CONST EFI_HII_PACKAGES               *Packages,
  UINTN                                   *IfrPackNum,
  UINTN                                   *StringPackNum
  )
{
  UINTN                         Index;
  TIANO_AUTOGEN_PACKAGES_HEADER **TianoAutogenPackageHdrArray;

  ASSERT (Packages != NULL);
  ASSERT (IfrPackNum != NULL);
  ASSERT (StringPackNum != NULL);

  *IfrPackNum = 0;
  *StringPackNum = 0;

  TianoAutogenPackageHdrArray = (TIANO_AUTOGEN_PACKAGES_HEADER **) (((UINT8 *) &Packages->GuidId) + sizeof (Packages->GuidId));
  for (Index = 0; Index < Packages->NumberOfPackages; Index++) {
    //
    // BugBug: The current UEFI HII build tool generate a binary in the format defined in: 
    // TIANO_AUTOGEN_PACKAGES_HEADER. We assume that all packages generated in
    // this binary is with same package type. So the returned IfrPackNum and StringPackNum
    // may not be the exact number of valid package number in the binary generated 
    // by HII Build tool.
    //
    switch (TianoAutogenPackageHdrArray[Index]->PackageHeader.Type) {
      case EFI_HII_PACKAGE_FORM:
        *IfrPackNum += 1;
        break;
      case EFI_HII_PACKAGE_STRINGS:
        *StringPackNum += 1;
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

EFI_STATUS 
LibExportPackageLists (
  IN EFI_HII_HANDLE                    UefiHiiHandle,
  OUT EFI_HII_PACKAGE_LIST_HEADER      **PackageListHeader,
  OUT UINTN                            *PackageListSize
  )
{
  EFI_STATUS                       Status;
  UINTN                            Size;
  EFI_HII_PACKAGE_LIST_HEADER      *PackageListHdr;

  ASSERT (PackageListSize != NULL);
  ASSERT (PackageListHeader != NULL);

  Size = 0;
  PackageListHdr = NULL;
  Status = mHiiDatabase->ExportPackageLists (
                                      mHiiDatabase,
                                      UefiHiiHandle,
                                      &Size,
                                      PackageListHdr
                                      );
  ASSERT_EFI_ERROR (Status == EFI_BUFFER_TOO_SMALL);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    PackageListHdr = AllocateZeroPool (Size);
    ASSERT (PackageListHdr != NULL);
    
    if (PackageListHeader == NULL) {
      return EFI_OUT_OF_RESOURCES;
    } else {
      Status = mHiiDatabase->ExportPackageLists (
                                          mHiiDatabase,
                                          UefiHiiHandle,
                                          &Size,
                                          PackageListHdr
                                           );
      ASSERT_EFI_ERROR (Status);
    }
  }

  if (!EFI_ERROR (Status)) {
    *PackageListHeader = PackageListHdr;
    *PackageListSize   = Size;
  }

  return Status;
}

EFI_STATUS
InsertStringPackagesToIfrPackageList (
  IN CONST EFI_HII_PACKAGE_LIST_HEADER *StringPackageListHeader,
  IN EFI_HII_HANDLE                    UefiHiiHandle  
 )
{
  EFI_STATUS                  Status;
  Status = mHiiDatabase->UpdatePackageList (
                                        mHiiDatabase,
                                        UefiHiiHandle,
                                        StringPackageListHeader
                                        );

  return Status;
}


/**
  Removes a node from a doubly linked list, and returns the node that follows
  the removed node.

  Removes the node Entry from a doubly linked list. It is up to the caller of
  this function to release the memory used by this node if that is required. On
  exit, the node following Entry in the doubly linked list is returned. If
  Entry is the only node in the linked list, then the head node of the linked
  list is returned.

  If Entry is NULL, then ASSERT().
  If Entry is the head node of an empty list, then ASSERT().
  If PcdMaximumLinkedListLength is not zero, and the number of nodes in the
  linked list containing Entry, including the Entry node, is greater than
  or equal to PcdMaximumLinkedListLength, then ASSERT().

  @param  Entry A pointer to a node in a linked list

  @return Entry

**/
EFI_STATUS
AddStringPackagesToMatchingIfrPackageList (
  IN       EFI_HII_THUNK_PRIVATE_DATA  *Private,
  IN CONST EFI_HII_PACKAGE_LIST_HEADER *StringPackageListHeader
  )
{
  EFI_STATUS                 Status;
  LIST_ENTRY                 *ListEntry;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *HandleMapEntry;

  for (ListEntry = Private->HiiThunkHandleMappingDBListHead.ForwardLink;
       ListEntry != &Private->HiiThunkHandleMappingDBListHead;
       ListEntry = ListEntry->ForwardLink
       ) {
    HandleMapEntry = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY (ListEntry);
    if (CompareGuid (&StringPackageListHeader->PackageListGuid, &HandleMapEntry->TagGuid)) {
      Status = InsertStringPackagesToIfrPackageList (StringPackageListHeader, HandleMapEntry->UefiHiiHandle);
    }
  }
  
  return EFI_NOT_FOUND;
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
GenerateGuidId (
  IN      CONST EFI_GUID * InGuid,
  OUT           EFI_GUID * OutGuid
  )
{
  UINT64                   MonotonicCount;

  CopyMem (OutGuid, InGuid, sizeof (EFI_GUID));
  
  gBS->GetNextMonotonicCount (&MonotonicCount);
  //
  // Use Monotonic Count as a psedo random number generator.
  //
  *((UINT64 *) OutGuid) = *((UINT64 *) OutGuid) + MonotonicCount;
}

EFI_STATUS
FindAndAddStringPackageToIfrPackageList(
  EFI_HII_THUNK_PRIVATE_DATA  *Private,
  EFI_GUID                    *GuidId,
  EFI_HII_HANDLE              UefiIfrHiiHandle
  )
{
  EFI_STATUS                 Status;
  LIST_ENTRY                 *ListEntry;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *HandleMapEntry;
  EFI_HII_PACKAGE_LIST_HEADER *StringPackageListHeader;
  UINTN                      Size;

  for (ListEntry = Private->HiiThunkHandleMappingDBListHead.ForwardLink;
       ListEntry != &Private->HiiThunkHandleMappingDBListHead;
       ListEntry = ListEntry->ForwardLink
       ) {
    HandleMapEntry = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY (ListEntry);
    if (CompareGuid (GuidId, &HandleMapEntry->TagGuid) && (!HandleMapEntry->DoesPackageListImportStringPackages)) {
      Status = LibExportPackageLists (HandleMapEntry->UefiHiiHandle, &StringPackageListHeader, &Size);
      ASSERT_EFI_ERROR (Status);

      //
      // Add Function to only get only String Packages from the Package List
      //

      Status = InsertStringPackagesToIfrPackageList (StringPackageListHeader, UefiIfrHiiHandle);
      ASSERT_EFI_ERROR (Status);
      
      FreePool (StringPackageListHeader);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;

}

CONST EFI_GUID mAGuid = 
  { 0x14f95e01, 0xd562, 0x432e, { 0x84, 0x4a, 0x95, 0xa4, 0x39, 0x5, 0x10, 0x7e } };

EFI_STATUS
UefiRegisterPackageList(
  EFI_HII_THUNK_PRIVATE_DATA  *Private,
  EFI_HII_PACKAGES            *Packages,
  FRAMEWORK_EFI_HII_HANDLE    *Handle
  )
{
  EFI_STATUS                  Status;
  UINTN                       StringPackNum;
  UINTN                       IfrPackNum;
  EFI_HII_PACKAGE_LIST_HEADER *UefiPackageListHeader;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *HandleMappingEntry;
  EFI_GUID                    GuidId;

  UefiPackageListHeader = NULL;

  Status = GetIfrAndStringPackNum (Packages, &IfrPackNum, &StringPackNum);
  ASSERT_EFI_ERROR (Status);
  //
  // Thunk Layer only handle the following combinations of IfrPack, StringPkg and FontPack.
  // Thunk Layer only allow zero or one IfrPack in the Package List.
  //
  if (IfrPackNum > 1) {
    return EFI_UNSUPPORTED;
  }

  HandleMappingEntry = AllocateZeroPool (sizeof (*HandleMappingEntry));
  ASSERT (HandleMappingEntry != NULL);
  
  HandleMappingEntry->Signature = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_SIGNATURE;
  Status = AssignHiiHandle (Private, &HandleMappingEntry->FrameworkHiiHandle);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Packages->GuidId may be NULL. In such case, caller of FramworkHii->NewPack is registering
  // package with StringPack and IfrPack.
  //
  if (Packages->GuidId == NULL) {
    Packages->GuidId = &GuidId;
    GenerateGuidId (&mAGuid, Packages->GuidId);
  } else {
    CopyGuid (&GuidId, Packages->GuidId);
  }
  
  CopyGuid (&HandleMappingEntry->TagGuid, Packages->GuidId);

  if ((StringPackNum == 0) && (IfrPackNum != 0)) {
    //
    // UEFI HII database does not allow two package list with the same GUID.
    // In Framework HII implementation, Packages->GuidId is used as an identifier to associate 
    // a PackageList with only IFR to a Package list the with String package.
    //
    GenerateGuidId (Packages->GuidId, &GuidId);
  }

  //
  // UEFI HII require EFI_HII_CONFIG_ACCESS_PROTOCOL to be installed on a EFI_HANDLE, so
  // that Setup Utility can load the Buffer Storage using this protocol.
  //
  if (IfrPackNum != 0) {
    InstallDefaultUefiConfigAccessProtocol (Packages, HandleMappingEntry);
  }
  UefiPackageListHeader = PrepareUefiPackageListFromFrameworkHiiPackages (Packages, &GuidId);
  Status = mHiiDatabase->NewPackageList (
              mHiiDatabase,
              UefiPackageListHeader,  
              HandleMappingEntry->UefiHiiDriverHandle,
              &HandleMappingEntry->UefiHiiHandle
              );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  if (IfrPackNum == 0) {
    if (StringPackNum != 0) {
      //
      // Look for a package list with IFR Pack which has already registed with HII Database
      //
      HandleMappingEntry->IsPackageListWithOnlyStringPackages = TRUE;
      Status = AddStringPackagesToMatchingIfrPackageList (
                  Private,
                  UefiPackageListHeader
                );

      if (!EFI_ERROR (Status) || Status == EFI_NOT_FOUND) {

        if (Status == EFI_NOT_FOUND) {
          Status = EFI_SUCCESS;
        }
      }
    }
  } else {
    if (StringPackNum == 0) {
      //
      // Register the Package List to UEFI HII first.
      //
      Status = FindAndAddStringPackageToIfrPackageList (
                  Private,
                  Packages->GuidId,
                  HandleMappingEntry->UefiHiiHandle
                  );
      ASSERT_EFI_ERROR (Status);
      if (!EFI_ERROR (Status)) {
        HandleMappingEntry->DoesPackageListImportStringPackages = TRUE;
      }
    }
  }

  if (!EFI_ERROR (Status)) {
    InsertTailList (&Private->HiiThunkHandleMappingDBListHead, &HandleMappingEntry->List);
  }

Done:
  if (EFI_ERROR (Status)) {
    FreePool (HandleMappingEntry);
  } else {
    *Handle = HandleMappingEntry->FrameworkHiiHandle;
  }

  SafeFreePool (UefiPackageListHeader);
  
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
  EFI_HII_THUNK_PRIVATE_DATA *Private;
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

  Private = EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

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
  EFI_HII_THUNK_PRIVATE_DATA *Private;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *HandleMapEntry;
  EFI_TPL                    OldTpl;
  EFI_HII_CONFIG_ACCESS_PROTOCOL *ConfigAccess;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  mInFrameworkHiiRemovePack = TRUE;

  Private = EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  HandleMapEntry = FrameworkHiiHandleToMapDatabaseEntry (Private, Handle);

  if (HandleMapEntry != NULL) {
    Status = mHiiDatabase->RemovePackageList (
                                          mHiiDatabase,
                                          HandleMapEntry->UefiHiiHandle
                                          );
    ASSERT_EFI_ERROR (Status);

    HiiLibDestroyHiiDriverHandle (HandleMapEntry->UefiHiiHandle);

    Status = gBS->HandleProtocol (
                    HandleMapEntry->UefiHiiDriverHandle,
                    &gEfiHiiConfigAccessProtocolGuid,
                    (VOID **) &ConfigAccess
                    );

    if (!EFI_ERROR (Status)) {
      Status = gBS->UninstallProtocolInterface (
                      HandleMapEntry->UefiHiiDriverHandle,
                      &gEfiHiiConfigAccessProtocolGuid,
                      ConfigAccess
                      );
      ASSERT_EFI_ERROR (Status);
    } else {
      Status = EFI_SUCCESS;
    }

    RemoveEntryList (&HandleMapEntry->List);

    FreePool (HandleMapEntry);

  }else {
    Status = EFI_NOT_FOUND;
  }

  mInFrameworkHiiRemovePack = FALSE;
  
  gBS->RestoreTPL (OldTpl);

  return Status;
}
