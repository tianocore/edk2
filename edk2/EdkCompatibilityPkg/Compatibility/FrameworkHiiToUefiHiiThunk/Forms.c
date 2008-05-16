/**@file
  This file contains the form processing code to the HII database.

Copyright (c) 2006 - 2008 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"
#include "UefiIfrDefault.h"
#include "OpcodeCreation.h"

EFI_STATUS
EFIAPI
HiiExportDatabase (
  IN     EFI_HII_PROTOCOL *This,
  IN     FRAMEWORK_EFI_HII_HANDLE    Handle,
  IN OUT UINTN            *BufferSize,
  OUT    VOID             *Buffer
  )
/*++

Routine Description:

  This function allows a program to extract a form or form package that has
  previously been registered with the EFI HII database.

Arguments:

Returns:

--*/
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

#pragma pack(push, 1)
typedef struct {
  EFI_HII_PACK_HEADER            PackageHeader;
  FRAMEWORK_EFI_IFR_FORM_SET     FormSet;
  FRAMEWORK_EFI_IFR_END_FORM_SET EndFormSet;
} FRAMEWORK_HII_FORMSET_TEMPLATE;
#pragma pack(pop)

FRAMEWORK_HII_FORMSET_TEMPLATE FormSetTemplate = {
  {
    sizeof (FRAMEWORK_HII_FORMSET_TEMPLATE),
    EFI_HII_IFR
  },
  {
    {
      FRAMEWORK_EFI_IFR_FORM_SET_OP,
      sizeof (FRAMEWORK_EFI_IFR_FORM_SET)
    },
    //Guid
    {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}},
    0,
    0,
    0,
    0,
    0,
    0
  },
  {
    {
      FRAMEWORK_EFI_IFR_END_FORM_SET_OP,
      sizeof (FRAMEWORK_EFI_IFR_END_FORM_SET)
    }
  }
};

EFI_STATUS
EFIAPI
HiiGetForms (
  IN     EFI_HII_PROTOCOL   *This,
  IN     FRAMEWORK_EFI_HII_HANDLE      Handle,
  IN     EFI_FORM_ID        FormId,
  IN OUT UINTN              *BufferLengthTemp,
  OUT    UINT8              *Buffer
  )
/*++

Routine Description:

  This function allows a program to extract a form or form package that has
  previously been registered with the EFI HII database.

Arguments:
  This         - A pointer to the EFI_HII_PROTOCOL instance.

  Handle       - Handle on which the form resides. Type FRAMEWORK_EFI_HII_HANDLE  is defined in
                 EFI_HII_PROTOCOL.NewPack() in the Packages section.

  FormId       - The ID of the form to return. If the ID is zero, the entire form package is returned.
                 Type EFI_FORM_ID is defined in "Related Definitions" below.

  BufferLength - On input, the length of the Buffer. On output, the length of the returned buffer, if
                 the length was sufficient and, if it was not, the length that is required to fit the
                 requested form(s).

  Buffer       - The buffer designed to receive the form(s).

Returns:

  EFI_SUCCESS           -  Buffer filled with the requested forms. BufferLength
                           was updated.

  EFI_INVALID_PARAMETER -  The handle is unknown.

  EFI_NOT_FOUND         -  A form on the requested handle cannot be found with the
                           requested FormId.

  EFI_BUFFER_TOO_SMALL  - The buffer provided was not large enough to allow the form to be stored.

--*/
{
  EFI_HII_THUNK_PRIVATE_DATA                *Private;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY  *MapEntry;
  FRAMEWORK_HII_FORMSET_TEMPLATE            *OutputFormSet;

  if (*BufferLengthTemp < sizeof(FRAMEWORK_HII_FORMSET_TEMPLATE)) {
    *BufferLengthTemp = sizeof(FRAMEWORK_HII_FORMSET_TEMPLATE);
    return EFI_BUFFER_TOO_SMALL;
  }
  
  Private = EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  MapEntry = FrameworkHiiHandleToMapDatabaseEntry (Private, Handle);

  if (MapEntry == NULL) {
    return EFI_NOT_FOUND;
  }

  OutputFormSet = (FRAMEWORK_HII_FORMSET_TEMPLATE *) Buffer;
  
  CopyMem (OutputFormSet, &FormSetTemplate, sizeof (FRAMEWORK_HII_FORMSET_TEMPLATE));
  CopyMem (&OutputFormSet->FormSet.Guid, &MapEntry->TagGuid, sizeof (EFI_GUID)); 
  
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
HiiGetDefaultImage (
  IN     EFI_HII_PROTOCOL            *This,
  IN     FRAMEWORK_EFI_HII_HANDLE    Handle,
  IN     UINTN                       DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST  **VariablePackList
  )
/*++

  Routine Description:

  This function allows a program to extract the NV Image
  that represents the default storage image

  Arguments:
    This             - A pointer to the EFI_HII_PROTOCOL instance.
    Handle           - The HII handle from which will have default data retrieved.
    UINTN            - Mask used to retrieve the default image.
    VariablePackList - Callee allocated, tightly-packed, link list data
                         structure that contain all default varaible packs
                         from the Hii Database.

  Returns:
    EFI_NOT_FOUND         - If Hii database does not contain any default images.
    EFI_INVALID_PARAMETER - Invalid input parameter.
    EFI_SUCCESS           - Operation successful.

--*/
{
  LIST_ENTRY        *UefiDefaults;
  EFI_HII_HANDLE    UefiHiiHandle;
  EFI_STATUS        Status;
  EFI_HII_THUNK_PRIVATE_DATA *Private;

  Private = EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  UefiHiiHandle = FrameworkHiiHandleToUefiHiiHandle (Private, Handle);
  if (UefiHiiHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  UefiDefaults = NULL;
  Status = UefiIfrGetBufferTypeDefaults (UefiHiiHandle, &UefiDefaults);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = UefiDefaultsToFrameworkDefaults (UefiDefaults, DefaultMask, VariablePackList);

Done:
  FreeDefaultList (UefiDefaults);
  
  return Status;
}

EFI_STATUS
ThunkUpdateFormCallBack (
  IN       EFI_HANDLE                                CallbackHandle,
  IN CONST HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY  *HandleMapEntry
  )
{
  EFI_STATUS                                Status;
  EFI_FORM_CALLBACK_PROTOCOL                *FrameworkFormCallbackProtocol;
  EFI_HII_CONFIG_ACCESS_PROTOCOL            *ConfigAccessProtocol;
  EFI_HANDLE                                UefiDriverHandle;
  HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE *ConfigAccessProtocolInstance;
  
  Status = gBS->HandleProtocol (
                   CallbackHandle,
                   &gEfiFormCallbackProtocolGuid,
                   (VOID **) &FrameworkFormCallbackProtocol
                   );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }
  
  Status = mHiiDatabase->GetPackageListHandle (
                                        mHiiDatabase,
                                        HandleMapEntry->UefiHiiHandle,
                                        &UefiDriverHandle
                                        );
  ASSERT_EFI_ERROR (Status);
  Status = gBS->HandleProtocol (
                   UefiDriverHandle,
                   &gEfiHiiConfigAccessProtocolGuid,
                   (VOID **) &ConfigAccessProtocol
                   );
  ASSERT_EFI_ERROR (Status);
  
  ConfigAccessProtocolInstance = HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE_FROM_PROTOCOL (ConfigAccessProtocol);
  
  ConfigAccessProtocolInstance->FrameworkFormCallbackProtocol = FrameworkFormCallbackProtocol;

  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
GetPackageDataFromPackageList (
  IN  EFI_HII_PACKAGE_LIST_HEADER *HiiPackageList,
  IN  UINT32                      PackageIndex,
  OUT UINT32                      *BufferLen,
  OUT EFI_HII_PACKAGE_HEADER      **Buffer
  )
{
  UINT32                        Index;
  EFI_HII_PACKAGE_HEADER        *Package;
  UINT32                        Offset;
  UINT32                        PackageListLength;
  EFI_HII_PACKAGE_HEADER        PackageHeader = {0, 0};

  ASSERT(HiiPackageList != NULL);

  if ((BufferLen == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Package = NULL;
  Index   = 0;
  Offset  = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  CopyMem (&PackageListLength, &HiiPackageList->PackageLength, sizeof (UINT32));
  while (Offset < PackageListLength) {
    Package = (EFI_HII_PACKAGE_HEADER *) (((UINT8 *) HiiPackageList) + Offset);
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
    if (Index == PackageIndex) {
      break;
    }
    Offset += PackageHeader.Length;
    Index++;
  }
  if (Offset >= PackageListLength) {
    //
    // no package found in this Package List
    //
    return EFI_NOT_FOUND;
  }

  *BufferLen = PackageHeader.Length;
  *Buffer    = Package;
  return EFI_SUCCESS;
}

/**
  Check if Label exist in the IFR form package.

  @param 

**/
EFI_STATUS
LocateLabel (
  IN CONST EFI_HII_PACKAGE_HEADER *Package,
  IN       EFI_FORM_LABEL          Label,
  OUT      EFI_GUID                *FormsetGuid,
  OUT      EFI_FORM_ID             *FormId
  )
{
  UINTN                     Offset;
  EFI_IFR_OP_HEADER         *IfrOpHdr;
  UINT8                     ExtendOpCode;
  UINT16                    LabelNumber;
  EFI_GUID                  InternalFormSetGuid;
  EFI_FORM_ID               InternalFormId;
  BOOLEAN                   GetFormSet;
  BOOLEAN                   GetForm;

  IfrOpHdr   = (EFI_IFR_OP_HEADER *)((UINT8 *) Package + sizeof (EFI_HII_PACKAGE_HEADER));
  Offset     = sizeof (EFI_HII_PACKAGE_HEADER);

  InternalFormId= 0;
  ZeroMem (&InternalFormSetGuid, sizeof (EFI_GUID));
  GetFormSet = FALSE;
  GetForm    = FALSE;

  while (Offset < Package->Length) {
    switch (IfrOpHdr->OpCode) {
    case EFI_IFR_FORM_SET_OP :
      CopyMem (&InternalFormSetGuid, &((EFI_IFR_FORM_SET *) IfrOpHdr)->Guid, sizeof (EFI_GUID));
      GetFormSet = TRUE;
      break;

    case EFI_IFR_FORM_OP:
      CopyMem (&InternalFormId, &((EFI_IFR_FORM *) IfrOpHdr)->FormId, sizeof (EFI_FORM_ID));
      GetForm = TRUE;
      break;

    case EFI_IFR_GUID_OP :
      ExtendOpCode = ((EFI_IFR_GUID_LABEL *) IfrOpHdr)->ExtendOpCode;
      
      if (ExtendOpCode != EFI_IFR_EXTEND_OP_LABEL) {
        //
        // Go to the next Op-Code
        //
        Offset   += IfrOpHdr->Length;
        IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
        continue;
      }
      
      CopyMem (&LabelNumber, &((EFI_IFR_GUID_LABEL *)IfrOpHdr)->Number, sizeof (UINT16));
      if (LabelNumber == Label) {
        ASSERT (GetForm && GetFormSet);
        CopyGuid (FormsetGuid, &InternalFormSetGuid);
        *FormId = InternalFormId;
        return EFI_SUCCESS;
      }
      

      break;
    default :
      break;
    }

    //
    // Go to the next Op-Code
    //
    Offset   += IfrOpHdr->Length;
    IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
  }

  return EFI_NOT_FOUND;
}

/**
  Find the first EFI_FORM_LABEL in FormSets for a given EFI_HII_HANLDE defined.
  
  EFI_FORM_LABEL is a specific to Tiano implementation. The current implementation
  does not restrict labels with same label value to be duplicated in either FormSet 
  scope or Form scope. This function will only locate the FIRST EFI_FORM_LABEL
  with value as the same as the input Label in the Formset registered with UefiHiiHandle. The FormSet GUID 
  and Form ID is returned if such Label is found.

  
  @retval EFI_INVALID_PARAMETER If UefiHiiHandle is not a valid handle.
  @retval EFI_NOT_FOUND   The package list identified by UefiHiiHandle deos not contain FormSet or
                                         There is no Form ID with value Label found in all Form Sets in the pacakge
                                         list.
                                         
  @retval EFI_SUCCESS       The first found Form ID is returned in FormId.
**/
EFI_STATUS
ThunkLocateFormId (
  IN  EFI_HII_HANDLE Handle,
  IN  EFI_FORM_LABEL Label,
  OUT EFI_GUID       *FormsetGuid,
  OUT EFI_FORM_ID    *FormId
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT32                       Index;
  UINTN                        BufferSize;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  EFI_HII_PACKAGE_HEADER       *Package;
  UINT32                       PackageLength;

  BufferSize = 0;
  HiiPackageList   = NULL;
  Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, Handle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = AllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, Handle, &BufferSize, HiiPackageList);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  for (Index = 0; ; Index++) {
    Status = GetPackageDataFromPackageList (HiiPackageList, Index, &PackageLength, &Package);
    if (!EFI_ERROR (Status)) {
      CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
      if (PackageHeader.Type == EFI_HII_PACKAGE_FORM) {
        Status = LocateLabel (Package, Label, FormsetGuid, FormId);
        if (!EFI_ERROR(Status)) {
          break;
        }
      }
    } else {
      break;
    }
  }

  
Done:
  FreePool (HiiPackageList);
  
  return Status;
}
EFI_STATUS
EFIAPI
HiiUpdateForm (
  IN EFI_HII_PROTOCOL       *This,
  IN FRAMEWORK_EFI_HII_HANDLE          Handle,
  IN EFI_FORM_LABEL         Label,
  IN BOOLEAN                AddData,
  IN FRAMEWORK_EFI_HII_UPDATE_DATA    *Data
  )
/*++

Routine Description:
  This function allows the caller to update a form that has
  previously been registered with the EFI HII database.

Arguments:
  Handle     - Hii Handle associated with the Formset to modify
  Label      - Update information starting immediately after this label in the IFR
  AddData    - If TRUE, add data.  If FALSE, remove data
  Data       - If adding data, this is the pointer to the data to add

Returns:
  EFI_SUCCESS - Update success.
  Other       - Update fail.

--*/
{
  EFI_STATUS                                Status;
  EFI_HII_THUNK_PRIVATE_DATA                *Private;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY  *HandleMapEntry;
  EFI_HII_UPDATE_DATA                       *UefiHiiUpdateData;
  EFI_HII_HANDLE                            UefiHiiHandle;
  EFI_GUID                                  FormsetGuid;
  EFI_FORM_ID                               FormId;

  Status = EFI_SUCCESS;

  Private = EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  HandleMapEntry = FrameworkHiiHandleToMapDatabaseEntry (Private, Handle);

  if (HandleMapEntry == NULL) {
    return EFI_NOT_FOUND;
  }
  
  if (Data->FormSetUpdate) {
    Status = ThunkUpdateFormCallBack ((EFI_HANDLE) (UINTN) Data->FormCallbackHandle, HandleMapEntry);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (HandleMapEntry->IsPackageListWithOnlyStringPackages) {
    UefiHiiHandle = TagGuidToUefiIfrHiiHandle (Private, &HandleMapEntry->TagGuid);
  
    if (UefiHiiHandle == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    UefiHiiHandle = HandleMapEntry->UefiHiiHandle;
  }

  UefiHiiUpdateData = NULL;

  if (AddData) {
    if (Data->DataCount != 0) {
      
      Status = ThunkFrameworkUpdateDataToUefiUpdateData (Data, AddData, &UefiHiiUpdateData);
      ASSERT_EFI_ERROR (Status);

      Status = ThunkLocateFormId (UefiHiiHandle, Label, &FormsetGuid, &FormId);
      ASSERT_EFI_ERROR (Status);

      Status = IfrLibUpdateForm (UefiHiiHandle, &FormsetGuid, FormId, Label, AddData, UefiHiiUpdateData);
      ASSERT_EFI_ERROR (Status);
      
    } 
  } else {
    Status = ThunkLocateFormId (UefiHiiHandle, Label, &FormsetGuid, &FormId);
    ASSERT_EFI_ERROR (Status);

    //
    // Delete Opcode starting from Labe in FormId found
    //
    UefiHiiUpdateData = AllocateZeroPool (sizeof (*UefiHiiUpdateData));
	
    Status = IfrLibUpdateForm (UefiHiiHandle, &FormsetGuid, FormId, Label, FALSE, UefiHiiUpdateData);
    ASSERT_EFI_ERROR (Status);
  }

  if (UefiHiiUpdateData != NULL) {
    SafeFreePool (UefiHiiUpdateData->Data);
    SafeFreePool (UefiHiiUpdateData);
  }

  return Status;
}
