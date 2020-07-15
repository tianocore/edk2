/** @file
Utility functions for UI presentation.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Setup.h"

BOOLEAN            mHiiPackageListUpdated;
UI_MENU_SELECTION  *gCurrentSelection;
EFI_HII_HANDLE     mCurrentHiiHandle = NULL;
EFI_GUID           mCurrentFormSetGuid = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
UINT16             mCurrentFormId = 0;
EFI_EVENT          mValueChangedEvent = NULL;
LIST_ENTRY         mRefreshEventList = INITIALIZE_LIST_HEAD_VARIABLE (mRefreshEventList);
UINT16             mCurFakeQestId;
FORM_DISPLAY_ENGINE_FORM gDisplayFormData;
BOOLEAN            mFinishRetrieveCall = FALSE;
BOOLEAN            mDynamicFormUpdated = FALSE;

/**
  Check whether the ConfigAccess protocol is available.

  @param FormSet           FormSet of which the ConfigAcces protocol need to be checked.

  @retval EFI_SUCCESS     The function executed successfully.

**/
EFI_STATUS
CheckConfigAccess(
  IN FORM_BROWSER_FORMSET  *FormSet
  )
{
  EFI_STATUS                      Status;

  Status = gBS->HandleProtocol (
                  FormSet->DriverHandle,
                  &gEfiHiiConfigAccessProtocolGuid,
                  (VOID **) &FormSet->ConfigAccess
                  );
  if (EFI_ERROR (Status)) {
    //
    // Configuration Driver don't attach ConfigAccess protocol to its HII package
    // list, then there will be no configuration action required.
    // Or the ConfigAccess protocol has been uninstalled.
    //
    FormSet->ConfigAccess = NULL;
  }

  return EFI_SUCCESS;
}

/**
  Evaluate all expressions in a Form.

  @param  FormSet        FormSet this Form belongs to.
  @param  Form           The Form.

  @retval EFI_SUCCESS    The expression evaluated successfuly

**/
EFI_STATUS
EvaluateFormExpressions (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN FORM_BROWSER_FORM     *Form
  )
{
  EFI_STATUS       Status;
  LIST_ENTRY       *Link;
  FORM_EXPRESSION  *Expression;

  Link = GetFirstNode (&Form->ExpressionListHead);
  while (!IsNull (&Form->ExpressionListHead, Link)) {
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    Link = GetNextNode (&Form->ExpressionListHead, Link);

    if (Expression->Type == EFI_HII_EXPRESSION_INCONSISTENT_IF ||
        Expression->Type == EFI_HII_EXPRESSION_NO_SUBMIT_IF ||
        Expression->Type == EFI_HII_EXPRESSION_WARNING_IF ||
        Expression->Type == EFI_HII_EXPRESSION_WRITE ||
        (Expression->Type == EFI_HII_EXPRESSION_READ && Form->FormType != STANDARD_MAP_FORM_TYPE)) {
      //
      // Postpone Form validation to Question editing or Form submitting or Question Write or Question Read for nonstandard form.
      //
      continue;
    }

    Status = EvaluateExpression (FormSet, Form, Expression);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Base on the opcode buffer info to get the display statement.

  @param OpCode    The input opcode buffer for this statement.

  @retval Statement  The statement use this opcode buffer.

**/
FORM_DISPLAY_ENGINE_STATEMENT *
GetDisplayStatement (
  IN EFI_IFR_OP_HEADER     *OpCode
  )
{
  FORM_DISPLAY_ENGINE_STATEMENT *DisplayStatement;
  LIST_ENTRY                    *Link;

  Link = GetFirstNode (&gDisplayFormData.StatementListHead);
  while (!IsNull (&gDisplayFormData.StatementListHead, Link)) {
    DisplayStatement = FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK (Link);

    if (DisplayStatement->OpCode == OpCode) {
      return DisplayStatement;
    }
    Link = GetNextNode (&gDisplayFormData.StatementListHead, Link);
  }

  return NULL;
}

/**
  Free the refresh event list.

**/
VOID
FreeRefreshEvent (
  VOID
  )
{
  LIST_ENTRY   *Link;
  FORM_BROWSER_REFRESH_EVENT_NODE *EventNode;

  while (!IsListEmpty (&mRefreshEventList)) {
    Link = GetFirstNode (&mRefreshEventList);
    EventNode = FORM_BROWSER_REFRESH_EVENT_FROM_LINK (Link);
    RemoveEntryList (&EventNode->Link);

    gBS->CloseEvent (EventNode->RefreshEvent);

    FreePool (EventNode);
  }
}

/**
  Check whether this statement value is changed. If yes, update the statement value and return TRUE;
  else return FALSE.

  @param Statement           The statement need to check.

**/
VOID
UpdateStatement (
  IN OUT FORM_BROWSER_STATEMENT        *Statement
  )
{
  GetQuestionValue (gCurrentSelection->FormSet, gCurrentSelection->Form, Statement, GetSetValueWithHiiDriver);

  //
  // Reset FormPackage update flag
  //
  mHiiPackageListUpdated = FALSE;

  //
  // Question value may be changed, need invoke its Callback()
  //
  ProcessCallBackFunction (gCurrentSelection, gCurrentSelection->FormSet, gCurrentSelection->Form, Statement, EFI_BROWSER_ACTION_RETRIEVE, FALSE);

  if (mHiiPackageListUpdated) {
    //
    // Package list is updated, force to reparse IFR binary of target Formset
    //
    mHiiPackageListUpdated = FALSE;
    gCurrentSelection->Action = UI_ACTION_REFRESH_FORMSET;
  }
}

/**
  Refresh the question which has refresh guid event attribute.

  @param Event    The event which has this function related.
  @param Context  The input context info related to this event or the status code return to the caller.
**/
VOID
EFIAPI
RefreshEventNotifyForStatement(
  IN      EFI_EVENT Event,
  IN      VOID      *Context
  )
{
  FORM_BROWSER_STATEMENT        *Statement;

  Statement = (FORM_BROWSER_STATEMENT *)Context;
  UpdateStatement(Statement);
  gBS->SignalEvent (mValueChangedEvent);
}

/**
  Refresh the questions within this form.

  @param Event    The event which has this function related.
  @param Context  The input context info related to this event or the status code return to the caller.
**/
VOID
EFIAPI
RefreshEventNotifyForForm(
  IN      EFI_EVENT Event,
  IN      VOID      *Context
  )
{
  gCurrentSelection->Action = UI_ACTION_REFRESH_FORMSET;

  gBS->SignalEvent (mValueChangedEvent);
}

/**
  Create refresh hook event for statement which has refresh event or interval.

  @param Statement           The statement need to check.

**/
VOID
CreateRefreshEventForStatement (
  IN     FORM_BROWSER_STATEMENT        *Statement
  )
{
  EFI_STATUS                      Status;
  EFI_EVENT                       RefreshEvent;
  FORM_BROWSER_REFRESH_EVENT_NODE *EventNode;

  //
  // If question has refresh guid, create the notify function.
  //
  Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    RefreshEventNotifyForStatement,
                    Statement,
                    &Statement->RefreshGuid,
                    &RefreshEvent);
  ASSERT_EFI_ERROR (Status);

  EventNode = AllocateZeroPool (sizeof (FORM_BROWSER_REFRESH_EVENT_NODE));
  ASSERT (EventNode != NULL);
  EventNode->RefreshEvent = RefreshEvent;
  InsertTailList(&mRefreshEventList, &EventNode->Link);
}

/**
  Create refresh hook event for form which has refresh event or interval.

  @param Form           The form need to check.

**/
VOID
CreateRefreshEventForForm (
  IN     FORM_BROWSER_FORM        *Form
  )
{
  EFI_STATUS                      Status;
  EFI_EVENT                       RefreshEvent;
  FORM_BROWSER_REFRESH_EVENT_NODE *EventNode;

  //
  // If question has refresh guid, create the notify function.
  //
  Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    RefreshEventNotifyForForm,
                    Form,
                    &Form->RefreshGuid,
                    &RefreshEvent);
  ASSERT_EFI_ERROR (Status);

  EventNode = AllocateZeroPool (sizeof (FORM_BROWSER_REFRESH_EVENT_NODE));
  ASSERT (EventNode != NULL);
  EventNode->RefreshEvent = RefreshEvent;
  InsertTailList(&mRefreshEventList, &EventNode->Link);
}

/**

  Initialize the Display statement structure data.

  @param DisplayStatement      Pointer to the display Statement data strucure.
  @param Statement             The statement need to check.
**/
VOID
InitializeDisplayStatement (
  IN OUT FORM_DISPLAY_ENGINE_STATEMENT *DisplayStatement,
  IN     FORM_BROWSER_STATEMENT        *Statement
  )
{
  LIST_ENTRY                 *Link;
  QUESTION_OPTION            *Option;
  DISPLAY_QUESTION_OPTION    *DisplayOption;
  FORM_DISPLAY_ENGINE_STATEMENT *ParentStatement;

  DisplayStatement->Signature = FORM_DISPLAY_ENGINE_STATEMENT_SIGNATURE;
  DisplayStatement->Version   = FORM_DISPLAY_ENGINE_STATEMENT_VERSION_1;
  DisplayStatement->OpCode    = Statement->OpCode;
  InitializeListHead (&DisplayStatement->NestStatementList);
  InitializeListHead (&DisplayStatement->OptionListHead);

  if ((EvaluateExpressionList(Statement->Expression, FALSE, NULL, NULL) == ExpressGrayOut) || Statement->Locked) {
    DisplayStatement->Attribute |= HII_DISPLAY_GRAYOUT;
  }
  if ((Statement->ValueExpression != NULL) || ((Statement->QuestionFlags & EFI_IFR_FLAG_READ_ONLY) != 0)) {
    DisplayStatement->Attribute |= HII_DISPLAY_READONLY;
  }

  //
  // Initilize the option list in statement.
  //
  Link = GetFirstNode (&Statement->OptionListHead);
  while (!IsNull (&Statement->OptionListHead, Link)) {
    Option = QUESTION_OPTION_FROM_LINK (Link);
    Link = GetNextNode (&Statement->OptionListHead, Link);
    if ((Option->SuppressExpression != NULL) &&
        ((EvaluateExpressionList(Option->SuppressExpression, FALSE, NULL, NULL) == ExpressSuppress))) {
      continue;
    }

    DisplayOption = AllocateZeroPool (sizeof (DISPLAY_QUESTION_OPTION));
    ASSERT (DisplayOption != NULL);

    DisplayOption->ImageId      = Option->ImageId;
    DisplayOption->Signature    = DISPLAY_QUESTION_OPTION_SIGNATURE;
    DisplayOption->OptionOpCode = Option->OpCode;
    InsertTailList(&DisplayStatement->OptionListHead, &DisplayOption->Link);
  }

  CopyMem (&DisplayStatement->CurrentValue, &Statement->HiiValue, sizeof (EFI_HII_VALUE));

  //
  // Some special op code need an extra buffer to save the data.
  // Such as string, password, orderedlist...
  //
  if (Statement->BufferValue != NULL) {
    //
    // Ordered list opcode may not initilized, get default value here.
    //
    if (Statement->OpCode->OpCode == EFI_IFR_ORDERED_LIST_OP && GetArrayData (Statement->BufferValue, Statement->ValueType, 0) == 0) {
      GetQuestionDefault (gCurrentSelection->FormSet, gCurrentSelection->Form, Statement, 0);
    }

    DisplayStatement->CurrentValue.Buffer    = AllocateCopyPool(Statement->StorageWidth,Statement->BufferValue);
    DisplayStatement->CurrentValue.BufferLen = Statement->StorageWidth;
  }

  DisplayStatement->SettingChangedFlag = Statement->ValueChanged;

  //
  // Get the highlight statement for current form.
  //
  if (((gCurrentSelection->QuestionId != 0) && (Statement->QuestionId == gCurrentSelection->QuestionId)) ||
      ((mCurFakeQestId != 0) && (Statement->FakeQuestionId == mCurFakeQestId))) {
    gDisplayFormData.HighLightedStatement = DisplayStatement;
  }

  //
  // Create the refresh event process function.
  //
  if (!IsZeroGuid (&Statement->RefreshGuid)) {
    CreateRefreshEventForStatement (Statement);
  }

  //
  // For RTC type of date/time, set default refresh interval to be 1 second.
  //
  if ((Statement->Operand == EFI_IFR_DATE_OP || Statement->Operand == EFI_IFR_TIME_OP) && Statement->Storage == NULL) {
    Statement->RefreshInterval = 1;
  }

  //
  // Create the refresh guid hook event.
  // If the statement in this form has refresh event or refresh interval, browser will create this event for display engine.
  //
  if ((!IsZeroGuid (&Statement->RefreshGuid)) || (Statement->RefreshInterval != 0)) {
    gDisplayFormData.FormRefreshEvent = mValueChangedEvent;
  }

  //
  // Save the password check function for later use.
  //
  if (Statement->Operand == EFI_IFR_PASSWORD_OP) {
    DisplayStatement->PasswordCheck = PasswordCheck;
  }

  //
  // If this statement is nest in the subtitle, insert to the host statement.
  // else insert to the form it belongs to.
  //
  if (Statement->ParentStatement != NULL) {
    ParentStatement = GetDisplayStatement(Statement->ParentStatement->OpCode);
    ASSERT (ParentStatement != NULL);
    InsertTailList(&ParentStatement->NestStatementList, &DisplayStatement->DisplayLink);
  } else {
    InsertTailList(&gDisplayFormData.StatementListHead, &DisplayStatement->DisplayLink);
  }
}

/**
  Process for the refresh interval statement.

  @param Event    The Event need to be process
  @param Context  The context of the event.

**/
VOID
EFIAPI
RefreshIntervalProcess (
  IN  EFI_EVENT    Event,
  IN  VOID         *Context
  )
{
  FORM_BROWSER_STATEMENT        *Statement;
  LIST_ENTRY                    *Link;

  Link = GetFirstNode (&gCurrentSelection->Form->StatementListHead);
  while (!IsNull (&gCurrentSelection->Form->StatementListHead, Link)) {
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
    Link = GetNextNode (&gCurrentSelection->Form->StatementListHead, Link);

    if (Statement->RefreshInterval == 0) {
      continue;
    }

    UpdateStatement(Statement);
  }

  gBS->SignalEvent (mValueChangedEvent);
}

/**

  Make a copy of the global hotkey info.

**/
VOID
UpdateHotkeyList (
  VOID
  )
{
  BROWSER_HOT_KEY  *HotKey;
  BROWSER_HOT_KEY  *CopyKey;
  LIST_ENTRY       *Link;

  Link = GetFirstNode (&gBrowserHotKeyList);
  while (!IsNull (&gBrowserHotKeyList, Link)) {
    HotKey = BROWSER_HOT_KEY_FROM_LINK (Link);

    CopyKey             = AllocateCopyPool(sizeof (BROWSER_HOT_KEY), HotKey);
    ASSERT (CopyKey != NULL);
    CopyKey->KeyData    = AllocateCopyPool(sizeof (EFI_INPUT_KEY), HotKey->KeyData);
    ASSERT (CopyKey->KeyData != NULL);
    CopyKey->HelpString = AllocateCopyPool(StrSize (HotKey->HelpString), HotKey->HelpString);
    ASSERT (CopyKey->HelpString != NULL);

    InsertTailList(&gDisplayFormData.HotKeyListHead, &CopyKey->Link);

    Link = GetNextNode (&gBrowserHotKeyList, Link);
  }
}

/**

  Get the extra question attribute from override question list.

  @param    QuestionId    The question id for this request question.

  @retval   The attribute for this question or NULL if not found this
            question in the list.

**/
UINT32
ProcessQuestionExtraAttr (
  IN   EFI_QUESTION_ID  QuestionId
  )
{
  LIST_ENTRY                   *Link;
  QUESTION_ATTRIBUTE_OVERRIDE  *QuestionDesc;

  //
  // Return HII_DISPLAY_NONE if input a invalid question id.
  //
  if (QuestionId == 0) {
    return HII_DISPLAY_NONE;
  }

  Link = GetFirstNode (&mPrivateData.FormBrowserEx2.OverrideQestListHead);
  while (!IsNull (&mPrivateData.FormBrowserEx2.OverrideQestListHead, Link)) {
    QuestionDesc = FORM_QUESTION_ATTRIBUTE_OVERRIDE_FROM_LINK (Link);
    Link = GetNextNode (&mPrivateData.FormBrowserEx2.OverrideQestListHead, Link);

    if ((QuestionDesc->QuestionId == QuestionId) &&
        (QuestionDesc->FormId     == gCurrentSelection->FormId) &&
        (QuestionDesc->HiiHandle  == gCurrentSelection->Handle) &&
        CompareGuid (&QuestionDesc->FormSetGuid, &gCurrentSelection->FormSetGuid)) {
      return QuestionDesc->Attribute;
    }
  }

  return HII_DISPLAY_NONE;
}

/**

  Enum all statement in current form, find all the statement can be display and
  add to the display form.

**/
VOID
AddStatementToDisplayForm (
  VOID
  )
{
  EFI_STATUS                    Status;
  LIST_ENTRY                    *Link;
  FORM_BROWSER_STATEMENT        *Statement;
  FORM_DISPLAY_ENGINE_STATEMENT *DisplayStatement;
  UINT8                         MinRefreshInterval;
  EFI_EVENT                     RefreshIntervalEvent;
  FORM_BROWSER_REFRESH_EVENT_NODE *EventNode;
  BOOLEAN                       FormEditable;
  UINT32                        ExtraAttribute;

  MinRefreshInterval   = 0;
  FormEditable         = FALSE;

  //
  // Process the statement outside the form, these statements are not recognized
  // by browser core.
  //
  Link = GetFirstNode (&gCurrentSelection->FormSet->StatementListOSF);
  while (!IsNull (&gCurrentSelection->FormSet->StatementListOSF, Link)) {
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
    Link = GetNextNode (&gCurrentSelection->FormSet->StatementListOSF, Link);

    DisplayStatement = AllocateZeroPool (sizeof (FORM_DISPLAY_ENGINE_STATEMENT));
    ASSERT (DisplayStatement != NULL);
    DisplayStatement->Signature = FORM_DISPLAY_ENGINE_STATEMENT_SIGNATURE;
    DisplayStatement->Version   = FORM_DISPLAY_ENGINE_STATEMENT_VERSION_1;
    DisplayStatement->OpCode = Statement->OpCode;

    InitializeListHead (&DisplayStatement->NestStatementList);
    InitializeListHead (&DisplayStatement->OptionListHead);

    InsertTailList(&gDisplayFormData.StatementListOSF, &DisplayStatement->DisplayLink);
  }

  //
  // treat formset as statement outside the form,get its opcode.
  //
  DisplayStatement = AllocateZeroPool (sizeof (FORM_DISPLAY_ENGINE_STATEMENT));
  ASSERT (DisplayStatement != NULL);

  DisplayStatement->Signature = FORM_DISPLAY_ENGINE_STATEMENT_SIGNATURE;
  DisplayStatement->Version   = FORM_DISPLAY_ENGINE_STATEMENT_VERSION_1;
  DisplayStatement->OpCode = gCurrentSelection->FormSet->OpCode;

  InitializeListHead (&DisplayStatement->NestStatementList);
  InitializeListHead (&DisplayStatement->OptionListHead);

  InsertTailList(&gDisplayFormData.StatementListOSF, &DisplayStatement->DisplayLink);

  //
  // Process the statement in this form.
  //
  Link = GetFirstNode (&gCurrentSelection->Form->StatementListHead);
  while (!IsNull (&gCurrentSelection->Form->StatementListHead, Link)) {
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
    Link = GetNextNode (&gCurrentSelection->Form->StatementListHead, Link);

    //
    // This statement can't be show, skip it.
    //
    if (EvaluateExpressionList(Statement->Expression, FALSE, NULL, NULL) > ExpressGrayOut) {
      continue;
    }

    //
    // Check the extra attribute.
    //
    ExtraAttribute = ProcessQuestionExtraAttr (Statement->QuestionId);
    if ((ExtraAttribute & HII_DISPLAY_SUPPRESS) != 0) {
      continue;
    }

    DisplayStatement = AllocateZeroPool (sizeof (FORM_DISPLAY_ENGINE_STATEMENT));
    ASSERT (DisplayStatement != NULL);

    //
    // Initialize this statement and add it to the display form.
    //
    InitializeDisplayStatement(DisplayStatement, Statement);

    //
    // Set the extra attribute.
    //
    DisplayStatement->Attribute |= ExtraAttribute;

    if (Statement->Storage != NULL) {
      FormEditable = TRUE;
    }

    //
    // Get the minimal refresh interval value for later use.
    //
    if ((Statement->RefreshInterval != 0) &&
      (MinRefreshInterval == 0 || Statement->RefreshInterval < MinRefreshInterval)) {
      MinRefreshInterval = Statement->RefreshInterval;
    }
  }

  //
  // Create the periodic timer for refresh interval statement.
  //
  if (MinRefreshInterval != 0) {
    Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, RefreshIntervalProcess, NULL, &RefreshIntervalEvent);
    ASSERT_EFI_ERROR (Status);
    Status = gBS->SetTimer (RefreshIntervalEvent, TimerPeriodic, MinRefreshInterval * ONE_SECOND);
    ASSERT_EFI_ERROR (Status);

    EventNode = AllocateZeroPool (sizeof (FORM_BROWSER_REFRESH_EVENT_NODE));
    ASSERT (EventNode != NULL);
    EventNode->RefreshEvent = RefreshIntervalEvent;
    InsertTailList(&mRefreshEventList, &EventNode->Link);
  }

  //
  // Create the refresh event process function for Form.
  //
  if (!IsZeroGuid (&gCurrentSelection->Form->RefreshGuid)) {
    CreateRefreshEventForForm (gCurrentSelection->Form);
    if (gDisplayFormData.FormRefreshEvent == NULL) {
      gDisplayFormData.FormRefreshEvent = mValueChangedEvent;
    }
  }

  //
  // Update hotkey list field.
  //
  if (gBrowserSettingScope == SystemLevel || FormEditable) {
    UpdateHotkeyList();
  }
}

/**

  Initialize the SettingChangedFlag variable in the display form.

**/
VOID
UpdateDataChangedFlag (
  VOID
  )
{
  LIST_ENTRY           *Link;
  FORM_BROWSER_FORMSET *LocalFormSet;

  gDisplayFormData.SettingChangedFlag   = FALSE;

  if (IsNvUpdateRequiredForForm (gCurrentSelection->Form)) {
    gDisplayFormData.SettingChangedFlag = TRUE;
    return;
  }

  //
  // Base on the system level to check whether need to show the NV flag.
  //
  switch (gBrowserSettingScope) {
  case SystemLevel:
    //
    // Check the maintain list to see whether there is any change.
    //
    Link = GetFirstNode (&gBrowserFormSetList);
    while (!IsNull (&gBrowserFormSetList, Link)) {
      LocalFormSet = FORM_BROWSER_FORMSET_FROM_LINK (Link);
      if (IsNvUpdateRequiredForFormSet(LocalFormSet)) {
        gDisplayFormData.SettingChangedFlag = TRUE;
        return;
      }
      Link = GetNextNode (&gBrowserFormSetList, Link);
    }
    break;

  case FormSetLevel:
    if (IsNvUpdateRequiredForFormSet(gCurrentSelection->FormSet)) {
      gDisplayFormData.SettingChangedFlag = TRUE;
      return;
    }
    break;

  default:
    break;
  }
}

/**

  Initialize the Display form structure data.

**/
VOID
InitializeDisplayFormData (
  VOID
  )
{
  EFI_STATUS  Status;

  gDisplayFormData.Signature   = FORM_DISPLAY_ENGINE_FORM_SIGNATURE;
  gDisplayFormData.Version     = FORM_DISPLAY_ENGINE_VERSION_1;
  gDisplayFormData.ImageId     = 0;
  gDisplayFormData.AnimationId = 0;

  InitializeListHead (&gDisplayFormData.StatementListHead);
  InitializeListHead (&gDisplayFormData.StatementListOSF);
  InitializeListHead (&gDisplayFormData.HotKeyListHead);

  Status = gBS->CreateEvent (
        EVT_NOTIFY_WAIT,
        TPL_CALLBACK,
        EfiEventEmptyFunction,
        NULL,
        &mValueChangedEvent
        );
  ASSERT_EFI_ERROR (Status);
}

/**

  Free the kotkey info saved in form data.

**/
VOID
FreeHotkeyList (
  VOID
  )
{
  BROWSER_HOT_KEY  *HotKey;
  LIST_ENTRY       *Link;

  while (!IsListEmpty (&gDisplayFormData.HotKeyListHead)) {
    Link = GetFirstNode (&gDisplayFormData.HotKeyListHead);
    HotKey = BROWSER_HOT_KEY_FROM_LINK (Link);

    RemoveEntryList (&HotKey->Link);

    FreePool (HotKey->KeyData);
    FreePool (HotKey->HelpString);
    FreePool (HotKey);
  }
}

/**

  Update the Display form structure data.

**/
VOID
UpdateDisplayFormData (
  VOID
  )
{
  gDisplayFormData.FormTitle        = gCurrentSelection->Form->FormTitle;
  gDisplayFormData.FormId           = gCurrentSelection->FormId;
  gDisplayFormData.HiiHandle        = gCurrentSelection->Handle;
  CopyGuid (&gDisplayFormData.FormSetGuid, &gCurrentSelection->FormSetGuid);

  gDisplayFormData.Attribute        = 0;
  gDisplayFormData.Attribute       |= gCurrentSelection->Form->ModalForm ? HII_DISPLAY_MODAL : 0;
  gDisplayFormData.Attribute       |= gCurrentSelection->Form->Locked    ? HII_DISPLAY_LOCK  : 0;

  gDisplayFormData.FormRefreshEvent     = NULL;
  gDisplayFormData.HighLightedStatement = NULL;

  UpdateDataChangedFlag ();

  AddStatementToDisplayForm ();
}

/**

  Free the Display Statement structure data.

  @param   StatementList         Point to the statement list which need to be free.

**/
VOID
FreeStatementData (
  LIST_ENTRY           *StatementList
  )
{
  LIST_ENTRY                    *Link;
  LIST_ENTRY                    *OptionLink;
  FORM_DISPLAY_ENGINE_STATEMENT *Statement;
  DISPLAY_QUESTION_OPTION       *Option;

  //
  // Free Statements/Questions
  //
  while (!IsListEmpty (StatementList)) {
    Link = GetFirstNode (StatementList);
    Statement = FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK (Link);

    //
    // Free Options List
    //
    while (!IsListEmpty (&Statement->OptionListHead)) {
      OptionLink = GetFirstNode (&Statement->OptionListHead);
      Option = DISPLAY_QUESTION_OPTION_FROM_LINK (OptionLink);
      RemoveEntryList (&Option->Link);
      FreePool (Option);
    }

    //
    // Free nest statement List
    //
    if (!IsListEmpty (&Statement->NestStatementList)) {
      FreeStatementData(&Statement->NestStatementList);
    }

    RemoveEntryList (&Statement->DisplayLink);
    FreePool (Statement);
  }
}

/**

  Free the Display form structure data.

**/
VOID
FreeDisplayFormData (
  VOID
  )
{
  FreeStatementData (&gDisplayFormData.StatementListHead);
  FreeStatementData (&gDisplayFormData.StatementListOSF);

  FreeRefreshEvent();

  FreeHotkeyList();
}

/**

  Get FORM_BROWSER_STATEMENT from FORM_DISPLAY_ENGINE_STATEMENT based on the OpCode info.

  @param DisplayStatement        The input FORM_DISPLAY_ENGINE_STATEMENT.

  @retval FORM_BROWSER_STATEMENT  The return FORM_BROWSER_STATEMENT info.

**/
FORM_BROWSER_STATEMENT *
GetBrowserStatement (
  IN FORM_DISPLAY_ENGINE_STATEMENT *DisplayStatement
  )
{
  FORM_BROWSER_STATEMENT *Statement;
  LIST_ENTRY             *Link;

  Link = GetFirstNode (&gCurrentSelection->Form->StatementListHead);
  while (!IsNull (&gCurrentSelection->Form->StatementListHead, Link)) {
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);

    if (Statement->OpCode == DisplayStatement->OpCode) {
      return Statement;
    }

    Link = GetNextNode (&gCurrentSelection->Form->StatementListHead, Link);
  }

  return NULL;
}

/**
  Update the ValueChanged status for questions in this form.

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.

**/
VOID
UpdateStatementStatusForForm (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form
  )
{
  LIST_ENTRY                  *Link;
  FORM_BROWSER_STATEMENT      *Question;

  Link = GetFirstNode (&Form->StatementListHead);
  while (!IsNull (&Form->StatementListHead, Link)) {
    Question = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
    Link = GetNextNode (&Form->StatementListHead, Link);

    //
    // For password opcode, not set the the value changed flag.
    //
    if (Question->Operand == EFI_IFR_PASSWORD_OP) {
      continue;
    }

    IsQuestionValueChanged(FormSet, Form, Question, GetSetValueWithBuffer);
  }
}

/**
  Update the ValueChanged status for questions in this formset.

  @param  FormSet                FormSet data structure.

**/
VOID
UpdateStatementStatusForFormSet (
  IN FORM_BROWSER_FORMSET                *FormSet
  )
{
  LIST_ENTRY                  *Link;
  FORM_BROWSER_FORM           *Form;

  Link = GetFirstNode (&FormSet->FormListHead);
  while (!IsNull (&FormSet->FormListHead, Link)) {
    Form = FORM_BROWSER_FORM_FROM_LINK (Link);
    Link = GetNextNode (&FormSet->FormListHead, Link);

    UpdateStatementStatusForForm (FormSet, Form);
  }
}

/**
  Update the ValueChanged status for questions.

  @param  FormSet                FormSet data structure.
  @param  Form                   Form data structure.
  @param  SettingScope           Setting Scope for Default action.

**/
VOID
UpdateStatementStatus (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_FORM                *Form,
  IN BROWSER_SETTING_SCOPE            SettingScope
  )
{
  LIST_ENTRY                  *Link;
  FORM_BROWSER_FORMSET        *LocalFormSet;

  switch (SettingScope) {
  case SystemLevel:
    Link = GetFirstNode (&gBrowserFormSetList);
    while (!IsNull (&gBrowserFormSetList, Link)) {
      LocalFormSet = FORM_BROWSER_FORMSET_FROM_LINK (Link);
      Link = GetNextNode (&gBrowserFormSetList, Link);
      if (!ValidateFormSet(LocalFormSet)) {
        continue;
      }

      UpdateStatementStatusForFormSet (LocalFormSet);
    }
    break;

  case FormSetLevel:
    UpdateStatementStatusForFormSet (FormSet);
    break;

  case FormLevel:
    UpdateStatementStatusForForm (FormSet, Form);
    break;

  default:
    break;
  }
}

/**

  Process the action request in user input.

  @param Action                  The user input action request info.
  @param DefaultId               The user input default Id info.

  @retval EFI_SUCESSS            This function always return successfully for now.

**/
EFI_STATUS
ProcessAction (
  IN UINT32        Action,
  IN UINT16        DefaultId
  )
{
  //
  // This is caused by use press ESC, and it should not combine with other action type.
  //
  if ((Action & BROWSER_ACTION_FORM_EXIT) == BROWSER_ACTION_FORM_EXIT) {
    FindNextMenu (gCurrentSelection, FormLevel);
    return EFI_SUCCESS;
  }

  //
  // Below is normal hotkey trigged action, these action maybe combine with each other.
  //
  if ((Action & BROWSER_ACTION_DISCARD) == BROWSER_ACTION_DISCARD) {
    DiscardForm (gCurrentSelection->FormSet, gCurrentSelection->Form, gBrowserSettingScope);
  }

  if ((Action & BROWSER_ACTION_DEFAULT) == BROWSER_ACTION_DEFAULT) {
    ExtractDefault (gCurrentSelection->FormSet, gCurrentSelection->Form, DefaultId, gBrowserSettingScope, GetDefaultForAll, NULL, FALSE, FALSE);
    UpdateStatementStatus (gCurrentSelection->FormSet, gCurrentSelection->Form, gBrowserSettingScope);
  }

  if ((Action & BROWSER_ACTION_SUBMIT) == BROWSER_ACTION_SUBMIT) {
    SubmitForm (gCurrentSelection->FormSet, gCurrentSelection->Form, gBrowserSettingScope);
  }

  if ((Action & BROWSER_ACTION_RESET) == BROWSER_ACTION_RESET) {
    gResetRequiredFormLevel = TRUE;
    gResetRequiredSystemLevel = TRUE;
  }

  if ((Action & BROWSER_ACTION_EXIT) == BROWSER_ACTION_EXIT) {
    //
    // Form Exit without saving, Similar to ESC Key.
    // FormSet Exit without saving, Exit SendForm.
    // System Exit without saving, CallExitHandler and Exit SendForm.
    //
    DiscardForm (gCurrentSelection->FormSet, gCurrentSelection->Form, gBrowserSettingScope);
    if (gBrowserSettingScope == FormLevel || gBrowserSettingScope == FormSetLevel) {
      FindNextMenu (gCurrentSelection, gBrowserSettingScope);
    } else if (gBrowserSettingScope == SystemLevel) {
      if (ExitHandlerFunction != NULL) {
        ExitHandlerFunction ();
      }
      gCurrentSelection->Action = UI_ACTION_EXIT;
    }
  }

  return EFI_SUCCESS;
}

/**
  Check whether the formset guid is in this Hii package list.

  @param  HiiHandle              The HiiHandle for this HII package list.
  @param  FormSetGuid            The formset guid for the request formset.

  @retval TRUE                   Find the formset guid.
  @retval FALSE                  Not found the formset guid.

**/
BOOLEAN
GetFormsetGuidFromHiiHandle (
  IN EFI_HII_HANDLE       HiiHandle,
  IN EFI_GUID             *FormSetGuid
  )
{
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINTN                        BufferSize;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  EFI_STATUS                   Status;
  BOOLEAN                      FindGuid;

  BufferSize     = 0;
  HiiPackageList = NULL;
  FindGuid       = FALSE;

  Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, HiiHandle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = AllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, HiiHandle, &BufferSize, HiiPackageList);
  }
  if (EFI_ERROR (Status) || HiiPackageList == NULL) {
    return FALSE;
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
    Offset += PackageHeader.Length;

    if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Search FormSet in this Form Package
      //
      Offset2 = sizeof (EFI_HII_PACKAGE_HEADER);
      while (Offset2 < PackageHeader.Length) {
        OpCodeData = Package + Offset2;

        if (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP) {
          if (CompareGuid (FormSetGuid, (EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER)))){
            FindGuid = TRUE;
            break;
          }
        }

        Offset2 += ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
      }
    }
    if (FindGuid) {
      break;
    }
  }

  FreePool (HiiPackageList);

  return FindGuid;
}

/**
  Find HII Handle in the HII database associated with given Device Path.

  If DevicePath is NULL, then ASSERT.

  @param  DevicePath             Device Path associated with the HII package list
                                 handle.
  @param  FormsetGuid            The formset guid for this formset.

  @retval Handle                 HII package list Handle associated with the Device
                                        Path.
  @retval NULL                   Hii Package list handle is not found.

**/
EFI_HII_HANDLE
DevicePathToHiiHandle (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN EFI_GUID                   *FormsetGuid
  )
{
  EFI_STATUS                  Status;
  EFI_DEVICE_PATH_PROTOCOL    *TmpDevicePath;
  UINTN                       Index;
  EFI_HANDLE                  Handle;
  EFI_HANDLE                  DriverHandle;
  EFI_HII_HANDLE              *HiiHandles;
  EFI_HII_HANDLE              HiiHandle;

  ASSERT (DevicePath != NULL);

  TmpDevicePath = DevicePath;
  //
  // Locate Device Path Protocol handle buffer
  //
  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &TmpDevicePath,
                  &DriverHandle
                  );
  if (EFI_ERROR (Status) || !IsDevicePathEnd (TmpDevicePath)) {
    return NULL;
  }

  //
  // Retrieve all HII Handles from HII database
  //
  HiiHandles = HiiGetHiiHandles (NULL);
  if (HiiHandles == NULL) {
    return NULL;
  }

  //
  // Search Hii Handle by Driver Handle
  //
  HiiHandle = NULL;
  for (Index = 0; HiiHandles[Index] != NULL; Index++) {
    Status = mHiiDatabase->GetPackageListHandle (
                             mHiiDatabase,
                             HiiHandles[Index],
                             &Handle
                             );
    if (!EFI_ERROR (Status) && (Handle == DriverHandle)) {
      if (GetFormsetGuidFromHiiHandle(HiiHandles[Index], FormsetGuid)) {
        HiiHandle = HiiHandles[Index];
        break;
      }

      if (HiiHandle != NULL) {
        break;
      }
    }
  }

  FreePool (HiiHandles);
  return HiiHandle;
}

/**
  Find HII Handle in the HII database associated with given form set guid.

  If FormSetGuid is NULL, then ASSERT.

  @param  ComparingGuid          FormSet Guid associated with the HII package list
                                 handle.

  @retval Handle                 HII package list Handle associated with the Device
                                        Path.
  @retval NULL                   Hii Package list handle is not found.

**/
EFI_HII_HANDLE
FormSetGuidToHiiHandle (
  EFI_GUID     *ComparingGuid
  )
{
  EFI_HII_HANDLE               *HiiHandles;
  EFI_HII_HANDLE               HiiHandle;
  UINTN                        Index;

  ASSERT (ComparingGuid != NULL);

  HiiHandle  = NULL;
  //
  // Get all the Hii handles
  //
  HiiHandles = HiiGetHiiHandles (NULL);
  ASSERT (HiiHandles != NULL);

  //
  // Search for formset of each class type
  //
  for (Index = 0; HiiHandles[Index] != NULL; Index++) {
    if (GetFormsetGuidFromHiiHandle(HiiHandles[Index], ComparingGuid)) {
      HiiHandle = HiiHandles[Index];
      break;
    }

    if (HiiHandle != NULL) {
      break;
    }
  }

  FreePool (HiiHandles);

  return HiiHandle;
}

/**
  check how to process the changed data in current form or form set.

  @param Selection       On input, Selection tell setup browser the information
                         about the Selection, form and formset to be displayed.
                         On output, Selection return the screen item that is selected
                         by user.

  @param Scope           Data save or discard scope, form or formset.

  @retval                TRUE   Success process the changed data, will return to the parent form.
  @retval                FALSE  Reject to process the changed data, will stay at  current form.
**/
BOOLEAN
ProcessChangedData (
  IN OUT UI_MENU_SELECTION       *Selection,
  IN     BROWSER_SETTING_SCOPE   Scope
  )
{
  BOOLEAN    RetValue;
  EFI_STATUS Status;

  RetValue = TRUE;
  switch (mFormDisplay->ConfirmDataChange()) {
    case BROWSER_ACTION_DISCARD:
      DiscardForm (Selection->FormSet, Selection->Form, Scope);
      break;

    case BROWSER_ACTION_SUBMIT:
      Status = SubmitForm (Selection->FormSet, Selection->Form, Scope);
      if (EFI_ERROR (Status)) {
        RetValue = FALSE;
      }
      break;

    case BROWSER_ACTION_NONE:
      RetValue = FALSE;
      break;

    default:
      //
      // if Invalid value return, process same as BROWSER_ACTION_NONE.
      //
      RetValue = FALSE;
      break;
  }

  return RetValue;
}

/**
  Find parent formset menu(the first menu which has different formset) for current menu.
  If not find, just return to the first menu.

  @param Selection    The selection info.

**/
VOID
FindParentFormSet (
  IN OUT   UI_MENU_SELECTION           *Selection
  )
{
  FORM_ENTRY_INFO            *CurrentMenu;
  FORM_ENTRY_INFO            *ParentMenu;

  CurrentMenu = Selection->CurrentMenu;
  ParentMenu  = UiFindParentMenu(CurrentMenu, FormSetLevel);

  if (ParentMenu != NULL) {
    CopyMem (&Selection->FormSetGuid, &ParentMenu->FormSetGuid, sizeof (EFI_GUID));
    Selection->Handle = ParentMenu->HiiHandle;
    Selection->FormId     = ParentMenu->FormId;
    Selection->QuestionId = ParentMenu->QuestionId;
  } else {
    Selection->FormId     = CurrentMenu->FormId;
    Selection->QuestionId = CurrentMenu->QuestionId;
  }

  Selection->Statement  = NULL;
}

/**
  Process the goto op code, update the info in the selection structure.

  @param Statement    The statement belong to goto op code.
  @param Selection    The selection info.

  @retval EFI_SUCCESS    The menu process successfully.
  @return Other value if the process failed.
**/
EFI_STATUS
ProcessGotoOpCode (
  IN OUT   FORM_BROWSER_STATEMENT      *Statement,
  IN OUT   UI_MENU_SELECTION           *Selection
  )
{
  CHAR16                          *StringPtr;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  FORM_BROWSER_FORM               *RefForm;
  EFI_STATUS                      Status;
  EFI_HII_HANDLE                  HiiHandle;

  Status    = EFI_SUCCESS;
  StringPtr = NULL;
  HiiHandle = NULL;

  //
  // Prepare the device path check, get the device path info first.
  //
  if (Statement->HiiValue.Value.ref.DevicePath != 0) {
    StringPtr = GetToken (Statement->HiiValue.Value.ref.DevicePath, Selection->FormSet->HiiHandle);
  }

  //
  // Check whether the device path string is a valid string.
  //
  if (Statement->HiiValue.Value.ref.DevicePath != 0 && StringPtr != NULL && StringPtr[0] != L'\0') {
    if (Selection->Form->ModalForm) {
      return Status;
    }

    //
    // Goto another Hii Package list
    //
    if (mPathFromText != NULL) {
      DevicePath = mPathFromText->ConvertTextToDevicePath(StringPtr);
      if (DevicePath != NULL) {
        HiiHandle = DevicePathToHiiHandle (DevicePath, &Statement->HiiValue.Value.ref.FormSetGuid);
        FreePool (DevicePath);
      }
      FreePool (StringPtr);
    } else {
      //
      // Not found the EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL protocol.
      //
      PopupErrorMessage(BROWSER_PROTOCOL_NOT_FOUND, NULL, NULL, NULL);
      FreePool (StringPtr);
      return Status;
    }

    if (HiiHandle != Selection->Handle) {
      //
      // Goto another Formset, check for uncommitted data
      //
      if ((gBrowserSettingScope == FormLevel || gBrowserSettingScope == FormSetLevel) &&
          IsNvUpdateRequiredForFormSet(Selection->FormSet)) {
        if (!ProcessChangedData(Selection, FormSetLevel)) {
          return EFI_SUCCESS;
        }
      }
    }

    Selection->Action = UI_ACTION_REFRESH_FORMSET;
    Selection->Handle = HiiHandle;
    if (Selection->Handle == NULL) {
      //
      // If target Hii Handle not found, exit current formset.
      //
      FindParentFormSet(Selection);
      return EFI_SUCCESS;
    }

    CopyMem (&Selection->FormSetGuid,&Statement->HiiValue.Value.ref.FormSetGuid, sizeof (EFI_GUID));
    Selection->FormId = Statement->HiiValue.Value.ref.FormId;
    Selection->QuestionId = Statement->HiiValue.Value.ref.QuestionId;
  } else if (!IsZeroGuid (&Statement->HiiValue.Value.ref.FormSetGuid)) {
    if (Selection->Form->ModalForm) {
      return Status;
    }
    if (!CompareGuid (&Statement->HiiValue.Value.ref.FormSetGuid, &Selection->FormSetGuid)) {
      //
      // Goto another Formset, check for uncommitted data
      //
      if ((gBrowserSettingScope == FormLevel || gBrowserSettingScope == FormSetLevel) &&
         IsNvUpdateRequiredForFormSet(Selection->FormSet)) {
        if (!ProcessChangedData(Selection, FormSetLevel)) {
          return EFI_SUCCESS;
        }
      }
    }

    Selection->Action = UI_ACTION_REFRESH_FORMSET;
    Selection->Handle = FormSetGuidToHiiHandle(&Statement->HiiValue.Value.ref.FormSetGuid);
    if (Selection->Handle == NULL) {
      //
      // If target Hii Handle not found, exit current formset.
      //
      FindParentFormSet(Selection);
      return EFI_SUCCESS;
    }

    CopyMem (&Selection->FormSetGuid, &Statement->HiiValue.Value.ref.FormSetGuid, sizeof (EFI_GUID));
    Selection->FormId = Statement->HiiValue.Value.ref.FormId;
    Selection->QuestionId = Statement->HiiValue.Value.ref.QuestionId;
  } else if (Statement->HiiValue.Value.ref.FormId != 0) {
    //
    // Goto another Form, check for uncommitted data
    //
    if (Statement->HiiValue.Value.ref.FormId != Selection->FormId) {
      if ((gBrowserSettingScope == FormLevel && IsNvUpdateRequiredForForm(Selection->Form))) {
        if (!ProcessChangedData (Selection, FormLevel)) {
          return EFI_SUCCESS;
        }
      }
    }

    RefForm = IdToForm (Selection->FormSet, Statement->HiiValue.Value.ref.FormId);
    if ((RefForm != NULL) && (RefForm->SuppressExpression != NULL)) {
      if (EvaluateExpressionList(RefForm->SuppressExpression, TRUE, Selection->FormSet, RefForm) != ExpressFalse) {
        //
        // Form is suppressed.
        //
        PopupErrorMessage(BROWSER_FORM_SUPPRESS, NULL, NULL, NULL);
        return EFI_SUCCESS;
      }
    }

    Selection->FormId = Statement->HiiValue.Value.ref.FormId;
    Selection->QuestionId = Statement->HiiValue.Value.ref.QuestionId;
  } else if (Statement->HiiValue.Value.ref.QuestionId != 0) {
    Selection->QuestionId = Statement->HiiValue.Value.ref.QuestionId;
  }

  return Status;
}


/**
  Process Question Config.

  @param  Selection              The UI menu selection.
  @param  Question               The Question to be peocessed.

  @retval EFI_SUCCESS            Question Config process success.
  @retval Other                  Question Config process fail.

**/
EFI_STATUS
ProcessQuestionConfig (
  IN  UI_MENU_SELECTION       *Selection,
  IN  FORM_BROWSER_STATEMENT  *Question
  )
{
  EFI_STATUS                      Status;
  CHAR16                          *ConfigResp;
  CHAR16                          *Progress;

  if (Question->QuestionConfig == 0) {
    return EFI_SUCCESS;
  }

  //
  // Get <ConfigResp>
  //
  ConfigResp = GetToken (Question->QuestionConfig, Selection->FormSet->HiiHandle);
  if (ConfigResp == NULL) {
    return EFI_NOT_FOUND;
  } else if (ConfigResp[0] == L'\0') {
    return EFI_SUCCESS;
  }

  //
  // Send config to Configuration Driver
  //
  Status = mHiiConfigRouting->RouteConfig (
                           mHiiConfigRouting,
                           ConfigResp,
                           &Progress
                           );

  return Status;
}

/**

  Process the user input data.

  @param UserInput               The user input data.

  @retval EFI_SUCESSS            This function always return successfully for now.

**/
EFI_STATUS
ProcessUserInput (
  IN USER_INPUT               *UserInput
  )
{
  EFI_STATUS                    Status;
  FORM_BROWSER_STATEMENT        *Statement;

  Status    = EFI_SUCCESS;
  Statement = NULL;

  //
  // When Exit from FormDisplay function, one of the below two cases must be true.
  //
  ASSERT (UserInput->Action != 0 || UserInput->SelectedStatement != NULL);

  //
  // Remove the last highligh question id, this id will update when show next form.
  //
  gCurrentSelection->QuestionId = 0;
  if (UserInput->SelectedStatement != NULL){
    Statement = GetBrowserStatement(UserInput->SelectedStatement);
    ASSERT (Statement != NULL);

    //
    // This question is the current user select one,record it and later
    // show it as the highlight question.
    //
    gCurrentSelection->CurrentMenu->QuestionId = Statement->QuestionId;
    //
    // For statement like text, actio, it not has question id.
    // So use FakeQuestionId to save the question.
    //
    if (gCurrentSelection->CurrentMenu->QuestionId == 0) {
      mCurFakeQestId = Statement->FakeQuestionId;
    } else {
      mCurFakeQestId = 0;
    }
  }

  //
  // First process the Action field in USER_INPUT.
  //
  if (UserInput->Action != 0) {
    Status = ProcessAction (UserInput->Action, UserInput->DefaultId);
    gCurrentSelection->Statement = NULL;
  } else {
    ASSERT (Statement != NULL);
    gCurrentSelection->Statement = Statement;
    switch (Statement->Operand) {
    case EFI_IFR_REF_OP:
      Status = ProcessGotoOpCode(Statement, gCurrentSelection);
      break;

    case EFI_IFR_ACTION_OP:
      //
      // Process the Config string <ConfigResp>
      //
      Status = ProcessQuestionConfig (gCurrentSelection, Statement);
      break;

    case EFI_IFR_RESET_BUTTON_OP:
      //
      // Reset Question to default value specified by DefaultId
      //
      Status = ExtractDefault (gCurrentSelection->FormSet, NULL, Statement->DefaultId, FormSetLevel, GetDefaultForAll, NULL, FALSE, FALSE);
      UpdateStatementStatus (gCurrentSelection->FormSet, NULL, FormSetLevel);
      break;

    default:
      switch (Statement->Operand) {
      case EFI_IFR_STRING_OP:
        DeleteString(Statement->HiiValue.Value.string, gCurrentSelection->FormSet->HiiHandle);
        Statement->HiiValue.Value.string = UserInput->InputValue.Value.string;
        CopyMem (Statement->BufferValue, UserInput->InputValue.Buffer, (UINTN) UserInput->InputValue.BufferLen);
        FreePool (UserInput->InputValue.Buffer);
        break;

      case EFI_IFR_PASSWORD_OP:
        if (UserInput->InputValue.Buffer == NULL) {
          //
          // User not input new password, just return.
          //
          break;
        }

        DeleteString(Statement->HiiValue.Value.string, gCurrentSelection->FormSet->HiiHandle);
        Statement->HiiValue.Value.string = UserInput->InputValue.Value.string;
        CopyMem (Statement->BufferValue, UserInput->InputValue.Buffer, (UINTN) UserInput->InputValue.BufferLen);
        ZeroMem (UserInput->InputValue.Buffer, (UINTN) UserInput->InputValue.BufferLen);
        FreePool (UserInput->InputValue.Buffer);
        //
        // Two password match, send it to Configuration Driver
        //
        if ((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
          PasswordCheck (NULL, UserInput->SelectedStatement, (CHAR16 *) Statement->BufferValue);
          //
          // Clean the value after saved it.
          //
          ZeroMem (Statement->BufferValue, (UINTN) UserInput->InputValue.BufferLen);
          HiiSetString (gCurrentSelection->FormSet->HiiHandle, Statement->HiiValue.Value.string, (CHAR16*)Statement->BufferValue, NULL);
        } else {
          SetQuestionValue (gCurrentSelection->FormSet, gCurrentSelection->Form, Statement, GetSetValueWithHiiDriver);
        }
        break;

      case EFI_IFR_ORDERED_LIST_OP:
        CopyMem (Statement->BufferValue, UserInput->InputValue.Buffer, UserInput->InputValue.BufferLen);
        break;

      default:
        CopyMem (&Statement->HiiValue, &UserInput->InputValue, sizeof (EFI_HII_VALUE));
        break;
      }
      break;
    }
  }

  return Status;
}

/**

  Display form and wait for user to select one menu option, then return it.

  @retval EFI_SUCESSS            This function always return successfully for now.

**/
EFI_STATUS
DisplayForm (
  VOID
  )
{
  EFI_STATUS               Status;
  USER_INPUT               UserInput;
  FORM_ENTRY_INFO          *CurrentMenu;

  ZeroMem (&UserInput, sizeof (USER_INPUT));

  //
  // Update the menu history data.
  //
  CurrentMenu = UiFindMenuList (gCurrentSelection->Handle, &gCurrentSelection->FormSetGuid, gCurrentSelection->FormId);
  if (CurrentMenu == NULL) {
    //
    // Current menu not found, add it to the menu tree
    //
    CurrentMenu = UiAddMenuList (gCurrentSelection->Handle, &gCurrentSelection->FormSetGuid,
                                 gCurrentSelection->FormId, gCurrentSelection->QuestionId);
    ASSERT (CurrentMenu != NULL);
  }

  //
  // Back up the form view history data for this form.
  //
  UiCopyMenuList(&gCurrentSelection->Form->FormViewListHead, &mPrivateData.FormBrowserEx2.FormViewHistoryHead);

  gCurrentSelection->CurrentMenu = CurrentMenu;

  if (gCurrentSelection->QuestionId == 0) {
    //
    // Highlight not specified, fetch it from cached menu
    //
    gCurrentSelection->QuestionId = CurrentMenu->QuestionId;
  }

  Status = EvaluateFormExpressions (gCurrentSelection->FormSet, gCurrentSelection->Form);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UpdateDisplayFormData ();

  ASSERT (gDisplayFormData.BrowserStatus == BROWSER_SUCCESS);
  Status = mFormDisplay->FormDisplay (&gDisplayFormData, &UserInput);
  if (EFI_ERROR (Status)) {
    FreeDisplayFormData();
    return Status;
  }

  CheckConfigAccess(gCurrentSelection->FormSet);

  Status = ProcessUserInput (&UserInput);
  FreeDisplayFormData();
  return Status;
}

/**
  Functions which are registered to receive notification of
  database events have this prototype. The actual event is encoded
  in NotifyType. The following table describes how PackageType,
  PackageGuid, Handle, and Package are used for each of the
  notification types.

  @param PackageType  Package type of the notification.

  @param PackageGuid  If PackageType is
                      EFI_HII_PACKAGE_TYPE_GUID, then this is
                      the pointer to the GUID from the Guid
                      field of EFI_HII_PACKAGE_GUID_HEADER.
                      Otherwise, it must be NULL.

  @param Package  Points to the package referred to by the
                  notification Handle The handle of the package
                  list which contains the specified package.

  @param Handle       The HII handle.

  @param NotifyType   The type of change concerning the
                      database. See
                      EFI_HII_DATABASE_NOTIFY_TYPE.

**/
EFI_STATUS
EFIAPI
FormUpdateNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  mHiiPackageListUpdated = TRUE;
  mDynamicFormUpdated = TRUE;

  return EFI_SUCCESS;
}

/**
  Update the NV flag info for this form set.

  @param  FormSet                FormSet data structure.

**/
BOOLEAN
IsNvUpdateRequiredForFormSet (
  IN FORM_BROWSER_FORMSET  *FormSet
  )
{
  LIST_ENTRY              *Link;
  FORM_BROWSER_FORM       *Form;
  BOOLEAN                 RetVal;

  //
  // Not finished question initialization, return FALSE.
  //
  if (!FormSet->QuestionInited) {
    return FALSE;
  }

  RetVal = FALSE;

  Link = GetFirstNode (&FormSet->FormListHead);
  while (!IsNull (&FormSet->FormListHead, Link)) {
    Form = FORM_BROWSER_FORM_FROM_LINK (Link);

    RetVal = IsNvUpdateRequiredForForm(Form);
    if (RetVal) {
      break;
    }

    Link = GetNextNode (&FormSet->FormListHead, Link);
  }

  return RetVal;
}

/**
  Update the NvUpdateRequired flag for a form.

  @param  Form                Form data structure.

**/
BOOLEAN
IsNvUpdateRequiredForForm (
  IN FORM_BROWSER_FORM    *Form
  )
{
  LIST_ENTRY              *Link;
  FORM_BROWSER_STATEMENT  *Statement;

  Link = GetFirstNode (&Form->StatementListHead);
  while (!IsNull (&Form->StatementListHead, Link)) {
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);

    if (Statement->ValueChanged) {
      return TRUE;
    }

    Link = GetNextNode (&Form->StatementListHead, Link);
  }

  return FALSE;
}

/**
  Find menu which will show next time.

  @param Selection       On input, Selection tell setup browser the information
                         about the Selection, form and formset to be displayed.
                         On output, Selection return the screen item that is selected
                         by user.
  @param SettingLevel    Input Settting level, if it is FormLevel, just exit current form.
                         else, we need to exit current formset.

  @retval TRUE           Exit current form.
  @retval FALSE          User press ESC and keep in current form.
**/
BOOLEAN
FindNextMenu (
  IN OUT UI_MENU_SELECTION        *Selection,
  IN     BROWSER_SETTING_SCOPE     SettingLevel
  )
{
  FORM_ENTRY_INFO            *CurrentMenu;
  FORM_ENTRY_INFO            *ParentMenu;
  BROWSER_SETTING_SCOPE      Scope;

  CurrentMenu = Selection->CurrentMenu;
  Scope       = FormSetLevel;

  ParentMenu = UiFindParentMenu(CurrentMenu, SettingLevel);
  while (ParentMenu != NULL && !ValidateHiiHandle(ParentMenu->HiiHandle)) {
    ParentMenu = UiFindParentMenu(ParentMenu, SettingLevel);
  }

  if (ParentMenu != NULL) {
    if (CompareGuid (&CurrentMenu->FormSetGuid, &ParentMenu->FormSetGuid)) {
      Scope = FormLevel;
    } else {
      Scope = FormSetLevel;
    }
  }

  //
  // Form Level Check whether the data is changed.
  //
  if ((gBrowserSettingScope == FormLevel && IsNvUpdateRequiredForForm (Selection->Form)) ||
      (gBrowserSettingScope == FormSetLevel && IsNvUpdateRequiredForFormSet(Selection->FormSet) && Scope == FormSetLevel)) {
    if (!ProcessChangedData(Selection, gBrowserSettingScope)) {
      return FALSE;
    }
  }

  if (ParentMenu != NULL) {
    //
    // ParentMenu is found. Then, go to it.
    //
    if (Scope == FormLevel) {
      Selection->Action = UI_ACTION_REFRESH_FORM;
    } else {
      Selection->Action = UI_ACTION_REFRESH_FORMSET;
      CopyMem (&Selection->FormSetGuid, &ParentMenu->FormSetGuid, sizeof (EFI_GUID));
      Selection->Handle = ParentMenu->HiiHandle;
    }

    Selection->Statement = NULL;

    Selection->FormId = ParentMenu->FormId;
    Selection->QuestionId = ParentMenu->QuestionId;

    //
    // Clear highlight record for this menu
    //
    CurrentMenu->QuestionId = 0;
    return FALSE;
  }

  //
  // Current in root page, exit the SendForm
  //
  Selection->Action = UI_ACTION_EXIT;

  return TRUE;
}

/**
  Reconnect the controller.

  @param DriverHandle          The controller handle which need to be reconnect.

  @retval   TRUE     do the reconnect behavior success.
  @retval   FALSE    do the reconnect behavior failed.

**/
BOOLEAN
ReconnectController (
  IN EFI_HANDLE   DriverHandle
  )
{
  EFI_STATUS                      Status;

  Status = gBS->DisconnectController(DriverHandle, NULL, NULL);
  if (!EFI_ERROR (Status)) {
    Status = gBS->ConnectController(DriverHandle, NULL, NULL, TRUE);
  }

  return Status == EFI_SUCCESS;
}

/**
  Call the call back function for the question and process the return action.

  @param Selection             On input, Selection tell setup browser the information
                               about the Selection, form and formset to be displayed.
                               On output, Selection return the screen item that is selected
                               by user.
  @param FormSet               The formset this question belong to.
  @param Form                  The form this question belong to.
  @param Question              The Question which need to call.
  @param Action                The action request.
  @param SkipSaveOrDiscard     Whether skip save or discard action.

  @retval EFI_SUCCESS          The call back function executes successfully.
  @return Other value if the call back function failed to execute.
**/
EFI_STATUS
ProcessCallBackFunction (
  IN OUT UI_MENU_SELECTION               *Selection,
  IN     FORM_BROWSER_FORMSET            *FormSet,
  IN     FORM_BROWSER_FORM               *Form,
  IN     FORM_BROWSER_STATEMENT          *Question,
  IN     EFI_BROWSER_ACTION              Action,
  IN     BOOLEAN                         SkipSaveOrDiscard
  )
{
  EFI_STATUS                      Status;
  EFI_STATUS                      InternalStatus;
  EFI_BROWSER_ACTION_REQUEST      ActionRequest;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  EFI_HII_VALUE                   *HiiValue;
  EFI_IFR_TYPE_VALUE              *TypeValue;
  FORM_BROWSER_STATEMENT          *Statement;
  BOOLEAN                         SubmitFormIsRequired;
  BOOLEAN                         DiscardFormIsRequired;
  BOOLEAN                         NeedExit;
  LIST_ENTRY                      *Link;
  BROWSER_SETTING_SCOPE           SettingLevel;
  EFI_IFR_TYPE_VALUE              BackUpValue;
  UINT8                           *BackUpBuffer;
  CHAR16                          *NewString;

  ConfigAccess = FormSet->ConfigAccess;
  SubmitFormIsRequired  = FALSE;
  SettingLevel          = FormSetLevel;
  DiscardFormIsRequired = FALSE;
  NeedExit              = FALSE;
  Status                = EFI_SUCCESS;
  ActionRequest         = EFI_BROWSER_ACTION_REQUEST_NONE;
  BackUpBuffer          = NULL;

  if (ConfigAccess == NULL) {
    return EFI_SUCCESS;
  }

  Link = GetFirstNode (&Form->StatementListHead);
  while (!IsNull (&Form->StatementListHead, Link)) {
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
    Link = GetNextNode (&Form->StatementListHead, Link);

    //
    // if Question != NULL, only process the question. Else, process all question in this form.
    //
    if ((Question != NULL) && (Statement != Question)) {
      continue;
    }

    if ((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != EFI_IFR_FLAG_CALLBACK) {
      continue;
    }

    //
    // Check whether Statement is disabled.
    //
    if (Statement->Expression != NULL) {
      if (EvaluateExpressionList(Statement->Expression, TRUE, FormSet, Form) == ExpressDisable) {
        continue;
      }
    }

    HiiValue = &Statement->HiiValue;
    TypeValue = &HiiValue->Value;
    if (HiiValue->Type == EFI_IFR_TYPE_BUFFER) {
      //
      // For OrderedList, passing in the value buffer to Callback()
      //
      TypeValue = (EFI_IFR_TYPE_VALUE *) Statement->BufferValue;
    }

    //
    // If EFI_BROWSER_ACTION_CHANGING type, back up the new question value.
    //
    if (Action == EFI_BROWSER_ACTION_CHANGING) {
      if (HiiValue->Type == EFI_IFR_TYPE_BUFFER) {
        BackUpBuffer = AllocateCopyPool(Statement->StorageWidth, Statement->BufferValue);
        ASSERT (BackUpBuffer != NULL);
      } else {
        CopyMem (&BackUpValue, &HiiValue->Value, sizeof (EFI_IFR_TYPE_VALUE));
      }
    }

    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    Status = ConfigAccess->Callback (
                             ConfigAccess,
                             Action,
                             Statement->QuestionId,
                             HiiValue->Type,
                             TypeValue,
                             &ActionRequest
                             );
    if (!EFI_ERROR (Status)) {
      //
      // Need to sync the value between Statement->HiiValue->Value and Statement->BufferValue
      //
      if (HiiValue->Type == EFI_IFR_TYPE_STRING) {
        NewString = GetToken (Statement->HiiValue.Value.string, FormSet->HiiHandle);
        ASSERT (NewString != NULL);

        ASSERT (StrLen (NewString) * sizeof (CHAR16) <= Statement->StorageWidth);
        if (StrLen (NewString) * sizeof (CHAR16) <= Statement->StorageWidth) {
          ZeroMem (Statement->BufferValue, Statement->StorageWidth);
          CopyMem (Statement->BufferValue, NewString, StrSize (NewString));
        } else {
          CopyMem (Statement->BufferValue, NewString, Statement->StorageWidth);
        }
        FreePool (NewString);
      }

      //
      // Only for EFI_BROWSER_ACTION_CHANGED need to handle this ActionRequest.
      //
      switch (Action) {
      case EFI_BROWSER_ACTION_CHANGED:
        switch (ActionRequest) {
        case EFI_BROWSER_ACTION_REQUEST_RESET:
          DiscardFormIsRequired = TRUE;
          gResetRequiredFormLevel = TRUE;
          gResetRequiredSystemLevel = TRUE;
          NeedExit              = TRUE;
          break;

        case EFI_BROWSER_ACTION_REQUEST_SUBMIT:
          SubmitFormIsRequired = TRUE;
          NeedExit              = TRUE;
          break;

        case EFI_BROWSER_ACTION_REQUEST_EXIT:
          DiscardFormIsRequired = TRUE;
          NeedExit              = TRUE;
          break;

        case EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT:
          SubmitFormIsRequired  = TRUE;
          SettingLevel          = FormLevel;
          NeedExit              = TRUE;
          break;

        case EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT:
          DiscardFormIsRequired = TRUE;
          SettingLevel          = FormLevel;
          NeedExit              = TRUE;
          break;

        case EFI_BROWSER_ACTION_REQUEST_FORM_APPLY:
          SubmitFormIsRequired  = TRUE;
          SettingLevel          = FormLevel;
          break;

        case EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD:
          DiscardFormIsRequired = TRUE;
          SettingLevel          = FormLevel;
          break;

        case EFI_BROWSER_ACTION_REQUEST_RECONNECT:
          gCallbackReconnect    = TRUE;
          break;

        default:
          break;
        }
        break;

      case EFI_BROWSER_ACTION_CHANGING:
        //
        // Do the question validation.
        //
        Status = ValueChangedValidation (gCurrentSelection->FormSet, gCurrentSelection->Form, Statement);
        if (!EFI_ERROR (Status)) {
          //
          //check whether the question value  changed compared with edit buffer before updating edit buffer
          // if changed, set the ValueChanged flag to TRUE,in order to trig the CHANGED callback function
          //
          IsQuestionValueChanged(gCurrentSelection->FormSet, gCurrentSelection->Form, Statement, GetSetValueWithEditBuffer);
          //
          // According the spec, return value from call back of "changing" and
          // "retrieve" should update to the question's temp buffer.
          //
          SetQuestionValue(FormSet, Form, Statement, GetSetValueWithEditBuffer);
        }
        break;

      case EFI_BROWSER_ACTION_RETRIEVE:
        //
        // According the spec, return value from call back of "changing" and
        // "retrieve" should update to the question's temp buffer.
        //
        SetQuestionValue(FormSet, Form, Statement, GetSetValueWithEditBuffer);
        break;

      default:
        break;
      }
    } else {
      //
      // If the callback returns EFI_UNSUPPORTED for EFI_BROWSER_ACTION_CHANGING,
      // then the browser will use the value passed to Callback() and ignore the
      // value returned by Callback().
      //
      if (Action  == EFI_BROWSER_ACTION_CHANGING && Status == EFI_UNSUPPORTED) {
        if (HiiValue->Type == EFI_IFR_TYPE_BUFFER) {
          CopyMem (Statement->BufferValue, BackUpBuffer, Statement->StorageWidth);
        } else {
          CopyMem (&HiiValue->Value, &BackUpValue, sizeof (EFI_IFR_TYPE_VALUE));
        }

        //
        // Do the question validation.
        //
        InternalStatus = ValueChangedValidation (gCurrentSelection->FormSet, gCurrentSelection->Form, Statement);
        if (!EFI_ERROR (InternalStatus)) {
          //
          //check whether the question value  changed compared with edit buffer before updating edit buffer
          // if changed, set the ValueChanged flag to TRUE,in order to trig the CHANGED callback function
          //
          IsQuestionValueChanged(gCurrentSelection->FormSet, gCurrentSelection->Form, Statement, GetSetValueWithEditBuffer);
          SetQuestionValue(FormSet, Form, Statement, GetSetValueWithEditBuffer);
        }
      }

      //
      // According the spec, return fail from call back of "changing" and
      // "retrieve", should restore the question's value.
      //
      if (Action == EFI_BROWSER_ACTION_CHANGING && Status != EFI_UNSUPPORTED) {
        if (Statement->Storage != NULL) {
          GetQuestionValue(FormSet, Form, Statement, GetSetValueWithEditBuffer);
        } else if ((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
          ProcessCallBackFunction (Selection, FormSet, Form, Question, EFI_BROWSER_ACTION_RETRIEVE, FALSE);
        }
      }

      if (Action == EFI_BROWSER_ACTION_RETRIEVE) {
        GetQuestionValue(FormSet, Form, Statement, GetSetValueWithEditBuffer);
      }

      if (Status == EFI_UNSUPPORTED) {
        //
        // If return EFI_UNSUPPORTED, also consider Hii driver suceess deal with it.
        //
        Status = EFI_SUCCESS;
      }
    }

    if (BackUpBuffer != NULL) {
      FreePool (BackUpBuffer);
    }

    //
    // If Question != NULL, means just process one question
    // and if code reach here means this question has finished
    // processing, so just break.
    //
    if (Question != NULL) {
      break;
    }
  }

  if (gCallbackReconnect && (EFI_BROWSER_ACTION_CHANGED == Action)) {
    //
    // Confirm changes with user first.
    //
    if (IsNvUpdateRequiredForFormSet(FormSet)) {
      if (BROWSER_ACTION_DISCARD == PopupErrorMessage(BROWSER_RECONNECT_SAVE_CHANGES, NULL, NULL, NULL)) {
        gCallbackReconnect = FALSE;
        DiscardFormIsRequired = TRUE;
      } else {
        SubmitFormIsRequired = TRUE;
      }
    } else {
      PopupErrorMessage(BROWSER_RECONNECT_REQUIRED, NULL, NULL, NULL);
    }

    //
    // Exit current formset before do the reconnect.
    //
    NeedExit = TRUE;
    SettingLevel = FormSetLevel;
  }

  if (SubmitFormIsRequired && !SkipSaveOrDiscard) {
    SubmitForm (FormSet, Form, SettingLevel);
  }

  if (DiscardFormIsRequired && !SkipSaveOrDiscard) {
    DiscardForm (FormSet, Form, SettingLevel);
  }

  if (NeedExit) {
    FindNextMenu (Selection, SettingLevel);
  }

  return Status;
}

/**
  Call the retrieve type call back function for one question to get the initialize data.

  This function only used when in the initialize stage, because in this stage, the
  Selection->Form is not ready. For other case, use the ProcessCallBackFunction instead.

  @param ConfigAccess          The config access protocol produced by the hii driver.
  @param Statement             The Question which need to call.
  @param FormSet               The formset this question belong to.

  @retval EFI_SUCCESS          The call back function executes successfully.
  @return Other value if the call back function failed to execute.
**/
EFI_STATUS
ProcessRetrieveForQuestion (
  IN     EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess,
  IN     FORM_BROWSER_STATEMENT          *Statement,
  IN     FORM_BROWSER_FORMSET            *FormSet
  )
{
  EFI_STATUS                      Status;
  EFI_BROWSER_ACTION_REQUEST      ActionRequest;
  EFI_HII_VALUE                   *HiiValue;
  EFI_IFR_TYPE_VALUE              *TypeValue;
  CHAR16                          *NewString;

  Status                = EFI_SUCCESS;
  ActionRequest         = EFI_BROWSER_ACTION_REQUEST_NONE;

  if (((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != EFI_IFR_FLAG_CALLBACK) || ConfigAccess == NULL) {
    return EFI_UNSUPPORTED;
  }

  HiiValue  = &Statement->HiiValue;
  TypeValue = &HiiValue->Value;
  if (HiiValue->Type == EFI_IFR_TYPE_BUFFER) {
    //
    // For OrderedList, passing in the value buffer to Callback()
    //
    TypeValue = (EFI_IFR_TYPE_VALUE *) Statement->BufferValue;
  }

  ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Status = ConfigAccess->Callback (
                           ConfigAccess,
                           EFI_BROWSER_ACTION_RETRIEVE,
                           Statement->QuestionId,
                           HiiValue->Type,
                           TypeValue,
                           &ActionRequest
                           );
  if (!EFI_ERROR (Status) && HiiValue->Type == EFI_IFR_TYPE_STRING) {
    NewString = GetToken (Statement->HiiValue.Value.string, FormSet->HiiHandle);
    ASSERT (NewString != NULL);

    ASSERT (StrLen (NewString) * sizeof (CHAR16) <= Statement->StorageWidth);
    if (StrLen (NewString) * sizeof (CHAR16) <= Statement->StorageWidth) {
      ZeroMem (Statement->BufferValue, Statement->StorageWidth);
      CopyMem (Statement->BufferValue, NewString, StrSize (NewString));
    } else {
      CopyMem (Statement->BufferValue, NewString, Statement->StorageWidth);
    }
    FreePool (NewString);
  }

  return Status;
}

/**
  The worker function that send the displays to the screen. On output,
  the selection made by user is returned.

  @param Selection       On input, Selection tell setup browser the information
                         about the Selection, form and formset to be displayed.
                         On output, Selection return the screen item that is selected
                         by user.

  @retval EFI_SUCCESS    The page is displayed successfully.
  @return Other value if the page failed to be diplayed.

**/
EFI_STATUS
SetupBrowser (
  IN OUT UI_MENU_SELECTION    *Selection
  )
{
  EFI_STATUS                      Status;
  LIST_ENTRY                      *Link;
  EFI_HANDLE                      NotifyHandle;
  FORM_BROWSER_STATEMENT          *Statement;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;

  ConfigAccess = Selection->FormSet->ConfigAccess;

  //
  // Register notify for Form package update
  //
  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_FORMS,
                           NULL,
                           FormUpdateNotify,
                           EFI_HII_DATABASE_NOTIFY_REMOVE_PACK,
                           &NotifyHandle
                           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize current settings of Questions in this FormSet
  //
  InitializeCurrentSetting (Selection->FormSet);

  //
  // Initilize Action field.
  //
  Selection->Action = UI_ACTION_REFRESH_FORM;

  //
  // Clean the mCurFakeQestId value is formset refreshed.
  //
  mCurFakeQestId = 0;

  do {

    //
    // Reset Status to prevent the next break from returning incorrect error status.
    //
    Status = EFI_SUCCESS;

    //
    // IFR is updated, force to reparse the IFR binary
    // This check is shared by EFI_BROWSER_ACTION_FORM_CLOSE and
    // EFI_BROWSER_ACTION_RETRIEVE, so code place here.
    //
    if (mHiiPackageListUpdated) {
      Selection->Action = UI_ACTION_REFRESH_FORMSET;
      mHiiPackageListUpdated = FALSE;
      break;
    }

    //
    // Initialize Selection->Form
    //
    if (Selection->FormId == 0) {
      //
      // Zero FormId indicates display the first Form in a FormSet
      //
      Link = GetFirstNode (&Selection->FormSet->FormListHead);

      Selection->Form = FORM_BROWSER_FORM_FROM_LINK (Link);
      Selection->FormId = Selection->Form->FormId;
    } else {
      Selection->Form = IdToForm (Selection->FormSet, Selection->FormId);
    }

    if (Selection->Form == NULL) {
      //
      // No Form to display
      //
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    //
    // Check Form is suppressed.
    //
    if (Selection->Form->SuppressExpression != NULL) {
      if (EvaluateExpressionList(Selection->Form->SuppressExpression, TRUE, Selection->FormSet, Selection->Form) == ExpressSuppress) {
        //
        // Form is suppressed.
        //
        PopupErrorMessage(BROWSER_FORM_SUPPRESS, NULL, NULL, NULL);
        Status = EFI_NOT_FOUND;
        goto Done;
      }
    }

    //
    // Before display new form, invoke ConfigAccess.Callback() with EFI_BROWSER_ACTION_FORM_OPEN
    // for each question with callback flag.
    // New form may be the first form, or the different form after another form close.
    //
    if (((Selection->Handle != mCurrentHiiHandle) ||
        (!CompareGuid (&Selection->FormSetGuid, &mCurrentFormSetGuid)) ||
        (Selection->FormId != mCurrentFormId))) {
      //
      // Update Retrieve flag.
      //
      mFinishRetrieveCall = FALSE;

      //
      // Keep current form information
      //
      mCurrentHiiHandle   = Selection->Handle;
      CopyGuid (&mCurrentFormSetGuid, &Selection->FormSetGuid);
      mCurrentFormId      = Selection->FormId;

      if (ConfigAccess != NULL) {
        Status = ProcessCallBackFunction (Selection, Selection->FormSet, Selection->Form, NULL, EFI_BROWSER_ACTION_FORM_OPEN, FALSE);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // IFR is updated during callback of EFI_BROWSER_ACTION_FORM_OPEN, force to reparse the IFR binary
        //
        if (mHiiPackageListUpdated) {
          Selection->Action = UI_ACTION_REFRESH_FORMSET;
          mHiiPackageListUpdated = FALSE;
          break;
        }
      }
    }

    //
    // Load Questions' Value for display
    //
    Status = LoadFormSetConfig (Selection, Selection->FormSet);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    if (!mFinishRetrieveCall) {
      //
      // Finish call RETRIEVE callback for this form.
      //
      mFinishRetrieveCall = TRUE;

      if (ConfigAccess != NULL) {
        Status = ProcessCallBackFunction (Selection, Selection->FormSet, Selection->Form, NULL, EFI_BROWSER_ACTION_RETRIEVE, FALSE);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // IFR is updated during callback of open form, force to reparse the IFR binary
        //
        if (mHiiPackageListUpdated) {
          Selection->Action = UI_ACTION_REFRESH_FORMSET;
          mHiiPackageListUpdated = FALSE;
          break;
        }
      }
    }

    //
    // Display form
    //
    Status = DisplayForm ();
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Check Selected Statement (if press ESC, Selection->Statement will be NULL)
    //
    Statement = Selection->Statement;
    if (Statement != NULL) {
      if ((ConfigAccess != NULL) &&
          ((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) == EFI_IFR_FLAG_CALLBACK) &&
          (Statement->Operand != EFI_IFR_PASSWORD_OP)) {
        Status = ProcessCallBackFunction(Selection, Selection->FormSet, Selection->Form, Statement, EFI_BROWSER_ACTION_CHANGING, FALSE);
        if (Statement->Operand == EFI_IFR_REF_OP) {
          //
          // Process dynamic update ref opcode.
          //
          if (!EFI_ERROR (Status)) {
            Status = ProcessGotoOpCode(Statement, Selection);
          }

          //
          // Callback return error status or status return from process goto opcode.
          //
          if (EFI_ERROR (Status)) {
            //
            // Cross reference will not be taken, restore all essential field
            //
            Selection->Handle = mCurrentHiiHandle;
            CopyMem (&Selection->FormSetGuid, &mCurrentFormSetGuid, sizeof (EFI_GUID));
            Selection->FormId = mCurrentFormId;
            Selection->QuestionId = 0;
            Selection->Action = UI_ACTION_REFRESH_FORM;
          }
        }


        if (!EFI_ERROR (Status) &&
            (Statement->Operand != EFI_IFR_REF_OP) &&
            ((Statement->Storage == NULL) || (Statement->Storage != NULL && Statement->ValueChanged))) {
          //
          // Only question value has been changed, browser will trig CHANGED callback.
          //
          ProcessCallBackFunction(Selection, Selection->FormSet, Selection->Form, Statement, EFI_BROWSER_ACTION_CHANGED, FALSE);
          //
          //check whether the question value changed compared with buffer value
          //if doesn't change ,set the ValueChanged flag to FALSE ,in order not to display the "configuration changed "information on the screen
          //
          IsQuestionValueChanged(gCurrentSelection->FormSet, gCurrentSelection->Form, Statement, GetSetValueWithBuffer);
        }
      } else {
        //
        // Do the question validation.
        //
        Status = ValueChangedValidation (gCurrentSelection->FormSet, gCurrentSelection->Form, Statement);
        if (!EFI_ERROR (Status) && (Statement->Operand != EFI_IFR_PASSWORD_OP)) {
          SetQuestionValue (gCurrentSelection->FormSet, gCurrentSelection->Form, Statement, GetSetValueWithEditBuffer);
          //
          // Verify whether question value has checked, update the ValueChanged flag in Question.
          //
          IsQuestionValueChanged(gCurrentSelection->FormSet, gCurrentSelection->Form, Statement, GetSetValueWithBuffer);
        }
      }

      //
      // If question has EFI_IFR_FLAG_RESET_REQUIRED/EFI_IFR_FLAG_RECONNECT_REQUIRED flag and without storage
      // and process question success till here, trig the gResetFlag/gFlagReconnect.
      //
      if ((Status == EFI_SUCCESS) &&
          (Statement->Storage == NULL)) {
        if ((Statement->QuestionFlags & EFI_IFR_FLAG_RESET_REQUIRED) != 0) {
          gResetRequiredFormLevel = TRUE;
          gResetRequiredSystemLevel = TRUE;
        }

        if ((Statement->QuestionFlags & EFI_IFR_FLAG_RECONNECT_REQUIRED) != 0) {
          gFlagReconnect = TRUE;
        }
      }
    }

    //
    // Check whether Exit flag is TRUE.
    //
    if (gExitRequired) {
      switch (gBrowserSettingScope) {
      case SystemLevel:
        Selection->Action = UI_ACTION_EXIT;
        break;

      case FormSetLevel:
      case FormLevel:
        FindNextMenu (Selection, gBrowserSettingScope);
        break;

      default:
        break;
      }

      gExitRequired = FALSE;
    }

    //
    // Before exit the form, invoke ConfigAccess.Callback() with EFI_BROWSER_ACTION_FORM_CLOSE
    // for each question with callback flag.
    //
    if ((ConfigAccess != NULL) &&
        ((Selection->Action == UI_ACTION_EXIT) ||
         (Selection->Handle != mCurrentHiiHandle) ||
         (!CompareGuid (&Selection->FormSetGuid, &mCurrentFormSetGuid)) ||
         (Selection->FormId != mCurrentFormId))) {

      Status = ProcessCallBackFunction (Selection, Selection->FormSet, Selection->Form, NULL, EFI_BROWSER_ACTION_FORM_CLOSE, FALSE);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }
  } while (Selection->Action == UI_ACTION_REFRESH_FORM);

Done:
  //
  // Reset current form information to the initial setting when error happens or form exit.
  //
  if (EFI_ERROR (Status) || Selection->Action == UI_ACTION_EXIT) {
    mCurrentHiiHandle = NULL;
    CopyGuid (&mCurrentFormSetGuid, &gZeroGuid);
    mCurrentFormId = 0;
  }

  //
  // Unregister notify for Form package update
  //
  mHiiDatabase->UnregisterPackageNotify (
                   mHiiDatabase,
                   NotifyHandle
                   );
  return Status;
}
