/**@file
  Dynamically Update the pages.
  
Copyright (c) 2006 - 2007 Intel Corporation.
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Generic/Bds.h"
#include "BootMaint.h"
#include "BdsPlatform.h"

EFI_GUID gTerminalDriverGuid = {
  0x9E863906, 0xA40F, 0x4875, {0x97, 0x7F, 0x5B, 0x93, 0xFF, 0x23, 0x7F, 0xC6}
};

VOID
RefreshUpdateData (
  IN BOOLEAN                      FormSetUpdate,
  IN EFI_PHYSICAL_ADDRESS         FormCallbackHandle,
  IN BOOLEAN                      FormUpdate,
  IN STRING_REF                   FormTitle,
  IN UINT16                       DataCount
  )
/*++
Routine Description:
  Refresh the global UpdateData structure.

Arguments:
  FormSetUpdate      - If TRUE, next variable is significant
  FormCallbackHandle - If not 0, will update FormSet with this info
  FormUpdate         - If TRUE, next variable is significant
  FormTitle          - If not 0, will update Form with this info
  DataCount          - The number of Data entries in this structure

Returns:
  None.
--*/
{
  UpdateData->FormSetUpdate = FormSetUpdate;
  if (FormSetUpdate) {
    ASSERT (0 != FormCallbackHandle);
    UpdateData->FormCallbackHandle = FormCallbackHandle;
  }

  UpdateData->FormUpdate  = FALSE;
  UpdateData->FormTitle   = FormTitle;
  UpdateData->DataCount   = DataCount;
}

VOID
UpdatePageStart (
  IN BMM_CALLBACK_DATA                *CallbackData,
  IN OUT UINT8                        **CurrentLocation
  )
{
  RefreshUpdateData (TRUE, (EFI_PHYSICAL_ADDRESS) (UINTN) CallbackData->BmmCallbackHandle, FALSE, 0, 0);

  if (!(CallbackData->BmmAskSaveOrNot)) {
    //
    // Add a "Go back to main page" tag in front of the form when there are no
    // "Apply changes" and "Discard changes" tags in the end of the form.
    //
    CreateGotoOpCode (
      FORM_MAIN_ID,
      STRING_TOKEN (STR_FORM_GOTO_MAIN),
      STRING_TOKEN (STR_FORM_GOTO_MAIN),
      FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
      FORM_MAIN_ID,
      *CurrentLocation
      );

    UpdateData->DataCount++;

    *CurrentLocation = *CurrentLocation + ((FRAMEWORK_EFI_IFR_OP_HEADER *) (*CurrentLocation))->Length;
  }

}

VOID
UpdatePageEnd (
  IN BMM_CALLBACK_DATA                *CallbackData,
  IN UINT8                            *CurrentLocation
  )
{
  //
  // Create the "Apply changes" and "Discard changes" tags.
  //
  if (CallbackData->BmmAskSaveOrNot) {
    CreateGotoOpCode (
      FORM_MAIN_ID,
      STRING_TOKEN (STR_SAVE_AND_EXIT),
      STRING_TOKEN (STR_NULL_STRING),
      FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
      KEY_VALUE_SAVE_AND_EXIT,
      CurrentLocation
      );

    UpdateData->DataCount++;

    CurrentLocation = CurrentLocation + ((FRAMEWORK_EFI_IFR_OP_HEADER *) CurrentLocation)->Length;

    CreateGotoOpCode (
      FORM_MAIN_ID,
      STRING_TOKEN (STR_NO_SAVE_AND_EXIT),
      STRING_TOKEN (STR_NULL_STRING),
      FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
      KEY_VALUE_NO_SAVE_AND_EXIT,
      CurrentLocation
      );

    UpdateData->DataCount++;
  }
  //
  // Ensure user can return to the main page.
  //
  if (0 == UpdateData->DataCount) {
    CreateGotoOpCode (
      FORM_MAIN_ID,
      STRING_TOKEN (STR_NO_SAVE_AND_EXIT),
      STRING_TOKEN (STR_NULL_STRING),
      FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
      KEY_VALUE_NO_SAVE_AND_EXIT,
      CurrentLocation
      );

    UpdateData->DataCount++;
  }

  CallbackData->Hii->UpdateForm (
                      CallbackData->Hii,
                      CallbackData->BmmHiiHandle,
                      CallbackData->BmmCurrentPageId,
                      TRUE,
                      UpdateData
                      );
}

VOID
CleanUpPage (
  IN EFI_FORM_LABEL                   LabelId,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  RefreshUpdateData (FALSE, 0, FALSE, 0, 0xff);

  //
  // Remove all op-codes from dynamic page
  //
  CallbackData->Hii->UpdateForm (
                      CallbackData->Hii,
                      CallbackData->BmmHiiHandle,
                      LabelId,
                      FALSE,
                      UpdateData
                      );
}

EFI_STATUS
BootThisFile (
  IN BM_FILE_CONTEXT                   *FileContext
  )
{
  EFI_STATUS        Status;
  UINTN             ExitDataSize;
  CHAR16            *ExitData;
  BDS_COMMON_OPTION *Option;

  Option                  = AllocatePool (sizeof (BDS_COMMON_OPTION));

  Option->Description     = FileContext->FileName;
  Option->DevicePath      = FileContext->DevicePath;
  Option->LoadOptionsSize = 0;
  Option->LoadOptions     = NULL;

  //
  // Since current no boot from removable media directly is allowed */
  //
  gST->ConOut->ClearScreen (gST->ConOut);

  ExitDataSize  = 0;

  Status        = BdsLibBootViaBootOption (Option, Option->DevicePath, &ExitDataSize, &ExitData);

  return Status;

}

VOID
UpdateConCOMPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINT16        Index;
  UINT8         *Location;
  EFI_STATUS    Status;
  VOID        	*Interface;

  Location                      = (UINT8 *) &UpdateData->Data;
  CallbackData->BmmAskSaveOrNot = FALSE;

  UpdatePageStart (CallbackData, &Location);

  Status = EfiLibLocateProtocol (&gTerminalDriverGuid, &Interface);
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
      NewMenuEntry = BOpt_GetMenuEntry (&TerminalMenu, Index);

      CreateGotoOpCode (
        FORM_CON_COM_SETUP_ID,
        NewMenuEntry->DisplayStringToken,
        STRING_TOKEN (STR_NULL_STRING),
        FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
        (UINT16) (TERMINAL_OPTION_OFFSET + Index),
        Location
        );
      Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
      UpdateData->DataCount++;
    }
  }

  UpdatePageEnd (CallbackData, Location);
}

VOID
UpdateBootDelPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  UINT16          Index;
  UINT8           *Location;

  Location                      = (UINT8 *) &UpdateData->Data;
  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData, &Location);
  CreateMenuStringToken (CallbackData, CallbackData->BmmHiiHandle, &BootOptionMenu);

  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry    = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    NewLoadContext  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    if (NewLoadContext->IsLegacy) {
      continue;
    }

    NewLoadContext->Deleted = FALSE;
    CallbackData->BmmFakeNvData->BootOptionDel[Index] = 0x00;

    CreateCheckBoxOpCode (
      (UINT16) (BOOT_OPTION_DEL_QUESTION_ID + Index),
      (UINT8) 1,
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
      (UINT16) BOOT_OPTION_DEL_QUESTION_ID,
      Location
      );

    Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
    UpdateData->DataCount++;
  }

  UpdatePageEnd (CallbackData, Location);
}

VOID
UpdateDrvAddHandlePage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINT16        Index;
  UINT8         *Location;

  Location                      = (UINT8 *) &UpdateData->Data;
  CallbackData->BmmAskSaveOrNot = FALSE;

  UpdatePageStart (CallbackData, &Location);

  for (Index = 0; Index < DriverMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&DriverMenu, Index);

    CreateGotoOpCode (
      FORM_DRV_ADD_HANDLE_DESC_ID,
      NewMenuEntry->DisplayStringToken,
      STRING_TOKEN (STR_NULL_STRING),
      FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
      (UINT16) (HANDLE_OPTION_OFFSET + Index),
      Location
      );

    Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
    UpdateData->DataCount++;
  }

  UpdatePageEnd (CallbackData, Location);
}

VOID
UpdateDrvDelPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  UINT16          Index;
  UINT8           *Location;

  Location                      = (UINT8 *) &UpdateData->Data;
  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData, &Location);

  CreateMenuStringToken (CallbackData, CallbackData->BmmHiiHandle, &DriverOptionMenu);

  for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
    NewMenuEntry            = BOpt_GetMenuEntry (&DriverOptionMenu, Index);

    NewLoadContext          = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    NewLoadContext->Deleted = FALSE;
    CallbackData->BmmFakeNvData->DriverOptionDel[Index] = 0x00;

    CreateCheckBoxOpCode (
      (UINT16) (DRIVER_OPTION_DEL_QUESTION_ID + Index),
      (UINT8) 1,
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
      (UINT16) DRIVER_OPTION_DEL_QUESTION_ID,
      Location
      );

    Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
    UpdateData->DataCount++;
  }

  UpdatePageEnd (CallbackData, Location);
}

VOID
UpdateDriverAddHandleDescPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINT8         *Location;

  Location  = (UINT8 *) &UpdateData->Data;
  CallbackData->BmmFakeNvData->DriverAddActive          = 0x01;
  CallbackData->BmmFakeNvData->DriverAddForceReconnect  = 0x00;
  CallbackData->BmmAskSaveOrNot                         = TRUE;
  NewMenuEntry = CallbackData->MenuEntry;

  UpdatePageStart (CallbackData, &Location);

  UpdateData->DataCount += (UINT16) 4;

  CreateSubTitleOpCode (
    NewMenuEntry->DisplayStringToken,
    Location
    );

  Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;

  CreateStringOpCode (
    DRV_ADD_HANDLE_DESC_QUESTION_ID,
    (UINT8) 150,
    STRING_TOKEN (STR_LOAD_OPTION_DESC),
    STRING_TOKEN (STR_NULL_STRING),
    6,
    75,
    FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
    KEY_VALUE_DRIVER_ADD_DESC_DATA,
    Location
    );

  Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;

  CreateCheckBoxOpCode (
    DRV_ADD_RECON_QUESTION_ID,
    (UINT8) 1,
    STRING_TOKEN (STR_LOAD_OPTION_FORCE_RECON),
    STRING_TOKEN (STR_LOAD_OPTION_FORCE_RECON),
    FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
    DRV_ADD_RECON_QUESTION_ID,
    Location
    );
  Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;

  CreateStringOpCode (
    DRIVER_ADD_OPTION_QUESTION_ID,
    (UINT8) 150,
    STRING_TOKEN (STR_OPTIONAL_DATA),
    STRING_TOKEN (STR_NULL_STRING),
    6,
    75,
    FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
    KEY_VALUE_DRIVER_ADD_OPT_DATA,
    Location
    );

  Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  UpdatePageEnd (CallbackData, Location);
}

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
  UINT8               *Location;
  UINT8               CheckFlags;
  EFI_STATUS          Status;
  VOID        	      *Interface;

  Location                      = (UINT8 *) &UpdateData->Data;
  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData, &Location);

  for (Index = 0; Index < ConsoleMenu->MenuNumber; Index++) {
    NewMenuEntry      = BOpt_GetMenuEntry (ConsoleMenu, Index);
    NewConsoleContext = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
    CheckFlags        = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
    if (NewConsoleContext->IsActive) {
      CheckFlags |= FRAMEWORK_EFI_IFR_FLAG_DEFAULT;
      CallbackData->BmmFakeNvData->ConsoleCheck[Index] = TRUE;
    } else {
      CallbackData->BmmFakeNvData->ConsoleCheck[Index] = FALSE;
    }

    CreateCheckBoxOpCode (
      (UINT16) (CON_DEVICE_QUESTION_ID + Index),
      (UINT8) 1,
      NewMenuEntry->DisplayStringToken,
      NewMenuEntry->HelpStringToken,
      CheckFlags,
      (UINT16) (CONSOLE_OPTION_OFFSET + Index),
      Location
      );
    Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
    UpdateData->DataCount++;
  }

  Status = EfiLibLocateProtocol (&gTerminalDriverGuid, &Interface);
  if (!EFI_ERROR (Status)) {
    for (Index2 = 0; Index2 < TerminalMenu.MenuNumber; Index2++) {
      CheckFlags          = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
      NewMenuEntry        = BOpt_GetMenuEntry (&TerminalMenu, Index2);
      NewTerminalContext  = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;

      if ((NewTerminalContext->IsConIn && (UpdatePageId == FORM_CON_IN_ID)) ||
          (NewTerminalContext->IsConOut && (UpdatePageId == FORM_CON_OUT_ID)) ||
          (NewTerminalContext->IsStdErr && (UpdatePageId == FORM_CON_ERR_ID))
          ) {
        CheckFlags |= FRAMEWORK_EFI_IFR_FLAG_DEFAULT;
        CallbackData->BmmFakeNvData->ConsoleCheck[Index] = TRUE;
      } else {
        CallbackData->BmmFakeNvData->ConsoleCheck[Index] = FALSE;
      }

      CreateCheckBoxOpCode (
        (UINT16) (CON_DEVICE_QUESTION_ID + Index),
        (UINT8) 1,
        NewMenuEntry->DisplayStringToken,
        NewMenuEntry->HelpStringToken,
        CheckFlags,
        (UINT16) (CONSOLE_OPTION_OFFSET + Index),
        Location
        );
      Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
      UpdateData->DataCount++;
      Index++;
    }
  }

  UpdatePageEnd (CallbackData, Location);
}

VOID
UpdateOrderPage (
  IN UINT16                           UpdatePageId,
  IN BM_MENU_OPTION                   *OptionMenu,
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINT16        Index;
  UINT8         *Location;
  IFR_OPTION    *IfrOptionList;

  Location                      = (UINT8 *) &UpdateData->Data;
  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData, &Location);

  CreateMenuStringToken (CallbackData, CallbackData->BmmHiiHandle, OptionMenu);

  ZeroMem (CallbackData->BmmFakeNvData->OptionOrder, 100);

  IfrOptionList = AllocateZeroPool (sizeof (IFR_OPTION) * OptionMenu->MenuNumber);
  if (NULL == IfrOptionList) {
    return ;
  }

  for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (OptionMenu, Index);
    IfrOptionList[Index].StringToken = NewMenuEntry->DisplayStringToken;
    IfrOptionList[Index].Value = (UINT16) (NewMenuEntry->OptionNumber + 1);
    IfrOptionList[Index].OptionString = NULL;
    CallbackData->BmmFakeNvData->OptionOrder[Index] = (UINT8) (IfrOptionList[Index].Value);
  }

  if (OptionMenu->MenuNumber > 0) {
    CreateOrderedListOpCode (
      (UINT16) OPTION_ORDER_QUESTION_ID,
      (UINT8) 100,
      STRING_TOKEN (STR_CHANGE_ORDER),
      STRING_TOKEN (STR_CHANGE_ORDER),
      IfrOptionList,
      OptionMenu->MenuNumber,
      Location
      );

    for (Index = 0; Index < OptionMenu->MenuNumber + 2; Index++) {
      Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
    }

    UpdateData->DataCount = (UINT16) (UpdateData->DataCount + OptionMenu->MenuNumber + 2);
  }

  SafeFreePool (IfrOptionList);

  UpdatePageEnd (CallbackData, Location);

  CopyMem (
    CallbackData->BmmOldFakeNVData.OptionOrder,
    CallbackData->BmmFakeNvData->OptionOrder,
    100
    );
}

VOID
UpdateBootNextPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  UINT8           *Location;
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  IFR_OPTION      *IfrOptionList;
  UINTN           NumberOfOptions;
  UINT16          Index;

  Location                      = (UINT8 *) &UpdateData->Data;
  IfrOptionList                 = NULL;
  NumberOfOptions               = BootOptionMenu.MenuNumber;
  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData, &Location);
  CreateMenuStringToken (CallbackData, CallbackData->BmmHiiHandle, &BootOptionMenu);

  if (NumberOfOptions > 0) {
    UpdateData->DataCount = (UINT8) (UpdateData->DataCount + NumberOfOptions);
    IfrOptionList = AllocateZeroPool ((NumberOfOptions + 1) * sizeof (IFR_OPTION));

    ASSERT (IfrOptionList);

    CallbackData->BmmFakeNvData->BootNext = (UINT16) (BootOptionMenu.MenuNumber);

    for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
      NewMenuEntry    = BOpt_GetMenuEntry (&BootOptionMenu, Index);
      NewLoadContext  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
      if (NewLoadContext->IsBootNext) {
        IfrOptionList[Index].Flags            = FRAMEWORK_EFI_IFR_FLAG_DEFAULT | FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
        CallbackData->BmmFakeNvData->BootNext = Index;
      } else {
        IfrOptionList[Index].Flags = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
      }

      IfrOptionList[Index].Key          = (UINT16) KEY_VALUE_MAIN_BOOT_NEXT;
      IfrOptionList[Index].Value        = Index;
      IfrOptionList[Index].StringToken  = NewMenuEntry->DisplayStringToken;
      IfrOptionList[Index].OptionString = NULL;
    }

    IfrOptionList[Index].Key          = (UINT16) KEY_VALUE_MAIN_BOOT_NEXT;
    IfrOptionList[Index].Value        = Index;
    IfrOptionList[Index].StringToken  = STRING_TOKEN (STR_NONE);
    IfrOptionList[Index].Flags        = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
    if (CallbackData->BmmFakeNvData->BootNext == Index) {
      IfrOptionList[Index].Flags |= FRAMEWORK_EFI_IFR_FLAG_DEFAULT;
    }

    IfrOptionList[Index].OptionString = NULL;

    CreateOneOfOpCode (
      (UINT16) BOOT_NEXT_QUESTION_ID,
      (UINT8) 2,
      STRING_TOKEN (STR_BOOT_NEXT),
      STRING_TOKEN (STR_BOOT_NEXT_HELP),
      IfrOptionList,
      (UINTN) (NumberOfOptions + 1),
      Location
      );
    Location  = Location + (NumberOfOptions + 2) * ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
    Location  = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;

    UpdateData->DataCount += 3;
    SafeFreePool (IfrOptionList);
    IfrOptionList = NULL;
  }

  UpdatePageEnd (CallbackData, Location);
}

VOID
UpdateTimeOutPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  UINT8   *Location;
  UINT16  BootTimeOut;

  Location                      = (UINT8 *) &UpdateData->Data;
  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData, &Location);

  BootTimeOut = BdsLibGetTimeout ();

  CreateNumericOpCode (
    (UINT16) BOOT_TIME_OUT_QUESTION_ID,
    (UINT8) 2,
    STRING_TOKEN (STR_NUM_AUTO_BOOT),
    STRING_TOKEN (STR_HLP_AUTO_BOOT),
    0,
    65535,
    0,
    10,
    0,
    0,
    Location
    );

  CallbackData->BmmFakeNvData->BootTimeOut = (UINT16) BootTimeOut;
  UpdateData->DataCount++;
  Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;

  UpdatePageEnd (CallbackData, Location);
}

VOID
UpdateTerminalPage (
  IN BMM_CALLBACK_DATA                *CallbackData
  )
{
  UINT16              Index;
  UINT8               *Location;
  UINT8               CheckFlags;
  IFR_OPTION          *IfrOptionList;
  BM_MENU_ENTRY       *NewMenuEntry;
  BM_TERMINAL_CONTEXT *NewTerminalContext;

  ZeroMem (UpdateData, UPDATE_DATA_SIZE);
  Location = (UINT8 *) &UpdateData->Data;
  UpdatePageStart (CallbackData, &Location);

  NewMenuEntry = BOpt_GetMenuEntry (
                  &TerminalMenu,
                  CallbackData->CurrentTerminal
                  );

  if (!NewMenuEntry) {
    return ;
  }

  NewTerminalContext  = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;

  IfrOptionList       = AllocateZeroPool (sizeof (IFR_OPTION) * 19);
  if (!IfrOptionList) {
    return ;
  }

  for (Index = 0; Index < 19; Index++) {
    CheckFlags = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
    if (NewTerminalContext->BaudRate == (UINT64) (BaudRateList[Index].Value)) {
      CheckFlags |= FRAMEWORK_EFI_IFR_FLAG_DEFAULT;
      NewTerminalContext->BaudRateIndex         = (UINT8) Index;
      CallbackData->BmmFakeNvData->COMBaudRate  = NewTerminalContext->BaudRateIndex;
    }

    IfrOptionList[Index].Flags        = CheckFlags;
    IfrOptionList[Index].Key          = KEY_VALUE_COM_SET_BAUD_RATE;
    IfrOptionList[Index].StringToken  = BaudRateList[Index].StringToken;
    IfrOptionList[Index].Value        = Index;
  }

  CreateOneOfOpCode (
    (UINT16) COM_BAUD_RATE_QUESTION_ID,
    (UINT8) 1,
    STRING_TOKEN (STR_COM_BAUD_RATE),
    STRING_TOKEN (STR_COM_BAUD_RATE),
    IfrOptionList,
    19,
    Location
    );

  Location              = Location + (Index + 1) * ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  Location              = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  UpdateData->DataCount = (UINT8) (UpdateData->DataCount + Index);
  UpdateData->DataCount += 2;

  SafeFreePool (IfrOptionList);

  IfrOptionList = AllocateZeroPool (sizeof (IFR_OPTION) * 4);
  if (!IfrOptionList) {
    return ;
  }

  for (Index = 0; Index < 4; Index++) {
    CheckFlags = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;

    if (NewTerminalContext->DataBits == DataBitsList[Index].Value) {
      NewTerminalContext->DataBitsIndex         = (UINT8) Index;
      CallbackData->BmmFakeNvData->COMDataRate  = NewTerminalContext->DataBitsIndex;
      CheckFlags |= FRAMEWORK_EFI_IFR_FLAG_DEFAULT;
    }

    IfrOptionList[Index].Flags        = CheckFlags;
    IfrOptionList[Index].Key          = KEY_VALUE_COM_SET_DATA_BITS;
    IfrOptionList[Index].StringToken  = DataBitsList[Index].StringToken;
    IfrOptionList[Index].Value        = Index;
  }

  CreateOneOfOpCode (
    (UINT16) COM_DATA_RATE_QUESTION_ID,
    (UINT8) 1,
    STRING_TOKEN (STR_COM_DATA_BITS),
    STRING_TOKEN (STR_COM_DATA_BITS),
    IfrOptionList,
    4,
    Location
    );

  Location              = Location + (Index + 1) * ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  Location              = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  UpdateData->DataCount = (UINT8) (UpdateData->DataCount + Index);
  UpdateData->DataCount += 2;

  SafeFreePool (IfrOptionList);

  IfrOptionList = AllocateZeroPool (sizeof (IFR_OPTION) * 5);
  if (!IfrOptionList) {
    return ;
  }

  for (Index = 0; Index < 5; Index++) {
    CheckFlags = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
    if (NewTerminalContext->Parity == ParityList[Index].Value) {
      CheckFlags |= FRAMEWORK_EFI_IFR_FLAG_DEFAULT;
      NewTerminalContext->ParityIndex         = (UINT8) Index;
      CallbackData->BmmFakeNvData->COMParity  = NewTerminalContext->ParityIndex;
    }

    IfrOptionList[Index].Flags        = CheckFlags;
    IfrOptionList[Index].Key          = KEY_VALUE_COM_SET_PARITY;
    IfrOptionList[Index].StringToken  = ParityList[Index].StringToken;
    IfrOptionList[Index].Value        = Index;
  }

  CreateOneOfOpCode (
    (UINT16) COM_PARITY_QUESTION_ID,
    (UINT8) 1,
    STRING_TOKEN (STR_COM_PARITY),
    STRING_TOKEN (STR_COM_PARITY),
    IfrOptionList,
    5,
    Location
    );

  Location              = Location + (Index + 1) * ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  Location              = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  UpdateData->DataCount = (UINT8) (UpdateData->DataCount + Index);
  UpdateData->DataCount += 2;

  SafeFreePool (IfrOptionList);

  IfrOptionList = AllocateZeroPool (sizeof (IFR_OPTION) * 3);
  if (!IfrOptionList) {
    return ;
  }

  for (Index = 0; Index < 3; Index++) {
    CheckFlags = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
    if (NewTerminalContext->StopBits == StopBitsList[Index].Value) {
      CheckFlags |= FRAMEWORK_EFI_IFR_FLAG_DEFAULT;
      NewTerminalContext->StopBitsIndex         = (UINT8) Index;
      CallbackData->BmmFakeNvData->COMStopBits  = NewTerminalContext->StopBitsIndex;
    }

    IfrOptionList[Index].Flags        = CheckFlags;
    IfrOptionList[Index].Key          = KEY_VALUE_COM_SET_STOP_BITS;
    IfrOptionList[Index].StringToken  = StopBitsList[Index].StringToken;
    IfrOptionList[Index].Value        = Index;
  }

  CreateOneOfOpCode (
    (UINT16) COM_STOP_BITS_QUESTION_ID,
    (UINT8) 1,
    STRING_TOKEN (STR_COM_STOP_BITS),
    STRING_TOKEN (STR_COM_STOP_BITS),
    IfrOptionList,
    3,
    Location
    );

  Location              = Location + (Index + 1) * ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  Location              = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  UpdateData->DataCount = (UINT8) (UpdateData->DataCount + Index);
  UpdateData->DataCount += 2;

  SafeFreePool (IfrOptionList);

  IfrOptionList = AllocateZeroPool (sizeof (IFR_OPTION) * 4);
  if (!IfrOptionList) {
    return ;
  }

  for (Index = 0; Index < 4; Index++) {
    CheckFlags = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
    if (NewTerminalContext->TerminalType == Index) {
      CheckFlags |= FRAMEWORK_EFI_IFR_FLAG_DEFAULT;
      CallbackData->BmmFakeNvData->COMTerminalType = NewTerminalContext->TerminalType;
    }

    IfrOptionList[Index].Flags        = CheckFlags;
    IfrOptionList[Index].Key          = KEY_VALUE_COM_SET_TERMI_TYPE;
    IfrOptionList[Index].StringToken  = (STRING_REF) TerminalType[Index];
    IfrOptionList[Index].Value        = Index;
  }

  CreateOneOfOpCode (
    (UINT16) COM_TERMINAL_QUESTION_ID,
    (UINT8) 1,
    STRING_TOKEN (STR_COM_TERMI_TYPE),
    STRING_TOKEN (STR_COM_TERMI_TYPE),
    IfrOptionList,
    4,
    Location
    );

  Location              = Location + (Index + 1) * ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  Location              = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  UpdateData->DataCount = (UINT8) (UpdateData->DataCount + Index);
  UpdateData->DataCount += 2;

  SafeFreePool (IfrOptionList);

  CreateGotoOpCode (
    FORM_MAIN_ID,
    STRING_TOKEN (STR_SAVE_AND_EXIT),
    STRING_TOKEN (STR_NULL_STRING),
    FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
    KEY_VALUE_SAVE_AND_EXIT,
    Location
    );

  Location = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
  UpdateData->DataCount++;

  CreateGotoOpCode (
    FORM_MAIN_ID,
    STRING_TOKEN (STR_NO_SAVE_AND_EXIT),
    STRING_TOKEN (STR_NULL_STRING),
    FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,
    KEY_VALUE_NO_SAVE_AND_EXIT,
    Location
    );

  UpdateData->DataCount++;

  CallbackData->Hii->UpdateForm (
                      CallbackData->Hii,
                      CallbackData->BmmHiiHandle,
                      (EFI_FORM_LABEL) FORM_CON_COM_SETUP_ID,
                      TRUE,
                      UpdateData
                      );

}

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
  UINT32                    Attribute;
  UINT16                    *OrderBuffer;
  CHAR16                    StrTemp[100];
  UINT16                    FilePathSize;
  CHAR16                    *Description;
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
    Attribute = *(UINT32 *) Ptr;
    Ptr += sizeof (UINT32);

    FilePathSize = *(UINT16 *) Ptr;
    Ptr += sizeof (UINT16);

    Description = (CHAR16 *) Ptr;
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
  STRING_REF                  StrRef;
  STRING_REF                  StrRefHelp;
  BBS_TYPE                    BbsType;
  UINTN                       VarSize;
  UINTN                       Pos;
  UINTN                       Bit;
  UINT16                      Index;
  UINT16                      Index2;
  UINT16                      Key;
  CHAR16                      String[100];
  CHAR16                      *TypeStr;
  CHAR16                      *TypeStrHelp;
  UINT16                      VarDevOrder;
  UINT8                       *Location;
  UINT8                       *VarData;
  UINT8                       *OriginalPtr;
  UINT8                       *LegacyOrder;
  UINT8                       *OldData;
  UINT8                       *DisMap;

  OptionMenu                    = NULL;
  Key = 0;
  StrRef = 0;
  StrRefHelp = 0;
  TypeStr = NULL;
  TypeStrHelp = NULL;
  BbsType = BBS_FLOPPY;
  LegacyOrder = NULL;
  OldData = NULL;
  DisMap = NULL;

  Location = (UINT8 *) &UpdateData->Data;
  CallbackData->BmmAskSaveOrNot = TRUE;

  UpdatePageStart (CallbackData, &Location);

  DisMap = CallbackData->BmmOldFakeNVData.DisableMap;

  SetMem (DisMap, 32, 0);
  //
  // Create oneof option list
  //
  switch (UpdatePageId) {
  case FORM_SET_FD_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyFDMenu;
    Key         = LEGACY_FD_QUESTION_ID;
    TypeStr     = StrFloppy;
    TypeStrHelp = StrFloppyHelp;
    BbsType     = BBS_FLOPPY;
    LegacyOrder = CallbackData->BmmFakeNvData->LegacyFD;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyFD;
    break;

  case FORM_SET_HD_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyHDMenu;
    Key         = LEGACY_HD_QUESTION_ID;
    TypeStr     = StrHardDisk;
    TypeStrHelp = StrHardDiskHelp;
    BbsType     = BBS_HARDDISK;
    LegacyOrder = CallbackData->BmmFakeNvData->LegacyHD;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyHD;
    break;

  case FORM_SET_CD_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyCDMenu;
    Key         = LEGACY_CD_QUESTION_ID;
    TypeStr     = StrCDROM;
    TypeStrHelp = StrCDROMHelp;
    BbsType     = BBS_CDROM;
    LegacyOrder = CallbackData->BmmFakeNvData->LegacyCD;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyCD;
    break;

  case FORM_SET_NET_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyNETMenu;
    Key         = LEGACY_NET_QUESTION_ID;
    TypeStr     = StrNET;
    TypeStrHelp = StrNETHelp;
    BbsType     = BBS_EMBED_NETWORK;
    LegacyOrder = CallbackData->BmmFakeNvData->LegacyNET;
    OldData     = CallbackData->BmmOldFakeNVData.LegacyNET;
    break;

  case FORM_SET_BEV_ORDER_ID:
    OptionMenu  = (BM_MENU_OPTION *) &LegacyBEVMenu;
    Key         = LEGACY_BEV_QUESTION_ID;
    TypeStr     = StrBEV;
    TypeStrHelp = StrBEVHelp;
    BbsType     = BBS_BEV_DEVICE;
    LegacyOrder = CallbackData->BmmFakeNvData->LegacyBEV;
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
    IfrOptionList[Index].Flags  = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
    if (0 == Index) {
      IfrOptionList[Index].Flags |= FRAMEWORK_EFI_IFR_FLAG_DEFAULT;
    }

    IfrOptionList[Index].Key          = Key;
    IfrOptionList[Index].StringToken  = NewMenuEntry->DisplayStringToken;
    IfrOptionList[Index].Value        = (UINT16) ((BM_LEGACY_DEVICE_CONTEXT *) NewMenuEntry->VariableContext)->Index;
    IfrOptionList[Index].OptionString = NULL;
  }
  //
  // for item "Disabled"
  //
  IfrOptionList[Index].Flags        = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
  IfrOptionList[Index].Key          = Key;
  IfrOptionList[Index].StringToken  = STRING_TOKEN (STR_DISABLE_LEGACY_DEVICE);
  IfrOptionList[Index].Value        = 0xFF;
  IfrOptionList[Index].OptionString = NULL;

  //
  // Get Device Order from variable
  //
  VarData = BdsLibGetVariableAndSize (
              VarLegacyDevOrder,
              &EfiLegacyDevOrderGuid,
              &VarSize
              );

  if (NULL != VarData) {
    OriginalPtr = VarData;
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
      for (Index2 = 0; Index2 <= OptionMenu->MenuNumber; Index2++) {
        IfrOptionList[Index2].Key = (UINT16) (Key + Index);
      }
      //
      // Create the string for oneof tag
      //
      UnicodeSPrint (String, sizeof (String), TypeStr, Index);
      StrRef = 0;
      CallbackData->Hii->NewString (
                          CallbackData->Hii,
                          NULL,
                          CallbackData->BmmHiiHandle,
                          &StrRef,
                          String
                          );

      UnicodeSPrint (String, sizeof (String), TypeStrHelp, Index);
      StrRefHelp = 0;
      CallbackData->Hii->NewString (
                          CallbackData->Hii,
                          NULL,
                          CallbackData->BmmHiiHandle,
                          &StrRefHelp,
                          String
                          );

      CreateOneOfOpCode (
        (UINT16) (Key + Index),
        (UINT8) 1,
        StrRef,
        StrRefHelp,
        IfrOptionList,
        OptionMenu->MenuNumber + 1,
        Location
        );

      VarDevOrder = *(UINT16 *) ((UINT8 *) DevOrder + sizeof (BBS_TYPE) + sizeof (UINT16) + Index * sizeof (UINT16));

      if (0xFF00 == (VarDevOrder & 0xFF00)) {
        LegacyOrder[Index]  = 0xFF;
        Pos                 = (VarDevOrder & 0xFF) / 8;
        Bit                 = 7 - ((VarDevOrder & 0xFF) % 8);
        DisMap[Pos] |= (UINT8) (1 << Bit);
      } else {
        LegacyOrder[Index] = (UINT8) (VarDevOrder & 0xFF);
      }

      Location              = Location + (OptionMenu->MenuNumber + 2) * ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
      Location              = Location + ((FRAMEWORK_EFI_IFR_OP_HEADER *) Location)->Length;
      UpdateData->DataCount = (UINT16) (UpdateData->DataCount + (OptionMenu->MenuNumber + 3));
    }
  }

  CopyMem (
    OldData,
    LegacyOrder,
    100
    );

  if (IfrOptionList != NULL) {
    SafeFreePool (IfrOptionList);
    IfrOptionList = NULL;
  }

  UpdatePageEnd (CallbackData, Location);
}

VOID
UpdatePageId (
  BMM_CALLBACK_DATA              *Private,
  UINT16                         NewPageId
  )
{
  UINT16  FileOptionMask;

  FileOptionMask = (UINT16) (FILE_OPTION_MASK & NewPageId);

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
  }

  if ((NewPageId > 0) && (NewPageId < MAXIMUM_FORM_ID)) {
    Private->BmmPreviousPageId  = Private->BmmCurrentPageId;
    Private->BmmCurrentPageId   = NewPageId;
  }
}
