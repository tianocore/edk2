/** @file
  HII Config Access protocol implementation of RamDiskDxe driver.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "RamDiskImpl.h"

CHAR16  mRamDiskStorageName[] = L"RAM_DISK_CONFIGURATION";

RAM_DISK_CONFIG_PRIVATE_DATA mRamDiskConfigPrivateDataTemplate = {
  RAM_DISK_CONFIG_PRIVATE_DATA_SIGNATURE,
  {
    RamDiskExtractConfig,
    RamDiskRouteConfig,
    RamDiskCallback
  }
};

HII_VENDOR_DEVICE_PATH       mRamDiskHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    RAM_DISK_FORM_SET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};


/**
  This function publish the RAM disk configuration Form.

  @param[in, out]  ConfigPrivateData
                             Points to RAM disk configuration private data.

  @retval EFI_SUCCESS             HII Form is installed successfully.
  @retval EFI_OUT_OF_RESOURCES    Not enough resource for HII Form installation.
  @retval Others                  Other errors as indicated.

**/
EFI_STATUS
InstallRamDiskConfigForm (
  IN OUT RAM_DISK_CONFIG_PRIVATE_DATA       *ConfigPrivateData
  )
{
  EFI_STATUS                      Status;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_HANDLE                      DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;

  DriverHandle = NULL;
  ConfigAccess = &ConfigPrivateData->ConfigAccess;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mRamDiskHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  ConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ConfigPrivateData->DriverHandle = DriverHandle;

  //
  // Publish the HII package list
  //
  HiiHandle = HiiAddPackages (
                &gRamDiskFormSetGuid,
                DriverHandle,
                RamDiskDxeStrings,
                RamDiskHiiBin,
                NULL
                );
  if (HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mRamDiskHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           ConfigAccess,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  ConfigPrivateData->HiiHandle = HiiHandle;

  return EFI_SUCCESS;
}


/**
  This function removes RAM disk configuration Form.

  @param[in, out]  ConfigPrivateData
                             Points to RAM disk configuration private data.

**/
VOID
UninstallRamDiskConfigForm (
  IN OUT RAM_DISK_CONFIG_PRIVATE_DATA       *ConfigPrivateData
  )
{
  //
  // Uninstall HII package list
  //
  if (ConfigPrivateData->HiiHandle != NULL) {
    HiiRemovePackages (ConfigPrivateData->HiiHandle);
    ConfigPrivateData->HiiHandle = NULL;
  }

  //
  // Uninstall HII Config Access Protocol
  //
  if (ConfigPrivateData->DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           ConfigPrivateData->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mRamDiskHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &ConfigPrivateData->ConfigAccess,
           NULL
           );
    ConfigPrivateData->DriverHandle = NULL;
  }

  FreePool (ConfigPrivateData);
}


/**
  Unregister all registered RAM disks.

**/
VOID
UnregisterAllRamDisks (
  VOID
  )
{
  LIST_ENTRY                      *Entry;
  LIST_ENTRY                      *NextEntry;
  RAM_DISK_PRIVATE_DATA           *PrivateData;

  if (!IsListEmpty(&RegisteredRamDisks)) {
    EFI_LIST_FOR_EACH_SAFE (Entry, NextEntry, &RegisteredRamDisks) {
      PrivateData = RAM_DISK_PRIVATE_FROM_THIS (Entry);

      gBS->UninstallMultipleProtocolInterfaces (
             PrivateData->Handle,
             &gEfiBlockIoProtocolGuid,
             &PrivateData->BlockIo,
             &gEfiBlockIo2ProtocolGuid,
             &PrivateData->BlockIo2,
             &gEfiDevicePathProtocolGuid,
             (EFI_DEVICE_PATH_PROTOCOL *) PrivateData->DevicePath,
             NULL
             );

      RemoveEntryList (&PrivateData->ThisInstance);

      if (RamDiskCreateHii == PrivateData->CreateMethod) {
        //
        // If a RAM disk is created within HII, then the RamDiskDxe driver
        // driver is responsible for freeing the allocated memory for the
        // RAM disk.
        //
        FreePool ((VOID *)(UINTN) PrivateData->StartingAddr);
      }


      gBS->DisconnectController (PrivateData->Handle, NULL, NULL);

      FreePool (PrivateData->DevicePath);
      FreePool (PrivateData);
      ListEntryNum--;
    }
  }
}


/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Request        A null-terminated Unicode string in
                             <ConfigRequest> format.
  @param[out] Progress       On return, points to a character in the Request
                             string. Points to the string's null terminator if
                             request was successful. Points to the most recent
                             '&' before the first failing name/value pair (or
                             the beginning of the string if the failure is in
                             the first name/value pair) if the request was not
                             successful.
  @param[out] Results        A null-terminated Unicode string in
                             <ConfigAltResp> format which has all values filled
                             in for the names in the Request string. String to
                             be allocated by the called function.

  @retval EFI_SUCCESS             The Results is filled with the requested
                                  values.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER   Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND           Routing data doesn't match any storage in
                                  this driver.

**/
EFI_STATUS
EFIAPI
RamDiskExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN CONST EFI_STRING                       Request,
       OUT EFI_STRING                       *Progress,
       OUT EFI_STRING                       *Results
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  RAM_DISK_CONFIGURATION           *Configuration;
  EFI_STRING                       ConfigRequest;
  EFI_STRING                       ConfigRequestHdr;
  RAM_DISK_CONFIG_PRIVATE_DATA     *ConfigPrivate;
  UINTN                            Size;
  BOOLEAN                          AllocatedRequest;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) &&
    !HiiIsConfigHdrMatch (Request, &gRamDiskFormSetGuid, mRamDiskStorageName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
  //
  ConfigPrivate = RAM_DISK_CONFIG_PRIVATE_FROM_THIS (This);
  BufferSize = sizeof (RAM_DISK_CONFIGURATION) + ListEntryNum;
  Configuration = AllocateZeroPool (BufferSize);
  if (Configuration == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (
                         &gRamDiskFormSetGuid,
                         mRamDiskStorageName,
                         ConfigPrivate->DriverHandle
                         );
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                (UINT8 *) &Configuration,
                                BufferSize,
                                Results,
                                Progress
                                );
  //
  // Free the allocated config request string and RAM disk configuration data.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }
  FreePool (Configuration);

  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}


/**
  This function processes the results of changes in configuration.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration  A null-terminated Unicode string in <ConfigResp>
                             format.
  @param[out] Progress       A pointer to a string filled in with the offset of
                             the most recent '&' before the first failing
                             name/value pair (or the beginning of the string if
                             the failure is in the first name/value pair) or
                             the terminating NULL if all was successful.

  @retval EFI_SUCCESS             The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER   Configuration is NULL.
  @retval EFI_NOT_FOUND           Routing data doesn't match any storage in
                                  this driver.

**/
EFI_STATUS
EFIAPI
RamDiskRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN CONST EFI_STRING                       Configuration,
       OUT EFI_STRING                       *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;
  if (!HiiIsConfigHdrMatch (Configuration, &gRamDiskFormSetGuid, mRamDiskStorageName)) {
    return EFI_NOT_FOUND;
  }

  *Progress = Configuration + StrLen (Configuration);

  return EFI_SUCCESS;
}


/**
  Allocate memory and register the RAM disk created within RamDiskDxe
  driver HII.

  @param[in] Size            If creating raw, size of the RAM disk to create.
                             If creating from file, zero.
  @param[in] FileHandle      If creating raw, NULL. If creating from file, the
                             file handle.

  @retval EFI_SUCCESS             RAM disk is created and registered.
  @retval EFI_OUT_OF_RESOURCES    Not enough storage is available to match the
                                  size required.

**/
EFI_STATUS
HiiCreateRamDisk (
  IN UINT64                                 Size,
  IN EFI_FILE_HANDLE                        FileHandle
  )
{
  EFI_STATUS                      Status;
  UINTN                           BufferSize;
  UINT64                          StartingAddr;
  EFI_INPUT_KEY                   Key;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  RAM_DISK_PRIVATE_DATA           *PrivateData;
  EFI_FILE_INFO                   *FileInformation;

  FileInformation = NULL;

  if (FileHandle != NULL) {
    //
    // Create from file.
    //
    FileInformation = FileInfo (FileHandle);
    if (NULL == FileInformation) {
      do {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"",
          L"Not enough memory to get the file information!",
          L"Press ENTER to continue ...",
          L"",
          NULL
          );
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Update the size of RAM disk according to the file size.
    //
    Size = FileInformation->FileSize;
  }

  StartingAddr = (UINTN) AllocatePool ((UINTN) Size);
  if (0 == StartingAddr) {
    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"",
        L"Not enough memory to create the RAM disk!",
        L"Press ENTER to continue ...",
        L"",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

    return EFI_OUT_OF_RESOURCES;
  }

  if (FileHandle != NULL) {
    //
    // Copy the file content to the RAM disk.
    //
    BufferSize = (UINTN) Size;
    FileHandle->Read (
                  FileHandle,
                  &BufferSize,
                  (VOID *)(UINTN) StartingAddr
                  );
    if (BufferSize != FileInformation->FileSize) {
      do {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"",
          L"File content read error!",
          L"Press ENTER to continue ...",
          L"",
          NULL
          );
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Register the newly created RAM disk.
  //
  Status = RamDiskRegister (
             StartingAddr,
             Size,
             &gEfiVirtualDiskGuid,
             NULL,
             &DevicePath
             );
  if (EFI_ERROR (Status)) {
    do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"",
        L"Fail to register the newly created RAM disk!",
        L"Press ENTER to continue ...",
        L"",
        NULL
        );
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

    return Status;
  }

  //
  // If RAM disk is created within HII, memory should be freed when the
  // RAM disk is unregisterd.
  //
  PrivateData = RAM_DISK_PRIVATE_FROM_THIS (RegisteredRamDisks.BackLink);
  PrivateData->CreateMethod = RamDiskCreateHii;

  return EFI_SUCCESS;
}


/**
  This function updates the registered RAM disks list on the main form.

  @param[in, out] ConfigPrivate
                             Private data for configurating hii data for RAM
                             disks.

**/
VOID
UpdateMainForm (
  IN OUT RAM_DISK_CONFIG_PRIVATE_DATA       *ConfigPrivate
  )
{
  VOID                      *StartOpCodeHandle;
  VOID                      *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL        *StartLabel;
  EFI_IFR_GUID_LABEL        *EndLabel;
  LIST_ENTRY                *Entry;
  UINTN                     Index;
  RAM_DISK_PRIVATE_DATA     *PrivateData;
  CHAR16                    *String;
  CHAR16                    RamDiskStr[128];
  EFI_STRING_ID             StringId;
  EFI_TPL                   OldTpl;

  //
  // Init OpCode Handle
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                        StartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number       = MAIN_LABEL_LIST_START;

  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number       = MAIN_LABEL_LIST_END;

  Index = 0;
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  EFI_LIST_FOR_EACH (Entry, &RegisteredRamDisks) {
    PrivateData = RAM_DISK_PRIVATE_FROM_THIS (Entry);
    String      = RamDiskStr;

    UnicodeSPrint (
      String,
      sizeof (RamDiskStr),
      L"  RAM Disk %d: [0x%lx, 0x%lx]\n",
      Index,
      PrivateData->StartingAddr,
      PrivateData->StartingAddr + PrivateData->Size
      );

    StringId = HiiSetString (ConfigPrivate->HiiHandle, 0, RamDiskStr, NULL);
    ASSERT (StringId != 0);

    HiiCreateCheckBoxOpCode (
      StartOpCodeHandle,
      (EFI_QUESTION_ID) (MAIN_CHECKBOX_QUESTION_ID_START + Index),
      RAM_DISK_CONFIGURATION_VARSTORE_ID,
      (UINT16) (RAM_DISK_LIST_VAR_OFFSET + Index),
      StringId,
      STRING_TOKEN (STR_RAM_DISK_LIST_HELP),
      0,
      0,
      NULL
      );

    Index++;
  }
  gBS->RestoreTPL (OldTpl);

  HiiUpdateForm (
    ConfigPrivate->HiiHandle,
    &gRamDiskFormSetGuid,
    MAIN_FORM_ID,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}


/**
  This function processes the results of changes in configuration.

  @param[in]  This           Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action         Specifies the type of action taken by the browser.
  @param[in]  QuestionId     A unique value which is sent to the original
                             exporting driver so that it can identify the type
                             of data to expect.
  @param[in]  Type           The type of value for the question.
  @param[in]  Value          A pointer to the data being sent to the original
                             exporting driver.
  @param[out] ActionRequest  On return, points to the action requested by the
                             callback function.

  @retval EFI_SUCCESS             The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES    Not enough storage is available to hold the
                                  variable and its data.
  @retval EFI_DEVICE_ERROR        The variable could not be saved.
  @retval EFI_UNSUPPORTED         The specified Action is not supported by the
                                  callback.

**/
EFI_STATUS
EFIAPI
RamDiskCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN     EFI_BROWSER_ACTION                 Action,
  IN     EFI_QUESTION_ID                    QuestionId,
  IN     UINT8                              Type,
  IN     EFI_IFR_TYPE_VALUE                 *Value,
     OUT EFI_BROWSER_ACTION_REQUEST         *ActionRequest
  )
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  RAM_DISK_PRIVATE_DATA           *PrivateData;
  RAM_DISK_CONFIG_PRIVATE_DATA    *ConfigPrivate;
  RAM_DISK_CONFIGURATION          *Configuration;
  EFI_DEVICE_PATH_PROTOCOL        *FileDevPath;
  EFI_FILE_HANDLE                 FileHandle;
  LIST_ENTRY                      *Entry;
  LIST_ENTRY                      *NextEntry;
  EFI_TPL                         OldTpl;

  if ((This == NULL) || (Value == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Action == EFI_BROWSER_ACTION_RETRIEVE) {
    Status = EFI_UNSUPPORTED;
    if (QuestionId == CREATE_RAW_SIZE_QUESTION_ID) {
      Value->u64 = EFI_PAGE_SIZE;
      Status = EFI_SUCCESS;
    }
    return Status;
  }

  if ((Action != EFI_BROWSER_ACTION_CHANGED) &&
      (Action != EFI_BROWSER_ACTION_CHANGING) &&
      (Action != EFI_BROWSER_ACTION_FORM_OPEN)) {
    return EFI_UNSUPPORTED;
  }

  ConfigPrivate = RAM_DISK_CONFIG_PRIVATE_FROM_THIS (This);

  //
  // Update the RAM disk list show at the main form first.
  //
  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    Status = EFI_UNSUPPORTED;
    if (QuestionId == MAIN_GOTO_FILE_EXPLORER_ID) {
      UpdateMainForm (ConfigPrivate);
      Status = EFI_SUCCESS;
    }
    return Status;
  }

  //
  // Get Browser data
  //
  Configuration = AllocateZeroPool (sizeof (RAM_DISK_CONFIGURATION) + ListEntryNum);
  if (Configuration == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_SUCCESS;

  HiiGetBrowserData (
    &gRamDiskFormSetGuid,
    mRamDiskStorageName,
    sizeof (RAM_DISK_CONFIGURATION) + ListEntryNum,
    (UINT8 *) Configuration
    );

  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    switch (QuestionId) {
    case MAIN_GOTO_FILE_EXPLORER_ID:
      Status = ChooseFile (NULL, NULL, NULL, &FileDevPath);
      if (EFI_ERROR (Status)) {
        break;
      }

      if (FileDevPath != NULL) {
        //
        // Open the file.
        //
        Status = OpenFileByDevicePath (
                   &FileDevPath,
                   &FileHandle,
                   EFI_FILE_MODE_READ,
                   0
                   );
        if (EFI_ERROR (Status)) {
          break;
        }

        //
        // Create from file, RAM disk size is zero. It will be updated
        // according to the file size.
        //
        Status = HiiCreateRamDisk (0, FileHandle);
        if (EFI_ERROR (Status)) {
          break;
        }

        //
        // Refresh the registered RAM disks list.
        //
        UpdateMainForm (ConfigPrivate);
      }

      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
      break;

    default:
      break;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    switch (QuestionId) {
    case MAIN_REMOVE_RD_QUESTION_ID:
      //
      // Remove the selected RAM disks
      //
      Index = 0;
      OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
      EFI_LIST_FOR_EACH_SAFE (Entry, NextEntry, &RegisteredRamDisks) {
        if (Configuration->RamDiskList[Index++] != 0) {
          PrivateData = RAM_DISK_PRIVATE_FROM_THIS (Entry);

          RamDiskUnregister (
            (EFI_DEVICE_PATH_PROTOCOL *) PrivateData->DevicePath
            );
        }
      }
      gBS->RestoreTPL (OldTpl);

      UpdateMainForm (ConfigPrivate);

      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
      ZeroMem (Configuration->RamDiskList, ListEntryNum);
      break;

    case CREATE_RAW_SUBMIT_QUESTION_ID:
      //
      // Create raw, FileHandle is NULL.
      //
      Status = HiiCreateRamDisk (Configuration->Size, NULL);
      if (EFI_ERROR (Status)) {
        break;
      }

      //
      // Refresh the registered RAM disks list.
      //
      UpdateMainForm (ConfigPrivate);

      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
      break;

    case CREATE_RAW_DISCARD_QUESTION_ID:
      *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
      break;

    default:
      break;
    }
  }

  if (!EFI_ERROR (Status)) {
    HiiSetBrowserData (
      &gRamDiskFormSetGuid,
      mRamDiskStorageName,
      sizeof (RAM_DISK_CONFIGURATION) + ListEntryNum,
      (UINT8 *) Configuration,
      NULL
      );
  }
  FreePool (Configuration);

  return EFI_SUCCESS;
}
