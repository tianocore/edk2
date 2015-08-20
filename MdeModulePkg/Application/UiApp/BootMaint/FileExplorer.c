/** @file
  File explorer related functions.

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BootMaint.h"

/**
  Update the File Explore page.

  @param CallbackData    The BMM context data.
  @param MenuOption      Pointer to menu options to display.

**/
VOID
UpdateFileExplorePage (
  IN BMM_CALLBACK_DATA            *CallbackData,
  BM_MENU_OPTION                  *MenuOption
  )
{
  UINTN           Index;
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_FILE_CONTEXT *NewFileContext;
  EFI_FORM_ID     FormId;

  NewMenuEntry    = NULL;
  NewFileContext  = NULL;
  FormId          = 0;

  RefreshUpdateData ();
  mStartLabel->Number = FORM_FILE_EXPLORER_ID;

  for (Index = 0; Index < MenuOption->MenuNumber; Index++) {
    NewMenuEntry    = BOpt_GetMenuEntry (MenuOption, Index);
    NewFileContext  = (BM_FILE_CONTEXT *) NewMenuEntry->VariableContext;

    if (NewFileContext->IsBootLegacy) {
      continue;
    }

    if ((NewFileContext->IsDir) || (FileExplorerStateBootFromFile == CallbackData->FeCurrentState)) {
      //
      // Create Text opcode for directory, also create Text opcode for file in FileExplorerStateBootFromFile.
      //
      HiiCreateActionOpCode (
        mStartOpCodeHandle,
        (UINT16) (FILE_OPTION_OFFSET + Index),
        NewMenuEntry->DisplayStringToken,
        STRING_TOKEN (STR_NULL_STRING),
        EFI_IFR_FLAG_CALLBACK,
        0
        );
    } else {
      //
      // Create Goto opcode for file in FileExplorerStateAddBootOption or FileExplorerStateAddDriverOptionState.
      //
      if (FileExplorerStateAddBootOption == CallbackData->FeCurrentState) {
        FormId = FORM_BOOT_ADD_DESCRIPTION_ID;
      } else if (FileExplorerStateAddDriverOptionState == CallbackData->FeCurrentState) {
        FormId = FORM_DRIVER_ADD_FILE_DESCRIPTION_ID;
      }

      HiiCreateGotoOpCode (
        mStartOpCodeHandle,
        FormId,
        NewMenuEntry->DisplayStringToken,
        STRING_TOKEN (STR_NULL_STRING),
        EFI_IFR_FLAG_CALLBACK,
        (UINT16) (FILE_OPTION_OFFSET + Index)
        );
    }
  }

  HiiUpdateForm (
    CallbackData->FeHiiHandle,
    &mFileExplorerGuid,
    FORM_FILE_EXPLORER_ID,
    mStartOpCodeHandle, // Label FORM_FILE_EXPLORER_ID
    mEndOpCodeHandle    // LABEL_END
    );
}

/**
  Update the file explower page with the refershed file system.


  @param CallbackData    BMM context data
  @param KeyValue        Key value to identify the type of data to expect.

  @retval  TRUE           Inform the caller to create a callback packet to exit file explorer.
  @retval  FALSE          Indicate that there is no need to exit file explorer.

**/
BOOLEAN
UpdateFileExplorer (
  IN BMM_CALLBACK_DATA            *CallbackData,
  IN UINT16                       KeyValue
  )
{
  UINT16          FileOptionMask;
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_FILE_CONTEXT *NewFileContext;
  EFI_FORM_ID     FormId;
  BOOLEAN         ExitFileExplorer;
  EFI_STATUS      Status;

  NewMenuEntry      = NULL;
  NewFileContext    = NULL;
  ExitFileExplorer  = FALSE;

  FileOptionMask    = (UINT16) (FILE_OPTION_MASK & KeyValue);

  if (FileExplorerDisplayUnknown == CallbackData->FeDisplayContext) {
    //
    // First in, display file system.
    //
    BOpt_FreeMenu (&FsOptionMenu);
    BOpt_FindFileSystem (CallbackData);
    CreateMenuStringToken (CallbackData, CallbackData->FeHiiHandle, &FsOptionMenu);

    UpdateFileExplorePage (CallbackData, &FsOptionMenu);

    CallbackData->FeDisplayContext = FileExplorerDisplayFileSystem;
  } else {
    if (FileExplorerDisplayFileSystem == CallbackData->FeDisplayContext) {
      NewMenuEntry = BOpt_GetMenuEntry (&FsOptionMenu, FileOptionMask);
    } else if (FileExplorerDisplayDirectory == CallbackData->FeDisplayContext) {
      NewMenuEntry = BOpt_GetMenuEntry (&DirectoryMenu, FileOptionMask);
    }

    NewFileContext                  = (BM_FILE_CONTEXT *) NewMenuEntry->VariableContext;

    if (NewFileContext->IsDir ) {
      CallbackData->FeDisplayContext = FileExplorerDisplayDirectory;

      RemoveEntryList (&NewMenuEntry->Link);
      BOpt_FreeMenu (&DirectoryMenu);
      Status = BOpt_FindFiles (CallbackData, NewMenuEntry);
       if (EFI_ERROR (Status)) {
         ExitFileExplorer = TRUE;
         goto exit;
       }
      CreateMenuStringToken (CallbackData, CallbackData->FeHiiHandle, &DirectoryMenu);
      BOpt_DestroyMenuEntry (NewMenuEntry);

      UpdateFileExplorePage (CallbackData, &DirectoryMenu);

    } else {
      switch (CallbackData->FeCurrentState) {
      case FileExplorerStateBootFromFile:
        //
        // Here boot from file
        //
        BootThisFile (NewFileContext);
        ExitFileExplorer = TRUE;
        break;

      case FileExplorerStateAddBootOption:
      case FileExplorerStateAddDriverOptionState:
        if (FileExplorerStateAddBootOption == CallbackData->FeCurrentState) {
          FormId = FORM_BOOT_ADD_DESCRIPTION_ID;
          if (!CallbackData->FeFakeNvData.BootOptionChanged) {
            ZeroMem (CallbackData->FeFakeNvData.BootDescriptionData, sizeof (CallbackData->FeFakeNvData.BootDescriptionData));
            ZeroMem (CallbackData->FeFakeNvData.BootOptionalData, sizeof (CallbackData->FeFakeNvData.BootOptionalData));
          }
        } else {
          FormId = FORM_DRIVER_ADD_FILE_DESCRIPTION_ID;
          if (!CallbackData->FeFakeNvData.DriverOptionChanged) {
            ZeroMem (CallbackData->FeFakeNvData.DriverDescriptionData, sizeof (CallbackData->FeFakeNvData.DriverDescriptionData));
            ZeroMem (CallbackData->FeFakeNvData.DriverOptionalData, sizeof (CallbackData->FeFakeNvData.DriverOptionalData));
          }
        }

        CallbackData->MenuEntry = NewMenuEntry;
        CallbackData->LoadContext->FilePathList = ((BM_FILE_CONTEXT *) (CallbackData->MenuEntry->VariableContext))->DevicePath;

        //
        // Create Subtitle op-code for the display string of the option.
        //
        RefreshUpdateData ();
        mStartLabel->Number = FormId;

        HiiCreateSubTitleOpCode (
          mStartOpCodeHandle,
          NewMenuEntry->DisplayStringToken,
          0,
          0,
          0
          );

        HiiUpdateForm (
          CallbackData->FeHiiHandle,
          &mFileExplorerGuid,
          FormId,
          mStartOpCodeHandle, // Label FormId
          mEndOpCodeHandle    // LABEL_END
          );
        break;

      default:
        break;
      }
    }
  }
  exit:
  return ExitFileExplorer;
}

/**
  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job. Currently not implemented.

  @param[in]  This                Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration       A null-terminated Unicode string in
                                  <ConfigString> format.   
  @param[out] Progress            A pointer to a string filled in with the
                                  offset of the most recent '&' before the
                                  first failing name / value pair (or the
                                  beginn ing of the string if the failure
                                  is in the first name / value pair) or
                                  the terminating NULL if all was
                                  successful.

  @retval EFI_SUCCESS             The results have been distributed or are
                                  awaiting distribution.  
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the
                                  parts of the results that must be
                                  stored awaiting possible future
                                  protocols.
  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.
  @retval EFI_NOT_FOUND           Target for the specified routing data
                                  was not found.
**/
EFI_STATUS
EFIAPI
FileExplorerRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN CONST EFI_STRING                     Configuration,
  OUT EFI_STRING                          *Progress
  )
{
  EFI_STATUS                      Status;
  UINTN                           BufferSize;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *ConfigRouting;
  FILE_EXPLORER_NV_DATA           *FeData;
  BMM_CALLBACK_DATA               *Private;

  if (Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Configuration;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: there is no name for Name/Value storage, only GUID will be checked
  //
  if (!HiiIsConfigHdrMatch (Configuration, &mFileExplorerGuid, mFileExplorerStorageName)) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid, 
                  NULL, 
                  (VOID**) &ConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private = FE_CALLBACK_DATA_FROM_THIS (This);
  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (FILE_EXPLORER_NV_DATA );
  FeData = &Private->FeFakeNvData;

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  Status = ConfigRouting->ConfigToBlock (
                            ConfigRouting,
                            Configuration,
                            (UINT8 *) FeData,
                            &BufferSize,
                            Progress
                            );
  ASSERT_EFI_ERROR (Status);

  if (FeData->BootDescriptionData[0] != 0x00 || FeData->BootOptionalData[0] != 0x00) {
    Status = Var_UpdateBootOption (Private, FeData);
    Private->FeFakeNvData.BootOptionChanged = FALSE;
    if (EFI_ERROR (Status)) {
      return Status;
    }

    BOpt_GetBootOptions (Private);
    CreateMenuStringToken (Private, Private->FeHiiHandle, &BootOptionMenu);
  }

  if (FeData->DriverDescriptionData[0] != 0x00 || FeData->DriverOptionalData[0] != 0x00) {
    Status = Var_UpdateDriverOption (
              Private,
              Private->FeHiiHandle,
              FeData->DriverDescriptionData,
              FeData->DriverOptionalData,
              FeData->ForceReconnect
              );
    Private->FeFakeNvData.DriverOptionChanged = FALSE;
    if (EFI_ERROR (Status)) {
      return Status;
    }

    BOpt_GetDriverOptions (Private);
    CreateMenuStringToken (Private, Private->FeHiiHandle, &DriverOptionMenu);
  }

  return EFI_SUCCESS;
}

/**
  This function processes the results of changes in configuration.
  When user select a interactive opcode, this callback will be triggered.
  Based on the Question(QuestionId) that triggers the callback, the corresponding
  actions is performed. It handles:

  1) the addition of boot option.
  2) the addition of driver option.
  3) exit from file browser
  4) update of file content if a dir is selected.
  5) boot the file if a file is selected in "boot from file"


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR      The variable could not be saved.
  @retval  EFI_UNSUPPORTED       The specified Action is not supported by the callback.
  @retval  EFI_INVALID_PARAMETER If paramter Value or ActionRequest is NULL.
**/
EFI_STATUS
EFIAPI
FileExplorerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  BMM_CALLBACK_DATA     *Private;
  FILE_EXPLORER_NV_DATA *NvRamMap;
  EFI_STATUS            Status;

  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed.
    //
    return EFI_UNSUPPORTED;
  }

  Status         = EFI_SUCCESS;
  Private        = FE_CALLBACK_DATA_FROM_THIS (This);

  //
  // Retrieve uncommitted data from Form Browser
  //
  NvRamMap = &Private->FeFakeNvData;
  HiiGetBrowserData (&mFileExplorerGuid, mFileExplorerStorageName, sizeof (FILE_EXPLORER_NV_DATA), (UINT8 *) NvRamMap);

  if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((Value == NULL) || (ActionRequest == NULL)) {
      return EFI_INVALID_PARAMETER;
    }

    if (QuestionId == KEY_VALUE_SAVE_AND_EXIT_BOOT){
      NvRamMap->BootOptionChanged = FALSE;
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
    } else if (QuestionId == KEY_VALUE_SAVE_AND_EXIT_DRIVER){
      NvRamMap->DriverOptionChanged = FALSE;
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
    } else if (QuestionId == KEY_VALUE_NO_SAVE_AND_EXIT_BOOT) {
      //
      // Discard changes and exit formset
      //
      NvRamMap->BootOptionalData[0]     = 0x0000;
      NvRamMap->BootDescriptionData[0]  = 0x0000;
      NvRamMap->BootOptionChanged = FALSE;
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
    } else if ( QuestionId == KEY_VALUE_NO_SAVE_AND_EXIT_DRIVER){
      NvRamMap->BootOptionalData[0]     = 0x0000;
      NvRamMap->BootDescriptionData[0]  = 0x0000;
      NvRamMap->DriverOptionChanged = FALSE;
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
    } else if (QuestionId == KEY_VALUE_BOOT_DESCRIPTION || QuestionId == KEY_VALUE_BOOT_OPTION){
      NvRamMap->BootOptionChanged = TRUE;
    } else if (QuestionId == KEY_VALUE_DRIVER_DESCRIPTION || QuestionId == KEY_VALUE_DRIVER_OPTION){
      NvRamMap->DriverOptionChanged = TRUE;
    } else if (QuestionId < FILE_OPTION_OFFSET) {
      //
      // Exit File Explorer formset
      //
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGING) {
    if (Value == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    
    if (QuestionId >= FILE_OPTION_OFFSET) {
      UpdateFileExplorer (Private, QuestionId);
    }
  }

  //
  // Pass changed uncommitted data back to Form Browser
  //
  HiiSetBrowserData (&mFileExplorerGuid, mFileExplorerStorageName, sizeof (FILE_EXPLORER_NV_DATA), (UINT8 *) NvRamMap, NULL);

  return Status;
}
