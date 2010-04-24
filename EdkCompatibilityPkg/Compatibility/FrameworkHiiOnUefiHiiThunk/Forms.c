/** @file
  This file contains the form processing code to the HII database.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"
#include "UefiIfrDefault.h"

//
// This structure is only intended to be used in this file.
//
#pragma pack(1)
typedef struct {
  EFI_HII_PACK_HEADER            PackageHeader;
  FRAMEWORK_EFI_IFR_FORM_SET     FormSet;
  EFI_IFR_END_FORM_SET           EndFormSet;
} FW_HII_FORMSET_TEMPLATE;
#pragma pack()

FW_HII_FORMSET_TEMPLATE FormSetTemplate = {
  {
    sizeof (FW_HII_FORMSET_TEMPLATE),
    EFI_HII_IFR
  },
  {
    {
      FRAMEWORK_EFI_IFR_FORM_SET_OP,
      sizeof (FRAMEWORK_EFI_IFR_FORM_SET)
    },
    {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}}, //Guid
    0,
    0,
    0,
    0,
    0,
    0
  },
  {
    {
      EFI_IFR_END_FORM_SET_OP,
      sizeof (EFI_IFR_END_FORM_SET)
    }
  }
};


EFI_GUID  mTianoHiiIfrGuid              = EFI_IFR_TIANO_GUID;

/**

  This thunk module only handles UEFI HII packages. The caller of this function 
  won't be able to parse the content. Therefore, it is not supported.
  
  This function will ASSERT and return EFI_UNSUPPORTED.

  @param This            N.A.
  @param Handle          N.A.
  @param BufferSize      N.A.
  @param Buffer          N.A.

  @retval EFI_UNSUPPORTED

**/
EFI_STATUS
EFIAPI
HiiExportDatabase (
  IN     EFI_HII_PROTOCOL *This,
  IN     FRAMEWORK_EFI_HII_HANDLE    Handle,
  IN OUT UINTN            *BufferSize,
  OUT    VOID             *Buffer
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}


/**
  This function allows a program to extract a form or form package that has
  previously been registered with the EFI HII database.

  In this thunk module, this function will create a IFR Package with only 
  one Formset. Effectively, only the GUID of the Formset is updated and return
  in this IFR package to caller. This is enable the Framework modules which call 
  a API named GetStringFromToken. GetStringFromToken retieves a String based on
  a String Token from a Package List known only by the Formset GUID.
  


  @param This             A pointer to the EFI_HII_PROTOCOL instance.
  @param Handle           Handle on which the form resides. Type FRAMEWORK_EFI_HII_HANDLE  is defined in
                          EFI_HII_PROTOCOL.NewPack() in the Packages section.
  @param FormId           Ignored by this implementation.
  @param BufferLengthTemp On input, the size of input buffer. On output, it
                          is the size of FW_HII_FORMSET_TEMPLATE.
  @param Buffer           The buffer designed to receive the form(s).

  @retval  EFI_SUCCESS            Buffer filled with the requested forms. BufferLength
                                  was updated.
  @retval  EFI_INVALID_PARAMETER  The handle is unknown.
  @retval  EFI_NOT_FOUND          A form on the requested handle cannot be found with the
                                  requested FormId.
  @retval  EFI_BUFFER_TOO_SMALL   The buffer provided was not large enough to allow the form to be stored.

**/
EFI_STATUS
EFIAPI
HiiGetForms (
  IN     EFI_HII_PROTOCOL             *This,
  IN     FRAMEWORK_EFI_HII_HANDLE     Handle,
  IN     EFI_FORM_ID                  FormId,
  IN OUT UINTN                        *BufferLengthTemp,
  OUT    UINT8                        *Buffer
  )
{
  HII_THUNK_PRIVATE_DATA                *Private;
  HII_THUNK_CONTEXT  *ThunkContext;
  FW_HII_FORMSET_TEMPLATE            *OutputFormSet;

  if (*BufferLengthTemp < sizeof(FW_HII_FORMSET_TEMPLATE)) {
    *BufferLengthTemp = sizeof(FW_HII_FORMSET_TEMPLATE);
    return EFI_BUFFER_TOO_SMALL;
  }
  
  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  ThunkContext = FwHiiHandleToThunkContext (Private, Handle);

  if (ThunkContext == NULL) {
    return EFI_NOT_FOUND;
  }

  OutputFormSet = (FW_HII_FORMSET_TEMPLATE *) Buffer;
  
  CopyMem (OutputFormSet, &FormSetTemplate, sizeof (FW_HII_FORMSET_TEMPLATE));
  CopyMem (&OutputFormSet->FormSet.Guid, &ThunkContext->TagGuid, sizeof (EFI_GUID)); 

  if (ThunkContext->FormSet != NULL) {
    OutputFormSet->FormSet.Class = ThunkContext->FormSet->Class;
    OutputFormSet->FormSet.SubClass = ThunkContext->FormSet->SubClass;
    OutputFormSet->FormSet.Help     = ThunkContext->FormSet->Help;
    OutputFormSet->FormSet.FormSetTitle = ThunkContext->FormSet->FormSetTitle;
  }

  return EFI_SUCCESS;
}


/**

  This function allows a program to extract the NV Image
  that represents the default storage image


  @param This            A pointer to the EFI_HII_PROTOCOL instance.
  @param Handle          The HII handle from which will have default data retrieved.
                         UINTN            - Mask used to retrieve the default image.
  @param DefaultMask     EDES_TODO: Add parameter description
  @param VariablePackList Callee allocated, tightly-packed, link list data
                         structure that contain all default varaible packs
                         from the Hii Database.

  @retval  EFI_NOT_FOUND          If Hii database does not contain any default images.
  @retval  EFI_INVALID_PARAMETER  Invalid input parameter.
  @retval  EFI_SUCCESS            Operation successful.

**/
EFI_STATUS
EFIAPI
HiiGetDefaultImage (
  IN     EFI_HII_PROTOCOL            *This,
  IN     FRAMEWORK_EFI_HII_HANDLE    Handle,
  IN     UINTN                       DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST  **VariablePackList
  )
{
  LIST_ENTRY        *UefiDefaults;
  EFI_STATUS        Status;
  HII_THUNK_PRIVATE_DATA *Private;
  HII_THUNK_CONTEXT *ThunkContext;

  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  ThunkContext = FwHiiHandleToThunkContext (Private, Handle);
  if (ThunkContext == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  UefiDefaults = NULL;
  Status = UefiIfrGetBufferTypeDefaults (ThunkContext, &UefiDefaults);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = UefiDefaultsToFwDefaults (UefiDefaults, DefaultMask, ThunkContext->FormSet->DefaultVarStoreId, VariablePackList);

Done:
  FreeDefaultList (UefiDefaults);
  
  return Status;
}

/**
  This function update the FormCallbackProtocol cached in Config Access
  private context data.

  @param CallbackHandle  The EFI Handle on which the Framework FormCallbackProtocol is 
                         installed.
  @param ThunkContext    The Thunk Context.

  @retval EFI_SUCCESS             The update is successful.
  @retval EFI_INVALID_PARAMETER   If no Framework FormCallbackProtocol is located on CallbackHandle.

**/
EFI_STATUS
UpdateFormCallBack (
  IN       EFI_HANDLE                                CallbackHandle,
  IN CONST HII_THUNK_CONTEXT                         *ThunkContext
  )
{
  EFI_STATUS                                Status;
  EFI_FORM_CALLBACK_PROTOCOL                *FormCallbackProtocol;
  EFI_HII_CONFIG_ACCESS_PROTOCOL            *ConfigAccessProtocol;
  EFI_HANDLE                                UefiDriverHandle;
  CONFIG_ACCESS_PRIVATE                     *ConfigAccessPrivate;
  
  Status = gBS->HandleProtocol (
                   CallbackHandle,
                   &gEfiFormCallbackProtocolGuid,
                   (VOID **) &FormCallbackProtocol
                   );
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }
  
  Status = mHiiDatabase->GetPackageListHandle (
                                        mHiiDatabase,
                                        ThunkContext->UefiHiiHandle,
                                        &UefiDriverHandle
                                        );
  ASSERT_EFI_ERROR (Status);
  Status = gBS->HandleProtocol (
                   UefiDriverHandle,
                   &gEfiHiiConfigAccessProtocolGuid,
                   (VOID **) &ConfigAccessProtocol
                   );
  ASSERT_EFI_ERROR (Status);
  
  ConfigAccessPrivate = CONFIG_ACCESS_PRIVATE_FROM_PROTOCOL (ConfigAccessProtocol);
  
  ConfigAccessPrivate->FormCallbackProtocol = FormCallbackProtocol;

  return EFI_SUCCESS;
}


/**
  Get the package data from the Package List.

  @param HiiPackageList  Package List.
  @param PackageIndex    The index of the Package in the Package List.
  @param BufferLen       The Length of the Pacage data.
  @param Buffer          On output, the Package data.

  @return EFI_NOT_FOUND  No Package is found for PackageIndex.
  @return EFI_SUCCESS    The package data is returned.

**/
EFI_STATUS
GetPackageData (
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
  EFI_HII_PACKAGE_HEADER        PackageHeader;

  ASSERT(HiiPackageList != NULL);

  if ((BufferLen == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&PackageHeader, sizeof (PackageHeader));
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
  Check if Label exist in the IFR form package and return the FormSet GUID
  and Form ID.

  @param Package      The Package Header.
  @param Label        The Label ID.
  @param FormsetGuid  Returns the FormSet GUID.
  @param FormId       Returns the Form ID.

  @retval EFI_SUCCESS     The FORM ID is found.
  @retval EFI_NOT_FOUND   The FORM ID is not found.
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
  EFI_GUID                  InternalFormSetGuid;
  EFI_FORM_ID               InternalFormId;
  BOOLEAN                   GetFormSet;
  BOOLEAN                   GetForm;
  EFI_IFR_GUID_LABEL        *LabelOpcode;

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
      LabelOpcode = (EFI_IFR_GUID_LABEL *) IfrOpHdr;
      //
      // If it is an Label opcode.
      //
      if ((LabelOpcode->ExtendOpCode == EFI_IFR_EXTEND_OP_LABEL) && (CompareMem (&LabelOpcode->Guid, &mTianoHiiIfrGuid, sizeof (EFI_GUID)) == 0)) {
        if (CompareMem (&Label, &LabelOpcode->Number, sizeof (UINT16)) == 0) {
          ASSERT (GetForm && GetFormSet);
          CopyGuid (FormsetGuid, &InternalFormSetGuid);
          *FormId = InternalFormId;
          return EFI_SUCCESS;
        }
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

  @param Handle       Uefi Hii Handle to be searched.
  @param Label        The first Label ID to be found.
  @param FormsetGuid  The matched FormSet GUID.
  @param FormId       The matched Form ID.
  
  @retval EFI_INVALID_PARAMETER If UefiHiiHandle is not a valid handle.
  @retval EFI_NOT_FOUND         The package list identified by UefiHiiHandle deos not contain FormSet or
                                Form ID with value Label found in all Form Sets in the pacakge list.
  @retval EFI_SUCCESS           The first found Form ID is returned in FormId.
**/
EFI_STATUS
LocateFormId (
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
    Status = GetPackageData (HiiPackageList, Index, &PackageLength, &Package);
    if (!EFI_ERROR (Status)) {
      CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
      if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
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

/**
  This function allows the caller to update a form that has
  previously been registered with the EFI HII database.


  @param This            EDES_TODO: Add parameter description
  @param Handle          Hii Handle associated with the Formset to modify
  @param Label           Update information starting immediately after this label in the IFR
  @param AddData         If TRUE, add data.  If FALSE, remove data
  @param Data            If adding data, this is the pointer to the data to add

  @retval  EFI_SUCCESS  Update success.
  @retval  Other        Update fail.

**/
EFI_STATUS
EFIAPI
HiiThunkUpdateForm (
  IN EFI_HII_PROTOCOL                  *This,
  IN FRAMEWORK_EFI_HII_HANDLE          Handle,
  IN EFI_FORM_LABEL                    Label,
  IN BOOLEAN                           AddData,
  IN EFI_HII_UPDATE_DATA               *Data
  )
{
  EFI_STATUS                                Status;
  HII_THUNK_PRIVATE_DATA                    *Private;
  HII_THUNK_CONTEXT                         *ThunkContext;
  EFI_HII_HANDLE                            UefiHiiHandle;
  EFI_GUID                                  FormsetGuid;
  EFI_FORM_ID                               FormId;
  EFI_TPL                                   OldTpl;
  VOID                                      *StartOpCodeHandle;
  VOID                                      *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL                        *StartLabel;
  EFI_IFR_GUID_LABEL                        *EndLabel;

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  
  mInFrameworkUpdatePakcage = TRUE;
  Status = EFI_SUCCESS;

  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  ThunkContext = FwHiiHandleToThunkContext (Private, Handle);

  if (ThunkContext == NULL) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }
  
  if (Data->FormSetUpdate) {
    Status = UpdateFormCallBack ((EFI_HANDLE) (UINTN) Data->FormCallbackHandle, ThunkContext);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  if (ThunkContext->IfrPackageCount == 0) {
    ASSERT (FALSE);
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  } else {
    UefiHiiHandle = ThunkContext->UefiHiiHandle;
  }

  Status = LocateFormId (UefiHiiHandle, Label, &FormsetGuid, &FormId);
  if (EFI_ERROR (Status)) {
    //
    // Can't find the label.
    //
    goto Done;
  }

  //
  // Init OpCode Handle
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = Label;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = 0xffff;

  if (AddData) {
    if (Data->DataCount != 0) {

      ThunkContext = UefiHiiHandleToThunkContext (Private, UefiHiiHandle);
      ASSERT (ThunkContext != NULL);
      Status = FwUpdateDataToUefiUpdateData (ThunkContext, Data, StartOpCodeHandle);
      ASSERT_EFI_ERROR (Status);

      Status = HiiUpdateForm (UefiHiiHandle, &FormsetGuid, FormId, StartOpCodeHandle, NULL);
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    //
    // Delete Opcode starting from Labe in FormId found
    //
    Status = HiiUpdateForm (UefiHiiHandle, &FormsetGuid, FormId, StartOpCodeHandle, EndOpCodeHandle);
    ASSERT_EFI_ERROR (Status);
  }

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

Done:

  mInFrameworkUpdatePakcage = FALSE; 

  gBS->RestoreTPL (OldTpl);

  return Status;
}
