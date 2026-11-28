/** @file
  Dynamic Hii Root Form Generator.

  This is the top level Form that references all other
  dynamically generated HII forms.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

// Module specific include files.
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <HiiFormsGenerator.h>
#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/MdeModuleHii.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HiiFormsLib.h>
#include <Library/HiiLib.h>
#include <Library/HiiStringsLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Uefi/UefiInternalFormRepresentation.h>

#include "RootFormGenerator.h"

static EFI_HANDLE  mDevHandle;
extern UINT8       RootFormGeneratorStrings[];

/** Get number of supported languages

  Add the number language-code strings fetched through the call to
  HiiGetSupportedLanguages() to the string table. Each language-code
  string is separated through a ';' character.

  @param [in]  Languages  List of supported languages, semicolon separated.

  @retval  Number of supported languages, 0 on empty input.

**/
static
UINT32
GetLanguageCount (
  IN CONST  CHAR8  *Languages
  )
{
  UINT32  Count;

  if ((Languages == NULL) || (*Languages == '\0')) {
    return 0;
  }

  Count = 1;
  while (*Languages != '\0') {
    if (*Languages++ == ';') {
      Count++;
    }
  }

  return Count;
}

/** Add the supported language-code strings

  Add the language-code strings fetched through the call to
  HiiGetSupportedLanguages() to the string table. The strings in the
  string table entries would then be added to the string package through
  a call to DynHiiAddString().

  @param [in]       Languages   List of supported languages, semicolon
                                separated.
  @param [in]       LangCount   Number of supported languages.
  @param [in, out]  StrTable    An array of entries of languagecode and
                                string pair to be added.

  @retval  EFI_SUCCESS  All supported languages were added successfully
                        or other failure codes as returned by the functions.

**/
static
EFI_STATUS
PopulateLangCode (
  IN     CONST  CHAR8              *Languages,
  IN            UINT32             LangCount,
  IN OUT        DYN_HII_STR_ENTRY  *StrTable
  )
{
  UINTN        LangLen;
  UINT32       Idx;
  EFI_STATUS   Status;
  CONST CHAR8  *LangPtr;

  ASSERT (Languages != NULL);
  ASSERT (StrTable != NULL);

  LangPtr = Languages;
  for (Idx = 0; Idx < LangCount; Idx++) {
    LangLen = 0;
    while ((*LangPtr != ';') && (*LangPtr != '\0')) {
      LangLen++;
      LangPtr++;
    }

    // Account for the '\0' character
    LangLen++;

    StrTable[Idx].LanguageCode = AllocateZeroPool (LangLen);
    if (StrTable[Idx].LanguageCode == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto err_out;
    }

    CopyMem (StrTable[Idx].LanguageCode, Languages, LangLen - 1);
    Languages = ++LangPtr;
  }

  return EFI_SUCCESS;

err_out:
  for (Idx = 0; Idx < LangCount; Idx++) {
    if (StrTable[Idx].LanguageCode != NULL) {
      FreePool (StrTable[Idx].LanguageCode);
    }
  }

  return Status;
}

/** Add Prompt and Help strings for the forms

  Get the Prompt and Help strings for all the supported language-codes
  and all them to the string package. If a string is not added for a
  language, do not fail, but simply add an empty dummy string.

  @param [in]  Priv       Pointer to the private structure of the form.
  @param [in]  Buffer     Pointer to the target formset object from which
                          the prompt and help strings are to be fetched.
  @param [in]  HiiHandle  HII handle of the target formset.
  @param [out] Prompt     Pointer to the prompt string-id.
  @param [out] Help       Pointer to the help string-id.

  @retval  EFI_SUCCESS  If the prompt and help strings were added successfully
                        or other failure codes as returned by the functions.

**/
static
EFI_STATUS
AddPromptHelpStrings (
  IN    HII_ROOT_FORM_GEN_PRIV  *Priv,
  IN    EFI_IFR_FORM_SET        *Buffer,
  IN    EFI_HII_HANDLE          HiiHandle,
  OUT   EFI_STRING_ID           *Prompt,
  OUT   EFI_STRING_ID           *Help
  )
{
  CHAR8              *Languages;
  UINT32             Idx;
  UINT32             LangCount;
  EFI_STATUS         Status;
  DYN_HII_STR_ENTRY  *StrTable;
  CONST EFI_STRING   DummyStr = L"";

  Languages = HiiGetSupportedLanguages (HiiHandle);
  if (Languages == NULL) {
    return EFI_NOT_FOUND;
  }

  LangCount = GetLanguageCount (Languages);
  if (LangCount == 0) {
    FreePool (Languages);
    return EFI_NOT_FOUND;
  }

  if (LangCount > MAX_UINT8) {
    FreePool (Languages);
    return EFI_BAD_BUFFER_SIZE;
  }

  StrTable = AllocateZeroPool (LangCount * sizeof (*StrTable));
  if (StrTable == NULL) {
    FreePool (Languages);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = PopulateLangCode (Languages, LangCount, StrTable);
  if (EFI_ERROR (Status)) {
    FreePool (Languages);
    FreePool (StrTable);
    return Status;
  }

  for (Idx = 0; Idx < LangCount; Idx++) {
    StrTable[Idx].String = HiiGetString (
                             HiiHandle,
                             Buffer->FormSetTitle,
                             StrTable[Idx].LanguageCode
                             );
    if (StrTable[Idx].String == NULL) {
      StrTable[Idx].String = DummyStr;
    }
  }

  Status = DynHiiAddString (Priv->StrPkg, StrTable, LangCount, Prompt);
  if (EFI_ERROR (Status)) {
    goto out;
  }

  for (Idx = 0; Idx < LangCount; Idx++) {
    if (StrTable[Idx].String != DummyStr) {
      FreePool (StrTable[Idx].String);
    }

    StrTable[Idx].String = NULL;
  }

  for (Idx = 0; Idx < LangCount; Idx++) {
    StrTable[Idx].String = HiiGetString (
                             HiiHandle,
                             Buffer->Help,
                             StrTable[Idx].LanguageCode
                             );
    if (StrTable[Idx].String == NULL) {
      StrTable[Idx].String = DummyStr;
    }
  }

  Status = DynHiiAddString (Priv->StrPkg, StrTable, LangCount, Help);
  if (EFI_ERROR (Status)) {
    goto out;
  }

  Status = EFI_SUCCESS;

out:
  for (Idx = 0; Idx < LangCount; Idx++) {
    if (StrTable[Idx].String != DummyStr) {
      FreePool (StrTable[Idx].String);
    }

    if (StrTable[Idx].LanguageCode != NULL) {
      FreePool (StrTable[Idx].LanguageCode);
    }
  }

  FreePool (Languages);
  FreePool (StrTable);

  return Status;
}

/** Add all question objects to the Form

  Add all the question objects, cross-links to other forms in this case,
  to the form.

  @param [in]  Priv      Pointer to the private structure of the form.

  @retval  EFI_SUCCESS  If the questions were added successfully or other
                        failure codes as returned by the form generation API's.

**/
static
EFI_STATUS
EFIAPI
PopulateQuestionObjs (
  IN     HII_ROOT_FORM_GEN_PRIV  *Priv
  )
{
  UINTN             BufferSize;
  EFI_GUID          FormsetGuid;
  EFI_STATUS        Status;
  EFI_STRING_ID     Prompt;
  EFI_STRING_ID     Help;
  EFI_QUESTION_ID   QuestionId;
  EFI_HII_HANDLE    HiiHandle;
  DYN_HII_HANDLE    FormHandle;
  EFI_IFR_FORM_SET  *Buffer;
  HII_FORM_HANDLE   *Entry;

  QuestionId = DYN_HII_ROOT_FORM_QUESTION_ID_START;
  FormHandle = DynHiiGetForm (Priv->FormsetHandle, 1);
  Entry      = (HII_FORM_HANDLE *)GetFirstNode (Priv->HiiHandleList);
  while (!IsNull (Priv->HiiHandleList, &Entry->List)) {
    Buffer     = NULL;
    BufferSize = 0;

    HiiHandle = Entry->HiiHandle;
    Status    = HiiGetFormSetFromHiiHandle (HiiHandle, &Buffer, &BufferSize);
    if (EFI_ERROR (Status)) {
      goto out;
    }

    Status = AddPromptHelpStrings (Priv, Buffer, HiiHandle, &Prompt, &Help);
    if (EFI_ERROR (Status)) {
      goto out;
    }

    CopyMem (&FormsetGuid, &Buffer->Guid, sizeof (EFI_GUID));
    Status = DynHiiAddRef3 (
               FormHandle,
               QuestionId++,
               Prompt,
               Help,
               0,              // QuestionFlags
               0,              // RefFormId
               0,              // RefQuestionId
               &FormsetGuid,   // RefFormsetId
               NULL            // StatementHandle
               );
    if (EFI_ERROR (Status)) {
      goto out;
    }

    FreePool (Buffer);

    Entry = (HII_FORM_HANDLE *)GetNextNode (Priv->HiiHandleList, &Entry->List);
  }

  return EFI_SUCCESS;

out:
  if (Buffer != NULL) {
    FreePool (Buffer);
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
  IN     HII_ROOT_FORM_GEN_PRIV  *Priv
  )
{
  EFI_FORM_ID  TempId;

  return DynHiiAddForm (
           Priv->FormsetHandle,
           STRING_TOKEN (STR_PLAT_CFG_FORM_TITLE),
           &TempId
           );
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
  IN     HII_ROOT_FORM_GEN_PRIV  *Priv
  )
{
  CONST EFI_GUID  FormsetGuid = DYNAMIC_HII_ROOT_FORM_GUID;
  CONST EFI_GUID  ClassGuid   = EFI_HII_PLATFORM_SETUP_FORMSET_GUID;

  return DynHiiCreateFormSet (
           &FormsetGuid,
           &ClassGuid,
           STRING_TOKEN (STR_PLAT_CFG_FORM_SET_TITLE),
           STRING_TOKEN (STR_PLAT_CFG_FORM_SET_TITLE_HELP),
           1,
           &Priv->FormsetHandle
           );
}

/** Initialize the string package info structure.

  Initialize the structure that contains information needed for the generation
  of the string package.

  @param [out]  StrPkg     Pointer to the string package info structure.

  @retval  EFI_SUCCESS  If the structure was successfully initialized or other
                        failure codes as returned by the string generation
                        API's.

**/
static
EFI_STATUS
InitStringPkgInfo (
  OUT DYN_HII_STR_HANDLE  *StrPkg
  )
{
  return DynHiiInitStringPkg (
           RootFormGeneratorStrings,
           StrPkg
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
  IN     LIST_ENTRY              *HiiHandleList,
  OUT    HII_ROOT_FORM_GEN_PRIV  **PlatCfgPriv
  )
{
  EFI_STATUS              Status;
  HII_ROOT_FORM_GEN_PRIV  *Priv;

  Priv = AllocateZeroPool (sizeof (*Priv));
  if (Priv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Priv->Signature     = DYN_HII_ROOT_FORM_GEN_SIGNATURE;
  Priv->HiiHandleList = HiiHandleList;

  Status = InitStringPkgInfo (&Priv->StrPkg);
  if (EFI_ERROR (Status)) {
    FreePool (Priv);
    return Status;
  }

  *PlatCfgPriv = Priv;

  return EFI_SUCCESS;
}

/** Construct Dynamic HII Root Form

  This function is invoked for building the dynamic HII root form.
  In turn, this calls the HII Form and String Generation API's
  for building the form and string package, and then adds these
  packages to the HII database through a call to HiiAddPackages().

  If this function allocates any resources then they must be freed
  in the FreeXXXXRes function.

  @param [in]  This            Pointer to the HII form generator.
  @param [in]  CmHiiFormInfo   Pointer to the HII form information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [in]  HiiHandleList   List of all Hii handles to which this
                               form creates references.
  @param [out] HiiHandle       Hii handle of this form.
  @param [out] GenPriv         Private structure of the form.

  @retval EFI_SUCCESS            Form generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
static
EFI_STATUS
EFIAPI
BuildHiiRootForm (
  IN  CONST HII_FORM_GENERATOR                            *This,
  IN  CONST CM_STD_OBJ_HII_FORM_INFO              *CONST  CmHiiFormInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST LIST_ENTRY                                    *HiiHandleList,
  OUT       EFI_HII_HANDLE                                *HiiHandle,
  OUT       VOID                                          **GenPriv
  )
{
  UINT8                   *IfrBuf;
  UINT8                   *StrPkgBuf;
  EFI_STATUS              Status;
  HII_ROOT_FORM_GEN_PRIV  *Priv;
  CONST EFI_GUID          FormsetGuid = DYNAMIC_HII_ROOT_FORM_GUID;

  if (This == NULL) {
    ASSERT (This != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (GenPriv == NULL) {
    ASSERT (GenPriv != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (HiiHandle == NULL) {
    ASSERT (HiiHandle != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (HiiHandleList == NULL) {
    ASSERT (HiiHandleList != NULL);
    return EFI_INVALID_PARAMETER;
  }

  *GenPriv   = NULL;
  *HiiHandle = NULL;
  IfrBuf     = NULL;
  StrPkgBuf  = NULL;

  Status = InitFormPrivateData ((LIST_ENTRY *)HiiHandleList, &Priv);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to Initialise the private data structure for the form."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  *GenPriv = Priv;

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
  if (EFI_ERROR (Status)) {
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
             Priv->FormsetHandle,
             &IfrBuf
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to serialize the IFR Byte Buffer."
      " Status = %r\n",
      __func__,
      Status
      ));

    goto out;
  }

  Status = DynHiiGenerateStringPackage (Priv->StrPkg, &StrPkgBuf);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to serialize the String Package Buffer."
      " Status = %r\n",
      __func__,
      Status
      ));

    goto out;
  }

  /// Add the template data to the Hii Package list
  *HiiHandle = HiiAddPackages (
                 &FormsetGuid,
                 mDevHandle,
                 IfrBuf,
                 StrPkgBuf,
                 NULL
                 );
  if (*HiiHandle == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to add packages to Hii Database\n",
      __func__
      ));

    Status = EFI_OUT_OF_RESOURCES;
  }

out:
  if (IfrBuf != NULL) {
    FreePool (IfrBuf);
  }

  if (StrPkgBuf != NULL) {
    FreePool (StrPkgBuf);
  }

  return Status;
}

/** Free up all resources associated with this form.

  Free up all resources associated with this form.

  @param [in]  Priv     Pointer to the private structure of the form.

**/
static
VOID
DeInitPrivateData (
  IN HII_ROOT_FORM_GEN_PRIV  *Priv
  )
{
  if (Priv->FormsetHandle != NULL) {
    DynHiiFreeFormSet (Priv->FormsetHandle);
  }

  if (Priv->StrPkg != NULL) {
    DynHiiFreeStringPackage (Priv->StrPkg);
  }

  FreePool (Priv);
}

/** Free up all resources associated with this form.

  Free up all resources associated with this form. In addition remove
  the Hii Handle from the Hii database.

  @param [in]  This           Pointer to the HII form generator.
  @param [in]  CmHiiFormInfo  Pointer to the HII form information.
  @param [in]  HiiHandle      HII handle of this form.
  @param [in]  GenPriv        Private structure for the form.

  @retval The resources were freed successfully.

**/
static
EFI_STATUS
EFIAPI
FreeHiiRootForm (
  IN  CONST HII_FORM_GENERATOR                    *This,
  IN  CONST CM_STD_OBJ_HII_FORM_INFO      *CONST  CmHiiFormInfo OPTIONAL,
  IN        EFI_HII_HANDLE                        *HiiHandle,
  IN        VOID                                  *GenPriv
  )
{
  if (This == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (This->GeneratorID != CREATE_HII_FORM_GEN_ID (EStdHiiFormsIdRootForm)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (GenPriv == NULL) {
    ASSERT (GenPriv != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (HiiHandle == NULL) {
    ASSERT (HiiHandle != NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (*HiiHandle != NULL) {
    HiiRemovePackages (*HiiHandle);
  }

  DeInitPrivateData (GenPriv);

  return EFI_SUCCESS;
}

/** The interface for the Root Form Generator.
*/
static
HII_FORM_GENERATOR  HiiRootFormGenerator = {
  // Generator ID
  CREATE_HII_FORM_GEN_ID (EStdHiiFormsIdRootForm),
  // Generator Description
  L"HII.STD.ROOT.FORM.GENERATOR",
  // Build form function.
  BuildHiiRootForm,
  // Free form function.
  FreeHiiRootForm,
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
HiiRootFormConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterHiiFormGenerator (&HiiRootFormGenerator);
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
HiiRootFormDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterHiiFormGenerator (&HiiRootFormGenerator);
  DEBUG ((
    DEBUG_INFO,
    "HII-PlatCfg: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
