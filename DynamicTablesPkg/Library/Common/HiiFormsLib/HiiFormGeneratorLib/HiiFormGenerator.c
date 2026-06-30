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
#include <Library/PrintLib.h>

#include <HiiFormGeneratorLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>

/** Check if the form has already been added.

  Check if the form has already been added to the formset.

  @param [in]  FormList         List under which form is to be searched.
  @param [in]  FormId           Form ID to look for in the formset.

  @retval TRUE if the form exists, FALSE otherwise

**/
static
BOOLEAN
IsFormPresent (
  IN  LIST_ENTRY   *FormList,
  IN  EFI_FORM_ID  FormId
  )
{
  LIST_ENTRY    *Link;
  DYN_HII_FORM  *Form;

  Link = GetFirstNode (FormList);
  while (!IsNull (FormList, Link)) {
    Form = BASE_CR (Link, DYN_HII_FORM, Hdr.Link);
    if (Form->FormId == FormId) {
      return TRUE;
    }

    Link = GetNextNode (FormList, Link);
  }

  return FALSE;
}

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

/** Check if the statement/question has already been added.

  Check if the statement/question has already been added to the provided
  StatementList.

  @param [in]  StatementList    List to check for the Statement.
  @param [in]  QuestionId       Question ID to look for in the list.

  @retval TRUE if the statement exists, FALSE otherwise

**/
static
BOOLEAN
IsStatementPresent (
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

/** Add buffer variable store under the given formset.

  Add the buffer variable store under a formset.

  @param [in]  Formset     Formset under which varstore is to be added.
  @param [in]  Guid        Varstore GUID.
  @param [in]  VarstoreId  Varstore ID.
  @param [in]  Size        Size of the varstore.
  @param [in]  Name        Ascii Name of the varstore.

  @retval EFI_SUCCESS             Varstore added successfully.
  @retval EFI_INVALID_PARAMETER   Invalid input parameters.
  @retval EFI_OUT_OF_RESOURCES    Unable to allocate memory.

**/
static
EFI_STATUS
EFIAPI
DynHiiAddVarstoreBuffer (
  IN         DYN_HII_FORMSET  *Formset,
  IN  CONST  EFI_GUID         *Guid,
  IN         EFI_VARSTORE_ID  VarstoreId,
  IN         UINT16           Size,
  IN         CHAR8            *Name
  )
{
  DYN_HII_VARSTORE  *Varstore;

  if ((Formset == NULL) || (Guid == NULL) || (Name == NULL) || (Size == 0)) {
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

  CopyGuid (&Varstore->Data.Buffer.Guid, Guid);
  Varstore->Data.Buffer.VarstoreId = VarstoreId;
  Varstore->Data.Buffer.Size       = Size;
  Varstore->Data.Buffer.Name       = Name;

  InsertTailList (&Formset->VarstoreList, &Varstore->Hdr.Link);

  return EFI_SUCCESS;
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

/** Get a Form from a given formset

  Get a form from the given formset.

  @param [in]  Formset           Pointer to the formset from which the form is
                                 to be fetched.
  @param [in]  FormId            The form ID of the required form.

  @retval  A pointer to the form object or NULL if form not found.

**/
DYN_HII_FORM *
EFIAPI
DynHiiGetForm (
  IN  DYN_HII_FORMSET  *Formset,
  IN  EFI_FORM_ID      FormId
  )
{
  LIST_ENTRY    *Link;
  DYN_HII_FORM  *Form;

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

/** Create a HII Formset.

  Create a HII Formset under which other HII nodes can
  be added subsequently.

  @param [in]  FormsetGuid      Formset GUID
  @param [in]  ClassGuid        Pointer to an array of Class GUIDs, if any.
  @param [in]  Title            String ID to the Formset title text.
  @param [in]  Help             String ID to the Formset help text.
  @param [in]  ClassGuidCount   Number of Class GUIDs(can be 0 - 3).
  @param [out] Formset          The Formset object created by the function.


  @retval  EFI_SUCCESS            The Formset was created successfully.
  @retval  EFI_INVALID_PARAMETER  The FormsetGuid or Formset is NULL, or the
                                  ClassGuidCount > 3.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiCreateFormSet (
  IN  CONST EFI_GUID   *FormsetGuid,
  IN  CONST EFI_GUID   *ClassGuid,
  IN  EFI_STRING_ID    Title,
  IN  EFI_STRING_ID    Help,
  IN  UINT8            ClassGuidCount,
  OUT DYN_HII_FORMSET  **Formset
  )
{
  UINT8            Idx;
  DYN_HII_FORMSET  *NewFormset;

  if ((FormsetGuid == NULL) || (Formset == NULL)) {
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
  NewFormset->ClassGuidCount = ClassGuidCount;

  for (Idx = 0; Idx < ClassGuidCount; Idx++) {
    CopyGuid (&NewFormset->ClassGuid[Idx], &ClassGuid[Idx]);
  }

  InitializeListHead (&NewFormset->FormList);
  InitializeListHead (&NewFormset->VarstoreList);
  InitializeListHead (&NewFormset->DefaultstoreList);

  *Formset = NewFormset;

  return EFI_SUCCESS;
}

/** Add a form to a Formset.

  Create a HII Form and add it to the given Formset.

  @param [in]  Formset          Formset under which Form is to be added.
  @param [in]  FormId           Form Identifier.
  @param [in]  Title            String ID to the Form's title text.

  @retval  EFI_SUCCESS            The Form was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Formset is NULL.
  @retval  EFI_ALREADY_STARTED    The Form has already been added to the Formset.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddForm (
  IN  DYN_HII_FORMSET  *Formset,
  IN  EFI_FORM_ID      FormId,
  IN  EFI_STRING_ID    Title
  )
{
  DYN_HII_FORM  *NewForm;

  if (Formset == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsFormPresent (&Formset->FormList, FormId)) {
    return EFI_ALREADY_STARTED;
  }

  NewForm = AllocateZeroPool (sizeof (*NewForm));
  if (NewForm == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewForm->Hdr.Type  = DynHiiNodeForm;
  NewForm->Hdr.Scope = TRUE;
  InitializeListHead (&NewForm->Hdr.Link);

  NewForm->FormId = FormId;
  NewForm->Title  = Title;

  InitializeListHead (&NewForm->StatementList);
  InsertTailList (&Formset->FormList, &NewForm->Hdr.Link);

  return EFI_SUCCESS;
}

/** Add a Statement/Question to a Form.

  Create a HII Statement/Question and add it to the given Form.

  @param [in]  Form             Form under which Statement is to be added.
  @param [in]  QuestionType     Type of Question to be added.
  @param [in]  QuestionData     Data of the corresponding Statement to be added.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Formset is NULL.
  @retval  EFI_ALREADY_STARTED    The Form has already been added to the Formset.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddStatement (
  IN  DYN_HII_FORM           *Form,
  IN  DYN_HII_QUESTION_TYPE  QuestionType,
  IN  DYN_HII_QUESTION_DATA  *QuestionData
  )
{
  DYN_HII_STATEMENT      *NewStmt;
  EFI_QUESTION_ID        QuestionId;
  DYN_HII_QUESTION_DATA  *NewQuestionData;

  if (Form == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (QuestionData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  QuestionId = QuestionData->QuestionHdr.QuestionId;
  if (IsStatementPresent (&Form->StatementList, QuestionId)) {
    return EFI_ALREADY_STARTED;
  }

  NewStmt = AllocateZeroPool (sizeof (*NewStmt));
  if (NewStmt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewStmt->Hdr.Type  = DynHiiNodeStatement;
  NewStmt->Hdr.Scope = TRUE;
  InitializeListHead (&NewStmt->Hdr.Link);

  NewStmt->QuestionType     = QuestionType;
  NewStmt->Data.QuestionHdr = QuestionData->QuestionHdr;

  InitializeListHead (&NewStmt->OptionList);
  InitializeListHead (&NewStmt->DefaultList);

  NewQuestionData = &NewStmt->Data;
  // Type-specific defaults.
  switch (QuestionType) {
    case DynHiiQtCheckbox:
      NewQuestionData->Question.Checkbox = QuestionData->Question.Checkbox;
      break;

    case DynHiiQtNumeric:
      NewQuestionData->Question.Numeric = QuestionData->Question.Numeric;
      break;

    case DynHiiQtRef:
      NewStmt->Hdr.Scope            = FALSE;
      NewQuestionData->Question.Ref = QuestionData->Question.Ref;
      break;

    case DynHiiQtOneOf:
      NewQuestionData->Question.OneOf = QuestionData->Question.OneOf;
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

/** Add an Option to a Question.

  Create a HII Option and add it to the given Statement.

  @param [in]  Statement        Question/Statement under which Option is to be
                                added.
  @param [in]  Text             The String ID for the option description.
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
  IN  DYN_HII_STATEMENT   *Statement,
  IN  EFI_STRING_ID       Text,
  IN  UINT8               Flags,
  IN  UINT8               Type,
  IN  EFI_IFR_TYPE_VALUE  Value
  )
{
  DYN_HII_ONE_OF_OPTION  *NewOption;

  if (Statement == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Statement->Hdr.Type != DynHiiNodeStatement) {
    return EFI_INVALID_PARAMETER;
  }

  /// For now, it is only the OneOf question type that
  /// will use the Option
  if (Statement->QuestionType != DynHiiQtOneOf) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Flags != 0x0) &&
      (Flags != EFI_IFR_OPTION_DEFAULT) &&
      (Flags != EFI_IFR_OPTION_DEFAULT_MFG))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Type > EFI_IFR_TYPE_REF) {
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

  InsertTailList (&Statement->OptionList, &NewOption->Hdr.Link);

  return EFI_SUCCESS;
}

/** Add a Default to a Question.

  Create a HII Default and add it to the given Statement.

  @param [in]  Form             Question/Statement under which Default is to be
                                added.
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
  IN  DYN_HII_STATEMENT   *Statement,
  IN  UINT16              DefaultId,
  IN  UINT8               Type,
  IN  EFI_IFR_TYPE_VALUE  Value
  )
{
  DYN_HII_DEFAULT  *NewDefault;

  if (Statement == NULL) {
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

/** Add a Variable Store (Varstore) under a Formset.

  Create a HII Varstore and add it to the given Formset.

  @param [in]  Formset          Formset under which the Varstore is to be added.
  @param [in]  VarstoreType     Type of Varstore being added.
  @param [in]  Data             Data of the corresponding Varstore to be added.

  @retval  EFI_SUCCESS            The varstore was created successfully.
  @retval  EFI_UNSUPPORTED        Adding Varstore of the given type is currently
                                  not supported.
  @retval  EFI_INVALID_PARAMETER  The VarstoreType, or any other input parameter
                                  passed through Data is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddFormsetVarStore (
  IN  DYN_HII_FORMSET        *Formset,
  IN  DYN_HII_VARSTORE_TYPE  VarstoreType,
  IN  DYN_HII_VARSTORE_DATA  *Data
  )
{
  EFI_STATUS                    Status;
  DYN_HII_VARSTORE_BUFFER_DATA  *Buffer;

  switch (VarstoreType) {
    case DynHiiVarstoreBuffer:
      Buffer = &Data->Buffer;
      Status = DynHiiAddVarstoreBuffer (
                 Formset,
                 &Buffer->Guid,
                 Buffer->VarstoreId,
                 Buffer->Size,
                 Buffer->Name
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      break;

    case DynHiiVarstoreEfi:
      return EFI_UNSUPPORTED;
      break;

    case DynHiiVarstoreNameValue:
      return EFI_UNSUPPORTED;
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Free up all the memory used by a Formset hierarchy.

  Free up all the memory that had been allocated for the Formset and it's
  child nodes, including Varstores, Forms, individual Statements etc.

  @param [in]  Formset          The formset hierarchy that needs to be
                                freed up.

**/
VOID
EFIAPI
DynHiiFreeFormSet (
  IN DYN_HII_FORMSET  *Formset
  )
{
  if (Formset == NULL) {
    return;
  }

  DynHiiFreeVarstoreList (&Formset->VarstoreList);
  DynHiiFreeDefaultStoreList (&Formset->DefaultstoreList);
  DynHiiFreeFormList (&Formset->FormList);

  FreePool (Formset);
}
