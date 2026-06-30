/** @file
  Dynamic Hii Platform Configuration Form Generator.

  This is the top level Form that references all other
  dynamically generated HII forms.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>

// Module specific include files.
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <HiiFormsGenerator.h>
#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/MdeModuleHii.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HiiFormsLib.h>
#include <HiiFormGeneratorLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Uefi/UefiInternalFormRepresentation.h>

#include "PlatformConfig.h"

static EFI_HANDLE       mDevHandle;
static EFI_STRING_ID    mStringId   = 1;
static EFI_FORM_ID      mFormId     = 1;
static EFI_QUESTION_ID  mQuestionId = DYN_HII_PLAT_CFG_QUESTION_ID_START;

/** Add all Question objects to the Form

  Add all the Question objects, cross-links to other forms in this case,
  to the Form.

  @param [in]  Priv      Pointer to the private structure of the form.

  @retval  EFI_SUCCESS  If the Questions were added successfully or other
                        failure codes as returned by the form generation API's.

**/
static
EFI_STATUS
EFIAPI
PopulateQuestionObjs (
  IN     HII_PLAT_CFG_GEN_PRIV  *Priv
  )
{
  EFI_STRING               String;
  EFI_STRING_ID            Prompt;
  EFI_STRING_ID            Help;
  EFI_HII_HANDLE           HiiHandle;
  EFI_IFR_FORM_SET         *Buffer;
  UINTN                    BufferSize;
  EFI_STATUS               Status;
  HII_FORM_HANDLE          *Entry;
  DYN_HII_QUESTION_DATA    QData;
  DYN_HII_REF_DATA         *Ref;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;

  Buffer = NULL;
  String = NULL;
  Entry  = (HII_FORM_HANDLE *)GetFirstNode (Priv->HiiHandleList);
  while (!IsNull (Priv->HiiHandleList, &Entry->List)) {
    HiiHandle = Entry->HiiHandle;
    Status    = HiiGetFormSetFromHiiHandle (HiiHandle, &Buffer, &BufferSize);
    if (EFI_ERROR (Status)) {
      Entry = (HII_FORM_HANDLE *)GetNextNode (Priv->HiiHandleList, &Entry->List);
      continue;
    }

    Prompt = mStringId++;
    String = HiiGetString (HiiHandle, Buffer->FormSetTitle, NULL);
    Status = DynHiiAddString (&Priv->StrPkgInfo->StringList, Prompt, String);
    if (EFI_ERROR (Status)) {
      goto out;
    }

    FreePool (String);

    Help   = mStringId++;
    String = HiiGetString (HiiHandle, Buffer->Help, NULL);
    Status = DynHiiAddString (&Priv->StrPkgInfo->StringList, Help, String);
    if (EFI_ERROR (Status)) {
      goto out;
    }

    FreePool (String);

    ZeroMem (&QData, sizeof (DYN_HII_QUESTION_DATA));
    QuestionHdr                = &QData.QuestionHdr;
    QuestionHdr->Header.Prompt = Prompt;
    QuestionHdr->Header.Help   = Help;
    QuestionHdr->QuestionId    = mQuestionId++;

    Ref          = &QData.Question.Ref;
    Ref->RefType = DynHiiRef4Op;
    CopyMem (&Ref->FormSetGuid, &Buffer->Guid, sizeof (EFI_GUID));

    Status = DynHiiAddStatement (
               DynHiiGetForm (Priv->Formset, 1),
               DynHiiQtRef,
               &QData
               );
    if (EFI_ERROR (Status)) {
      goto out;
    }

    FreePool (Buffer);
    Buffer     = NULL;
    String     = NULL;
    BufferSize = 0;

    Entry = (HII_FORM_HANDLE *)GetNextNode (Priv->HiiHandleList, &Entry->List);
  }

  return EFI_SUCCESS;

out:
  if (Buffer != NULL) {
    FreePool (Buffer);
  }

  if (String != NULL) {
    FreePool (String);
  }

  return Status;
}

/** Add the Form object to the Formset

  Add the form to the given formset.

  @param [in]  Priv      Pointer to the private structure of the form.

  @retval  EFI_SUCCESS  If the Form was added successfully or other failure
                        codes as returned by the form generation API's.

**/
static
EFI_STATUS
SetupFormsetForms (
  IN     HII_PLAT_CFG_GEN_PRIV  *Priv
  )
{
  EFI_STATUS     Status;
  EFI_STRING_ID  Title;

  Title  = mStringId++;
  Status = DynHiiAddString (
             &Priv->StrPkgInfo->StringList,
             Title,
             L"Dynamic Platform Configuration"
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return DynHiiAddForm (Priv->Formset, mFormId++, Title);
}

/** Create the Formset object for the form.

  Add the Formset object for the form.

  @param [in]  Priv      Pointer to the private structure of the form.

  @retval  EFI_SUCCESS  If the Formset was added successfully or other failure
                        codes as returned by the form generation API's.

**/
static
EFI_STATUS
SetupFormset (
  IN     HII_PLAT_CFG_GEN_PRIV  *Priv
  )
{
  EFI_STATUS      Status;
  EFI_STRING_ID   Title;
  EFI_STRING_ID   Help;
  CONST EFI_GUID  FormsetGuid = DYNAMIC_PLATFORM_CFG_GUID;
  CONST EFI_GUID  ClassGuid   = EFI_HII_PLATFORM_SETUP_FORMSET_GUID;

  Title  = mStringId++;
  Status = DynHiiAddString (
             &Priv->StrPkgInfo->StringList,
             Title,
             L"Dynamic Platform Configuration"
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Help   = mStringId++;
  Status = DynHiiAddString (
             &Priv->StrPkgInfo->StringList,
             Help,
             L"Dynamic Platform Configuration"
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return DynHiiCreateFormSet (
           &FormsetGuid,
           &ClassGuid,
           Title,
           Help,
           1,
           &Priv->Formset
           );
}

/** Initialize the string package info structure.

  Initialize the structure that contains information needed for the generation
  of the string package.

  @param [out]  StrPkgInfo      Pointer to the string package info structure.

  @retval  EFI_SUCCESS  If the structure was successfully initialized or other
                        failure codes as returned by the string generation
                        API's.

**/
static
EFI_STATUS
InitStringPkgInfo (
  OUT DYN_HII_STR_PKG_INFO  **StrPkgInfo
  )
{
  EFI_STATUS            Status;
  EFI_STRING_ID         LangStrId;
  DYN_HII_STR_PKG_INFO  *Info;

  LangStrId = mStringId++;

  Status = DynHiiInitStringPkgInfo (
             "en-US",
             LangStrId,
             StrPkgInfo
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Info = *StrPkgInfo;

  return DynHiiAddString (
           &Info->StringList,
           LangStrId,
           L"English"
           );
}

/** Initialize the form's private data structure.

  Allocate and initialize the form's private data structure.

  @param [in]  HiiHandleList    Pointer to the list of HII handles.
  @param [out] PlatCfgPriv      Pointer to the private structure of the form.

  @retval  EFI_SUCCESS  If the structure was initialized successfully or other
                        failure codes as returned by the string generation
                        API's.

**/
static
EFI_STATUS
InitFormPrivateData (
  IN     LIST_ENTRY             *HiiHandleList,
  OUT    HII_PLAT_CFG_GEN_PRIV  **PlatCfgPriv
  )
{
  EFI_STATUS             Status;
  HII_PLAT_CFG_GEN_PRIV  *Priv;

  Priv = AllocateZeroPool (sizeof (*Priv));
  if (Priv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Priv->Signature     = DYN_HII_PLAT_CFG_GEN_SIGNATURE;
  Priv->HiiHandleList = HiiHandleList;

  Status = InitStringPkgInfo (&Priv->StrPkgInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *PlatCfgPriv = Priv;

  return EFI_SUCCESS;
}

/** Construct Platform Configuration Hii Form

  This function is invoked for building the HII Platform Configuration
  Form. In turn, this calls the HII Form and String Generation API's
  for building the form and string package, and then adds these
  packages to the HII database through a call to HiiAddPackages().

  If this function allocates any resources then they must be freed
  in the FreeXXXXRes function.

  @param [in]  This            Pointer to the HII forms generator.
  @param [in]  HiiFormsInfo    Pointer to the HII forms information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [in]  HiiHandleList   List of all Hii handles to which this
                               form creates references.
  @param [out] HiiHandle       Hii handle of this form.

  @retval EFI_SUCCESS            Form generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
static
EFI_STATUS
EFIAPI
BuildHiiPlatCfgForm (
  IN        HII_FORMS_GENERATOR                            *This,
  IN  CONST CM_HII_FORMS_OBJ_INFO                  *CONST  HiiFormsInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN  CONST LIST_ENTRY                                     *HiiHandleList,
  OUT       EFI_HII_HANDLE                                 *HiiHandle
  )
{
  EFI_STATUS             Status;
  HII_PLAT_CFG_GEN_PRIV  *Priv;
  CONST EFI_GUID         FormsetGuid = DYNAMIC_PLATFORM_CFG_GUID;

  ASSERT (HiiHandleList != NULL);

  Status = InitFormPrivateData (
             (LIST_ENTRY *)HiiHandleList,
             (HII_PLAT_CFG_GEN_PRIV **)&This->Priv
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to Initialise the private data structure for the form."
      " Status = %r\n",
      __func__,
      Status
      ));
  }

  Priv = This->Priv;

  Status = SetupFormset (Priv);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to create the Formset."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  Status = SetupFormsetForms (Priv);
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to add the Form information."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  Status = PopulateQuestionObjs (Priv);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to populate the Question information."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  Status = DynHiiGenerateFormPackage (
             Priv->Formset,
             &Priv->IfrBuffer
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to serialize the IFR Byte Buffer."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  Status = DynHiiGenerateStringPackage (Priv->StrPkgInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to serialize the String Package Buffer."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  /// Add the template data to the Hii Package list
  *HiiHandle = HiiAddPackages (
                 &FormsetGuid,
                 mDevHandle,
                 Priv->IfrBuffer->Data,
                 Priv->StrPkgInfo->StrPkgBuf,
                 NULL
                 );
  if (*HiiHandle == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to add packages to Hii Database\n",
      __func__
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  return Status;
}

/** Free up all resources associated with this form.

  Free up all resources associated with this form.

  @param [in]  Priv      Pointer to the private structure of the form.

**/
static
VOID
DeInitPrivateData (
  IN HII_PLAT_CFG_GEN_PRIV  *Priv
  )
{
  if (Priv->Formset != NULL) {
    DynHiiFreeFormSet (Priv->Formset);
  }

  if (Priv->IfrBuffer != NULL) {
    DynHiiFreeFormPackage (Priv->IfrBuffer);
  }

  if (Priv->StrPkgInfo != NULL) {
    DynHiiFreeStrings (&Priv->StrPkgInfo->StringList);
    DynHiiFreeStringPackage (Priv->StrPkgInfo);
  }

  FreePool (Priv);
}

/** Free up all resources associated with this form.

  Free up all resources associated with this form. In addition remove
  the Hii Handle from the Hii database.

  @param [in]  This            Pointer to the HII forms generator.
  @param [in]  HiiFormsInfo    Pointer to the HII forms information.
  @param [in]  HiiHandle       Hii handle of this form.

  @retval The resources were freed successfully.

**/
static
EFI_STATUS
EFIAPI
FreeHiiPlatCfgForm (
  IN        HII_FORMS_GENERATOR                            *This,
  IN  CONST CM_HII_FORMS_OBJ_INFO                  *CONST  HiiFormsInfo,
  IN  EFI_HII_HANDLE                                       *HiiHandle
  )
{
  ASSERT (This != NULL);
  ASSERT (This->GeneratorID == HiiFormsInfo->FormGeneratorId);

  if (*HiiHandle != NULL) {
    HiiRemovePackages (*HiiHandle);
  }

  if (This->Priv != NULL) {
    DeInitPrivateData (This->Priv);
  }

  return EFI_SUCCESS;
}

/** The interface for the Platform Config Form Generator.
*/
static
HII_FORMS_GENERATOR  HiiPlatCfgGenerator = {
  // Generator ID
  CREATE_HII_FORMS_GEN_ID (EStdHiiFormsIdPlatCfg),
  // Generator Description
  L"HII.STD.PLAT.CFG.FORM.GENERATOR",
  // Private structure for the Form (to be populated at runtime)
  NULL,
  // Build form function.
  BuildHiiPlatCfgForm,
  // Free form function.
  FreeHiiPlatCfgForm,
};

/** Register the Generator with the HII Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
HiiPlatCfgFormConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterHiiFormsGenerator (&HiiPlatCfgGenerator);
  DEBUG ((
    DEBUG_INFO,
    "HII-PlatCfg: Register Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the HII Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
HiiPlatCfgFormDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterHiiFormsGenerator (&HiiPlatCfgGenerator);
  DEBUG ((
    DEBUG_INFO,
    "HII-PlatCfg: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
