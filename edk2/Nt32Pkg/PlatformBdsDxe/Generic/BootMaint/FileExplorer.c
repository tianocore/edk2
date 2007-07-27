/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FileExplorer.c
    
AgBStract:

  File explorer related functions.

--*/

#include "Generic/Bds.h"
#include "BootMaint.h"
#include "BdsPlatform.h"

VOID
UpdateFileExplorePage (
  IN BMM_CALLBACK_DATA            *CallbackData,
  BM_MENU_OPTION                  *MenuOption
  )
/*++
Routine Description:
  Update the File Explore page.

Arguments:
  MenuOption      - Pointer to menu options to display.

Returns:
  None.

--*/
{
  UINT8           *Location;
  UINTN           Index;
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_FILE_CONTEXT *NewFileContext;
  FORM_ID         FormId;

  NewMenuEntry    = NULL;
  NewFileContext  = NULL;
  FormId          = 0;

  //
  // Clean up file explore page.
  //
  RefreshUpdateData (FALSE, 0, FALSE, 0, 0xff);

  //
  // Remove all op-codes from dynamic page
  //
  CallbackData->Hii->UpdateForm (
                      CallbackData->Hii,
                      CallbackData->FeHiiHandle,
                      FORM_FILE_EXPLORER_ID,
                      FALSE,
                      UpdateData
                      );

  RefreshUpdateData (TRUE, (EFI_PHYSICAL_ADDRESS) (UINTN) CallbackData->FeCallbackHandle, FALSE, 0, 0);

  Location = (UINT8 *) &UpdateData->Data;

  for (Index = 0; Index < MenuOption->MenuNumber; Index++) {
    NewMenuEntry    = BOpt_GetMenuEntry (MenuOption, Index);
    NewFileContext  = (BM_FILE_CONTEXT *) NewMenuEntry->VariableContext;

    if (NewFileContext->IsBootLegacy) {
      continue;
    }

    if ((NewFileContext->IsDir) || (BOOT_FROM_FILE_STATE == CallbackData->FeCurrentState)) {
      //
      // Create Text opcode for directory, also create Text opcode for file in BOOT_FROM_FILE_STATE.
      //
      CreateTextOpCode (
        NewMenuEntry->DisplayStringToken,
        STR_NULL_STRING,
        STR_NULL_STRING,
        EFI_IFR_FLAG_INTERACTIVE | EFI_IFR_FLAG_NV_ACCESS,
        (UINT16) (FILE_OPTION_OFFSET + Index),
        Location
        );
    } else {
      //
      // Create Goto opcode for file in ADD_BOOT_OPTION_STATE or ADD_DRIVER_OPTION_STATE.
      //
      if (ADD_BOOT_OPTION_STATE == CallbackData->FeCurrentState) {
        FormId = FORM_BOOT_ADD_DESCRIPTION_ID;
      } else if (ADD_DRIVER_OPTION_STATE == CallbackData->FeCurrentState) {
        FormId = FORM_DRIVER_ADD_FILE_DESCRIPTION_ID;
      }

      CreateGotoOpCode (
        FormId,
        NewMenuEntry->DisplayStringToken,
        STRING_TOKEN (STR_NULL_STRING),
        EFI_IFR_FLAG_INTERACTIVE | EFI_IFR_FLAG_NV_ACCESS,
        (UINT16) (FILE_OPTION_OFFSET + Index),
        Location
        );
    }

    UpdateData->DataCount++;
    Location = Location + ((EFI_IFR_OP_HEADER *) Location)->Length;
  }

  CallbackData->Hii->UpdateForm (
                      CallbackData->Hii,
                      CallbackData->FeHiiHandle,
                      FORM_FILE_EXPLORER_ID,
                      TRUE,
                      UpdateData
                      );
}

BOOLEAN
UpdateFileExplorer (
  IN BMM_CALLBACK_DATA            *CallbackData,
  IN UINT16                       KeyValue
  )
/*++

Routine Description:
  Update the file explower page with the refershed file system.

Arguments:
  CallbackData  -   BMM context data
  KeyValue        - Key value to identify the type of data to expect.

Returns:
  TRUE          - Inform the caller to create a callback packet to exit file explorer.
  FALSE         - Indicate that there is no need to exit file explorer.

--*/
{
  UINT16          FileOptionMask;
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_FILE_CONTEXT *NewFileContext;
  FORM_ID         FormId;
  BOOLEAN         ExitFileExplorer;
  EFI_STATUS      Status;
  
  NewMenuEntry      = NULL;
  NewFileContext    = NULL;
  ExitFileExplorer  = FALSE;

  FileOptionMask    = (UINT16) (FILE_OPTION_MASK & KeyValue);

  if (UNKNOWN_CONTEXT == CallbackData->FeDisplayContext) {
    //
    // First in, display file system.
    //
    BOpt_FreeMenu (&FsOptionMenu);
    BOpt_FindFileSystem (CallbackData);
    CreateMenuStringToken (CallbackData, CallbackData->FeHiiHandle, &FsOptionMenu);

    UpdateFileExplorePage (CallbackData, &FsOptionMenu);

    CallbackData->FeDisplayContext = FILE_SYSTEM;
  } else {
    if (FILE_SYSTEM == CallbackData->FeDisplayContext) {
      NewMenuEntry = BOpt_GetMenuEntry (&FsOptionMenu, FileOptionMask);
    } else if (DIRECTORY == CallbackData->FeDisplayContext) {
      NewMenuEntry = BOpt_GetMenuEntry (&DirectoryMenu, FileOptionMask);
    }

    CallbackData->FeDisplayContext  = DIRECTORY;

    NewFileContext                  = (BM_FILE_CONTEXT *) NewMenuEntry->VariableContext;

    if (NewFileContext->IsDir ) {
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
      case BOOT_FROM_FILE_STATE:
        //
        // Here boot from file
        //
        BootThisFile (NewFileContext);
        ExitFileExplorer = TRUE;
        break;

      case ADD_BOOT_OPTION_STATE:
      case ADD_DRIVER_OPTION_STATE:
        if (ADD_BOOT_OPTION_STATE == CallbackData->FeCurrentState) {
          FormId = FORM_BOOT_ADD_DESCRIPTION_ID;
        } else {
          FormId = FORM_DRIVER_ADD_FILE_DESCRIPTION_ID;
        }

        CallbackData->MenuEntry = NewMenuEntry;
        CallbackData->LoadContext->FilePathList = ((BM_FILE_CONTEXT *) (CallbackData->MenuEntry->VariableContext))->DevicePath;

        //
        // Clean up file explore page.
        //
        RefreshUpdateData (FALSE, 0, FALSE, 0, 1);

        //
        // Remove the Subtitle op-code.
        //
        CallbackData->Hii->UpdateForm (
                            CallbackData->Hii,
                            CallbackData->FeHiiHandle,
                            FormId,
                            FALSE,
                            UpdateData
                            );

        //
        // Create Subtitle op-code for the display string of the option.
        //
        RefreshUpdateData (TRUE, (EFI_PHYSICAL_ADDRESS) (UINTN) CallbackData->FeCallbackHandle, FALSE, 0, 1);

        CreateSubTitleOpCode (
          NewMenuEntry->DisplayStringToken,
          &UpdateData->Data
          );

        CallbackData->Hii->UpdateForm (
                            CallbackData->Hii,
                            CallbackData->FeHiiHandle,
                            FormId,
                            TRUE,
                            UpdateData
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

EFI_STATUS
EFIAPI
FileExplorerCallback (
  IN EFI_FORM_CALLBACK_PROTOCOL       *This,
  IN UINT16                           KeyValue,
  IN EFI_IFR_DATA_ARRAY               *Data,
  OUT EFI_HII_CALLBACK_PACKET         **Packet
  )
/*++
Routine Description:
  Callback Function for file exploration and file interaction.

Arguments:
  This            - File explorer callback protocol pointer.     
  KeyValue        - Key value to identify the type of data to expect.
  Data            - A pointer to the data being sent to the original exporting driver.
  Packet          - A pointer to a packet of information which a driver passes back to the browser.

Returns:
  EFI_SUCCESS     - Callback ended successfully.
  Others          - Contain some errors.
  
--*/
{
  BMM_CALLBACK_DATA     *Private;
  FILE_EXPLORER_NV_DATA *NvRamMap;
  EFI_STATUS            Status;

  Status                          = EFI_SUCCESS;
  Private                         = FE_CALLBACK_DATA_FROM_THIS (This);
  UpdateData->FormCallbackHandle  = (EFI_PHYSICAL_ADDRESS) (UINTN) Private->FeCallbackHandle;
  NvRamMap                        = (FILE_EXPLORER_NV_DATA *) Data->NvRamMap;

  if (KEY_VALUE_SAVE_AND_EXIT == KeyValue) {
    //
    // Apply changes and exit formset.
    //
    if (ADD_BOOT_OPTION_STATE == Private->FeCurrentState) {
      Status = Var_UpdateBootOption (Private, NvRamMap);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      BOpt_GetBootOptions (Private);
      CreateMenuStringToken (Private, Private->FeHiiHandle, &BootOptionMenu);
    } else if (ADD_DRIVER_OPTION_STATE == Private->FeCurrentState) {
      Status = Var_UpdateDriverOption (
                Private,
                Private->FeHiiHandle,
                NvRamMap->DescriptionData,
                NvRamMap->OptionalData,
                NvRamMap->ForceReconnect
                );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      BOpt_GetDriverOptions (Private);
      CreateMenuStringToken (Private, Private->FeHiiHandle, &DriverOptionMenu);
    }

    CreateCallbackPacket (Packet, EXIT_REQUIRED | NV_NOT_CHANGED);
  } else if (KEY_VALUE_NO_SAVE_AND_EXIT == KeyValue) {
    //
    // Discard changes and exit formset.
    //
    NvRamMap->OptionalData[0]     = 0x0000;
    NvRamMap->DescriptionData[0]  = 0x0000;
    CreateCallbackPacket (Packet, EXIT_REQUIRED | NV_NOT_CHANGED);
  } else if (KeyValue < FILE_OPTION_OFFSET) {
    //
    // Exit File Explorer formset.
    //
    CreateCallbackPacket (Packet, EXIT_REQUIRED);
  } else {
    if (UpdateFileExplorer (Private, KeyValue)) {
      CreateCallbackPacket (Packet, EXIT_REQUIRED);
    }
  }

  return Status;
}
