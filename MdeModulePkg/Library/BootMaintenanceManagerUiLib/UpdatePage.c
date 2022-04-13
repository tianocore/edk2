/** @file
Dynamically update the pages.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootMaintenanceManager.h"

/**
 Create the global UpdateData structure.

**/
VOID
CreateUpdateData (
  VOID
  )
{
  //
  // Init OpCode Handle and Allocate space for creation of Buffer
  //
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (mStartOpCodeHandle != NULL);

  mEndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (mEndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mStartLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (mStartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  mEndLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (mEndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mEndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  mEndLabel->Number       = LABEL_END;
}

/**
  Refresh the global UpdateData structure.

**/
VOID
RefreshUpdateData (
  VOID
  )
{
  //
  // Free current updated date
  //
  if (mStartOpCodeHandle != NULL) {
    HiiFreeOpCodeHandle (mStartOpCodeHandle);
  }

  //
  // Create new OpCode Handle
  //
  mStartOpCodeHandle = HiiAllocateOpCodeHandle ();

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  mStartLabel               = (EFI_IFR_GUID_LABEL *)HiiCreateGuidOpCode (mStartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  mStartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
}

/**
  Add a "Go back to main page" tag in front of the form when there are no
  "Apply changes" and "Discard changes" tags in the end of the form.

  @param CallbackData    The BMM context data.

**/
VOID
UpdatePageStart (
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  RefreshUpdateData ();
  mStartLabel->Number = CallbackData->BmmCurrentPageId;

  if (!(CallbackData->BmmAskSaveOrNot)) {
    //
    // Add a "Go back to main page" tag in front of the form when there are no
    // "Apply changes" and "Discard changes" tags in the end of the form.
    //
    HiiCreateGotoOpCode (
      mStartOpCodeHandle,
      FORM_MAIN_ID,
      STRING_TOKEN (STR_FORM_GOTO_MAIN),
      STRING_TOKEN (STR_FORM_GOTO_MAIN),
      0,
      FORM_MAIN_ID
      );
  }
}

/**
  Create the "Apply changes" and "Discard changes" tags. And
  ensure user can return to the main page.

  @param CallbackData    The BMM context data.

**/
VOID
UpdatePageEnd (
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  //
  // Create the "Apply changes" and "Discard changes" tags.
  //
  if (CallbackData->BmmAskSaveOrNot) {
    HiiCreateSubTitleOpCode (
      mStartOpCodeHandle,
      STRING_TOKEN (STR_NULL_STRING),
      0,
      0,
      0
      );

    HiiCreateActionOpCode (
      mStartOpCodeHandle,
      KEY_VALUE_SAVE_AND_EXIT,
      STRING_TOKEN (STR_SAVE_AND_EXIT),
      STRING_TOKEN (STR_NULL_STRING),
      EFI_IFR_FLAG_CALLBACK,
      0
      );
  }

  //
  // Ensure user can return to the main page.
  //
  HiiCreateActionOpCode (
    mStartOpCodeHandle,
    KEY_VALUE_NO_SAVE_AND_EXIT,
    STRING_TOKEN (STR_NO_SAVE_AND_EXIT),
    STRING_TOKEN (STR_NULL_STRING),
    EFI_IFR_FLAG_CALLBACK,
    0
    );

  HiiUpdateForm (
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    CallbackData->BmmCurrentPageId,
    mStartOpCodeHandle, // Label CallbackData->BmmCurrentPageId
    mEndOpCodeHandle    // LABEL_END
    );
}

/**
  Clean up the dynamic opcode at label and form specified by both LabelId.

  @param LabelId         It is both the Form ID and Label ID for opcode deletion.
  @param CallbackData    The BMM context data.

**/
VOID
CleanUpPage (
  IN UINT16             LabelId,
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  RefreshUpdateData ();

  //
  // Remove all op-codes from dynamic page
  //
  mStartLabel->Number = LabelId;
  HiiUpdateForm (
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    LabelId,
    mStartOpCodeHandle, // Label LabelId
    mEndOpCodeHandle    // LABEL_END
    );
}

/**
  Create a list of Goto Opcode for all terminal devices logged
  by TerminaMenu. This list will be inserted to form FORM_CON_COM_SETUP_ID.

  @param CallbackData    The BMM context data.
**/
VOID
UpdateConCOMPage (
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  BM_MENU_ENTRY  *NewMenuEntry;
  UINT16         Index;

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&TerminalMenu, Index);

    HiiCreateGotoOpCode (
      mStartOpCodeHandle,
      FORM_CON_COM_SETUP_ID,
      NewMenuEntry->DisplayStringToken,
      STRING_TOKEN (STR_NULL_STRING),
      EFI_IFR_FLAG_CALLBACK,
      (UINT16)(TERMINAL_OPTION_OFFSET + Index)
      );
  }

  UpdatePageEnd (CallbackData);
}

/**
  Create a list of boot option from global BootOptionMenu. It
  allow user to delete the boot option.

  @param CallbackData    The BMM context data.

**/
VOID
UpdateBootDelPage (
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  BM_MENU_ENTRY    *NewMenuEntry;
  BM_LOAD_CONTEXT  *NewLoadContext;
  UINT16           Index;

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  ASSERT (BootOptionMenu.MenuNumber <= (sizeof (CallbackData->BmmFakeNvData.BootOptionDel) / sizeof (CallbackData->BmmFakeNvData.BootOptionDel[0])));
  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry   = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    NewLoadContext = (BM_LOAD_CONTEXT *)NewMenuEntry->VariableContext;
    if (NewLoadContext->IsLegacy) {
      continue;
    }

    NewLoadContext->Deleted = FALSE;

    if (CallbackData->BmmFakeNvData.BootOptionDel[Index] && !CallbackData->BmmFakeNvData.BootOptionDelMark[Index]) {
      //
      // CallbackData->BmmFakeNvData.BootOptionDel[Index] == TRUE means browser knows this boot option is selected
      // CallbackData->BmmFakeNvData.BootOptionDelMark[Index] = FALSE means BDS knows the selected boot option has
      // deleted, browser maintains old useless info. So clear this info here, and later update this info to browser
      // through HiiSetBrowserData function.
      //
      CallbackData->BmmFakeNvData.BootOptionDel[Index]    = FALSE;
      CallbackData->BmmOldFakeNVData.BootOptionDel[Index] = FALSE;
    }

    HiiCreateCheckBoxOpCode (
      mStartOpCodeHandle,
      (EFI_QUESTION_ID)(BOOT_OPTION_DEL_QUESTION_ID + Index),
      VARSTORE_ID_BOOT_MAINT,
      (UINT16)(BOOT_OPTION_DEL_VAR_OFFSET + Index),
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      EFI_IFR_FLAG_CALLBACK,
      0,
      NULL
      );
  }

  UpdatePageEnd (CallbackData);
}

/**
  Create a lit of driver option from global DriverMenu.

  @param CallbackData    The BMM context data.

**/
VOID
UpdateDrvAddHandlePage (
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  BM_MENU_ENTRY  *NewMenuEntry;
  UINT16         Index;

  CallbackData->BmmAskSaveOrNot = FALSE;

  UpdatePageStart (CallbackData);

  for (Index = 0; Index < DriverMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&DriverMenu, Index);

    HiiCreateGotoOpCode (
      mStartOpCodeHandle,
      FORM_DRV_ADD_HANDLE_DESC_ID,
      NewMenuEntry->DisplayStringToken,
      STRING_TOKEN (STR_NULL_STRING),
      EFI_IFR_FLAG_CALLBACK,
      (UINT16)(HANDLE_OPTION_OFFSET + Index)
      );
  }

  UpdatePageEnd (CallbackData);
}

/**
  Create a lit of driver option from global DriverOptionMenu. It
  allow user to delete the driver option.

  @param CallbackData    The BMM context data.

**/
VOID
UpdateDrvDelPage (
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  BM_MENU_ENTRY    *NewMenuEntry;
  BM_LOAD_CONTEXT  *NewLoadContext;
  UINT16           Index;

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  ASSERT (DriverOptionMenu.MenuNumber <= (sizeof (CallbackData->BmmFakeNvData.DriverOptionDel) / sizeof (CallbackData->BmmFakeNvData.DriverOptionDel[0])));
  for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&DriverOptionMenu, Index);

    NewLoadContext          = (BM_LOAD_CONTEXT *)NewMenuEntry->VariableContext;
    NewLoadContext->Deleted = FALSE;

    if (CallbackData->BmmFakeNvData.DriverOptionDel[Index] && !CallbackData->BmmFakeNvData.DriverOptionDelMark[Index]) {
      //
      // CallbackData->BmmFakeNvData.BootOptionDel[Index] == TRUE means browser knows this boot option is selected
      // CallbackData->BmmFakeNvData.BootOptionDelMark[Index] = FALSE means BDS knows the selected boot option has
      // deleted, browser maintains old useless info. So clear this info here, and later update this info to browser
      // through HiiSetBrowserData function.
      //
      CallbackData->BmmFakeNvData.DriverOptionDel[Index]    = FALSE;
      CallbackData->BmmOldFakeNVData.DriverOptionDel[Index] = FALSE;
    }

    HiiCreateCheckBoxOpCode (
      mStartOpCodeHandle,
      (EFI_QUESTION_ID)(DRIVER_OPTION_DEL_QUESTION_ID + Index),
      VARSTORE_ID_BOOT_MAINT,
      (UINT16)(DRIVER_OPTION_DEL_VAR_OFFSET + Index),
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      EFI_IFR_FLAG_CALLBACK,
      0,
      NULL
      );
  }

  UpdatePageEnd (CallbackData);
}

/**
  Prepare the page to allow user to add description for
  a Driver Option.

  @param CallbackData    The BMM context data.

**/
VOID
UpdateDriverAddHandleDescPage (
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  BM_MENU_ENTRY  *NewMenuEntry;

  CallbackData->BmmFakeNvData.DriverAddActive         = 0x01;
  CallbackData->BmmFakeNvData.DriverAddForceReconnect = 0x00;
  CallbackData->BmmAskSaveOrNot                       = TRUE;
  NewMenuEntry                                        = CallbackData->MenuEntry;

  UpdatePageStart (CallbackData);

  HiiCreateSubTitleOpCode (
    mStartOpCodeHandle,
    NewMenuEntry->DisplayStringToken,
    0,
    0,
    0
    );

  HiiCreateStringOpCode (
    mStartOpCodeHandle,
    (EFI_QUESTION_ID)DRV_ADD_HANDLE_DESC_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    DRV_ADD_HANDLE_DESC_VAR_OFFSET,
    STRING_TOKEN (STR_LOAD_OPTION_DESC),
    STRING_TOKEN (STR_NULL_STRING),
    0,
    0,
    6,
    75,
    NULL
    );

  HiiCreateCheckBoxOpCode (
    mStartOpCodeHandle,
    (EFI_QUESTION_ID)DRV_ADD_RECON_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    DRV_ADD_RECON_VAR_OFFSET,
    STRING_TOKEN (STR_LOAD_OPTION_FORCE_RECON),
    STRING_TOKEN (STR_LOAD_OPTION_FORCE_RECON),
    0,
    0,
    NULL
    );

  HiiCreateStringOpCode (
    mStartOpCodeHandle,
    (EFI_QUESTION_ID)DRIVER_ADD_OPTION_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    DRIVER_ADD_OPTION_VAR_OFFSET,
    STRING_TOKEN (STR_OPTIONAL_DATA),
    STRING_TOKEN (STR_NULL_STRING),
    0,
    0,
    6,
    75,
    NULL
    );

  UpdatePageEnd (CallbackData);
}

/**
  Update console page.

  @param UpdatePageId    The form ID to be updated.
  @param ConsoleMenu     The console menu list.
  @param CallbackData    The BMM context data.

**/
VOID
UpdateConsolePage (
  IN UINT16             UpdatePageId,
  IN BM_MENU_OPTION     *ConsoleMenu,
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  BM_MENU_ENTRY        *NewMenuEntry;
  BM_CONSOLE_CONTEXT   *NewConsoleContext;
  BM_TERMINAL_CONTEXT  *NewTerminalContext;
  UINT16               Index;
  UINT16               Index2;
  UINT8                CheckFlags;
  UINT8                *ConsoleCheck;
  EFI_QUESTION_ID      QuestionIdBase;
  UINT16               VariableOffsetBase;

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  ConsoleCheck       = NULL;
  QuestionIdBase     = 0;
  VariableOffsetBase = 0;

  switch (UpdatePageId) {
    case FORM_CON_IN_ID:
      ConsoleCheck       = &CallbackData->BmmFakeNvData.ConsoleInCheck[0];
      QuestionIdBase     = CON_IN_DEVICE_QUESTION_ID;
      VariableOffsetBase = CON_IN_DEVICE_VAR_OFFSET;
      break;

    case FORM_CON_OUT_ID:
      ConsoleCheck       = &CallbackData->BmmFakeNvData.ConsoleOutCheck[0];
      QuestionIdBase     = CON_OUT_DEVICE_QUESTION_ID;
      VariableOffsetBase = CON_OUT_DEVICE_VAR_OFFSET;
      break;

    case FORM_CON_ERR_ID:
      ConsoleCheck       = &CallbackData->BmmFakeNvData.ConsoleErrCheck[0];
      QuestionIdBase     = CON_ERR_DEVICE_QUESTION_ID;
      VariableOffsetBase = CON_ERR_DEVICE_VAR_OFFSET;
      break;
  }

  ASSERT (ConsoleCheck != NULL);

  for (Index = 0; ((Index < ConsoleMenu->MenuNumber) && \
                   (Index < MAX_MENU_NUMBER)); Index++)
  {
    CheckFlags        = 0;
    NewMenuEntry      = BOpt_GetMenuEntry (ConsoleMenu, Index);
    NewConsoleContext = (BM_CONSOLE_CONTEXT *)NewMenuEntry->VariableContext;
    if (NewConsoleContext->IsActive) {
      CheckFlags         |= EFI_IFR_CHECKBOX_DEFAULT;
      ConsoleCheck[Index] = TRUE;
    } else {
      ConsoleCheck[Index] = FALSE;
    }

    HiiCreateCheckBoxOpCode (
      mStartOpCodeHandle,
      (EFI_QUESTION_ID)(QuestionIdBase + Index),
      VARSTORE_ID_BOOT_MAINT,
      (UINT16)(VariableOffsetBase + Index),
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      EFI_IFR_FLAG_CALLBACK,
      CheckFlags,
      NULL
      );
  }

  for (Index2 = 0; ((Index2 < TerminalMenu.MenuNumber) && \
                    (Index2 < MAX_MENU_NUMBER)); Index2++)
  {
    CheckFlags         = 0;
    NewMenuEntry       = BOpt_GetMenuEntry (&TerminalMenu, Index2);
    NewTerminalContext = (BM_TERMINAL_CONTEXT *)NewMenuEntry->VariableContext;

    ASSERT (Index < MAX_MENU_NUMBER);
    if (((NewTerminalContext->IsConIn != 0) && (UpdatePageId == FORM_CON_IN_ID)) ||
        ((NewTerminalContext->IsConOut != 0) && (UpdatePageId == FORM_CON_OUT_ID)) ||
        ((NewTerminalContext->IsStdErr != 0) && (UpdatePageId == FORM_CON_ERR_ID))
        )
    {
      CheckFlags         |= EFI_IFR_CHECKBOX_DEFAULT;
      ConsoleCheck[Index] = TRUE;
    } else {
      ConsoleCheck[Index] = FALSE;
    }

    HiiCreateCheckBoxOpCode (
      mStartOpCodeHandle,
      (EFI_QUESTION_ID)(QuestionIdBase + Index),
      VARSTORE_ID_BOOT_MAINT,
      (UINT16)(VariableOffsetBase + Index),
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      EFI_IFR_FLAG_CALLBACK,
      CheckFlags,
      NULL
      );

    Index++;
  }

  UpdatePageEnd (CallbackData);
}

/**
  Update the page's NV Map if user has changed the order
  a list. This list can be Boot Order or Driver Order.

  @param UpdatePageId    The form ID to be updated.
  @param OptionMenu      The new list.
  @param CallbackData    The BMM context data.

**/
VOID
UpdateOrderPage (
  IN UINT16             UpdatePageId,
  IN BM_MENU_OPTION     *OptionMenu,
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  BM_MENU_ENTRY    *NewMenuEntry;
  UINT16           Index;
  UINT16           OptionIndex;
  VOID             *OptionsOpCodeHandle;
  BOOLEAN          BootOptionFound;
  UINT32           *OptionOrder;
  EFI_QUESTION_ID  QuestionId;
  UINT16           VarOffset;

  CallbackData->BmmAskSaveOrNot = TRUE;
  UpdatePageStart (CallbackData);

  OptionOrder = NULL;
  QuestionId  = 0;
  VarOffset   = 0;
  switch (UpdatePageId) {
    case FORM_BOOT_CHG_ID:
      //
      // If the BootOptionOrder in the BmmFakeNvData are same with the date in the BmmOldFakeNVData,
      // means all Boot Options has been save in BootOptionMenu, we can get the date from the menu.
      // else means browser maintains some uncommitted date which are not saved in BootOptionMenu,
      // so we should not get the data from BootOptionMenu to show it.
      //
      if (CompareMem (CallbackData->BmmFakeNvData.BootOptionOrder, CallbackData->BmmOldFakeNVData.BootOptionOrder, sizeof (CallbackData->BmmFakeNvData.BootOptionOrder)) == 0) {
        GetBootOrder (CallbackData);
      }

      OptionOrder = CallbackData->BmmFakeNvData.BootOptionOrder;
      QuestionId  = BOOT_OPTION_ORDER_QUESTION_ID;
      VarOffset   = BOOT_OPTION_ORDER_VAR_OFFSET;
      break;

    case FORM_DRV_CHG_ID:
      //
      // If the DriverOptionOrder in the BmmFakeNvData are same with the date in the BmmOldFakeNVData,
      // means all Driver Options has been save in DriverOptionMenu, we can get the DriverOptionOrder from the menu.
      // else means browser maintains some uncommitted date which are not saved in DriverOptionMenu,
      // so we should not get the data from DriverOptionMenu to show it.
      //
      if (CompareMem (CallbackData->BmmFakeNvData.DriverOptionOrder, CallbackData->BmmOldFakeNVData.DriverOptionOrder, sizeof (CallbackData->BmmFakeNvData.DriverOptionOrder)) == 0) {
        GetDriverOrder (CallbackData);
      }

      OptionOrder = CallbackData->BmmFakeNvData.DriverOptionOrder;
      QuestionId  = DRIVER_OPTION_ORDER_QUESTION_ID;
      VarOffset   = DRIVER_OPTION_ORDER_VAR_OFFSET;
      break;
  }

  ASSERT (OptionOrder != NULL);

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  NewMenuEntry = NULL;
  for (OptionIndex = 0; (OptionOrder[OptionIndex] != 0 && OptionIndex < MAX_MENU_NUMBER); OptionIndex++) {
    BootOptionFound = FALSE;
    for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
      NewMenuEntry = BOpt_GetMenuEntry (OptionMenu, Index);
      if ((UINT32)(NewMenuEntry->OptionNumber + 1) == OptionOrder[OptionIndex]) {
        BootOptionFound = TRUE;
        break;
      }
    }

    if (BootOptionFound) {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        NewMenuEntry->DisplayStringToken,
        0,
        EFI_IFR_TYPE_NUM_SIZE_32,
        OptionOrder[OptionIndex]
        );
    }
  }

  if (OptionMenu->MenuNumber > 0) {
    HiiCreateOrderedListOpCode (
      mStartOpCodeHandle,                          // Container for dynamic created opcodes
      QuestionId,                                  // Question ID
      VARSTORE_ID_BOOT_MAINT,                      // VarStore ID
      VarOffset,                                   // Offset in Buffer Storage
      STRING_TOKEN (STR_CHANGE_ORDER),             // Question prompt text
      STRING_TOKEN (STR_CHANGE_ORDER),             // Question help text
      0,                                           // Question flag
      0,                                           // Ordered list flag, e.g. EFI_IFR_UNIQUE_SET
      EFI_IFR_TYPE_NUM_SIZE_32,                    // Data type of Question value
      100,                                         // Maximum container
      OptionsOpCodeHandle,                         // Option Opcode list
      NULL                                         // Default Opcode is NULL
      );
  }

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  UpdatePageEnd (CallbackData);
}

/**
  Refresh the text mode page.

  @param CallbackData    The BMM context data.

**/
VOID
UpdateConModePage (
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  UINTN                            Mode;
  UINTN                            Index;
  UINTN                            Col;
  UINTN                            Row;
  CHAR16                           ModeString[50];
  CHAR16                           *PStr;
  UINTN                            MaxMode;
  UINTN                            ValidMode;
  EFI_STRING_ID                    *ModeToken;
  EFI_STATUS                       Status;
  VOID                             *OptionsOpCodeHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ConOut;

  ConOut    = gST->ConOut;
  Index     = 0;
  ValidMode = 0;
  MaxMode   = (UINTN)(ConOut->Mode->MaxMode);

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  //
  // Check valid mode
  //
  for (Mode = 0; Mode < MaxMode; Mode++) {
    Status = ConOut->QueryMode (ConOut, Mode, &Col, &Row);
    if (EFI_ERROR (Status)) {
      continue;
    }

    ValidMode++;
  }

  if (ValidMode == 0) {
    return;
  }

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  ModeToken = AllocateZeroPool (sizeof (EFI_STRING_ID) * ValidMode);
  ASSERT (ModeToken != NULL);

  //
  // Determin which mode should be the first entry in menu
  //
  GetConsoleOutMode (CallbackData);

  //
  // Build text mode options
  //
  for (Mode = 0; Mode < MaxMode; Mode++) {
    Status = ConOut->QueryMode (ConOut, Mode, &Col, &Row);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Build mode string Column x Row
    //
    UnicodeValueToStringS (ModeString, sizeof (ModeString), 0, Col, 0);
    PStr = &ModeString[0];
    StrnCatS (PStr, ARRAY_SIZE (ModeString), L" x ", StrLen (L" x ") + 1);
    PStr = PStr + StrLen (PStr);
    UnicodeValueToStringS (
      PStr,
      sizeof (ModeString) - ((UINTN)PStr - (UINTN)&ModeString[0]),
      0,
      Row,
      0
      );

    ModeToken[Index] = HiiSetString (CallbackData->BmmHiiHandle, 0, ModeString, NULL);

    if (Mode == CallbackData->BmmFakeNvData.ConsoleOutMode) {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        ModeToken[Index],
        EFI_IFR_OPTION_DEFAULT,
        EFI_IFR_TYPE_NUM_SIZE_16,
        (UINT16)Mode
        );
    } else {
      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        ModeToken[Index],
        0,
        EFI_IFR_TYPE_NUM_SIZE_16,
        (UINT16)Mode
        );
    }

    Index++;
  }

  HiiCreateOneOfOpCode (
    mStartOpCodeHandle,
    (EFI_QUESTION_ID)CON_MODE_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    CON_MODE_VAR_OFFSET,
    STRING_TOKEN (STR_CON_MODE_SETUP),
    STRING_TOKEN (STR_CON_MODE_SETUP),
    EFI_IFR_FLAG_RESET_REQUIRED,
    EFI_IFR_NUMERIC_SIZE_2,
    OptionsOpCodeHandle,
    NULL
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);
  FreePool (ModeToken);

  UpdatePageEnd (CallbackData);
}

/**
 Create the dynamic page which allows user to set the property such as Baud Rate, Data Bits,
 Parity, Stop Bits, Terminal Type.

 @param CallbackData    The BMM context data.

**/
VOID
UpdateTerminalPage (
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  UINT8          Index;
  UINT8          CheckFlags;
  BM_MENU_ENTRY  *NewMenuEntry;
  VOID           *OptionsOpCodeHandle;
  UINTN          CurrentTerminal;

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  CurrentTerminal = CallbackData->CurrentTerminal;
  NewMenuEntry    = BOpt_GetMenuEntry (
                      &TerminalMenu,
                      CurrentTerminal
                      );

  if (NewMenuEntry == NULL) {
    return;
  }

  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (Index = 0; Index < sizeof (BaudRateList) / sizeof (BaudRateList[0]); Index++) {
    CheckFlags = 0;
    if (BaudRateList[Index].Value == 115200) {
      CheckFlags |= EFI_IFR_OPTION_DEFAULT;
    }

    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      BaudRateList[Index].StringToken,
      CheckFlags,
      EFI_IFR_TYPE_NUM_SIZE_8,
      Index
      );
  }

  HiiCreateOneOfOpCode (
    mStartOpCodeHandle,
    (EFI_QUESTION_ID)(COM_BAUD_RATE_QUESTION_ID + CurrentTerminal),
    VARSTORE_ID_BOOT_MAINT,
    (UINT16)(COM_BAUD_RATE_VAR_OFFSET + CurrentTerminal),
    STRING_TOKEN (STR_COM_BAUD_RATE),
    STRING_TOKEN (STR_COM_BAUD_RATE),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_1,
    OptionsOpCodeHandle,
    NULL
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (Index = 0; Index < ARRAY_SIZE (DataBitsList); Index++) {
    CheckFlags = 0;

    if (DataBitsList[Index].Value == 8) {
      CheckFlags |= EFI_IFR_OPTION_DEFAULT;
    }

    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      DataBitsList[Index].StringToken,
      CheckFlags,
      EFI_IFR_TYPE_NUM_SIZE_8,
      Index
      );
  }

  HiiCreateOneOfOpCode (
    mStartOpCodeHandle,
    (EFI_QUESTION_ID)(COM_DATA_RATE_QUESTION_ID + CurrentTerminal),
    VARSTORE_ID_BOOT_MAINT,
    (UINT16)(COM_DATA_RATE_VAR_OFFSET + CurrentTerminal),
    STRING_TOKEN (STR_COM_DATA_BITS),
    STRING_TOKEN (STR_COM_DATA_BITS),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_1,
    OptionsOpCodeHandle,
    NULL
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (Index = 0; Index < ARRAY_SIZE (ParityList); Index++) {
    CheckFlags = 0;
    if (ParityList[Index].Value ==  NoParity) {
      CheckFlags |= EFI_IFR_OPTION_DEFAULT;
    }

    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      ParityList[Index].StringToken,
      CheckFlags,
      EFI_IFR_TYPE_NUM_SIZE_8,
      Index
      );
  }

  HiiCreateOneOfOpCode (
    mStartOpCodeHandle,
    (EFI_QUESTION_ID)(COM_PARITY_QUESTION_ID + CurrentTerminal),
    VARSTORE_ID_BOOT_MAINT,
    (UINT16)(COM_PARITY_VAR_OFFSET + CurrentTerminal),
    STRING_TOKEN (STR_COM_PARITY),
    STRING_TOKEN (STR_COM_PARITY),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_1,
    OptionsOpCodeHandle,
    NULL
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (Index = 0; Index < ARRAY_SIZE (StopBitsList); Index++) {
    CheckFlags = 0;
    if (StopBitsList[Index].Value == OneStopBit) {
      CheckFlags |= EFI_IFR_OPTION_DEFAULT;
    }

    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      StopBitsList[Index].StringToken,
      CheckFlags,
      EFI_IFR_TYPE_NUM_SIZE_8,
      Index
      );
  }

  HiiCreateOneOfOpCode (
    mStartOpCodeHandle,
    (EFI_QUESTION_ID)(COM_STOP_BITS_QUESTION_ID + CurrentTerminal),
    VARSTORE_ID_BOOT_MAINT,
    (UINT16)(COM_STOP_BITS_VAR_OFFSET + CurrentTerminal),
    STRING_TOKEN (STR_COM_STOP_BITS),
    STRING_TOKEN (STR_COM_STOP_BITS),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_1,
    OptionsOpCodeHandle,
    NULL
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (Index = 0; Index < ARRAY_SIZE (TerminalType); Index++) {
    CheckFlags = 0;
    if (Index == 0) {
      CheckFlags |= EFI_IFR_OPTION_DEFAULT;
    }

    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      (EFI_STRING_ID)TerminalType[Index],
      CheckFlags,
      EFI_IFR_TYPE_NUM_SIZE_8,
      Index
      );
  }

  HiiCreateOneOfOpCode (
    mStartOpCodeHandle,
    (EFI_QUESTION_ID)(COM_TERMINAL_QUESTION_ID + CurrentTerminal),
    VARSTORE_ID_BOOT_MAINT,
    (UINT16)(COM_TERMINAL_VAR_OFFSET + CurrentTerminal),
    STRING_TOKEN (STR_COM_TERMI_TYPE),
    STRING_TOKEN (STR_COM_TERMI_TYPE),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_1,
    OptionsOpCodeHandle,
    NULL
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  for (Index = 0; Index < ARRAY_SIZE (mFlowControlType); Index++) {
    CheckFlags = 0;
    if (Index == 0) {
      CheckFlags |= EFI_IFR_OPTION_DEFAULT;
    }

    HiiCreateOneOfOptionOpCode (
      OptionsOpCodeHandle,
      (EFI_STRING_ID)mFlowControlType[Index],
      CheckFlags,
      EFI_IFR_TYPE_NUM_SIZE_8,
      mFlowControlValue[Index]
      );
  }

  HiiCreateOneOfOpCode (
    mStartOpCodeHandle,
    (EFI_QUESTION_ID)(COM_FLOWCONTROL_QUESTION_ID + CurrentTerminal),
    VARSTORE_ID_BOOT_MAINT,
    (UINT16)(COM_FLOWCONTROL_VAR_OFFSET + CurrentTerminal),
    STRING_TOKEN (STR_COM_FLOW_CONTROL),
    STRING_TOKEN (STR_COM_FLOW_CONTROL),
    EFI_IFR_FLAG_CALLBACK,
    EFI_IFR_NUMERIC_SIZE_1,
    OptionsOpCodeHandle,
    NULL
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  UpdatePageEnd (CallbackData);
}

/**
Update add boot/driver option page.

@param CallbackData    The BMM context data.
@param FormId             The form ID to be updated.
@param DevicePath       Device path.

**/
VOID
UpdateOptionPage (
  IN   BMM_CALLBACK_DATA         *CallbackData,
  IN   EFI_FORM_ID               FormId,
  IN   EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CHAR16         *String;
  EFI_STRING_ID  StringToken;

  String = NULL;

  if (DevicePath != NULL) {
    String = ExtractFileNameFromDevicePath (DevicePath);
  }

  if (String == NULL) {
    String = HiiGetString (CallbackData->BmmHiiHandle, STRING_TOKEN (STR_NULL_STRING), NULL);
    ASSERT (String != NULL);
  }

  StringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, String, NULL);
  FreePool (String);

  if (FormId == FORM_BOOT_ADD_ID) {
    if (!CallbackData->BmmFakeNvData.BootOptionChanged) {
      ZeroMem (CallbackData->BmmFakeNvData.BootOptionalData, sizeof (CallbackData->BmmFakeNvData.BootOptionalData));
      ZeroMem (CallbackData->BmmFakeNvData.BootDescriptionData, sizeof (CallbackData->BmmFakeNvData.BootDescriptionData));
      ZeroMem (CallbackData->BmmOldFakeNVData.BootOptionalData, sizeof (CallbackData->BmmOldFakeNVData.BootOptionalData));
      ZeroMem (CallbackData->BmmOldFakeNVData.BootDescriptionData, sizeof (CallbackData->BmmOldFakeNVData.BootDescriptionData));
    }
  } else if (FormId == FORM_DRV_ADD_FILE_ID) {
    if (!CallbackData->BmmFakeNvData.DriverOptionChanged) {
      ZeroMem (CallbackData->BmmFakeNvData.DriverOptionalData, sizeof (CallbackData->BmmFakeNvData.DriverOptionalData));
      ZeroMem (CallbackData->BmmFakeNvData.DriverDescriptionData, sizeof (CallbackData->BmmFakeNvData.DriverDescriptionData));
      ZeroMem (CallbackData->BmmOldFakeNVData.DriverOptionalData, sizeof (CallbackData->BmmOldFakeNVData.DriverOptionalData));
      ZeroMem (CallbackData->BmmOldFakeNVData.DriverDescriptionData, sizeof (CallbackData->BmmOldFakeNVData.DriverDescriptionData));
    }
  }

  RefreshUpdateData ();
  mStartLabel->Number = FormId;

  HiiCreateSubTitleOpCode (
    mStartOpCodeHandle,
    StringToken,
    0,
    0,
    0
    );

  HiiUpdateForm (
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    FormId,
    mStartOpCodeHandle, // Label FormId
    mEndOpCodeHandle    // LABEL_END
    );
}

/**
  Dispatch the correct update page function to call based on
  the UpdatePageId.

  @param UpdatePageId    The form ID.
  @param CallbackData    The BMM context data.

**/
VOID
UpdatePageBody (
  IN UINT16             UpdatePageId,
  IN BMM_CALLBACK_DATA  *CallbackData
  )
{
  CleanUpPage (UpdatePageId, CallbackData);
  switch (UpdatePageId) {
    case FORM_CON_IN_ID:
      UpdateConsolePage (UpdatePageId, &ConsoleInpMenu, CallbackData);
      break;

    case FORM_CON_OUT_ID:
      UpdateConsolePage (UpdatePageId, &ConsoleOutMenu, CallbackData);
      break;

    case FORM_CON_ERR_ID:
      UpdateConsolePage (UpdatePageId, &ConsoleErrMenu, CallbackData);
      break;

    case FORM_BOOT_CHG_ID:
      UpdateOrderPage (UpdatePageId, &BootOptionMenu, CallbackData);
      break;

    case FORM_DRV_CHG_ID:
      UpdateOrderPage (UpdatePageId, &DriverOptionMenu, CallbackData);
      break;

    default:
      break;
  }
}

/**
  Dispatch the display to the next page based on NewPageId.

  @param Private         The BMM context data.
  @param NewPageId       The original page ID.

**/
VOID
UpdatePageId (
  BMM_CALLBACK_DATA  *Private,
  UINT16             NewPageId
  )
{
  if ((NewPageId < FILE_OPTION_OFFSET) && (NewPageId >= HANDLE_OPTION_OFFSET)) {
    //
    // If we select a handle to add driver option, advance to the add handle description page.
    //
    NewPageId = FORM_DRV_ADD_HANDLE_DESC_ID;
  } else if ((NewPageId == KEY_VALUE_SAVE_AND_EXIT) || (NewPageId == KEY_VALUE_NO_SAVE_AND_EXIT)) {
    //
    // Return to main page after "Save Changes" or "Discard Changes".
    //
    NewPageId = FORM_MAIN_ID;
  } else if ((NewPageId >= TERMINAL_OPTION_OFFSET) && (NewPageId < CONSOLE_OPTION_OFFSET)) {
    NewPageId = FORM_CON_COM_SETUP_ID;
  }

  if ((NewPageId > 0) && (NewPageId < MAXIMUM_FORM_ID)) {
    Private->BmmPreviousPageId = Private->BmmCurrentPageId;
    Private->BmmCurrentPageId  = NewPageId;
  }
}
