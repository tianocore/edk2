/** @file
  Boot Maintainence Main File

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BootMaint.h"
#include "FormGuid.h"
#include "Bds.h"
#include "FrontPage.h"

EFI_GUID EfiLegacyDevOrderGuid = EFI_LEGACY_DEV_ORDER_VARIABLE_GUID;
EFI_GUID mBootMaintGuid = BOOT_MAINT_FORMSET_GUID;
EFI_GUID mFileExplorerGuid = FILE_EXPLORE_FORMSET_GUID;

CHAR16  mBootMaintStorageName[] = L"BmData";
CHAR16  mFileExplorerStorageName[] = L"FeData";

VOID
InitAllMenu (
  IN  BMM_CALLBACK_DATA    *CallbackData
  );

VOID
FreeAllMenu (
  VOID
  );

EFI_STATUS
CreateMenuStringToken (
  IN BMM_CALLBACK_DATA                *CallbackData,
  IN EFI_HII_HANDLE                   HiiHandle,
  IN BM_MENU_OPTION                   *MenuOption
  )
/*++

Routine Description:
  Create string tokens for a menu from its help strings and display strings

Arguments:
  HiiHandle       - Hii Handle of the package to be updated.
  MenuOption      - The Menu whose string tokens need to be created

Returns:
  EFI_SUCCESS     - string tokens created successfully
  others          - contain some errors

--*/
{
  BM_MENU_ENTRY *NewMenuEntry;
  UINTN         Index;

  for (Index = 0; Index < MenuOption->MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (MenuOption, Index);

    HiiLibNewString (
      HiiHandle,
      &NewMenuEntry->DisplayStringToken,
      NewMenuEntry->DisplayString
      );

    if (NULL == NewMenuEntry->HelpString) {
      NewMenuEntry->HelpStringToken = NewMenuEntry->DisplayStringToken;
    } else {
      HiiLibNewString (
        HiiHandle,
        &NewMenuEntry->HelpStringToken,
        NewMenuEntry->HelpString
        );
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
BootMaintExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
/*++

  Routine Description:
    This function allows a caller to extract the current configuration for one
    or more named elements from the target driver.

  Arguments:
    This       - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
    Request    - A null-terminated Unicode string in <ConfigRequest> format.
    Progress   - On return, points to a character in the Request string.
                 Points to the string's null terminator if request was successful.
                 Points to the most recent '&' before the first failing name/value
                 pair (or the beginning of the string if the failure is in the
                 first name/value pair) if the request was not successful.
    Results    - A null-terminated Unicode string in <ConfigAltResp> format which
                 has all values filled in for the names in the Request string.
                 String to be allocated by the called function.

  Returns:
    EFI_SUCCESS           - The Results is filled with the requested values.
    EFI_OUT_OF_RESOURCES  - Not enough memory to store the results.
    EFI_INVALID_PARAMETER - Request is NULL, illegal syntax, or unknown name.
    EFI_NOT_FOUND         - Routing data doesn't match any storage in this driver.

--*/
{
  EFI_STATUS         Status;
  UINTN              BufferSize;
  BMM_CALLBACK_DATA  *Private;

  Private = BMM_CALLBACK_DATA_FROM_THIS (This);

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  BufferSize = sizeof (BMM_FAKE_NV_DATA);
  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                Request,
                                (UINT8 *) &Private->BmmFakeNvData,
                                BufferSize,
                                Results,
                                Progress
                                );
  return Status;
}

EFI_STATUS
EFIAPI
BootMaintCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
/*++

  Routine Description:
    This function processes the results of changes in configuration.

  Arguments:
    This          - Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
    Action        - Specifies the type of action taken by the browser.
    QuestionId    - A unique value which is sent to the original exporting driver
                    so that it can identify the type of data to expect.
    Type          - The type of value for the question.
    Value         - A pointer to the data being sent to the original exporting driver.
    ActionRequest - On return, points to the action requested by the callback function.

  Returns:
    EFI_SUCCESS          - The callback successfully handled the action.
    EFI_OUT_OF_RESOURCES - Not enough storage is available to hold the variable and its data.
    EFI_DEVICE_ERROR     - The variable could not be saved.
    EFI_UNSUPPORTED      - The specified Action is not supported by the callback.

--*/
{
  BMM_CALLBACK_DATA *Private;
  BM_MENU_ENTRY     *NewMenuEntry;
  BMM_FAKE_NV_DATA  *CurrentFakeNVMap;
  EFI_STATUS        Status;
  UINTN             OldValue;
  UINTN             NewValue;
  UINTN             Number;
  UINTN             Pos;
  UINTN             Bit;
  UINT16            NewValuePos;
  UINT16            Index2;
  UINT16            Index;
  UINT8             *OldLegacyDev;
  UINT8             *NewLegacyDev;
  UINT8             *DisMap;
  EFI_FORM_ID       FormId;
  UINTN             BufferSize;

  if ((Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  OldValue       = 0;
  NewValue       = 0;
  Number         = 0;
  OldLegacyDev   = NULL;
  NewLegacyDev   = NULL;
  NewValuePos    = 0;
  DisMap         = NULL;
  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;

  Private        = BMM_CALLBACK_DATA_FROM_THIS (This);
  UpdatePageId (Private, QuestionId);

  //
  // Retrive uncommitted data from Form Browser
  //
  CurrentFakeNVMap = &Private->BmmFakeNvData;
  BufferSize = sizeof (BMM_FAKE_NV_DATA);
  Status = GetBrowserData (NULL, NULL, &BufferSize, (UINT8 *) CurrentFakeNVMap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // need to be subtituded.
  //
  // Update Select FD/HD/CD/NET/BEV Order Form
  //
  if (FORM_SET_FD_ORDER_ID == Private->BmmPreviousPageId ||
      FORM_SET_HD_ORDER_ID == Private->BmmPreviousPageId ||
      FORM_SET_CD_ORDER_ID == Private->BmmPreviousPageId ||
      FORM_SET_NET_ORDER_ID == Private->BmmPreviousPageId ||
      FORM_SET_BEV_ORDER_ID == Private->BmmPreviousPageId ||
      ((FORM_BOOT_SETUP_ID == Private->BmmPreviousPageId) &&
      (QuestionId >= LEGACY_FD_QUESTION_ID) &&
       (QuestionId < (LEGACY_BEV_QUESTION_ID + 100)) )
      ) {

    DisMap  = Private->BmmOldFakeNVData.DisableMap;

    FormId  = Private->BmmPreviousPageId;
    if (FormId == FORM_BOOT_SETUP_ID) {
      FormId = Private->BmmCurrentPageId;
    }

    switch (FormId) {
    case FORM_SET_FD_ORDER_ID:
      Number        = (UINT16) LegacyFDMenu.MenuNumber;
      OldLegacyDev  = Private->BmmOldFakeNVData.LegacyFD;
      NewLegacyDev  = CurrentFakeNVMap->LegacyFD;
      break;

    case FORM_SET_HD_ORDER_ID:
      Number        = (UINT16) LegacyHDMenu.MenuNumber;
      OldLegacyDev  = Private->BmmOldFakeNVData.LegacyHD;
      NewLegacyDev  = CurrentFakeNVMap->LegacyHD;
      break;

    case FORM_SET_CD_ORDER_ID:
      Number        = (UINT16) LegacyCDMenu.MenuNumber;
      OldLegacyDev  = Private->BmmOldFakeNVData.LegacyCD;
      NewLegacyDev  = CurrentFakeNVMap->LegacyCD;
      break;

    case FORM_SET_NET_ORDER_ID:
      Number        = (UINT16) LegacyNETMenu.MenuNumber;
      OldLegacyDev  = Private->BmmOldFakeNVData.LegacyNET;
      NewLegacyDev  = CurrentFakeNVMap->LegacyNET;
      break;

    case FORM_SET_BEV_ORDER_ID:
      Number        = (UINT16) LegacyBEVMenu.MenuNumber;
      OldLegacyDev  = Private->BmmOldFakeNVData.LegacyBEV;
      NewLegacyDev  = CurrentFakeNVMap->LegacyBEV;
      break;

    default:
      break;
    }
    //
    //  First, find the different position
    //  if there is change, it should be only one
    //
    for (Index = 0; Index < Number; Index++) {
      if (OldLegacyDev[Index] != NewLegacyDev[Index]) {
        OldValue  = OldLegacyDev[Index];
        NewValue  = NewLegacyDev[Index];
        break;
      }
    }

    if (Index != Number) {
      //
      // there is change, now process
      //
      if (0xFF == NewValue) {
        //
        // This item will be disable
        // Just move the items behind this forward to overlap it
        //
        Pos = OldValue / 8;
        Bit = 7 - (OldValue % 8);
        DisMap[Pos] = (UINT8) (DisMap[Pos] | (UINT8) (1 << Bit));
        for (Index2 = Index; Index2 < Number - 1; Index2++) {
          NewLegacyDev[Index2] = NewLegacyDev[Index2 + 1];
        }

        NewLegacyDev[Index2] = 0xFF;
      } else {
        for (Index2 = 0; Index2 < Number; Index2++) {
          if (Index2 == Index) {
            continue;
          }

          if (OldLegacyDev[Index2] == NewValue) {
            //
            // If NewValue is in OldLegacyDev array
            // remember its old position
            //
            NewValuePos = Index2;
            break;
          }
        }

        if (Index2 != Number) {
          //
          // We will change current item to an existing item
          // (It's hard to describe here, please read code, it's like a cycle-moving)
          //
          for (Index2 = NewValuePos; Index2 != Index;) {
            if (NewValuePos < Index) {
              NewLegacyDev[Index2] = OldLegacyDev[Index2 + 1];
              Index2++;
            } else {
              NewLegacyDev[Index2] = OldLegacyDev[Index2 - 1];
              Index2--;
            }
          }
        } else {
          //
          // If NewValue is not in OldlegacyDev array, we are changing to a disabled item
          // so we should modify DisMap to reflect the change
          //
          Pos = NewValue / 8;
          Bit = 7 - (NewValue % 8);
          DisMap[Pos] = (UINT8) (DisMap[Pos] & (~ (UINT8) (1 << Bit)));
          if (0xFF != OldValue) {
            //
            // Because NewValue is a item that was disabled before
            // so after changing the OldValue should be disabled
            // actually we are doing a swap of enable-disable states of two items
            //
            Pos = OldValue / 8;
            Bit = 7 - (OldValue % 8);
            DisMap[Pos] = (UINT8) (DisMap[Pos] | (UINT8) (1 << Bit));
          }
        }
      }
      //
      // To prevent DISABLE appears in the middle of the list
      // we should perform a re-ordering
      //
      Index = 0;
      while (Index < Number) {
        if (0xFF != NewLegacyDev[Index]) {
          Index++;
          continue;
        }

        Index2 = Index;
        Index2++;
        while (Index2 < Number) {
          if (0xFF != NewLegacyDev[Index2]) {
            break;
          }

          Index2++;
        }

        if (Index2 < Number) {
          NewLegacyDev[Index]   = NewLegacyDev[Index2];
          NewLegacyDev[Index2]  = 0xFF;
        }

        Index++;
      }

      CopyMem (
        OldLegacyDev,
        NewLegacyDev,
        Number
        );
    }
  }

  if (QuestionId < FILE_OPTION_OFFSET) {
    if (QuestionId < CONFIG_OPTION_OFFSET) {
      switch (QuestionId) {
      case KEY_VALUE_BOOT_FROM_FILE:
        Private->FeCurrentState = BOOT_FROM_FILE_STATE;

        //
        // Exit Bmm main formset to send File Explorer formset.
        //
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
        break;

      case FORM_BOOT_ADD_ID:
        Private->FeCurrentState = ADD_BOOT_OPTION_STATE;

        //
        // Exit Bmm main formset to send File Explorer formset.
        //
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
        break;

      case FORM_DRV_ADD_FILE_ID:
        Private->FeCurrentState = ADD_DRIVER_OPTION_STATE;

        //
        // Exit Bmm main formset to send File Explorer formset.
        //
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
        break;

      case FORM_DRV_ADD_HANDLE_ID:
        CleanUpPage (FORM_DRV_ADD_HANDLE_ID, Private);
        UpdateDrvAddHandlePage (Private);
        break;

      case FORM_BOOT_DEL_ID:
        CleanUpPage (FORM_BOOT_DEL_ID, Private);
        UpdateBootDelPage (Private);
        break;

      case FORM_BOOT_CHG_ID:
      case FORM_DRV_CHG_ID:
        UpdatePageBody (QuestionId, Private);
        break;

      case FORM_DRV_DEL_ID:
        CleanUpPage (FORM_DRV_DEL_ID, Private);
        UpdateDrvDelPage (Private);
        break;

      case FORM_BOOT_NEXT_ID:
        CleanUpPage (FORM_BOOT_NEXT_ID, Private);
        UpdateBootNextPage (Private);
        break;

      case FORM_TIME_OUT_ID:
        CleanUpPage (FORM_TIME_OUT_ID, Private);
        UpdateTimeOutPage (Private);
        break;

      case FORM_RESET:
        gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
        return EFI_UNSUPPORTED;

      case FORM_CON_IN_ID:
      case FORM_CON_OUT_ID:
      case FORM_CON_ERR_ID:
        UpdatePageBody (QuestionId, Private);
        break;

      case FORM_CON_MODE_ID:
        CleanUpPage (FORM_CON_MODE_ID, Private);
        UpdateConModePage (Private);
        break;

      case FORM_CON_COM_ID:
        CleanUpPage (FORM_CON_COM_ID, Private);
        UpdateConCOMPage (Private);
        break;

      case FORM_SET_FD_ORDER_ID:
      case FORM_SET_HD_ORDER_ID:
      case FORM_SET_CD_ORDER_ID:
      case FORM_SET_NET_ORDER_ID:
      case FORM_SET_BEV_ORDER_ID:
        CleanUpPage (QuestionId, Private);
        UpdateSetLegacyDeviceOrderPage (QuestionId, Private);
        break;

      case KEY_VALUE_SAVE_AND_EXIT:
      case KEY_VALUE_NO_SAVE_AND_EXIT:

        if (QuestionId == KEY_VALUE_SAVE_AND_EXIT) {
          Status = ApplyChangeHandler (Private, CurrentFakeNVMap, Private->BmmPreviousPageId);
          if (EFI_ERROR (Status)) {
            return Status;
          }
        } else if (QuestionId == KEY_VALUE_NO_SAVE_AND_EXIT) {
          DiscardChangeHandler (Private, CurrentFakeNVMap);
        }

        //
        // Tell browser not to ask for confirmation of changes,
        // since we have already applied or discarded.
        //
        *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
        break;

      default:
        break;
      }
    } else if ((QuestionId >= TERMINAL_OPTION_OFFSET) && (QuestionId < CONSOLE_OPTION_OFFSET)) {
      Index2                    = (UINT16) (QuestionId - TERMINAL_OPTION_OFFSET);
      Private->CurrentTerminal  = Index2;

      CleanUpPage (FORM_CON_COM_SETUP_ID, Private);
      UpdateTerminalPage (Private);

    } else if (QuestionId >= HANDLE_OPTION_OFFSET) {
      Index2                  = (UINT16) (QuestionId - HANDLE_OPTION_OFFSET);

      NewMenuEntry            = BOpt_GetMenuEntry (&DriverMenu, Index2);
      ASSERT (NewMenuEntry != NULL);
      Private->HandleContext  = (BM_HANDLE_CONTEXT *) NewMenuEntry->VariableContext;

      CleanUpPage (FORM_DRV_ADD_HANDLE_DESC_ID, Private);

      Private->MenuEntry                  = NewMenuEntry;
      Private->LoadContext->FilePathList  = Private->HandleContext->DevicePath;

      UpdateDriverAddHandleDescPage (Private);
    }
  }

  //
  // Pass changed uncommitted data back to Form Browser
  //
  BufferSize = sizeof (BMM_FAKE_NV_DATA);
  Status = SetBrowserData (NULL, NULL, BufferSize, (UINT8 *) CurrentFakeNVMap, NULL);

  return Status;
}

EFI_STATUS
ApplyChangeHandler (
  IN  BMM_CALLBACK_DATA               *Private,
  IN  BMM_FAKE_NV_DATA                *CurrentFakeNVMap,
  IN  EFI_FORM_ID                     FormId
  )
/*++

Routine Description:
  Function handling request to apply changes for BMM pages.

Arguments:
  Private          - Pointer to callback data buffer.
  CurrentFakeNVMap - Pointer to buffer holding data of various values used by BMM
  FormId           - ID of the form which has sent the request to apply change.

Returns:
  EFI_SUCCESS      - Change successfully applied.
  Other            - Error occurs while trying to apply changes.

--*/
{
  BM_CONSOLE_CONTEXT  *NewConsoleContext;
  BM_TERMINAL_CONTEXT *NewTerminalContext;
  BM_LOAD_CONTEXT     *NewLoadContext;
  BM_MENU_ENTRY       *NewMenuEntry;
  EFI_STATUS          Status;
  UINT16              Index;

  Status = EFI_SUCCESS;

  switch (FormId) {
  case FORM_SET_FD_ORDER_ID:
  case FORM_SET_HD_ORDER_ID:
  case FORM_SET_CD_ORDER_ID:
  case FORM_SET_NET_ORDER_ID:
  case FORM_SET_BEV_ORDER_ID:
    Var_UpdateBBSOption (Private);
    break;

  case FORM_BOOT_DEL_ID:
    for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
      NewMenuEntry            = BOpt_GetMenuEntry (&BootOptionMenu, Index);
      NewLoadContext          = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
      NewLoadContext->Deleted = CurrentFakeNVMap->BootOptionDel[Index];
    }

    Var_DelBootOption ();
    break;

  case FORM_DRV_DEL_ID:
    for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
      NewMenuEntry            = BOpt_GetMenuEntry (&DriverOptionMenu, Index);
      NewLoadContext          = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
      NewLoadContext->Deleted = CurrentFakeNVMap->DriverOptionDel[Index];
    }

    Var_DelDriverOption ();
    break;

  case FORM_BOOT_CHG_ID:
    Status = Var_UpdateBootOrder (Private);
    break;

  case FORM_DRV_CHG_ID:
    Status = Var_UpdateDriverOrder (Private);
    break;

  case FORM_TIME_OUT_ID:
    Status = gRT->SetVariable (
                    L"Timeout",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    sizeof (UINT16),
                    &(CurrentFakeNVMap->BootTimeOut)
                    );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    Private->BmmOldFakeNVData.BootTimeOut = CurrentFakeNVMap->BootTimeOut;
    break;

  case FORM_BOOT_NEXT_ID:
    Status = Var_UpdateBootNext (Private);
    break;

  case FORM_CON_MODE_ID:
    Status = Var_UpdateConMode (Private);
    break;

  case FORM_CON_COM_SETUP_ID:
    NewMenuEntry                      = BOpt_GetMenuEntry (&TerminalMenu, Private->CurrentTerminal);

    ASSERT (NewMenuEntry != NULL);

    NewTerminalContext                = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;

    NewTerminalContext->BaudRateIndex = CurrentFakeNVMap->COMBaudRate;
    NewTerminalContext->BaudRate      = BaudRateList[CurrentFakeNVMap->COMBaudRate].Value;
    NewTerminalContext->DataBitsIndex = CurrentFakeNVMap->COMDataRate;
    NewTerminalContext->DataBits      = (UINT8) DataBitsList[CurrentFakeNVMap->COMDataRate].Value;
    NewTerminalContext->StopBitsIndex = CurrentFakeNVMap->COMStopBits;
    NewTerminalContext->StopBits      = (UINT8) StopBitsList[CurrentFakeNVMap->COMStopBits].Value;
    NewTerminalContext->ParityIndex   = CurrentFakeNVMap->COMParity;
    NewTerminalContext->Parity        = (UINT8) ParityList[CurrentFakeNVMap->COMParity].Value;
    NewTerminalContext->TerminalType  = CurrentFakeNVMap->COMTerminalType;

    ChangeTerminalDevicePath (
      NewTerminalContext->DevicePath,
      FALSE
      );

    Var_UpdateConsoleInpOption ();
    Var_UpdateConsoleOutOption ();
    Var_UpdateErrorOutOption ();
    break;

  case FORM_CON_IN_ID:
    for (Index = 0; Index < ConsoleInpMenu.MenuNumber; Index++) {
      NewMenuEntry                = BOpt_GetMenuEntry (&ConsoleInpMenu, Index);
      NewConsoleContext           = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
      NewConsoleContext->IsActive = CurrentFakeNVMap->ConsoleCheck[Index];
    }

    for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
      NewMenuEntry                = BOpt_GetMenuEntry (&TerminalMenu, Index);
      NewTerminalContext          = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
      NewTerminalContext->IsConIn = CurrentFakeNVMap->ConsoleCheck[Index + ConsoleInpMenu.MenuNumber];
    }

    Var_UpdateConsoleInpOption ();
    break;

  case FORM_CON_OUT_ID:
    for (Index = 0; Index < ConsoleOutMenu.MenuNumber; Index++) {
      NewMenuEntry                = BOpt_GetMenuEntry (&ConsoleOutMenu, Index);
      NewConsoleContext           = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
      NewConsoleContext->IsActive = CurrentFakeNVMap->ConsoleCheck[Index];
    }

    for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
      NewMenuEntry                  = BOpt_GetMenuEntry (&TerminalMenu, Index);
      NewTerminalContext            = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
      NewTerminalContext->IsConOut  = CurrentFakeNVMap->ConsoleCheck[Index + ConsoleOutMenu.MenuNumber];
    }

    Var_UpdateConsoleOutOption ();
    break;

  case FORM_CON_ERR_ID:
    for (Index = 0; Index < ConsoleErrMenu.MenuNumber; Index++) {
      NewMenuEntry                = BOpt_GetMenuEntry (&ConsoleErrMenu, Index);
      NewConsoleContext           = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
      NewConsoleContext->IsActive = CurrentFakeNVMap->ConsoleCheck[Index];
    }

    for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
      NewMenuEntry                  = BOpt_GetMenuEntry (&TerminalMenu, Index);
      NewTerminalContext            = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
      NewTerminalContext->IsStdErr  = CurrentFakeNVMap->ConsoleCheck[Index + ConsoleErrMenu.MenuNumber];
    }

    Var_UpdateErrorOutOption ();
    break;

  case FORM_DRV_ADD_HANDLE_DESC_ID:
    Status = Var_UpdateDriverOption (
               Private,
               Private->BmmHiiHandle,
               CurrentFakeNVMap->DriverAddHandleDesc,
               CurrentFakeNVMap->DriverAddHandleOptionalData,
               CurrentFakeNVMap->DriverAddForceReconnect
               );
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    BOpt_GetDriverOptions (Private);
    CreateMenuStringToken (Private, Private->BmmHiiHandle, &DriverOptionMenu);
    break;

  default:
    break;
  }

Error:
  return Status;
}

VOID
DiscardChangeHandler (
  IN  BMM_CALLBACK_DATA               *Private,
  IN  BMM_FAKE_NV_DATA                *CurrentFakeNVMap
  )
{
  UINT16  Index;

  switch (Private->BmmPreviousPageId) {
  case FORM_BOOT_CHG_ID:
  case FORM_DRV_CHG_ID:
    CopyMem (CurrentFakeNVMap->OptionOrder, Private->BmmOldFakeNVData.OptionOrder, 100);
    break;

  case FORM_BOOT_DEL_ID:
    for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
      CurrentFakeNVMap->BootOptionDel[Index] = 0x00;
    }
    break;

  case FORM_DRV_DEL_ID:
    for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
      CurrentFakeNVMap->DriverOptionDel[Index] = 0x00;
    }
    break;

  case FORM_BOOT_NEXT_ID:
    CurrentFakeNVMap->BootNext = Private->BmmOldFakeNVData.BootNext;
    break;

  case FORM_TIME_OUT_ID:
    CurrentFakeNVMap->BootTimeOut = Private->BmmOldFakeNVData.BootTimeOut;
    break;

  case FORM_DRV_ADD_HANDLE_DESC_ID:
  case FORM_DRV_ADD_FILE_ID:
  case FORM_DRV_ADD_HANDLE_ID:
    CurrentFakeNVMap->DriverAddHandleDesc[0]          = 0x0000;
    CurrentFakeNVMap->DriverAddHandleOptionalData[0]  = 0x0000;
    break;

  default:
    break;
  }
}

EFI_STATUS
InitializeBM (
  VOID
  )
/*++

Routine Description:
  Initialize the Boot Maintenance Utitliy

Arguments:
  ImageHandle     - caller provided handle
  SystemTable     - caller provided system tables

Returns:
  EFI_SUCCESS     - utility ended successfully
  others          - contain some errors

--*/
{
  EFI_LEGACY_BIOS_PROTOCOL    *LegacyBios;
  EFI_HII_PACKAGE_LIST_HEADER *PackageList;
  BMM_CALLBACK_DATA           *BmmCallbackInfo;
  EFI_STATUS                  Status;
  UINT8                       *Ptr;

  Status = EFI_SUCCESS;

  //
  // Create CallbackData structures for Driver Callback
  //
  BmmCallbackInfo = EfiAllocateZeroPool (sizeof (BMM_CALLBACK_DATA));
  if (BmmCallbackInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create LoadOption in BmmCallbackInfo for Driver Callback
  //
  Ptr = EfiAllocateZeroPool (sizeof (BM_LOAD_CONTEXT) + sizeof (BM_FILE_CONTEXT) + sizeof (BM_HANDLE_CONTEXT) + sizeof (BM_MENU_ENTRY));
  if (!Ptr) {
    SafeFreePool (BmmCallbackInfo);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize Bmm callback data.
  //
  BmmCallbackInfo->LoadContext = (BM_LOAD_CONTEXT *) Ptr;
  Ptr += sizeof (BM_LOAD_CONTEXT);

  BmmCallbackInfo->FileContext = (BM_FILE_CONTEXT *) Ptr;
  Ptr += sizeof (BM_FILE_CONTEXT);

  BmmCallbackInfo->HandleContext = (BM_HANDLE_CONTEXT *) Ptr;
  Ptr += sizeof (BM_HANDLE_CONTEXT);

  BmmCallbackInfo->MenuEntry      = (BM_MENU_ENTRY *) Ptr;

  BmmCallbackInfo->Signature                     = BMM_CALLBACK_DATA_SIGNATURE;
  BmmCallbackInfo->BmmConfigAccess.ExtractConfig = BootMaintExtractConfig;
  BmmCallbackInfo->BmmConfigAccess.RouteConfig   = FakeRouteConfig;
  BmmCallbackInfo->BmmConfigAccess.Callback      = BootMaintCallback;
  BmmCallbackInfo->BmmPreviousPageId             = FORM_MAIN_ID;
  BmmCallbackInfo->BmmCurrentPageId              = FORM_MAIN_ID;
  BmmCallbackInfo->FeConfigAccess.ExtractConfig  = FakeExtractConfig;
  BmmCallbackInfo->FeConfigAccess.RouteConfig    = FakeRouteConfig;
  BmmCallbackInfo->FeConfigAccess.Callback       = FileExplorerCallback;
  BmmCallbackInfo->FeCurrentState                = INACTIVE_STATE;
  BmmCallbackInfo->FeDisplayContext              = UNKNOWN_CONTEXT;

  //
  // Create driver handle used by HII database
  //
  Status = HiiLibCreateHiiDriverHandle (&BmmCallbackInfo->BmmDriverHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Config Access protocol to driver handle
  //
  Status = gBS->InstallProtocolInterface (
                  &BmmCallbackInfo->BmmDriverHandle,
                  &gEfiHiiConfigAccessProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &BmmCallbackInfo->BmmConfigAccess
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Create driver handle used by HII database
  //
  Status = HiiLibCreateHiiDriverHandle (&BmmCallbackInfo->FeDriverHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install Config Access protocol to driver handle
  //
  Status = gBS->InstallProtocolInterface (
                  &BmmCallbackInfo->FeDriverHandle,
                  &gEfiHiiConfigAccessProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &BmmCallbackInfo->FeConfigAccess
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Post our Boot Maint VFR binnary to the HII database.
  //
  PackageList = HiiLibPreparePackageList (2, &mBootMaintGuid, BmBin, BdsDxeStrings);
  ASSERT (PackageList != NULL);

  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           BmmCallbackInfo->BmmDriverHandle,
                           &BmmCallbackInfo->BmmHiiHandle
                           );
  FreePool (PackageList);

  //
  // Post our File Explorer VFR binary to the HII database.
  //
  PackageList = HiiLibPreparePackageList (2, &mFileExplorerGuid, FEBin, BdsDxeStrings);
  ASSERT (PackageList != NULL);

  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           BmmCallbackInfo->FeDriverHandle,
                           &BmmCallbackInfo->FeHiiHandle
                           );
  FreePool (PackageList);

  //
  // Allocate space for creation of Buffer
  //
  gUpdateData.BufferSize = UPDATE_DATA_SIZE;
  gUpdateData.Data = EfiAllocateZeroPool (UPDATE_DATA_SIZE);
  if (gUpdateData.Data == NULL) {
    SafeFreePool (BmmCallbackInfo->LoadContext);
    SafeFreePool (BmmCallbackInfo);
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeStringDepository ();

  InitAllMenu (BmmCallbackInfo);

  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &ConsoleInpMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &ConsoleOutMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &ConsoleErrMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &BootOptionMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &DriverOptionMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &TerminalMenu);
  CreateMenuStringToken (BmmCallbackInfo, BmmCallbackInfo->BmmHiiHandle, &DriverMenu);

  UpdateBootDelPage (BmmCallbackInfo);
  UpdateDrvDelPage (BmmCallbackInfo);

  if (TerminalMenu.MenuNumber > 0) {
    BmmCallbackInfo->CurrentTerminal = 0;
    UpdateTerminalPage (BmmCallbackInfo);
  }

  Status = EfiLibLocateProtocol (&gEfiLegacyBiosProtocolGuid, (VOID **) &LegacyBios);
  if (!EFI_ERROR (Status)) {
    RefreshUpdateData ();

    //
    // If LegacyBios Protocol is installed, add 3 tags about legacy boot option
    // in BootOption form: legacy FD/HD/CD/NET/BEV
    //
    CreateGotoOpCode (
      FORM_SET_FD_ORDER_ID,
      STRING_TOKEN (STR_FORM_SET_FD_ORDER_TITLE),
      STRING_TOKEN (STR_FORM_SET_FD_ORDER_TITLE),
      EFI_IFR_FLAG_CALLBACK,
      FORM_SET_FD_ORDER_ID,
      &gUpdateData
      );

    CreateGotoOpCode (
      FORM_SET_HD_ORDER_ID,
      STRING_TOKEN (STR_FORM_SET_HD_ORDER_TITLE),
      STRING_TOKEN (STR_FORM_SET_HD_ORDER_TITLE),
      EFI_IFR_FLAG_CALLBACK,
      FORM_SET_HD_ORDER_ID,
      &gUpdateData
      );

    CreateGotoOpCode (
      FORM_SET_CD_ORDER_ID,
      STRING_TOKEN (STR_FORM_SET_CD_ORDER_TITLE),
      STRING_TOKEN (STR_FORM_SET_CD_ORDER_TITLE),
      EFI_IFR_FLAG_CALLBACK,
      FORM_SET_CD_ORDER_ID,
      &gUpdateData
      );

    CreateGotoOpCode (
      FORM_SET_NET_ORDER_ID,
      STRING_TOKEN (STR_FORM_SET_NET_ORDER_TITLE),
      STRING_TOKEN (STR_FORM_SET_NET_ORDER_TITLE),
      EFI_IFR_FLAG_CALLBACK,
      FORM_SET_NET_ORDER_ID,
      &gUpdateData
      );

    CreateGotoOpCode (
      FORM_SET_BEV_ORDER_ID,
      STRING_TOKEN (STR_FORM_SET_BEV_ORDER_TITLE),
      STRING_TOKEN (STR_FORM_SET_BEV_ORDER_TITLE),
      EFI_IFR_FLAG_CALLBACK,
      FORM_SET_BEV_ORDER_ID,
      &gUpdateData
      );

    IfrLibUpdateForm (
      BmmCallbackInfo->BmmHiiHandle,
      &mBootMaintGuid,
      FORM_MAIN_ID,
      FORM_BOOT_LEGACY_DEVICE_ID,
      FALSE,
      &gUpdateData
      );
  }

  //
  // Dispatch BMM main formset and File Explorer formset.
  //
  FormSetDispatcher (BmmCallbackInfo);

  //
  // Remove our IFR data from HII database
  //
  gHiiDatabase->RemovePackageList (gHiiDatabase, BmmCallbackInfo->BmmHiiHandle);
  gHiiDatabase->RemovePackageList (gHiiDatabase, BmmCallbackInfo->FeHiiHandle);

  CleanUpStringDepository ();

  FreeAllMenu ();

  SafeFreePool (BmmCallbackInfo->LoadContext);
  SafeFreePool (BmmCallbackInfo);
  SafeFreePool (gUpdateData.Data);
  gUpdateData.Data = NULL;

  return Status;
}

VOID
InitAllMenu (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
{
  InitializeListHead (&BootOptionMenu.Head);
  InitializeListHead (&DriverOptionMenu.Head);
  BOpt_GetBootOptions (CallbackData);
  BOpt_GetDriverOptions (CallbackData);
  BOpt_GetLegacyOptions ();
  InitializeListHead (&FsOptionMenu.Head);
  BOpt_FindDrivers ();
  InitializeListHead (&DirectoryMenu.Head);
  InitializeListHead (&ConsoleInpMenu.Head);
  InitializeListHead (&ConsoleOutMenu.Head);
  InitializeListHead (&ConsoleErrMenu.Head);
  InitializeListHead (&TerminalMenu.Head);
  LocateSerialIo ();
  GetAllConsoles ();
}

VOID
FreeAllMenu (
  VOID
  )
{
  BOpt_FreeMenu (&DirectoryMenu);
  BOpt_FreeMenu (&FsOptionMenu);
  BOpt_FreeMenu (&BootOptionMenu);
  BOpt_FreeMenu (&DriverOptionMenu);
  BOpt_FreeMenu (&DriverMenu);
  BOpt_FreeLegacyOptions ();
  FreeAllConsoles ();
}

VOID
InitializeStringDepository (
  VOID
  )
/*++

Routine Description:
  Intialize all the string depositories.

Arguments:
  None.

Returns:
  None.

--*/
{
  STRING_DEPOSITORY *StringDepository;
  StringDepository              = EfiAllocateZeroPool (sizeof (STRING_DEPOSITORY) * STRING_DEPOSITORY_NUMBER);
  FileOptionStrDepository       = StringDepository++;
  ConsoleOptionStrDepository    = StringDepository++;
  BootOptionStrDepository       = StringDepository++;
  BootOptionHelpStrDepository   = StringDepository++;
  DriverOptionStrDepository     = StringDepository++;
  DriverOptionHelpStrDepository = StringDepository++;
  TerminalStrDepository         = StringDepository;
}

EFI_STRING_ID
GetStringTokenFromDepository (
  IN   BMM_CALLBACK_DATA     *CallbackData,
  IN   STRING_DEPOSITORY     *StringDepository
  )
/*++

Routine Description:
  Fetch a usable string node from the string depository and return the string token.

Arguments:
  StringDepository       - Pointer of the string depository.

Returns:
  EFI_STRING_ID          - String token.

--*/
{
  STRING_LIST_NODE  *CurrentListNode;
  STRING_LIST_NODE  *NextListNode;

  CurrentListNode = StringDepository->CurrentNode;

  if ((NULL != CurrentListNode) && (NULL != CurrentListNode->Next)) {
    //
    // Fetch one reclaimed node from the list.
    //
    NextListNode = StringDepository->CurrentNode->Next;
  } else {
    //
    // If there is no usable node in the list, update the list.
    //
    NextListNode = EfiAllocateZeroPool (sizeof (STRING_LIST_NODE));

    HiiLibNewString (CallbackData->BmmHiiHandle, &(NextListNode->StringToken), L" ");
    ASSERT (NextListNode->StringToken != 0);

    StringDepository->TotalNodeNumber++;

    if (NULL == CurrentListNode) {
      StringDepository->ListHead = NextListNode;
    } else {
      CurrentListNode->Next = NextListNode;
    }
  }

  StringDepository->CurrentNode = NextListNode;

  return StringDepository->CurrentNode->StringToken;
}

VOID
ReclaimStringDepository (
  VOID
  )
/*++

Routine Description:
  Reclaim string depositories by moving the current node pointer to list head..

Arguments:
  None.

Returns:
  None.

--*/
{
  UINTN             DepositoryIndex;
  STRING_DEPOSITORY *StringDepository;

  StringDepository = FileOptionStrDepository;
  for (DepositoryIndex = 0; DepositoryIndex < STRING_DEPOSITORY_NUMBER; DepositoryIndex++) {
    StringDepository->CurrentNode = StringDepository->ListHead;
    StringDepository++;
  }
}

VOID
CleanUpStringDepository (
  VOID
  )
/*++

Routine Description:
  Release resource for all the string depositories.

Arguments:
  None.

Returns:
  None.

--*/
{
  UINTN             NodeIndex;
  UINTN             DepositoryIndex;
  STRING_LIST_NODE  *CurrentListNode;
  STRING_LIST_NODE  *NextListNode;
  STRING_DEPOSITORY *StringDepository;

  //
  // Release string list nodes.
  //
  StringDepository = FileOptionStrDepository;
  for (DepositoryIndex = 0; DepositoryIndex < STRING_DEPOSITORY_NUMBER; DepositoryIndex++) {
    CurrentListNode = StringDepository->ListHead;
    for (NodeIndex = 0; NodeIndex < StringDepository->TotalNodeNumber; NodeIndex++) {
      NextListNode = CurrentListNode->Next;
      SafeFreePool (CurrentListNode);
      CurrentListNode = NextListNode;
    }

    StringDepository++;
  }
  //
  // Release string depository.
  //
  SafeFreePool (FileOptionStrDepository);
}

EFI_STATUS
BdsStartBootMaint (
  VOID
  )
/*++

Routine Description:
  Start boot maintenance manager

Arguments:

Returns:

--*/
{
  EFI_STATUS      Status;
  LIST_ENTRY      BdsBootOptionList;

  InitializeListHead (&BdsBootOptionList);

  //
  // Connect all prior to entering the platform setup menu.
  //
  if (!gConnectAllHappened) {
    BdsLibConnectAllDriversToAllControllers ();
    gConnectAllHappened = TRUE;
  }
  //
  // Have chance to enumerate boot device
  //
  BdsLibEnumerateAllBootOption (&BdsBootOptionList);

  //
  // Drop the TPL level from TPL_APPLICATION to TPL_APPLICATION
  //
  gBS->RestoreTPL (TPL_APPLICATION);

  //
  // Init the BMM
  //
  Status = InitializeBM ();

  //
  // Raise the TPL level back to TPL_APPLICATION
  //
  gBS->RaiseTPL (TPL_APPLICATION);

  return Status;
}

EFI_STATUS
FormSetDispatcher (
  IN  BMM_CALLBACK_DATA    *CallbackData
  )
/*++

Routine Description:
  Dispatch BMM formset and FileExplorer formset.

Arguments:

Returns:

--*/
{
  EFI_STATUS                 Status;
  EFI_BROWSER_ACTION_REQUEST ActionRequest;

  while (TRUE) {
    UpdatePageId (CallbackData, FORM_MAIN_ID);

    ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    Status = gFormBrowser2->SendForm (
                             gFormBrowser2,
                             &CallbackData->BmmHiiHandle,
                             1,
                             NULL,
                             0,
                             NULL,
                             &ActionRequest
                             );
    if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
      EnableResetRequired ();
    }

    ReclaimStringDepository ();

    //
    // When this Formset returns, check if we are going to explore files.
    //
    if (INACTIVE_STATE != CallbackData->FeCurrentState) {
      UpdateFileExplorer (CallbackData, 0);

      ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
      Status = gFormBrowser2->SendForm (
                               gFormBrowser2,
                               &CallbackData->FeHiiHandle,
                               1,
                               NULL,
                               0,
                               NULL,
                               &ActionRequest
                               );
      if (ActionRequest == EFI_BROWSER_ACTION_REQUEST_RESET) {
        EnableResetRequired ();
      }

      CallbackData->FeCurrentState    = INACTIVE_STATE;
      CallbackData->FeDisplayContext  = UNKNOWN_CONTEXT;
      ReclaimStringDepository ();
    } else {
      break;
    }
  }

  return Status;
}

