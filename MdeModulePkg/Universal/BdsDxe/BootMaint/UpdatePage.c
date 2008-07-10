/** @file
Dynamically update the pages.

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BootMaint.h"

/**
  Refresh the global UpdateData structure.

  

  

**/
VOID
RefreshUpdateData (
  VOID
  )
{
  gUpdateData.Offset = 0;
}

/**
  Add a "Go back to main page" tag in front of the form when there are no
  "Apply changes" and "Discard changes" tags in the end of the form.
  

  @param CallbackData    The BMM context data.

  

**/
VOID
UpdatePageStart (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  RefreshUpdateData ();

  if (!(CallbackData->BmmAskSaveOrNot)) {
    //
    // Add a "Go back to main page" tag in front of the form when there are no
    // "Apply changes" and "Discard changes" tags in the end of the form.
    //
    CreateGotoOpCode (
      FORM_MAIN_ID,
      STRING_TOKEN (STR_FORM_GOTO_MAIN),
      STRING_TOKEN (STR_FORM_GOTO_MAIN),
      0,
      FORM_MAIN_ID,
      &gUpdateData
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
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  //
  // Create the "Apply changes" and "Discard changes" tags.
  //
  if (CallbackData->BmmAskSaveOrNot) {
    CreateSubTitleOpCode (
      STRING_TOKEN (STR_NULL_STRING),
      0,
      0,
      0,
      &gUpdateData
      );

    CreateGotoOpCode (
      FORM_MAIN_ID,
      STRING_TOKEN (STR_SAVE_AND_EXIT),
      STRING_TOKEN (STR_NULL_STRING),
      EFI_IFR_FLAG_CALLBACK,
      KEY_VALUE_SAVE_AND_EXIT,
      &gUpdateData
      );
  }

  //
  // Ensure user can return to the main page.
  //
  CreateGotoOpCode (
    FORM_MAIN_ID,
    STRING_TOKEN (STR_NO_SAVE_AND_EXIT),
    STRING_TOKEN (STR_NULL_STRING),
    EFI_IFR_FLAG_CALLBACK,
    KEY_VALUE_NO_SAVE_AND_EXIT,
    &gUpdateData
    );

  IfrLibUpdateForm (
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    CallbackData->BmmCurrentPageId,
    CallbackData->BmmCurrentPageId,
    FALSE,
    &gUpdateData
    );
}

/**
  Clean up the dynamic opcode at label and form specified by
  both LabelId. 

  @param LabelId         It is both the Form ID and Label ID for
                         opcode deletion.
  @param CallbackData    The BMM context data.

  

**/
VOID
CleanUpPage (
  IN UINT16                           LabelId,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  RefreshUpdateData ();

  //
  // Remove all op-codes from dynamic page
  //
  IfrLibUpdateForm (
    CallbackData->BmmHiiHandle,
    &mBootMaintGuid,
    LabelId,
    LabelId,
    FALSE,
    &gUpdateData
    );
}

/**
  Boot a file selected by user at File Expoloer of BMM.

  @param FileContext     The file context data, which contains the device path
                         of the file to be boot from.

  @retval EFI_SUCCESS    The function completed successfull.
  @return                 Other value if the boot from the file fails.

**/
EFI_STATUS
BootThisFile (
  IN BM_FILE_CONTEXT                   *FileContext
  )
{
  EFI_STATUS        Status;
  UINTN             ExitDataSize;
  CHAR16            *ExitData;
  BDS_COMMON_OPTION *Option;

  Status                  = gBS->AllocatePool (EfiBootServicesData, sizeof (BDS_COMMON_OPTION), (VOID **) &Option);
  Option->Description     = FileContext->FileName;
  Option->DevicePath      = FileContext->DevicePath;
  Option->LoadOptionsSize = 0;
  Option->LoadOptions     = NULL;

  //
  // Since current no boot from removable media directly is allowed */
  //
  gST->ConOut->ClearScreen (gST->ConOut);

  gBS->RaiseTPL (TPL_APPLICATION);

  ExitDataSize  = 0;

  Status        = BdsLibBootViaBootOption (Option, Option->DevicePath, &ExitDataSize, &ExitData);

  gBS->RestoreTPL (TPL_APPLICATION);

  return Status;

}

/**
  Create a list of Goto Opcode for all terminal devices logged
  by TerminaMenu. This list will be inserted to form FORM_CON_COM_SETUP_ID.

  @param CallbackData    The BMM context data.

  

**/
VOID
UpdateConCOMPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINT16        Index;

  CallbackData->BmmAskSaveOrNot = FALSE;

  UpdatePageStart (CallbackData);


  for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&TerminalMenu, Index);

    CreateGotoOpCode (
      FORM_CON_COM_SETUP_ID,
      NewMenuEntry->DisplayStringToken,
      STRING_TOKEN (STR_NULL_STRING),
      EFI_IFR_FLAG_CALLBACK,
      (UINT16) (TERMINAL_OPTION_OFFSET + Index),
      &gUpdateData
      );
  }

  UpdatePageEnd (CallbackData);
}

/**
  Create a lit of boot option from global BootOptionMenu. It
  allow user to delete the boot option.

  @param CallbackData    The BMM context data.

  

**/
VOID
UpdateBootDelPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  UINT16          Index;

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);
  CreateMenuStringToken (CallbackData, CallbackData->BmmHiiHandle, &BootOptionMenu);

  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry    = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    NewLoadContext  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    if (NewLoadContext->IsLegacy) {
      continue;
    }

    NewLoadContext->Deleted = FALSE;
    CallbackData->BmmFakeNvData.BootOptionDel[Index] = 0x00;

    CreateCheckBoxOpCode (
      (EFI_QUESTION_ID) (BOOT_OPTION_DEL_QUESTION_ID + Index),
      VARSTORE_ID_BOOT_MAINT,
      (UINT16) (BOOT_OPTION_DEL_VAR_OFFSET + Index),
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      0,
      0,
      &gUpdateData
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
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINT16        Index;

  CallbackData->BmmAskSaveOrNot = FALSE;

  UpdatePageStart (CallbackData);

  for (Index = 0; Index < DriverMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&DriverMenu, Index);

    CreateGotoOpCode (
      FORM_DRV_ADD_HANDLE_DESC_ID,
      NewMenuEntry->DisplayStringToken,
      STRING_TOKEN (STR_NULL_STRING),
      EFI_IFR_FLAG_CALLBACK,
      (UINT16) (HANDLE_OPTION_OFFSET + Index),
      &gUpdateData
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
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  UINT16          Index;

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  CreateMenuStringToken (CallbackData, CallbackData->BmmHiiHandle, &DriverOptionMenu);

  for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
    NewMenuEntry            = BOpt_GetMenuEntry (&DriverOptionMenu, Index);

    NewLoadContext          = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    NewLoadContext->Deleted = FALSE;
    CallbackData->BmmFakeNvData.DriverOptionDel[Index] = 0x00;

    CreateCheckBoxOpCode (
      (EFI_QUESTION_ID) (DRIVER_OPTION_DEL_QUESTION_ID + Index),
      VARSTORE_ID_BOOT_MAINT,
      (UINT16) (DRIVER_OPTION_DEL_VAR_OFFSET + Index),
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      0,
      0,
      &gUpdateData
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
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY *NewMenuEntry;

  CallbackData->BmmFakeNvData.DriverAddActive          = 0x01;
  CallbackData->BmmFakeNvData.DriverAddForceReconnect  = 0x00;
  CallbackData->BmmAskSaveOrNot                        = TRUE;
  NewMenuEntry = CallbackData->MenuEntry;

  UpdatePageStart (CallbackData);

  CreateSubTitleOpCode (
    NewMenuEntry->DisplayStringToken,
    0,
    0,
    0,
    &gUpdateData
    );

  CreateStringOpCode (
    (EFI_QUESTION_ID) DRV_ADD_HANDLE_DESC_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    DRV_ADD_HANDLE_DESC_VAR_OFFSET,
    STRING_TOKEN (STR_LOAD_OPTION_DESC),
    STRING_TOKEN (STR_NULL_STRING),
    0,
    0,
    6,
    75,
    &gUpdateData
    );

  CreateCheckBoxOpCode (
    (EFI_QUESTION_ID) DRV_ADD_RECON_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    DRV_ADD_RECON_VAR_OFFSET,
    STRING_TOKEN (STR_LOAD_OPTION_FORCE_RECON),
    STRING_TOKEN (STR_LOAD_OPTION_FORCE_RECON),
    0,
    0,
    &gUpdateData
    );

  CreateStringOpCode (
    (EFI_QUESTION_ID) DRIVER_ADD_OPTION_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    DRIVER_ADD_OPTION_VAR_OFFSET,
    STRING_TOKEN (STR_OPTIONAL_DATA),
    STRING_TOKEN (STR_NULL_STRING),
    0,
    0,
    6,
    75,
    &gUpdateData
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
  IN UINT16                           UpdatePageId,
  IN BM_MENU_OPTION                   *ConsoleMenu,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY       *NewMenuEntry;
  BM_CONSOLE_CONTEXT  *NewConsoleContext;
  BM_TERMINAL_CONTEXT *NewTerminalContext;
  UINT16              Index;
  UINT16              Index2;
  UINT8               CheckFlags;
 
  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  for (Index = 0; Index < ConsoleMenu->MenuNumber; Index++) {
    NewMenuEntry      = BOpt_GetMenuEntry (ConsoleMenu, Index);
    NewConsoleContext = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
    CheckFlags        = 0;
    if (NewConsoleContext->IsActive) {
      CheckFlags |= EFI_IFR_CHECKBOX_DEFAULT;
      CallbackData->BmmFakeNvData.ConsoleCheck[Index] = TRUE;
    } else {
      CallbackData->BmmFakeNvData.ConsoleCheck[Index] = FALSE;
    }

    CreateCheckBoxOpCode (
      (EFI_QUESTION_ID) (CON_DEVICE_QUESTION_ID + Index),
      VARSTORE_ID_BOOT_MAINT,
      (UINT16) (CON_DEVICE_VAR_OFFSET + Index),
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      0,
      CheckFlags,
      &gUpdateData
      );
  }

  for (Index2 = 0; Index2 < TerminalMenu.MenuNumber; Index2++) {
    CheckFlags          = 0;
    NewMenuEntry        = BOpt_GetMenuEntry (&TerminalMenu, Index2);
    NewTerminalContext  = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;

    if (((NewTerminalContext->IsConIn != 0) && (UpdatePageId == FORM_CON_IN_ID)) ||
        ((NewTerminalContext->IsConOut != 0) && (UpdatePageId == FORM_CON_OUT_ID)) ||
        ((NewTerminalContext->IsStdErr != 0) && (UpdatePageId == FORM_CON_ERR_ID))
        ) {
      CheckFlags |= EFI_IFR_CHECKBOX_DEFAULT;
      CallbackData->BmmFakeNvData.ConsoleCheck[Index] = TRUE;
    } else {
      CallbackData->BmmFakeNvData.ConsoleCheck[Index] = FALSE;
    }

    CreateCheckBoxOpCode (
      (EFI_QUESTION_ID) (CON_DEVICE_QUESTION_ID + Index),
      VARSTORE_ID_BOOT_MAINT,
      (UINT16) (CON_DEVICE_VAR_OFFSET + Index),
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      0,
      CheckFlags,
      &gUpdateData
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
  IN UINT16                           UpdatePageId,
  IN BM_MENU_OPTION                   *OptionMenu,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINT16        Index;
  IFR_OPTION    *IfrOptionList;

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  CreateMenuStringToken (CallbackData, CallbackData->BmmHiiHandle, OptionMenu);

  ZeroMem (CallbackData->BmmFakeNvData.OptionOrder, 100);

  IfrOptionList = AllocateZeroPool (sizeof (IFR_OPTION) * OptionMenu->MenuNumber);
  if (NULL == IfrOptionList) {
    return ;
  }

  for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (OptionMenu, Index);
    IfrOptionList[Index].StringToken = NewMenuEntry->DisplayStringToken;
    IfrOptionList[Index].Value.u8 = (UINT8) (NewMenuEntry->OptionNumber + 1);
    IfrOptionList[Index].Flags = 0;
    CallbackData->BmmFakeNvData.OptionOrder[Index] = IfrOptionList[Index].Value.u8;
  }

  if (OptionMenu->MenuNumber > 0) {
    CreateOrderedListOpCode (
      (EFI_QUESTION_ID) OPTION_ORDER_QUESTION_ID,
      VARSTORE_ID_BOOT_MAINT,
      OPTION_ORDER_VAR_OFFSET,
      STRING_TOKEN (STR_CHANGE_ORDER),
      STRING_TOKEN (STR_CHANGE_ORDER),
      0,
      0,
      EFI_IFR_NUMERIC_SIZE_1,
      100,
      IfrOptionList,
      OptionMenu->MenuNumber,
      &gUpdateData
      );
  }

  SafeFreePool (IfrOptionList);

  UpdatePageEnd (CallbackData);

  CopyMem (
    CallbackData->BmmOldFakeNVData.OptionOrder,
    CallbackData->BmmFakeNvData.OptionOrder,
    100
    );
}

/**
  Create the dynamic page to allow user to set
  the "BootNext" vaule.

  @param CallbackData    The BMM context data.

  

**/
VOID
UpdateBootNextPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  IFR_OPTION      *IfrOptionList;
  UINTN           NumberOfOptions;
  UINT16          Index;

  IfrOptionList                 = NULL;
  NumberOfOptions               = BootOptionMenu.MenuNumber;
  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);
  CreateMenuStringToken (CallbackData, CallbackData->BmmHiiHandle, &BootOptionMenu);

  if (NumberOfOptions > 0) {
    IfrOptionList = AllocateZeroPool ((NumberOfOptions + 1) * sizeof (IFR_OPTION));

    ASSERT (IfrOptionList);

    CallbackData->BmmFakeNvData.BootNext = (UINT16) (BootOptionMenu.MenuNumber);

    for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
      NewMenuEntry    = BOpt_GetMenuEntry (&BootOptionMenu, Index);
      NewLoadContext  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
      if (NewLoadContext->IsBootNext) {
        IfrOptionList[Index].Flags            = EFI_IFR_OPTION_DEFAULT;
        CallbackData->BmmFakeNvData.BootNext = Index;
      } else {
        IfrOptionList[Index].Flags = 0;
      }

      IfrOptionList[Index].Value.u16    = Index;
      IfrOptionList[Index].StringToken  = NewMenuEntry->DisplayStringToken;
    }

    IfrOptionList[Index].Value.u16        = Index;
    IfrOptionList[Index].StringToken  = STRING_TOKEN (STR_NONE);
    IfrOptionList[Index].Flags        = 0;
    if (CallbackData->BmmFakeNvData.BootNext == Index) {
      IfrOptionList[Index].Flags |= EFI_IFR_OPTION_DEFAULT;
    }

    CreateOneOfOpCode (
      (EFI_QUESTION_ID) BOOT_NEXT_QUESTION_ID,
      VARSTORE_ID_BOOT_MAINT,
      BOOT_NEXT_VAR_OFFSET,
      STRING_TOKEN (STR_BOOT_NEXT),
      STRING_TOKEN (STR_BOOT_NEXT_HELP),
      0,
      EFI_IFR_NUMERIC_SIZE_2,
      IfrOptionList,
      (UINTN) (NumberOfOptions + 1),
      &gUpdateData
      );

    SafeFreePool (IfrOptionList);
  }

  UpdatePageEnd (CallbackData);
}

/**
  Create the dynamic page to allow user to set
  the "TimeOut" vaule.

  @param CallbackData    The BMM context data.

  

**/
VOID
UpdateTimeOutPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  UINT16  BootTimeOut;

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  BootTimeOut = BdsLibGetTimeout ();

  CreateNumericOpCode (
    (EFI_QUESTION_ID) BOOT_TIME_OUT_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    BOOT_TIME_OUT_VAR_OFFSET,
    STRING_TOKEN (STR_NUM_AUTO_BOOT),
    STRING_TOKEN (STR_HLP_AUTO_BOOT),
    0,
    EFI_IFR_NUMERIC_SIZE_2 | EFI_IFR_DISPLAY_UINT_DEC,
    0,
    65535,
    0,
    BootTimeOut,
    &gUpdateData
    );

  CallbackData->BmmFakeNvData.BootTimeOut = BootTimeOut;

  UpdatePageEnd (CallbackData);
}

/**
  Refresh the text mode page


  @param CallbackData    The BMM context data.

  

**/
VOID
UpdateConModePage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  UINTN                         Mode;
  UINTN                         Index;
  UINTN                         Col;
  UINTN                         Row;
  CHAR16                        RowString[50];
  CHAR16                        ModeString[50];
  UINTN                         MaxMode;
  UINTN                         ValidMode;
  EFI_STRING_ID                 *ModeToken;
  IFR_OPTION                    *IfrOptionList;
  EFI_STATUS                    Status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ConOut;

  ConOut    = gST->ConOut;
  Index     = 0;
  ValidMode = 0;
  MaxMode   = (UINTN) (ConOut->Mode->MaxMode);

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

  IfrOptionList       = AllocateZeroPool (sizeof (IFR_OPTION) * ValidMode);
  ASSERT(IfrOptionList != NULL);

  ModeToken           = AllocateZeroPool (sizeof (EFI_STRING_ID) * ValidMode);
  ASSERT(ModeToken != NULL);

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
    UnicodeValueToString (ModeString, 0, Col, 0);
    StrCat (ModeString, L" x ");
    UnicodeValueToString (RowString, 0, Row, 0);
    StrCat (ModeString, RowString);

    HiiLibNewString (CallbackData->BmmHiiHandle, &ModeToken[Index], ModeString);

    IfrOptionList[Index].StringToken  = ModeToken[Index];
    IfrOptionList[Index].Value.u16    = (UINT16) Mode;
    if (Mode == CallbackData->BmmFakeNvData.ConsoleOutMode) {
      IfrOptionList[Index].Flags      = EFI_IFR_OPTION_DEFAULT;
    } else {
      IfrOptionList[Index].Flags      = 0;
    }
    Index++;
  }

  CreateOneOfOpCode (
    (EFI_QUESTION_ID) CON_MODE_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    CON_MODE_VAR_OFFSET,
    STRING_TOKEN (STR_CON_MODE_SETUP),
    STRING_TOKEN (STR_CON_MODE_SETUP),
    EFI_IFR_FLAG_RESET_REQUIRED,
    EFI_IFR_NUMERIC_SIZE_2,
    IfrOptionList,
    ValidMode,
    &gUpdateData
    );
  SafeFreePool (IfrOptionList);
  SafeFreePool (ModeToken);

  UpdatePageEnd (CallbackData);
}

/**
  Create the dynamic page which allows user to 
  set the property such as Baud Rate, Data Bits,
  Parity, Stop Bits, Terminal Type.

  @param CallbackData    The BMM context data.

  

**/
VOID
UpdateTerminalPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  UINT8               Index;
  UINT8               CheckFlags;
  IFR_OPTION          *IfrOptionList;
  BM_MENU_ENTRY       *NewMenuEntry;
  BM_TERMINAL_CONTEXT *NewTerminalContext;

  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData);

  NewMenuEntry = BOpt_GetMenuEntry (
                  &TerminalMenu,
                  CallbackData->CurrentTerminal
                  );

  if (NewMenuEntry == NULL) {
    return ;
  }

  NewTerminalContext  = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;

  IfrOptionList       = AllocateZeroPool (sizeof (IFR_OPTION) * 19);
  if (IfrOptionList == NULL) {
    return ;
  }

  for (Index = 0; Index < sizeof (BaudRateList) / sizeof (BaudRateList [0]); Index++) {
    CheckFlags = 0;
    if (NewTerminalContext->BaudRate == (UINT64) (BaudRateList[Index].Value)) {
      CheckFlags |= EFI_IFR_OPTION_DEFAULT;
      NewTerminalContext->BaudRateIndex         = Index;
      CallbackData->BmmFakeNvData.COMBaudRate  = NewTerminalContext->BaudRateIndex;
    }

    IfrOptionList[Index].Flags        = CheckFlags;
    IfrOptionList[Index].StringToken  = BaudRateList[Index].StringToken;
    IfrOptionList[Index].Value.u8     = Index;
  }

  CreateOneOfOpCode (
    (EFI_QUESTION_ID) COM_BAUD_RATE_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    COM_BAUD_RATE_VAR_OFFSET,
    STRING_TOKEN (STR_COM_BAUD_RATE),
    STRING_TOKEN (STR_COM_BAUD_RATE),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    IfrOptionList,
    19,
    &gUpdateData
    );

  for (Index = 0; Index < sizeof (DataBitsList) / sizeof (DataBitsList[0]); Index++) {
    CheckFlags = 0;

    if (NewTerminalContext->DataBits == DataBitsList[Index].Value) {
      NewTerminalContext->DataBitsIndex         = Index;
      CallbackData->BmmFakeNvData.COMDataRate  = NewTerminalContext->DataBitsIndex;
      CheckFlags |= EFI_IFR_OPTION_DEFAULT;
    }

    IfrOptionList[Index].Flags        = CheckFlags;
    IfrOptionList[Index].StringToken  = DataBitsList[Index].StringToken;
    IfrOptionList[Index].Value.u8     = Index;
  }

  CreateOneOfOpCode (
    (EFI_QUESTION_ID) COM_DATA_RATE_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    COM_DATA_RATE_VAR_OFFSET,
    STRING_TOKEN (STR_COM_DATA_BITS),
    STRING_TOKEN (STR_COM_DATA_BITS),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    IfrOptionList,
    4,
    &gUpdateData
    );

  for (Index = 0; Index < sizeof (ParityList) / sizeof (ParityList[0]); Index++) {
    CheckFlags = 0;
    if (NewTerminalContext->Parity == ParityList[Index].Value) {
      CheckFlags |= EFI_IFR_OPTION_DEFAULT;
      NewTerminalContext->ParityIndex         = (UINT8) Index;
      CallbackData->BmmFakeNvData.COMParity  = NewTerminalContext->ParityIndex;
    }

    IfrOptionList[Index].Flags        = CheckFlags;
    IfrOptionList[Index].StringToken  = ParityList[Index].StringToken;
    IfrOptionList[Index].Value.u8     = Index;
  }

  CreateOneOfOpCode (
    (EFI_QUESTION_ID) COM_PARITY_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    COM_PARITY_VAR_OFFSET,
    STRING_TOKEN (STR_COM_PARITY),
    STRING_TOKEN (STR_COM_PARITY),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    IfrOptionList,
    5,
    &gUpdateData
    );

  for (Index = 0; Index < sizeof (StopBitsList) / sizeof (StopBitsList[0]); Index++) {
    CheckFlags = 0;
    if (NewTerminalContext->StopBits == StopBitsList[Index].Value) {
      CheckFlags |= EFI_IFR_OPTION_DEFAULT;
      NewTerminalContext->StopBitsIndex         = (UINT8) Index;
      CallbackData->BmmFakeNvData.COMStopBits  = NewTerminalContext->StopBitsIndex;
    }

    IfrOptionList[Index].Flags        = CheckFlags;
    IfrOptionList[Index].StringToken  = StopBitsList[Index].StringToken;
    IfrOptionList[Index].Value.u8     = Index;
  }

  CreateOneOfOpCode (
    (EFI_QUESTION_ID) COM_STOP_BITS_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    COM_STOP_BITS_VAR_OFFSET,
    STRING_TOKEN (STR_COM_STOP_BITS),
    STRING_TOKEN (STR_COM_STOP_BITS),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    IfrOptionList,
    3,
    &gUpdateData
    );

  for (Index = 0; Index < 4; Index++) {
    CheckFlags = 0;
    if (NewTerminalContext->TerminalType == Index) {
      CheckFlags |= EFI_IFR_OPTION_DEFAULT;
      CallbackData->BmmFakeNvData.COMTerminalType = NewTerminalContext->TerminalType;
    }

    IfrOptionList[Index].Flags        = CheckFlags;
    IfrOptionList[Index].StringToken  = (EFI_STRING_ID) TerminalType[Index];
    IfrOptionList[Index].Value.u8     = Index;
  }

  CreateOneOfOpCode (
    (EFI_QUESTION_ID) COM_TERMINAL_QUESTION_ID,
    VARSTORE_ID_BOOT_MAINT,
    COM_TERMINAL_VAR_OFFSET,
    STRING_TOKEN (STR_COM_TERMI_TYPE),
    STRING_TOKEN (STR_COM_TERMI_TYPE),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    IfrOptionList,
    4,
    &gUpdateData
    );

  SafeFreePool (IfrOptionList);

  UpdatePageEnd (CallbackData);
}

/**
  Dispatch the correct update page function to call based on
  the UpdatePageId.

  @param UpdatePageId    The form ID.
  @param CallbackData    The BMM context data.

  

**/
VOID
UpdatePageBody (
  IN UINT16                           UpdatePageId,
  IN BMM_CALLBACK_DATA                *CallbackData
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
  Get the index number (#### in Boot####) for the boot option pointed to a BBS legacy device type
  specified by DeviceType.

  @param DeviceType      The legacy device type. It can be floppy, network, harddisk, cdrom,
                         etc.
  @param OptionIndex     Returns the index number (#### in Boot####).
  @param OptionSize      Return the size of the Boot### variable.

  

**/
VOID *
GetLegacyBootOptionVar (
  IN  UINTN                            DeviceType,
  OUT UINTN                            *OptionIndex,
  OUT UINTN                            *OptionSize
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  VOID                      *OptionBuffer;
  UINTN                     OrderSize;
  UINTN                     Index;
  UINT16                    *OrderBuffer;
  CHAR16                    StrTemp[100];
  UINT16                    FilePathSize;
  UINT8                     *Ptr;
  UINT8                     *OptionalData;

  //
  // Get Boot Option number from the size of BootOrder
  //
  OrderBuffer = BdsLibGetVariableAndSize (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  &OrderSize
                  );

  for (Index = 0; Index < OrderSize / sizeof (UINT16); Index++) {
    UnicodeSPrint (StrTemp, 100, L"Boot%04x", OrderBuffer[Index]);
    OptionBuffer = BdsLibGetVariableAndSize (
                    StrTemp,
                    &gEfiGlobalVariableGuid,
                    OptionSize
                    );
    if (NULL == OptionBuffer) {
      continue;
    }

    Ptr       = (UINT8 *) OptionBuffer;
    Ptr += sizeof (UINT32);

    FilePathSize = *(UINT16 *) Ptr;
    Ptr += sizeof (UINT16);

    Ptr += StrSize ((CHAR16 *) Ptr);

    //
    // Now Ptr point to Device Path
    //
    DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
    Ptr += FilePathSize;

    //
    // Now Ptr point to Optional Data
    //
    OptionalData = Ptr;

    if ((DeviceType == ((BBS_TABLE *) OptionalData)->DeviceType) &&
        (BBS_DEVICE_PATH == DevicePath->Type) &&
        (BBS_BBS_DP == DevicePath->SubType)
        ) {
      *OptionIndex = OrderBuffer[Index];
      SafeFreePool (OrderBuffer);
      return OptionBuffer;
    } else {
      SafeFreePool (OptionBuffer);
    }
  }

  SafeFreePool (OrderBuffer);
  return NULL;
}

/**
  Create a dynamic page so that Legacy Device boot order
  can be set for specified device type.

  @param UpdatePageId    The form ID. It also spefies the legacy device type.
  @param CallbackData    The BMM context data.

  

**/
VOID
UpdateSetLegacyDeviceOrderPage (
  IN UINT16                           UpdatePageId,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_LEGACY_DEV_ORDER_CONTEXT *DevOrder;
  BM_MENU_OPTION              *OptionMenu;
  BM_MENU_ENTRY               *NewMenuEntry;
  IFR_OPTION                  *IfrOptionList;
  EFI_STRING_ID               StrRef;
  EFI_STRING_ID               StrRefHelp;
  BBS_TYPE                    BbsType;
  UINTN                       VarSize;
  UINTN                       Pos;
  UINTN                       Bit;
  UINT16                      Index;
  UINT16                      Key;
  CHAR16                      String[100];
  CHAR16                      *TypeStr;
  CHAR16                      *TypeStrHelp;
  UINT16                      VarDevOrder;
  UINT8                       *VarData;
  UINT8                       *LegacyOrder;
  UINT8                       *OldData;
  UINT8                       *DisMap;

  OptionMenu = NULL;
  Key = 0;
  StrRef = 0;
  StrRefHelp = 0;
  TypeStr = NULL;
  TypeStrHelp = NULL;
  BbsType = BBS_FLOPPY;
  LegacyOrder = NULL;
  OldData = NULL;
  DisMap = NULL;

  CallbackData->BmmAskSaveOrNot = TRUE;
  UpdatePageStart (CallbackData);

  DisMap = CallbackData->BmmOldFakeNVData.DisableMap;

  SetMem (DisMap, 32, 0);
  //
  // Create oneof option list
  //
  switch (UpdatePageId) {
  case FORM_SET_FD_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyFDMenu;
    Key         = (UINT16) LEGACY_FD_QUESTION_ID;
    TypeStr     = STR_FLOPPY;
    TypeStrHelp = STR_FLOPPY_HELP;
    BbsType     = BBS_FLOPPY;
    LegacyOrder = CallbackData->BmmFakeNvData.LegacyFD;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyFD;
    break;

  case FORM_SET_HD_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyHDMenu;
    Key         = (UINT16) LEGACY_HD_QUESTION_ID;
    TypeStr     = STR_HARDDISK;
    TypeStrHelp = STR_HARDDISK_HELP;
    BbsType     = BBS_HARDDISK;
    LegacyOrder = CallbackData->BmmFakeNvData.LegacyHD;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyHD;
    break;

  case FORM_SET_CD_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyCDMenu;
    Key         = (UINT16) LEGACY_CD_QUESTION_ID;
    TypeStr     = STR_CDROM;
    TypeStrHelp = STR_CDROM_HELP;
    BbsType     = BBS_CDROM;
    LegacyOrder = CallbackData->BmmFakeNvData.LegacyCD;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyCD;
    break;

  case FORM_SET_NET_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyNETMenu;
    Key         = (UINT16) LEGACY_NET_QUESTION_ID;
    TypeStr     = STR_NET;
    TypeStrHelp = STR_NET_HELP;
    BbsType     = BBS_EMBED_NETWORK;
    LegacyOrder = CallbackData->BmmFakeNvData.LegacyNET;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyNET;
    break;

  case FORM_SET_BEV_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyBEVMenu;
    Key         = (UINT16) LEGACY_BEV_QUESTION_ID;
    TypeStr     = STR_BEV;
    TypeStrHelp = STR_BEV_HELP;
    BbsType     = BBS_BEV_DEVICE;
    LegacyOrder = CallbackData->BmmFakeNvData.LegacyBEV;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyBEV;
    break;

  }

  CreateMenuStringToken (CallbackData, CallbackData->BmmHiiHandle, OptionMenu);

  IfrOptionList = AllocateZeroPool (sizeof (IFR_OPTION) * (OptionMenu->MenuNumber + 1));
  if (NULL == IfrOptionList) {
    return ;
  }

  for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
    NewMenuEntry                = BOpt_GetMenuEntry (OptionMenu, Index);
    IfrOptionList[Index].Flags  = 0;
    if (0 == Index) {
      IfrOptionList[Index].Flags |= EFI_IFR_OPTION_DEFAULT;
    }

    IfrOptionList[Index].StringToken  = NewMenuEntry->DisplayStringToken;
    IfrOptionList[Index].Value.u8     = (UINT8) ((BM_LEGACY_DEVICE_CONTEXT *) NewMenuEntry->VariableContext)->Index;
  }
  //
  // for item "Disabled"
  //
  IfrOptionList[Index].Flags        = 0;
  IfrOptionList[Index].StringToken  = STRING_TOKEN (STR_DISABLE_LEGACY_DEVICE);
  IfrOptionList[Index].Value.u8     = 0xFF;

  //
  // Get Device Order from variable
  //
  VarData = BdsLibGetVariableAndSize (
              VAR_LEGACY_DEV_ORDER,
              &EfiLegacyDevOrderGuid,
              &VarSize
              );

  if (NULL != VarData) {
    DevOrder    = (BM_LEGACY_DEV_ORDER_CONTEXT *) VarData;
    while (VarData < VarData + VarSize) {
      if (DevOrder->BbsType == BbsType) {
        break;
      }

      VarData += sizeof (BBS_TYPE);
      VarData += *(UINT16 *) VarData;
      DevOrder = (BM_LEGACY_DEV_ORDER_CONTEXT *) VarData;
    }
    //
    // Create oneof tag here for FD/HD/CD #1 #2
    //
    for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
      //
      // Create the string for oneof tag
      //
      UnicodeSPrint (String, sizeof (String), TypeStr, Index);
      StrRef = 0;
      HiiLibNewString (CallbackData->BmmHiiHandle, &StrRef, String);

      UnicodeSPrint (String, sizeof (String), TypeStrHelp, Index);
      StrRefHelp = 0;
      HiiLibNewString (CallbackData->BmmHiiHandle, &StrRefHelp, String);

      CreateOneOfOpCode (
        (EFI_QUESTION_ID) (Key + Index),
        VARSTORE_ID_BOOT_MAINT,
        (UINT16) (Key + Index - CONFIG_OPTION_OFFSET),
        StrRef,
        StrRefHelp,
        EFI_IFR_FLAG_CALLBACK,
        EFI_IFR_NUMERIC_SIZE_1,
        IfrOptionList,
        OptionMenu->MenuNumber + 1,
        &gUpdateData
        );

      VarDevOrder = *(UINT16 *) ((UINT8 *) DevOrder + sizeof (BBS_TYPE) + sizeof (UINT16) + Index * sizeof (UINT16));

      if (0xFF00 == (VarDevOrder & 0xFF00)) {
        LegacyOrder[Index]  = 0xFF;
        Pos                 = (VarDevOrder & 0xFF) / 8;
        Bit                 = 7 - ((VarDevOrder & 0xFF) % 8);
        DisMap[Pos] = (UINT8) (DisMap[Pos] | (UINT8) (1 << Bit));
      } else {
        LegacyOrder[Index] = (UINT8) (VarDevOrder & 0xFF);
      }
    }
  }

  CopyMem (OldData, LegacyOrder, 100);

  if (IfrOptionList != NULL) {
    SafeFreePool (IfrOptionList);
    IfrOptionList = NULL;
  }

  UpdatePageEnd (CallbackData);
}

/**
  Dispatch the display to the next page based on NewPageId.

  @param Private         The BMM context data.
  @param NewPageId       The original page ID.

  

**/
VOID
UpdatePageId (
  BMM_CALLBACK_DATA              *Private,
  UINT16                         NewPageId
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
    Private->BmmPreviousPageId  = Private->BmmCurrentPageId;
    Private->BmmCurrentPageId   = NewPageId;
  }
}
