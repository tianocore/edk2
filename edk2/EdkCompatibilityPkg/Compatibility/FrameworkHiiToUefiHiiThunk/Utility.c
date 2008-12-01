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
#include "HiiHandle.h"
#include <Library/DebugLib.h>

CONST EFI_GUID  gZeroGuid = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

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
  )
{
  HII_THUNK_CONTEXT            *ThunkContext;

  ASSERT (FwHiiHandle != (FRAMEWORK_EFI_HII_HANDLE) 0);
  ASSERT (Private != NULL);

  ThunkContext = FwHiiHandleToThunkContext (Private, FwHiiHandle);

  if (ThunkContext != NULL) {
    return ThunkContext->UefiHiiHandle;
  }
  
  return (EFI_HII_HANDLE) NULL;
}


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
  )
{
  LIST_ENTRY                 *Link;
  HII_THUNK_CONTEXT           *ThunkContext;


  Link = GetFirstNode (&Private->ThunkContextListHead);

  while (!IsNull (&Private->ThunkContextListHead, Link)) {
    ThunkContext = HII_THUNK_CONTEXT_FROM_LINK (Link);

    if (FwHiiHandle == ThunkContext->FwHiiHandle) {
      return ThunkContext;
    }

    Link = GetNextNode (&Private->ThunkContextListHead, Link);
  }

  return NULL;
}

/**
  Find the corressponding HII Thunk Context from a UEFI HII Handle given.

  @param Private      The HII Thunk Module Private context.
  @param UEFIHiiHandle  The UEFI HII Handle.

  @return NULL        If UEFI HII Handle is invalid.
  @return The corresponding HII Thunk Context.
**/
HII_THUNK_CONTEXT *
UefiHiiHandleToThunkContext (
  IN CONST HII_THUNK_PRIVATE_DATA     *Private,
  IN EFI_HII_HANDLE                   UefiHiiHandle
  )
{
  LIST_ENTRY                 *Link;
  HII_THUNK_CONTEXT           *ThunkContext;

  Link = GetFirstNode (&Private->ThunkContextListHead);

  while (!IsNull (&Private->ThunkContextListHead, Link)) {
    ThunkContext = HII_THUNK_CONTEXT_FROM_LINK (Link);

    if (UefiHiiHandle == ThunkContext->UefiHiiHandle) {
      return ThunkContext;
    }
    Link = GetNextNode (&Private->ThunkContextListHead, Link);
  }

  return NULL;
}

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
  )
{
  LIST_ENTRY                 *Link;
  HII_THUNK_CONTEXT           *ThunkContext;

  Link = GetFirstNode (&Private->ThunkContextListHead);

  while (!IsNull (&Private->ThunkContextListHead, Link)) {
    ThunkContext = HII_THUNK_CONTEXT_FROM_LINK (Link);

    if (CompareGuid (Guid, &ThunkContext->TagGuid) && (ThunkContext->IfrPackageCount != 0)) {
      return ThunkContext;
    }

    Link = GetNextNode (&Private->ThunkContextListHead, Link);
  }

  return NULL;
  
}

/**
  Clean up the HII Thunk Context for a UEFI HII Handle.

  @param Private      The HII Thunk Module Private context.
  @param UEFIHiiHandle  The UEFI HII Handle.

**/
VOID
DestroyThunkContextForUefiHiiHandle (
  IN HII_THUNK_PRIVATE_DATA     *Private,
  IN EFI_HII_HANDLE             UefiHiiHandle
 )
{
  HII_THUNK_CONTEXT     *ThunkContext;

  ThunkContext = UefiHiiHandleToThunkContext (Private, UefiHiiHandle);
  ASSERT (ThunkContext != NULL);

  DestroyThunkContext (ThunkContext);
}


/**
  This function create a HII_THUNK_CONTEXT for a package list registered
  by a module calling EFI_HII_DATABASE_PROTOCOL.NewPackageList. It records
  the PackageListGuid in EFI_HII_PACKAGE_LIST_HEADER in the TagGuid in 
  HII_THUNK_CONTEXT created. This TagGuid will be used as a key to s

**/
HII_THUNK_CONTEXT *
CreateThunkContextForUefiHiiHandle (
  IN  EFI_HII_HANDLE             UefiHiiHandle
 )
{
  EFI_STATUS            Status;
  EFI_GUID              PackageGuid;
  HII_THUNK_CONTEXT      *ThunkContext;

  ThunkContext = AllocateZeroPool (sizeof (*ThunkContext));
  ASSERT (ThunkContext != NULL);
  
  ThunkContext->Signature = HII_THUNK_CONTEXT_SIGNATURE;

  Status = AllocateHiiHandle (&ThunkContext->FwHiiHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  
  ThunkContext->UefiHiiHandle = UefiHiiHandle;
  
  Status = HiiLibExtractGuidFromHiiHandle (UefiHiiHandle, &PackageGuid);
  ASSERT_EFI_ERROR (Status);
  
  CopyGuid(&ThunkContext->TagGuid, &PackageGuid);

  return ThunkContext;
}


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
  )
{
  UINTN                     Count;
  EFI_HII_PACKAGE_HEADER    *PackageHeader;

  PackageHeader = (EFI_HII_PACKAGE_HEADER *) ((UINT8 *) PackageListHeader + sizeof (EFI_HII_PACKAGE_LIST_HEADER));
  Count = 0;
  
  while (PackageHeader->Type != EFI_HII_PACKAGE_END) {
    if (PackageHeader->Type == PackageType ) {
      Count++;
    }
    PackageHeader = (EFI_HII_PACKAGE_HEADER *) ((UINT8 *) PackageHeader + PackageHeader->Length);
  }
  
  
  return Count;
}

/**
  Get the Form Package from a Framework Package List.

  @param Packages               Framework Package List.

  @return The Form Package Header found.
**/
EFI_HII_PACKAGE_HEADER *
GetIfrPackage (
  IN CONST EFI_HII_PACKAGES               *Packages
  )
{
  UINTN                         Index;
  TIANO_AUTOGEN_PACKAGES_HEADER **TianoAutogenPackageHdrArray;

  ASSERT (Packages != NULL);

  TianoAutogenPackageHdrArray = (TIANO_AUTOGEN_PACKAGES_HEADER **) (((UINT8 *) &Packages->GuidId) + sizeof (Packages->GuidId));
  
  for (Index = 0; Index < Packages->NumberOfPackages; Index++) {
    //
    // The current UEFI HII build tool generate a binary in the format defined by 
    // TIANO_AUTOGEN_PACKAGES_HEADER. We assume that all packages generated in
    // this binary is with same package type. So the returned IfrPackageCount and StringPackageCount
    // may not be the exact number of valid package number in the binary generated 
    // by HII Build tool.
    //
    switch (TianoAutogenPackageHdrArray[Index]->FrameworkPackageHeader.Type) {
      case EFI_HII_IFR:
        return &TianoAutogenPackageHdrArray[Index]->PackageHeader;
        break;
      case EFI_HII_STRING:
      case EFI_HII_FONT:
        break;

      default:
        ASSERT (FALSE);
        return NULL;
        break;
    }
  }

  return NULL;
}

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
  )
{
  UINTN                         Offset;
  EFI_IFR_OP_HEADER             *OpCode;
  EFI_IFR_FORM_SET              *FormSet;

  Offset = sizeof (EFI_HII_PACKAGE_HEADER);
  while (Offset < Package->Length) {
    OpCode = (EFI_IFR_OP_HEADER *)((UINT8 *) Package + Offset);

    switch (OpCode->OpCode) {
    case EFI_IFR_FORM_SET_OP:
      FormSet = (EFI_IFR_FORM_SET *) OpCode;
      CopyGuid (FormSetGuid, (EFI_GUID *)(VOID *)&FormSet->Guid);
      return;
      
      default:
        break;
      
    }
    Offset += OpCode->Length;
  }

  //
  // A proper IFR must have a formset opcode.
  //
  ASSERT (FALSE);

}

/**
  Creat a Thunk Context.

  ASSERT if no FormSet Opcode is found.

  @param Private             The HII Thunk Private Context.
  @param StringPackageCount  The String package count.
  @param FormSetGuid         The IFR Package count.

  @return  A newly created Thunk Context.
  @retval  NULL  No resource to create a new Thunk Context.
**/
HII_THUNK_CONTEXT *
CreateThunkContext (
  IN  HII_THUNK_PRIVATE_DATA      *Private,
  IN  UINTN                       StringPackageCount,
  IN  UINTN                       IfrPackageCount
  )
{
  EFI_STATUS                   Status;
  HII_THUNK_CONTEXT            *ThunkContext;

  ThunkContext = AllocateZeroPool (sizeof (HII_THUNK_CONTEXT));
  ASSERT (ThunkContext != NULL);
  
  ThunkContext->Signature = HII_THUNK_CONTEXT_SIGNATURE;
  ThunkContext->IfrPackageCount = IfrPackageCount;
  ThunkContext->StringPackageCount = StringPackageCount;
  Status = AllocateHiiHandle (&ThunkContext->FwHiiHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return ThunkContext;
     
}

/**
  Destroy the Thunk Context and free up all resource.

  @param ThunkContext        The HII Thunk Private Context to be freed.

**/
VOID
DestroyThunkContext (
  IN HII_THUNK_CONTEXT          *ThunkContext
  )
{
  ASSERT (ThunkContext != NULL);

  FreeHiiHandle (ThunkContext->FwHiiHandle);

  RemoveEntryList (&ThunkContext->Link);

  if (ThunkContext->FormSet != NULL) {
    DestroyFormSet (ThunkContext->FormSet);
  }

  FreePool (ThunkContext);
}

/**
  Get the FormSet's Default Varstore ID based on the rule (Descending Priority):

  1) If VarStore ID of FRAMEWORK_RESERVED_VARSTORE_ID (0x01) is found, Var Store ID is used.
  2) If VarStore ID of FRAMEWORK_RESERVED_VARSTORE_ID is not found, First Var Store ID is used 
     as the default Var Store ID.

  @param FormSet The Form Set. The Default Varstore ID is updated if found.
  
**/
VOID
GetFormsetDefaultVarstoreId (
  IN OUT FORM_BROWSER_FORMSET  * FormSet
  )
{
  LIST_ENTRY             *StorageList;
  FORMSET_STORAGE        *Storage;

  //
  // VarStoreId 0 is invalid in UEFI IFR.
  //
  FormSet->DefaultVarStoreId = 0;
  StorageList = GetFirstNode (&FormSet->StorageListHead);

  while (!IsNull (&FormSet->StorageListHead, StorageList)) {
    Storage = FORMSET_STORAGE_FROM_LINK (StorageList);

    DEBUG ((EFI_D_INFO, "FormSet %g: Found Varstore ID %x\n", &FormSet->Guid, Storage->VarStoreId));

    if (Storage->VarStoreId == FRAMEWORK_RESERVED_VARSTORE_ID) {
      FormSet->DefaultVarStoreId = FRAMEWORK_RESERVED_VARSTORE_ID;
      break;
    }

    StorageList = GetNextNode (&FormSet->StorageListHead, StorageList);
  }

  if (FormSet->DefaultVarStoreId != FRAMEWORK_RESERVED_VARSTORE_ID) {
    StorageList = GetFirstNode (&FormSet->StorageListHead);
    if (!IsNull (&FormSet->StorageListHead, StorageList)) {
      Storage = FORMSET_STORAGE_FROM_LINK (StorageList);
      FormSet->DefaultVarStoreId = Storage->VarStoreId;
    }
    
  }

  DEBUG_CODE_BEGIN ();
  if (FormSet->DefaultVarStoreId == 0) {
    DEBUG ((EFI_D_INFO, "FormSet %g: No Varstore Found\n", &FormSet->Guid));
  } else {
    DEBUG ((EFI_D_INFO, "FormSet %g: Default Varstore ID is %x\n", &FormSet->Guid, FormSet->DefaultVarStoreId));
  }
  DEBUG_CODE_END ();
  
  return;
}

/**
  Fetch the Ifr binary data of a FormSet.

  @param  Handle                 PackageList Handle
  @param  FormSetGuid            GUID of a formset. If not specified (NULL or zero
                                 GUID), take the first FormSet found in package
                                 list.
  @param  BinaryLength           The length of the FormSet IFR binary.
  @param  BinaryData             The buffer designed to receive the FormSet.

  @retval EFI_SUCCESS            Buffer filled with the requested FormSet.
                                 BufferLength was updated.
  @retval EFI_INVALID_PARAMETER  The handle is unknown.
  @retval EFI_NOT_FOUND          A form or FormSet on the requested handle cannot
                                 be found with the requested FormId.

**/
EFI_STATUS
GetIfrBinaryData (
  IN  EFI_HII_HANDLE   Handle,
  IN OUT EFI_GUID      *FormSetGuid,
  OUT UINTN            *BinaryLength,
  OUT UINT8            **BinaryData
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINTN                        BufferSize;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  UINT32                       Offset;
  UINT32                       Offset2;
  BOOLEAN                      ReturnDefault;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       PackageHeader;

  OpCodeData = NULL;
  Package = NULL;
  ZeroMem (&PackageHeader, sizeof (EFI_HII_PACKAGE_HEADER));;

  //
  // if FormSetGuid is NULL or zero GUID, return first FormSet in the package list
  //
  if (FormSetGuid == NULL || CompareGuid (FormSetGuid, &gZeroGuid)) {
    ReturnDefault = TRUE;
  } else {
    ReturnDefault = FALSE;
  }

  //
  // Get HII PackageList
  //
  BufferSize = 0;
  HiiPackageList = NULL;
  Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, Handle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = AllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, Handle, &BufferSize, HiiPackageList);
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get Form package from this HII package List
  //
  Offset = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  Offset2 = 0;
  CopyMem (&PackageListLength, &HiiPackageList->PackageLength, sizeof (UINT32));

  while (Offset < PackageListLength) {
    Package = ((UINT8 *) HiiPackageList) + Offset;
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));

    if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Search FormSet in this Form Package
      //
      Offset2 = sizeof (EFI_HII_PACKAGE_HEADER);
      while (Offset2 < PackageHeader.Length) {
        OpCodeData = Package + Offset2;

        if (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP) {
          //
          // Check whether return default FormSet
          //
          if (ReturnDefault) {
            break;
          }

          //
          // FormSet GUID is specified, check it
          //
          if (CompareGuid (FormSetGuid, (EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER)))) {
            break;
          }
        }

        Offset2 += ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
      }

      if (Offset2 < PackageHeader.Length) {
        //
        // Target formset found
        //
        break;
      }
    }

    Offset += PackageHeader.Length;
  }

  if (Offset >= PackageListLength) {
    //
    // Form package not found in this Package List
    //
    gBS->FreePool (HiiPackageList);
    return EFI_NOT_FOUND;
  }

  if (ReturnDefault && FormSetGuid != NULL) {
    //
    // Return the default FormSet GUID
    //
    CopyMem (FormSetGuid, &((EFI_IFR_FORM_SET *) OpCodeData)->Guid, sizeof (EFI_GUID));
  }

  //
  // To determine the length of a whole FormSet IFR binary, one have to parse all the Opcodes
  // in this FormSet; So, here just simply copy the data from start of a FormSet to the end
  // of the Form Package.
  //
  *BinaryLength = PackageHeader.Length - Offset2;
  *BinaryData = AllocateCopyPool (*BinaryLength, OpCodeData);

  gBS->FreePool (HiiPackageList);

  if (*BinaryData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Initialize the internal data structure of a FormSet.

  @param  Handle                 PackageList Handle
  @param  FormSetGuid            GUID of a formset. If not specified (NULL or zero
                                 GUID), take the first FormSet found in package
                                 list.
  @param  FormSet                FormSet data structure.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The specified FormSet could not be found.

**/
EFI_STATUS
InitializeFormSet (
  IN  EFI_HII_HANDLE                   Handle,
  IN OUT EFI_GUID                      *FormSetGuid,
  OUT FORM_BROWSER_FORMSET             *FormSet
  )
{
  EFI_STATUS                Status;

  Status = GetIfrBinaryData (Handle, FormSetGuid, &FormSet->IfrBinaryLength, &FormSet->IfrBinaryData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FormSet->HiiHandle = Handle;
  CopyMem (&FormSet->Guid, FormSetGuid, sizeof (EFI_GUID));

  //
  // Parse the IFR binary OpCodes
  //
  Status = ParseOpCodes (FormSet);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  GetFormsetDefaultVarstoreId (FormSet);
  return Status;
}

/**
  Parse the Form Package and build a FORM_BROWSER_FORMSET structure.

  @param  UefiHiiHandle          PackageList Handle

  @return A pointer to FORM_BROWSER_FORMSET.

**/
FORM_BROWSER_FORMSET *
ParseFormSet (
  IN EFI_HII_HANDLE   UefiHiiHandle
  )
{
  FORM_BROWSER_FORMSET  *FormSet;
  EFI_GUID              FormSetGuid;
  EFI_STATUS            Status;
  
  FormSet = AllocateZeroPool (sizeof (FORM_BROWSER_FORMSET));    
  ASSERT (FormSet != NULL);

  CopyGuid (&FormSetGuid, &gZeroGuid);
  Status = InitializeFormSet (UefiHiiHandle, &FormSetGuid, FormSet);
  ASSERT_EFI_ERROR (Status);

  return FormSet;
}

