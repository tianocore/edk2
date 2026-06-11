/** @file
  Dynamic Hii Form Generation API functions

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Guid/MdeModuleHii.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>

#include "InternalHiiFormsLib.h"

/** Check if the default has already been added.

  Check if the default has already been added for the provided
  DefaultId which corresponds to a default store.

  @param [in]  DefaultList      List to check for the Default.
  @param [in]  DefaultId        Default ID to look for in the list.

  @retval TRUE if the default exists, FALSE otherwise

**/
static
BOOLEAN
IsDefaultPresent (
  IN  LIST_ENTRY  *DefaultList,
  IN  UINT16      DefaultId
  )
{
  LIST_ENTRY       *Link;
  DYN_HII_DEFAULT  *Default;

  Link = GetFirstNode (DefaultList);
  while (!IsNull (DefaultList, Link)) {
    Default = BASE_CR (Link, DYN_HII_DEFAULT, Hdr.Link);
    if (Default->DefaultId == DefaultId) {
      return TRUE;
    }

    Link = GetNextNode (DefaultList, Link);
  }

  return FALSE;
}

/** Check if the question has already been added.

  Check if the question has already been added to the provided
  StatementList.

  @param [in]  StatementList    List to check for the question.
  @param [in]  QuestionId       Question ID to look for in the list.

  @retval TRUE if the question exists, FALSE otherwise

**/
static
BOOLEAN
IsQuestionPresent (
  IN  LIST_ENTRY       *StatementList,
  IN  EFI_QUESTION_ID  QuestionId
  )
{
  LIST_ENTRY               *Link;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;
  DYN_HII_STATEMENT        *Statement;

  Link = GetFirstNode (StatementList);
  while (!IsNull (StatementList, Link)) {
    Statement   = BASE_CR (Link, DYN_HII_STATEMENT, Hdr.Link);
    QuestionHdr = &Statement->Data.QuestionHdr;
    if (QuestionHdr->QuestionId == QuestionId) {
      return TRUE;
    }

    Link = GetNextNode (StatementList, Link);
  }

  return FALSE;
}

/** Check if this is a question or a statement

  Check if the statement passed is a question, as not all
  statements are questions.

  @param [in]  StatementType    Type of the statement

  @retval TRUE if question, FALSE otherwise

**/
static
BOOLEAN
IsTypeQuestion (
  IN  DYN_HII_STATEMENT_TYPE  StatementType
  )
{
  switch (StatementType) {
    case DynHiiStSubtitle:
    case DynHiiStText:
      return FALSE;

    default:
      return TRUE;
  }
}

/** Free all the added Defaults.

  Iterate through the Default list, and free all allocated memory.

  @param [in]  DefaultList    List of defaults that need to be freed up.

**/
static
VOID
DynHiiFreeDefaultList (
  IN LIST_ENTRY  *DefaultList
  )
{
  LIST_ENTRY       *Link;
  LIST_ENTRY       *Next;
  DYN_HII_DEFAULT  *Default;

  if (DefaultList == NULL) {
    return;
  }

  Link = GetFirstNode (DefaultList);
  while (!IsNull (DefaultList, Link)) {
    Next = GetNextNode (DefaultList, Link);

    Default = BASE_CR (Link, DYN_HII_DEFAULT, Hdr.Link);
    RemoveEntryList (Link);
    FreePool (Default);
    Link = Next;
  }
}

/** Free all the added Options.

  Iterate through the Option list, and free all allocated memory.

  @param [in]  OptionList    List of options that need to be freed up.

**/
static
VOID
DynHiiFreeOptionList (
  IN LIST_ENTRY  *OptionList
  )
{
  LIST_ENTRY             *Link;
  LIST_ENTRY             *Next;
  DYN_HII_ONE_OF_OPTION  *Option;

  if (OptionList == NULL) {
    return;
  }

  Link = GetFirstNode (OptionList);
  while (!IsNull (OptionList, Link)) {
    Next = GetNextNode (OptionList, Link);

    Option = BASE_CR (Link, DYN_HII_ONE_OF_OPTION, Hdr.Link);
    RemoveEntryList (Link);
    FreePool (Option);
    Link = Next;
  }
}

/** Free all the added Statements/Questions.

  Iterate through the statement list, and free all allocated memory.

  @param [in]  StatementList      List of statements that need to be freed up.

**/
static
VOID
DynHiiFreeStatementList (
  IN LIST_ENTRY  *StatementList
  )
{
  LIST_ENTRY         *Link;
  LIST_ENTRY         *Next;
  DYN_HII_STATEMENT  *Statement;

  if (StatementList == NULL) {
    return;
  }

  Link = GetFirstNode (StatementList);
  while (!IsNull (StatementList, Link)) {
    Next      = GetNextNode (StatementList, Link);
    Statement = BASE_CR (Link, DYN_HII_STATEMENT, Hdr.Link);

    DynHiiFreeOptionList (&Statement->OptionList);
    DynHiiFreeDefaultList (&Statement->DefaultList);

    RemoveEntryList (Link);
    FreePool (Statement);
    Link = Next;
  }
}

/** Free all the added Forms.

  Iterate through the form list, and free all allocated memory.

  @param [in]  FormList      List of forms that need to be freed up.

**/
static
VOID
DynHiiFreeFormList (
  IN LIST_ENTRY  *FormList
  )
{
  LIST_ENTRY    *Link;
  LIST_ENTRY    *Next;
  DYN_HII_FORM  *Form;

  if (FormList == NULL) {
    return;
  }

  Link = GetFirstNode (FormList);
  while (!IsNull (FormList, Link)) {
    Next = GetNextNode (FormList, Link);
    Form = BASE_CR (Link, DYN_HII_FORM, Hdr.Link);
    DynHiiFreeStatementList (&Form->StatementList);

    RemoveEntryList (Link);
    FreePool (Form);
    Link = Next;
  }
}

/** Free all the added Variable Stores.

  Iterate through the varstore list, and free all allocated memory.

  @param [in]  VarstoreList  List of variable stores that need to be
                             freed up.

**/
static
VOID
DynHiiFreeVarstoreList (
  IN LIST_ENTRY  *VarstoreList
  )
{
  CHAR8             *Name;
  LIST_ENTRY        *Link;
  LIST_ENTRY        *Next;
  DYN_HII_VARSTORE  *Varstore;

  if (VarstoreList == NULL) {
    return;
  }

  Link = GetFirstNode (VarstoreList);
  while (!IsNull (VarstoreList, Link)) {
    Next     = GetNextNode (VarstoreList, Link);
    Varstore = BASE_CR (Link, DYN_HII_VARSTORE, Hdr.Link);

    if ((Varstore->VarstoreType == DynHiiVarstoreBuffer) ||
        (Varstore->VarstoreType == DynHiiVarstoreEfi))
    {
      Name = Varstore->VarstoreType == DynHiiVarstoreBuffer ?
             Varstore->Data.Buffer.Name : Varstore->Data.Efi.Name;
      FreePool (Name);
    }

    RemoveEntryList (Link);
    FreePool (Varstore);
    Link = Next;
  }
}

/** Free all the added Default Stores.

  Iterate through the default store list, and free all allocated memory.

  @param [in]  DefaultStoreList  List of default stores that need to be
                                 freed up.

**/
static
VOID
DynHiiFreeDefaultStoreList (
  IN LIST_ENTRY  *DefaultStoreList
  )
{
  LIST_ENTRY            *Link;
  LIST_ENTRY            *Next;
  DYN_HII_DEFAULTSTORE  *DefaultStore;

  if (DefaultStoreList == NULL) {
    return;
  }

  Link = GetFirstNode (DefaultStoreList);
  while (!IsNull (DefaultStoreList, Link)) {
    Next         = GetNextNode (DefaultStoreList, Link);
    DefaultStore = BASE_CR (Link, DYN_HII_DEFAULTSTORE, Hdr.Link);
    RemoveEntryList (Link);
    FreePool (DefaultStore);
    Link = Next;
  }
}

/** Add a Statement/Question to a Form.

  Create a HII Statement/Question and add it to the given Form.

  @param [in]  Form             Form under which Statement is to be added.
  @param [in]  StatementType     Type of statement to be added.
  @param [in]  StatementData     Data of the corresponding Statement to be added.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameter is invalid.
  @retval  EFI_ALREADY_STARTED    The Statement has already been added to the
                                  form.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
EFIAPI
DynHiiAddStatement (
  IN  DYN_HII_FORM            *Form,
  IN  DYN_HII_STATEMENT_TYPE  StatementType,
  IN  DYN_HII_STATEMENT_DATA  *StatementData
  )
{
  DYN_HII_STATEMENT       *NewStmt;
  EFI_QUESTION_ID         QuestionId;
  DYN_HII_STATEMENT_DATA  *NewStatementData;

  if (Form == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (StatementData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  QuestionId = StatementData->QuestionHdr.QuestionId;
  if (IsTypeQuestion (StatementType) &&
      IsQuestionPresent (&Form->StatementList, QuestionId))
  {
    return EFI_ALREADY_STARTED;
  }

  NewStmt = AllocateZeroPool (sizeof (*NewStmt));
  if (NewStmt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewStmt->Hdr.Type  = DynHiiNodeStatement;
  NewStmt->Hdr.Scope = TRUE;
  InitializeListHead (&NewStmt->Hdr.Link);

  NewStmt->StatementType    = StatementType;
  NewStmt->Data.QuestionHdr = StatementData->QuestionHdr;

  InitializeListHead (&NewStmt->OptionList);
  InitializeListHead (&NewStmt->DefaultList);

  NewStatementData = &NewStmt->Data;
  // Type-specific defaults.
  switch (StatementType) {
    case DynHiiStCheckbox:
      NewStatementData->Statement.Checkbox = StatementData->Statement.Checkbox;
      break;

    case DynHiiStNumeric:
      NewStatementData->Statement.Numeric = StatementData->Statement.Numeric;
      break;

    case DynHiiStRef:
      NewStmt->Hdr.Scope              = FALSE;
      NewStatementData->Statement.Ref = StatementData->Statement.Ref;
      break;

    case DynHiiStOneOf:
      NewStatementData->Statement.OneOf = StatementData->Statement.OneOf;
      break;

    case DynHiiStSubtitle:
      NewStmt->Hdr.Scope                   = FALSE;
      NewStatementData->Statement.Subtitle = StatementData->Statement.Subtitle;
      break;

    default:
      goto err_out;
      break;
  }

  InsertTailList (&Form->StatementList, &NewStmt->Hdr.Link);

  return EFI_SUCCESS;

err_out:
  FreePool (NewStmt);

  return EFI_UNSUPPORTED;
}

/** Add the Varstore Name.

  Add the name of the variable store.

  @param [in]   Name               Ascii name of the varstore.
  @param [out]  VarstoreName       Pointer to the varstore name data structure.

  @retval  EFI_SUCCESS            The varstore was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Name is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
EFIAPI
DynHiiAddVarstoreName (
  IN   CONST   CHAR8  *Name,
  OUT          CHAR8  **VarstoreName
  )
{
  UINTN       StrBytes;
  EFI_STATUS  Status;

  StrBytes = AsciiStrSize (Name);
  if (StrBytes == 0) {
    return EFI_INVALID_PARAMETER;
  }

  *VarstoreName = AllocateZeroPool (StrBytes);
  if (*VarstoreName == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AsciiStrCpyS (*VarstoreName, StrBytes, Name);
  if (EFI_ERROR (Status)) {
    FreePool (*VarstoreName);
  }

  return Status;
}

/** Get a Form from a given formset

  Get a form from the given formset.

  @param [in]  FormsetHandle     Handle to the formset from which the form is
                                 to be fetched.
  @param [in]  FormId            The form ID of the required form.

  @retval  A pointer to the form object or NULL if form not found.

**/
DYN_HII_HANDLE
EFIAPI
DynHiiGetForm (
  IN  DYN_HII_HANDLE  FormsetHandle,
  IN  EFI_FORM_ID     FormId
  )
{
  LIST_ENTRY       *Link;
  DYN_HII_FORM     *Form;
  DYN_HII_FORMSET  *Formset;

  Formset = FormsetHandle;
  if ((Formset == NULL) || (Formset->Hdr.Type != DynHiiNodeFormset)) {
    return NULL;
  }

  Link = GetFirstNode (&Formset->FormList);
  while (!IsNull (&Formset->FormList, Link)) {
    Form = BASE_CR (Link, DYN_HII_FORM, Hdr.Link);
    if (Form->FormId == FormId) {
      return Form;
    }

    Link = GetNextNode (&Formset->FormList, Link);
  }

  return NULL;
}

/** Get a question from a given form

  Get a question from the given form.

  @param [in]  FormHandle        Handle to the form from which the
                                 question is to be fetched.
  @param [in]  QuestionId        The question ID of the required
                                 question.

  @retval  A pointer to the question object or NULL if form not found.

**/
DYN_HII_HANDLE
EFIAPI
DynHiiGetQuestion (
  IN  DYN_HII_HANDLE   FormHandle,
  IN  EFI_QUESTION_ID  QuestionId
  )
{
  LIST_ENTRY               *Link;
  DYN_HII_FORM             *Form;
  DYN_HII_STATEMENT        *Statement;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;

  Form = FormHandle;
  if ((Form == NULL) || (Form->Hdr.Type != DynHiiNodeForm)) {
    return NULL;
  }

  Link = GetFirstNode (&Form->StatementList);
  while (!IsNull (&Form->StatementList, Link)) {
    Statement   = BASE_CR (Link, DYN_HII_STATEMENT, Hdr.Link);
    QuestionHdr = &Statement->Data.QuestionHdr;
    if (QuestionHdr->QuestionId == QuestionId) {
      return Statement;
    }

    Link = GetNextNode (&Form->StatementList, Link);
  }

  return NULL;
}

/** Create a HII Formset.

  Create a HII Formset under which other HII nodes can
  be added subsequently.

  @param [in]  FormsetGuid      Formset GUID
  @param [in]  ClassGuid        Pointer to an array of Class GUIDs, if any.
  @param [in]  Title            String ID to the Formset title text.
  @param [in]  Help             String ID to the Formset help text.
  @param [in]  ClassGuidCount   Number of Class GUIDs(can be 0 - 3).
  @param [out] FormsetHandle    The Formset object created by the function.


  @retval  EFI_SUCCESS            The Formset was created successfully.
  @retval  EFI_INVALID_PARAMETER  The FormsetGuid or Formset is NULL, or the
                                  ClassGuidCount > 3.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiCreateFormSet (
  IN  CONST EFI_GUID  *FormsetGuid,
  IN  CONST EFI_GUID  *ClassGuid,
  IN  EFI_STRING_ID   Title,
  IN  EFI_STRING_ID   Help,
  IN  UINT8           ClassGuidCount,
  OUT DYN_HII_HANDLE  *FormsetHandle
  )
{
  UINT8            Idx;
  DYN_HII_FORMSET  *NewFormset;

  if ((FormsetHandle == NULL) || (FormsetGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ClassGuidCount > MAX_FORMSET_CLASS_GUID) {
    return EFI_INVALID_PARAMETER;
  }

  NewFormset = AllocateZeroPool (sizeof (*NewFormset));
  if (NewFormset == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewFormset->Hdr.Type  = DynHiiNodeFormset;
  NewFormset->Hdr.Scope = TRUE;
  InitializeListHead (&NewFormset->Hdr.Link);

  CopyGuid (&NewFormset->FormsetGuid, FormsetGuid);
  NewFormset->Title          = Title;
  NewFormset->Help           = Help;
  NewFormset->FormIdCounter  = 0;
  NewFormset->ClassGuidCount = ClassGuidCount;

  for (Idx = 0; Idx < ClassGuidCount; Idx++) {
    CopyGuid (&NewFormset->ClassGuid[Idx], &ClassGuid[Idx]);
  }

  InitializeListHead (&NewFormset->FormList);
  InitializeListHead (&NewFormset->VarstoreList);
  InitializeListHead (&NewFormset->DefaultstoreList);

  *FormsetHandle = NewFormset;

  return EFI_SUCCESS;
}

/** Add a form to a Formset.

  Create a HII Form and add it to the given Formset.

  @param [in]   FormsetHandle    Formset handle under which Form is to be added.
  @param [in]   Title            String ID to the Form's title text.
  @param [out]  FormId           Form Identifier.

  @retval  EFI_SUCCESS            The Form was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Formset is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddForm (
  IN   DYN_HII_HANDLE  FormsetHandle,
  IN   EFI_STRING_ID   Title,
  OUT  EFI_FORM_ID     *FormId
  )
{
  DYN_HII_FORM     *NewForm;
  DYN_HII_FORMSET  *Formset;

  Formset = FormsetHandle;
  if ((Formset == NULL) || (Formset->Hdr.Type != DynHiiNodeFormset)) {
    return EFI_INVALID_PARAMETER;
  }

  NewForm = AllocateZeroPool (sizeof (*NewForm));
  if (NewForm == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewForm->Hdr.Type  = DynHiiNodeForm;
  NewForm->Hdr.Scope = TRUE;
  InitializeListHead (&NewForm->Hdr.Link);

  NewForm->FormId = ++Formset->FormIdCounter;
  NewForm->Title  = Title;

  InitializeListHead (&NewForm->StatementList);
  InsertTailList (&Formset->FormList, &NewForm->Hdr.Link);
  *FormId = NewForm->FormId;

  return EFI_SUCCESS;
}

/** Add a Subtitle statement to a Form.

  Create a HII Subtitle statement and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  SubtitleFlags    The flags for subtitle opcode.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Form is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddSubtitle (
  IN   DYN_HII_HANDLE  FormHandle,
  IN   EFI_STRING_ID   Prompt,
  IN   EFI_STRING_ID   Help,
  IN   UINT8           SubtitleFlags
  )
{
  DYN_HII_FORM            *Form;
  DYN_HII_STATEMENT_DATA  StatementData;
  DYN_HII_SUBTITLE_DATA   *Subtitle;

  Form = FormHandle;
  if ((Form == NULL) || (Form->Hdr.Type != DynHiiNodeForm)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&StatementData, sizeof (DYN_HII_STATEMENT_DATA));
  Subtitle = &StatementData.Statement.Subtitle;

  Subtitle->Prompt = Prompt;
  Subtitle->Help   = Help;
  Subtitle->Flags  = SubtitleFlags;

  return DynHiiAddStatement (
           Form,
           DynHiiStSubtitle,
           &StatementData
           );
}

/** Add a Checkbox Question to a Form.

  Create a HII Checkbox Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  VarstoreId       The variable store ID.
  @param [in]  VarOffset        Offset in Storage or String ID of the name (VarName)
                                for the name/value pair.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  CheckboxFlags    The flags for checkbox opcode.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Form is NULL.
  @retval  EFI_ALREADY_STARTED    The Question has already been added to the
                                  Form.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddCheckbox (
  IN   DYN_HII_HANDLE   FormHandle,
  IN   EFI_QUESTION_ID  QuestionId,
  IN   EFI_VARSTORE_ID  VarstoreId,
  IN   UINT16           VarOffset,
  IN   EFI_STRING_ID    Prompt,
  IN   EFI_STRING_ID    Help,
  IN   UINT8            QuestionFlags,
  IN   UINT8            CheckboxFlags
  )
{
  DYN_HII_FORM             *Form;
  DYN_HII_STATEMENT_DATA   StatementData;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;

  Form = FormHandle;
  if ((Form == NULL) || (Form->Hdr.Type != DynHiiNodeForm)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&StatementData, sizeof (DYN_HII_STATEMENT_DATA));
  QuestionHdr = &StatementData.QuestionHdr;

  QuestionHdr->Header.Prompt          = Prompt;
  QuestionHdr->Header.Help            = Help;
  QuestionHdr->QuestionId             = QuestionId;
  QuestionHdr->VarStoreId             = VarstoreId;
  QuestionHdr->Flags                  = QuestionFlags;
  QuestionHdr->VarStoreInfo.VarOffset = VarOffset;

  StatementData.Statement.Checkbox.Flags = CheckboxFlags;

  return DynHiiAddStatement (
           Form,
           DynHiiStCheckbox,
           &StatementData
           );
}

/** Add a Numeric Question to a Form.

  Create a HII Numeric Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  VarstoreId       The variable store ID.
  @param [in]  VarOffset        Offset in Storage or String ID of the name (VarName)
                                for the name/value pair.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  NumericFlags     The flags for numeric opcode.
  @param [in]  Minimum          The numeric minimum value.
  @param [in]  Maximum          The numeric maximum value.
  @param [in]  Step             The numeric step for edit.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Form is NULL.
  @retval  EFI_ALREADY_STARTED    The question has already been added to the
                                  Form.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddNumeric (
  IN   DYN_HII_HANDLE   FormHandle,
  IN   EFI_QUESTION_ID  QuestionId,
  IN   EFI_VARSTORE_ID  VarstoreId,
  IN   UINT16           VarOffset,
  IN   EFI_STRING_ID    Prompt,
  IN   EFI_STRING_ID    Help,
  IN   UINT8            QuestionFlags,
  IN   UINT8            NumericFlags,
  IN   UINT64           Minimum,
  IN   UINT64           Maximum,
  IN   UINT64           Step
  )
{
  DYN_HII_FORM             *Form;
  DYN_HII_NUMERIC_DATA     *Numeric;
  DYN_HII_STATEMENT_DATA   StatementData;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;

  Form = FormHandle;
  if ((Form == NULL) || (Form->Hdr.Type != DynHiiNodeForm)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&StatementData, sizeof (DYN_HII_STATEMENT_DATA));
  QuestionHdr = &StatementData.QuestionHdr;

  QuestionHdr->Header.Prompt          = Prompt;
  QuestionHdr->Header.Help            = Help;
  QuestionHdr->QuestionId             = QuestionId;
  QuestionHdr->VarStoreId             = VarstoreId;
  QuestionHdr->Flags                  = QuestionFlags;
  QuestionHdr->VarStoreInfo.VarOffset = VarOffset;

  Numeric           = &StatementData.Statement.Numeric;
  Numeric->Flags    = NumericFlags;
  Numeric->MinValue = Minimum;
  Numeric->MaxValue = Maximum;
  Numeric->Step     = Step;

  return DynHiiAddStatement (
           Form,
           DynHiiStNumeric,
           &StatementData
           );
}

/** Add a Ref Question to a Form.

  Create a HII Ref Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  RefFormId        Target Form ID.
                                If its value is zero, then the link refers to
                                the top of the form.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_UNSUPPORTED        The Ref type is unsupported.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddRef (
  IN          DYN_HII_HANDLE   FormHandle,
  IN          EFI_QUESTION_ID  QuestionId,
  IN          EFI_STRING_ID    Prompt,
  IN          EFI_STRING_ID    Help,
  IN          UINT8            QuestionFlags,
  IN          EFI_FORM_ID      RefFormId
  )
{
  DYN_HII_FORM             *Form;
  DYN_HII_REF_DATA         *Ref;
  DYN_HII_STATEMENT_DATA   StatementData;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;

  Form = FormHandle;
  if ((Form == NULL) || (Form->Hdr.Type != DynHiiNodeForm)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&StatementData, sizeof (DYN_HII_STATEMENT_DATA));
  Ref = &StatementData.Statement.Ref;

  Ref->RefType = DynHiiRef1;
  Ref->FormId  = RefFormId;

  QuestionHdr = &StatementData.QuestionHdr;

  QuestionHdr->Header.Prompt = Prompt;
  QuestionHdr->Header.Help   = Help;
  QuestionHdr->QuestionId    = QuestionId;
  QuestionHdr->Flags         = QuestionFlags;

  return DynHiiAddStatement (
           Form,
           DynHiiStRef,
           &StatementData
           );
}

/** Add a Ref2 Question to a Form.

  Create a HII Ref2 Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  RefFormId        Target Form ID.
  @param [in]  RefQuestionId    Target Question ID.
                                If its value is zero, then the link refers to
                                the top of the form.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddRef2 (
  IN          DYN_HII_HANDLE   FormHandle,
  IN          EFI_QUESTION_ID  QuestionId,
  IN          EFI_STRING_ID    Prompt,
  IN          EFI_STRING_ID    Help,
  IN          UINT8            QuestionFlags,
  IN          EFI_FORM_ID      RefFormId,
  IN          EFI_QUESTION_ID  RefQuestionId
  )
{
  DYN_HII_FORM             *Form;
  DYN_HII_REF_DATA         *Ref;
  DYN_HII_STATEMENT_DATA   StatementData;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;

  Form = FormHandle;
  if ((Form == NULL) || (Form->Hdr.Type != DynHiiNodeForm)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&StatementData, sizeof (DYN_HII_STATEMENT_DATA));
  Ref = &StatementData.Statement.Ref;

  Ref->RefType    = DynHiiRef2;
  Ref->FormId     = RefFormId;
  Ref->QuestionId = RefQuestionId;

  QuestionHdr = &StatementData.QuestionHdr;

  QuestionHdr->Header.Prompt = Prompt;
  QuestionHdr->Header.Help   = Help;
  QuestionHdr->QuestionId    = QuestionId;
  QuestionHdr->Flags         = QuestionFlags;

  return DynHiiAddStatement (
           Form,
           DynHiiStRef,
           &StatementData
           );
}

/** Add a Ref3 Question to a Form.

  Create a HII Ref3 Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  RefFormId        Target Form ID.
  @param [in]  RefQuestionId    Target Question ID.
                                If its value is zero, then the link refers to
                                the top of the form.
  @param [in]  RefFormsetId     Target Formset GUID.
                                If its value is NULL, and RefDevicePath is zero,
                                then the link is to the current form set.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddRef3 (
  IN          DYN_HII_HANDLE   FormHandle,
  IN          EFI_QUESTION_ID  QuestionId,
  IN          EFI_STRING_ID    Prompt,
  IN          EFI_STRING_ID    Help,
  IN          UINT8            QuestionFlags,
  IN          EFI_FORM_ID      RefFormId,
  IN          EFI_QUESTION_ID  RefQuestionId,
  IN  CONST   EFI_GUID         *RefFormsetId
  )
{
  DYN_HII_FORM             *Form;
  DYN_HII_REF_DATA         *Ref;
  DYN_HII_STATEMENT_DATA   StatementData;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;

  if (RefFormsetId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Form = FormHandle;
  if ((Form == NULL) || (Form->Hdr.Type != DynHiiNodeForm)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&StatementData, sizeof (DYN_HII_STATEMENT_DATA));
  Ref = &StatementData.Statement.Ref;

  Ref->RefType    = DynHiiRef3;
  Ref->FormId     = RefFormId;
  Ref->QuestionId = RefQuestionId;
  CopyMem (&Ref->FormSetGuid, RefFormsetId, sizeof (EFI_GUID));

  QuestionHdr = &StatementData.QuestionHdr;

  QuestionHdr->Header.Prompt = Prompt;
  QuestionHdr->Header.Help   = Help;
  QuestionHdr->QuestionId    = QuestionId;
  QuestionHdr->Flags         = QuestionFlags;

  return DynHiiAddStatement (
           Form,
           DynHiiStRef,
           &StatementData
           );
}

/** Add a Ref4 Question to a Form.

  Create a HII Ref4 Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  RefFormId        Target Form ID.
  @param [in]  RefQuestionId    Target Question ID.
                                If its value is zero, then the link refers to
                                the top of the form.
  @param [in]  RefFormsetId     Target Formset GUID.
                                If its value is NULL, and RefDevicePath is zero,
                                then the link is to the current form set.
  @param [in]  RefDevicePath    The string ID that specifies the string
                                containing the text representation of the device
                                path to which the form set containing the form
                                specified by FormId. If its value is zero,
                                then the link refers to the current page.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddRef4 (
  IN          DYN_HII_HANDLE   FormHandle,
  IN          EFI_QUESTION_ID  QuestionId,
  IN          EFI_STRING_ID    Prompt,
  IN          EFI_STRING_ID    Help,
  IN          UINT8            QuestionFlags,
  IN          EFI_FORM_ID      RefFormId,
  IN          EFI_QUESTION_ID  RefQuestionId,
  IN  CONST   EFI_GUID         *RefFormsetId,
  IN          EFI_STRING_ID    RefDevicePath
  )
{
  DYN_HII_FORM             *Form;
  DYN_HII_REF_DATA         *Ref;
  DYN_HII_STATEMENT_DATA   StatementData;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;

  if (RefDevicePath == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Form = FormHandle;
  if ((Form == NULL) || (Form->Hdr.Type != DynHiiNodeForm)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&StatementData, sizeof (DYN_HII_STATEMENT_DATA));
  Ref = &StatementData.Statement.Ref;

  Ref->RefType    = DynHiiRef4;
  Ref->FormId     = RefFormId;
  Ref->QuestionId = RefQuestionId;
  Ref->DevicePath = RefDevicePath;
  if (RefFormsetId != NULL) {
    CopyMem (&Ref->FormSetGuid, RefFormsetId, sizeof (EFI_GUID));
  }

  QuestionHdr = &StatementData.QuestionHdr;

  QuestionHdr->Header.Prompt = Prompt;
  QuestionHdr->Header.Help   = Help;
  QuestionHdr->QuestionId    = QuestionId;
  QuestionHdr->Flags         = QuestionFlags;

  return DynHiiAddStatement (
           Form,
           DynHiiStRef,
           &StatementData
           );
}

/** Add a Ref5 Question to a Form.

  Create a HII Ref5 Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_UNSUPPORTED        The Ref type is unsupported.

**/
EFI_STATUS
EFIAPI
DynHiiAddRef5 (
  IN          DYN_HII_HANDLE   FormHandle,
  IN          EFI_QUESTION_ID  QuestionId,
  IN          EFI_STRING_ID    Prompt,
  IN          EFI_STRING_ID    Help,
  IN          UINT8            QuestionFlags
  )
{
  return EFI_UNSUPPORTED;
}

/** Add a OneOf Question to a Form.

  Create a HII OneOf Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  VarstoreId       The variable store ID.
  @param [in]  VarOffset        Offset in Storage or String ID of the name (VarName)
                                for the name/value pair.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  OneOfFlags       The flags for one-of opcode.
  @param [in]  ValueType        The value type taken by the one-of opcode.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Form is NULL.
  @retval  EFI_ALREADY_STARTED    The question has already been added to the
                                  Form.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddOneOf (
  IN   DYN_HII_HANDLE   FormHandle,
  IN   EFI_QUESTION_ID  QuestionId,
  IN   EFI_VARSTORE_ID  VarstoreId,
  IN   UINT16           VarOffset,
  IN   EFI_STRING_ID    Prompt,
  IN   EFI_STRING_ID    Help,
  IN   UINT8            QuestionFlags,
  IN   UINT8            OneOfFlags,
  IN   UINT8            ValueType
  )
{
  DYN_HII_FORM             *Form;
  DYN_HII_ONE_OF_DATA      *OneOf;
  DYN_HII_STATEMENT_DATA   StatementData;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;

  Form = FormHandle;
  if ((Form == NULL) || (Form->Hdr.Type != DynHiiNodeForm)) {
    return EFI_INVALID_PARAMETER;
  }

  // Only numeric values supported with options
  if ((ValueType != EFI_IFR_TYPE_NUM_SIZE_8) &&
      (ValueType != EFI_IFR_TYPE_NUM_SIZE_16) &&
      (ValueType != EFI_IFR_TYPE_NUM_SIZE_32) &&
      (ValueType != EFI_IFR_TYPE_NUM_SIZE_64))
  {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&StatementData, sizeof (DYN_HII_STATEMENT_DATA));
  QuestionHdr = &StatementData.QuestionHdr;

  QuestionHdr->Header.Prompt          = Prompt;
  QuestionHdr->Header.Help            = Help;
  QuestionHdr->QuestionId             = QuestionId;
  QuestionHdr->VarStoreId             = VarstoreId;
  QuestionHdr->Flags                  = QuestionFlags;
  QuestionHdr->VarStoreInfo.VarOffset = VarOffset;

  OneOf            = &StatementData.Statement.OneOf;
  OneOf->Flags     = OneOfFlags;
  OneOf->ValueType = ValueType;

  return DynHiiAddStatement (
           Form,
           DynHiiStOneOf,
           &StatementData
           );
}

/** Add an Option to a Question.

  Create a HII Option and add it to the given Statement. Currently, adding
  of options only to the OneOf question type is supported.

  @param [in]  QuestionHandle   Question/Statement handle under which option
                                is to be added.
  @param [in]  Text             The string ID for the option description.
  @param [in]  Flags            Flags associated with the option.
  @param [in]  Type             Type of data in the value field.
  @param [in]  Value            Value of the Option.

  @retval  EFI_SUCCESS            The Option was created successfully.
  @retval  EFI_INVALID_PARAMETER  The any input parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddOption (
  IN  DYN_HII_HANDLE      QuestionHandle,
  IN  EFI_STRING_ID       Text,
  IN  UINT8               Flags,
  IN  UINT8               Type,
  IN  EFI_IFR_TYPE_VALUE  Value
  )
{
  UINT64                 OptionValue;
  DYN_HII_STATEMENT      *Statement;
  DYN_HII_ONE_OF_DATA    *OneOf;
  DYN_HII_ONE_OF_OPTION  *NewOption;

  Statement = QuestionHandle;
  if ((Statement == NULL) || (Statement->Hdr.Type != DynHiiNodeStatement)) {
    return EFI_INVALID_PARAMETER;
  }

  /// For now, it is only the OneOf question type that
  /// will use the Option
  if (Statement->StatementType != DynHiiStOneOf) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Flags != 0x0) &&
      (Flags != EFI_IFR_OPTION_DEFAULT) &&
      (Flags != EFI_IFR_OPTION_DEFAULT_MFG))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Only numeric values supported with options
  if ((Type != EFI_IFR_TYPE_NUM_SIZE_8) &&
      (Type != EFI_IFR_TYPE_NUM_SIZE_16) &&
      (Type != EFI_IFR_TYPE_NUM_SIZE_32) &&
      (Type != EFI_IFR_TYPE_NUM_SIZE_64))
  {
    return EFI_INVALID_PARAMETER;
  }

  OneOf = &Statement->Data.Statement.OneOf;
  if (OneOf->ValueType != Type) {
    return EFI_INVALID_PARAMETER;
  }

  NewOption = AllocateZeroPool (sizeof (*NewOption));
  if (NewOption == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewOption->Hdr.Type  = DynHiiNodeOption;
  NewOption->Hdr.Scope = FALSE;
  InitializeListHead (&NewOption->Hdr.Link);

  NewOption->Text  = Text;
  NewOption->Flags = Flags;
  NewOption->Type  = Type;
  NewOption->Value = Value;

  switch (Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      OptionValue = (UINT64)Value.u8;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_16:
      OptionValue = (UINT64)Value.u16;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_32:
      OptionValue = (UINT64)Value.u32;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_64:
      OptionValue = Value.u64;
      break;
  }

  if ((OneOf->MinValue == 0) || (OptionValue < OneOf->MinValue)) {
    OneOf->MinValue = OptionValue;
  }

  if ((OneOf->MaxValue == 0) || (OptionValue > OneOf->MaxValue)) {
    OneOf->MaxValue = OptionValue;
  }

  InsertTailList (&Statement->OptionList, &NewOption->Hdr.Link);

  return EFI_SUCCESS;
}

/** Add a Default to a Question.

  Create a HII Default and add it to the given Statement.

  @param [in]  QuestionHandle   Question/Statement handle under which option
                                is to be added.
  @param [in]  DefaultId        The Default Store ID for this Default.
  @param [in]  Type             Type of data in the value field.
  @param [in]  Value            Value of the Default.

  @retval  EFI_SUCCESS            The Default was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Statement is NULL.
  @retval  EFI_ALREADY_STARTED    The Default for the corresponding DefaultId
                                  has already been added for the Statement.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddDefault (
  IN  DYN_HII_HANDLE      QuestionHandle,
  IN  UINT16              DefaultId,
  IN  UINT8               Type,
  IN  EFI_IFR_TYPE_VALUE  Value
  )
{
  DYN_HII_DEFAULT    *NewDefault;
  DYN_HII_STATEMENT  *Statement;

  Statement = QuestionHandle;
  if ((Statement == NULL) || (Statement->Hdr.Type != DynHiiNodeStatement)) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsDefaultPresent (&Statement->DefaultList, DefaultId)) {
    return EFI_ALREADY_STARTED;
  }

  NewDefault = AllocateZeroPool (sizeof (*NewDefault));
  if (NewDefault == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewDefault->Hdr.Type  = DynHiiNodeDefault;
  NewDefault->Hdr.Scope = FALSE;
  InitializeListHead (&NewDefault->Hdr.Link);

  NewDefault->DefaultId = DefaultId;
  NewDefault->Type      = Type;
  NewDefault->Value     = Value;

  InsertTailList (&Statement->DefaultList, &NewDefault->Hdr.Link);

  return EFI_SUCCESS;
}

/** Add a Buffer type Variable Store (Varstore) under a Formset.

  Add the buffer variable store under a formset.

  @param [in]  FormsetHandle      Formset handle under which Varstore is to be
                                  added.
  @param [in]  Guid               Varstore GUID.
  @param [in]  VarstoreId         Varstore ID.
  @param [in]  Size               Size of the varstore.
  @param [in]  Name               Ascii name of the varstore.

  @retval  EFI_SUCCESS            The varstore was created successfully.
  @retval  EFI_UNSUPPORTED        Adding Varstore of the given type is currently
                                  not supported.
  @retval  EFI_INVALID_PARAMETER  The VarstoreType, or any other input parameter
                                  is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddVarstoreBuffer (
  IN            DYN_HII_HANDLE   FormsetHandle,
  IN  CONST     EFI_GUID         *Guid,
  IN            EFI_VARSTORE_ID  VarstoreId,
  IN            UINT16           Size,
  IN  CONST     CHAR8            *Name
  )
{
  EFI_STATUS                    Status;
  DYN_HII_FORMSET               *Formset;
  DYN_HII_VARSTORE              *Varstore;
  DYN_HII_VARSTORE_BUFFER_DATA  *Buffer;

  Formset = FormsetHandle;
  if ((Formset == NULL) || (Formset->Hdr.Type != DynHiiNodeFormset)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Guid == NULL) || (Name == NULL) || (Size == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Varstore = AllocateZeroPool (sizeof (*Varstore));
  if (Varstore == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Varstore->Hdr.Type  = DynHiiNodeVarstore;
  Varstore->Hdr.Scope = FALSE;
  InitializeListHead (&Varstore->Hdr.Link);

  Varstore->VarstoreType = DynHiiVarstoreBuffer;

  Buffer = &Varstore->Data.Buffer;

  CopyGuid (&Buffer->Guid, Guid);
  Buffer->VarstoreId = VarstoreId;
  Buffer->Size       = Size;

  Status = DynHiiAddVarstoreName (Name, &Buffer->Name);
  if (EFI_ERROR (Status)) {
    FreePool (Varstore);
    return Status;
  }

  InsertTailList (&Formset->VarstoreList, &Varstore->Hdr.Link);

  return EFI_SUCCESS;
}

/** Add an EFI type Variable Store (Varstore) under a Formset.

  Add the EFI variable store under a formset.

  @param [in]  FormsetHandle      Formset handle under which Varstore is to be
                                  added.
  @param [in]  Guid               Varstore GUID.
  @param [in]  VarstoreId         Varstore ID.
  @param [in]  Size               Size of the varstore.
  @param [in]  Attributes         Flags for the varstore.
  @param [in]  Name               Ascii name of the varstore.

  @retval  EFI_SUCCESS            The varstore was created successfully.
  @retval  EFI_UNSUPPORTED        Adding Varstore of the given type is currently
                                  not supported.
  @retval  EFI_INVALID_PARAMETER  The VarstoreType, or any other input parameter
                                  is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddVarstoreEfi (
  IN            DYN_HII_HANDLE   FormsetHandle,
  IN  CONST     EFI_GUID         *Guid,
  IN            EFI_VARSTORE_ID  VarstoreId,
  IN            UINT16           Size,
  IN            UINT32           Attributes,
  IN  CONST     CHAR8            *Name
  )
{
  EFI_STATUS                 Status;
  DYN_HII_FORMSET            *Formset;
  DYN_HII_VARSTORE           *Varstore;
  DYN_HII_VARSTORE_EFI_DATA  *Efi;

  Formset = FormsetHandle;
  if ((Formset == NULL) || (Formset->Hdr.Type != DynHiiNodeFormset)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Guid == NULL) || (Name == NULL) || (Size == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Varstore = AllocateZeroPool (sizeof (*Varstore));
  if (Varstore == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Varstore->VarstoreType = DynHiiVarstoreEfi;
  Varstore->Hdr.Type     = DynHiiNodeVarstore;
  Varstore->Hdr.Scope    = FALSE;
  InitializeListHead (&Varstore->Hdr.Link);

  Efi = &Varstore->Data.Efi;

  CopyGuid (&Efi->Guid, Guid);
  Efi->VarstoreId = VarstoreId;
  Efi->Size       = Size;
  Efi->Attributes = Attributes;

  Status = DynHiiAddVarstoreName (Name, &Efi->Name);
  if (EFI_ERROR (Status)) {
    FreePool (Varstore);
    return Status;
  }

  InsertTailList (&Formset->VarstoreList, &Varstore->Hdr.Link);

  return EFI_SUCCESS;
}

/** Add an name-value type Variable Store (Varstore) under a Formset.

  Add the name-value variable store under a formset.

  @param [in]  FormsetHandle      Formset handle under which Varstore is to be
                                  added.
  @param [in]  Guid               Varstore GUID.
  @param [in]  VarstoreId         Varstore ID.

  @retval  EFI_SUCCESS            The varstore was created successfully.
  @retval  EFI_UNSUPPORTED        Adding Varstore of the given type is currently
                                  not supported.
  @retval  EFI_INVALID_PARAMETER  The VarstoreType, or any other input parameter
                                  is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddVarstoreNameValue (
  IN            DYN_HII_HANDLE   FormsetHandle,
  IN  CONST     EFI_GUID         *Guid,
  IN            EFI_VARSTORE_ID  VarstoreId
  )
{
  DYN_HII_FORMSET   *Formset;
  DYN_HII_VARSTORE  *Varstore;

  Formset = FormsetHandle;
  if ((Formset == NULL) || (Formset->Hdr.Type != DynHiiNodeFormset)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Varstore = AllocateZeroPool (sizeof (*Varstore));
  if (Varstore == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Varstore->Hdr.Type  = DynHiiNodeVarstore;
  Varstore->Hdr.Scope = FALSE;
  InitializeListHead (&Varstore->Hdr.Link);

  Varstore->VarstoreType = DynHiiVarstoreNameValue;

  CopyGuid (&Varstore->Data.NameValue.Guid, Guid);
  Varstore->Data.NameValue.VarstoreId = VarstoreId;

  InsertTailList (&Formset->VarstoreList, &Varstore->Hdr.Link);

  return EFI_SUCCESS;
}

/** Free up all the memory used by a Formset hierarchy.

  Free up all the memory that had been allocated for the Formset and it's
  child nodes, including Varstores, Forms, individual Statements etc.

  @param [in]  FormsetHandle    Handle to the formset hierarchy that needs to be
                                freed up.

**/
VOID
EFIAPI
DynHiiFreeFormSet (
  IN DYN_HII_HANDLE  FormsetHandle
  )
{
  DYN_HII_FORMSET  *Formset;

  Formset = FormsetHandle;
  if (Formset == NULL) {
    return;
  }

  DynHiiFreeVarstoreList (&Formset->VarstoreList);
  DynHiiFreeDefaultStoreList (&Formset->DefaultstoreList);
  DynHiiFreeFormList (&Formset->FormList);

  FreePool (Formset);
}
