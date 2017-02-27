/** @file
Variable operation that will be used by bootmaint

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BootMaintenanceManager.h"

/**
  Delete Boot Option that represent a Deleted state in BootOptionMenu.

  @retval EFI_SUCCESS   If all boot load option EFI Variables corresponding to  
                        BM_LOAD_CONTEXT marked for deletion is deleted.
  @retval EFI_NOT_FOUND If can not find the boot option want to be deleted.
  @return Others        If failed to update the "BootOrder" variable after deletion. 

**/
EFI_STATUS
Var_DelBootOption (
  VOID
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  EFI_STATUS      Status;
  UINTN           Index;
  UINTN           Index2;

  Index2  = 0;
  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&BootOptionMenu, (Index - Index2));
    if (NULL == NewMenuEntry) {
      return EFI_NOT_FOUND;
    }

    NewLoadContext = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    if (!NewLoadContext->Deleted) {
      continue;
    }

    Status = EfiBootManagerDeleteLoadOptionVariable (NewMenuEntry->OptionNumber,LoadOptionTypeBoot);
    if (EFI_ERROR (Status)) {
     return Status;
    }
    Index2++;
    //
    // If current Load Option is the same as BootNext,
    // must delete BootNext in order to make sure
    // there will be no panic on next boot
    //
    if (NewLoadContext->IsBootNext) {
      EfiLibDeleteVariable (L"BootNext", &gEfiGlobalVariableGuid);
    }

    RemoveEntryList (&NewMenuEntry->Link);
    BOpt_DestroyMenuEntry (NewMenuEntry);
    NewMenuEntry = NULL;
  }

  BootOptionMenu.MenuNumber -= Index2;

  return EFI_SUCCESS;
}

/**
  Delete Load Option that represent a Deleted state in DriverOptionMenu.

  @retval EFI_SUCCESS       Load Option is successfully updated.
  @retval EFI_NOT_FOUND     Fail to find the driver option want to be deleted.
  @return Other value than EFI_SUCCESS if failed to update "Driver Order" EFI
          Variable.

**/
EFI_STATUS
Var_DelDriverOption (
  VOID
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  EFI_STATUS      Status;
  UINTN           Index;
  UINTN           Index2;

  Index2  = 0;
  for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&DriverOptionMenu, (Index - Index2));
    if (NULL == NewMenuEntry) {
      return EFI_NOT_FOUND;
    }

    NewLoadContext = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    if (!NewLoadContext->Deleted) {
      continue;
    }
    Status = EfiBootManagerDeleteLoadOptionVariable (NewMenuEntry->OptionNumber,LoadOptionTypeDriver);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Index2++;

    RemoveEntryList (&NewMenuEntry->Link);
    BOpt_DestroyMenuEntry (NewMenuEntry);
    NewMenuEntry = NULL;
  }

  DriverOptionMenu.MenuNumber -= Index2;

  return EFI_SUCCESS;
}

/**
  This function delete and build multi-instance device path for
  specified type of console device.

  This function clear the EFI variable defined by ConsoleName and
  gEfiGlobalVariableGuid. It then build the multi-instance device
  path by appending the device path of the Console (In/Out/Err) instance 
  in ConsoleMenu. Then it scan all corresponding console device by
  scanning Terminal (built from device supporting Serial I/O instances)
  devices in TerminalMenu. At last, it save a EFI variable specifed
  by ConsoleName and gEfiGlobalVariableGuid.

  @param ConsoleName     The name for the console device type. They are
                         usually "ConIn", "ConOut" and "ErrOut".
  @param ConsoleMenu     The console memu which is a list of console devices.
  @param UpdatePageId    The flag specifying which type of console device
                         to be processed.

  @retval EFI_SUCCESS    The function complete successfully.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
Var_UpdateConsoleOption (
  IN UINT16                     *ConsoleName,
  IN BM_MENU_OPTION             *ConsoleMenu,
  IN UINT16                     UpdatePageId
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *ConDevicePath;
  BM_MENU_ENTRY             *NewMenuEntry;
  BM_CONSOLE_CONTEXT        *NewConsoleContext;
  BM_TERMINAL_CONTEXT       *NewTerminalContext;
  EFI_STATUS                Status;
  VENDOR_DEVICE_PATH        Vendor;
  EFI_DEVICE_PATH_PROTOCOL  *TerminalDevicePath;
  UINTN                     Index;

  GetEfiGlobalVariable2 (ConsoleName, (VOID**)&ConDevicePath, NULL);
  if (ConDevicePath != NULL) {
    EfiLibDeleteVariable (ConsoleName, &gEfiGlobalVariableGuid);
    FreePool (ConDevicePath);
    ConDevicePath = NULL;
  };

  //
  // First add all console input device from console input menu
  //
  for (Index = 0; Index < ConsoleMenu->MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (ConsoleMenu, Index);

    NewConsoleContext = (BM_CONSOLE_CONTEXT *) NewMenuEntry->VariableContext;
    if (NewConsoleContext->IsActive) {
      ConDevicePath = AppendDevicePathInstance (
                        ConDevicePath,
                        NewConsoleContext->DevicePath
                        );
    }
  }

  for (Index = 0; Index < TerminalMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&TerminalMenu, Index);

    NewTerminalContext = (BM_TERMINAL_CONTEXT *) NewMenuEntry->VariableContext;
    if (((NewTerminalContext->IsConIn != 0) && (UpdatePageId == FORM_CON_IN_ID)) ||
        ((NewTerminalContext->IsConOut != 0)  && (UpdatePageId == FORM_CON_OUT_ID)) ||
        ((NewTerminalContext->IsStdErr  != 0) && (UpdatePageId == FORM_CON_ERR_ID))
        ) {
      Vendor.Header.Type    = MESSAGING_DEVICE_PATH;
      Vendor.Header.SubType = MSG_VENDOR_DP;
      
      ASSERT (NewTerminalContext->TerminalType < (ARRAY_SIZE (TerminalTypeGuid)));
      CopyMem (
        &Vendor.Guid,
        &TerminalTypeGuid[NewTerminalContext->TerminalType],
        sizeof (EFI_GUID)
        );
      SetDevicePathNodeLength (&Vendor.Header, sizeof (VENDOR_DEVICE_PATH));
      TerminalDevicePath = AppendDevicePathNode (
                            NewTerminalContext->DevicePath,
                            (EFI_DEVICE_PATH_PROTOCOL *) &Vendor
                            );
      ASSERT (TerminalDevicePath != NULL);
      ChangeTerminalDevicePath (TerminalDevicePath, TRUE);
      ConDevicePath = AppendDevicePathInstance (
                        ConDevicePath,
                        TerminalDevicePath
                        );
    }
  }

  if (ConDevicePath != NULL) {
    Status = gRT->SetVariable (
                    ConsoleName,
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    GetDevicePathSize (ConDevicePath),
                    ConDevicePath
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;

}

/**
  This function delete and build multi-instance device path ConIn
  console device.

  @retval EFI_SUCCESS    The function complete successfully.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.
**/
EFI_STATUS
Var_UpdateConsoleInpOption (
  VOID
  )
{
  return Var_UpdateConsoleOption (L"ConIn", &ConsoleInpMenu, FORM_CON_IN_ID);
}

/**
  This function delete and build multi-instance device path ConOut
  console device.

  @retval EFI_SUCCESS    The function complete successfully.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.
**/
EFI_STATUS
Var_UpdateConsoleOutOption (
  VOID
  )
{
  return Var_UpdateConsoleOption (L"ConOut", &ConsoleOutMenu, FORM_CON_OUT_ID);
}

/**
  This function delete and build multi-instance device path ErrOut
  console device.

  @retval EFI_SUCCESS    The function complete successfully.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.  
**/
EFI_STATUS
Var_UpdateErrorOutOption (
  VOID
  )
{
  return Var_UpdateConsoleOption (L"ErrOut", &ConsoleErrMenu, FORM_CON_ERR_ID);
}

/**
  This function create a currently loaded Drive Option from 
  the BMM. It then appends this Driver Option to the end of 
  the "DriverOrder" list. It append this Driver Opotion to the end
  of DriverOptionMenu.

  @param CallbackData    The BMM context data.
  @param HiiHandle       The HII handle associated with the BMM formset.
  @param DescriptionData The description of this driver option.
  @param OptionalData    The optional load option.
  @param ForceReconnect  If to force reconnect.

  @retval other                Contain some errors when excuting this function.See function
                               EfiBootManagerInitializeLoadOption/EfiBootManagerAddLoadOptionVariabl
                               for detail return information.
  @retval EFI_SUCCESS          If function completes successfully.

**/
EFI_STATUS
Var_UpdateDriverOption (
  IN  BMM_CALLBACK_DATA         *CallbackData,
  IN  EFI_HII_HANDLE            HiiHandle,
  IN  UINT16                    *DescriptionData,
  IN  UINT16                    *OptionalData,
  IN  UINT8                     ForceReconnect
  )
{
  UINT16          Index;
  UINT16          DriverString[12];
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  BOOLEAN         OptionalDataExist;
  EFI_STATUS      Status;
  EFI_BOOT_MANAGER_LOAD_OPTION  LoadOption;
  UINT8                         *OptionalDesData;
  UINT32                        OptionalDataSize;

  OptionalDataExist = FALSE;
  OptionalDesData = NULL;
  OptionalDataSize = 0;

  Index             = BOpt_GetDriverOptionNumber ();
  UnicodeSPrint (
    DriverString,
    sizeof (DriverString),
    L"Driver%04x",
    Index
    );

  if (*DescriptionData == 0x0000) {
    StrCpyS (DescriptionData, MAX_MENU_NUMBER, DriverString);
  }

  if (*OptionalData != 0x0000) {
    OptionalDataExist = TRUE;
    OptionalDesData = (UINT8 *)OptionalData;
    OptionalDataSize = (UINT32)StrSize (OptionalData);
  }

  NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
  if (NULL == NewMenuEntry) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EfiBootManagerInitializeLoadOption (
             &LoadOption,
             Index,
             LoadOptionTypeDriver,
             LOAD_OPTION_ACTIVE | (ForceReconnect << 1),
             DescriptionData,
             CallbackData->LoadContext->FilePathList,
             OptionalDesData,
             OptionalDataSize
           );
  if (EFI_ERROR (Status)){
    return Status;
  }

  Status = EfiBootManagerAddLoadOptionVariable (&LoadOption,(UINTN) -1 );
  if (EFI_ERROR (Status)) {
    EfiBootManagerFreeLoadOption(&LoadOption);
    return Status;
  }

  NewLoadContext                  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
  NewLoadContext->Deleted         = FALSE;
  NewLoadContext->Attributes = LoadOption.Attributes;
  NewLoadContext->FilePathListLength = (UINT16)GetDevicePathSize (LoadOption.FilePath);

  NewLoadContext->Description = AllocateZeroPool (StrSize (DescriptionData));
  ASSERT (NewLoadContext->Description != NULL);
  NewMenuEntry->DisplayString = NewLoadContext->Description;
  CopyMem (
    NewLoadContext->Description,
    LoadOption.Description,
    StrSize (DescriptionData)
    );

  NewLoadContext->FilePathList = AllocateZeroPool (GetDevicePathSize (CallbackData->LoadContext->FilePathList));
  ASSERT (NewLoadContext->FilePathList != NULL);
  CopyMem (
    NewLoadContext->FilePathList,
    LoadOption.FilePath,
    GetDevicePathSize (CallbackData->LoadContext->FilePathList)
    );

  NewMenuEntry->HelpString    = UiDevicePathToStr (NewLoadContext->FilePathList);
  NewMenuEntry->OptionNumber  = Index;
  NewMenuEntry->DisplayStringToken = HiiSetString (HiiHandle, 0, NewMenuEntry->DisplayString, NULL);
  NewMenuEntry->HelpStringToken = HiiSetString (HiiHandle, 0, NewMenuEntry->HelpString, NULL);

  if (OptionalDataExist) {
    NewLoadContext->OptionalData = AllocateZeroPool (LoadOption.OptionalDataSize);
    ASSERT (NewLoadContext->OptionalData != NULL);
    CopyMem (
      NewLoadContext->OptionalData,
      LoadOption.OptionalData,
      LoadOption.OptionalDataSize
      );
  }

  InsertTailList (&DriverOptionMenu.Head, &NewMenuEntry->Link);
  DriverOptionMenu.MenuNumber++;

  EfiBootManagerFreeLoadOption(&LoadOption);

  return EFI_SUCCESS;
}

/**
  This function create a currently loaded Boot Option from 
  the BMM. It then appends this Boot Option to the end of 
  the "BootOrder" list. It also append this Boot Opotion to the end
  of BootOptionMenu.

  @param CallbackData    The BMM context data.

  @retval other                Contain some errors when excuting this function. See function
                               EfiBootManagerInitializeLoadOption/EfiBootManagerAddLoadOptionVariabl
                               for detail return information.
  @retval EFI_SUCCESS          If function completes successfully.

**/
EFI_STATUS
Var_UpdateBootOption (
  IN  BMM_CALLBACK_DATA              *CallbackData
  )
{
  UINT16          BootString[10];
  UINT16          Index;
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  BOOLEAN         OptionalDataExist;
  EFI_STATUS      Status;
  BMM_FAKE_NV_DATA  *NvRamMap;
  EFI_BOOT_MANAGER_LOAD_OPTION  LoadOption;
  UINT8                         *OptionalData;
  UINT32                        OptionalDataSize;

  OptionalDataExist = FALSE;
  NvRamMap = &CallbackData->BmmFakeNvData;
  OptionalData = NULL;
  OptionalDataSize = 0;

  Index = BOpt_GetBootOptionNumber () ;
  UnicodeSPrint (BootString, sizeof (BootString), L"Boot%04x", Index);

  if (NvRamMap->BootDescriptionData[0] == 0x0000) {
    StrCpyS (NvRamMap->BootDescriptionData, sizeof (NvRamMap->BootDescriptionData) / sizeof (NvRamMap->BootDescriptionData[0]), BootString);
  }

  if (NvRamMap->BootOptionalData[0] != 0x0000) {
    OptionalDataExist = TRUE;
    OptionalData = (UINT8 *)NvRamMap->BootOptionalData;
    OptionalDataSize = (UINT32)StrSize (NvRamMap->BootOptionalData);
  }

  NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
  if (NULL == NewMenuEntry) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EfiBootManagerInitializeLoadOption (
             &LoadOption,
             Index,
             LoadOptionTypeBoot,
             LOAD_OPTION_ACTIVE,
             NvRamMap->BootDescriptionData,
             CallbackData->LoadContext->FilePathList,
             OptionalData,
             OptionalDataSize
           );
  if (EFI_ERROR (Status)){
    return Status;
  }

  Status = EfiBootManagerAddLoadOptionVariable (&LoadOption,(UINTN) -1 );
  if (EFI_ERROR (Status)) {
    EfiBootManagerFreeLoadOption(&LoadOption);
    return Status;
  }

  NewLoadContext                  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
  NewLoadContext->Deleted         = FALSE;
  NewLoadContext->Attributes = LoadOption.Attributes;
  NewLoadContext->FilePathListLength = (UINT16) GetDevicePathSize (LoadOption.FilePath);

  NewLoadContext->Description = AllocateZeroPool (StrSize (NvRamMap->BootDescriptionData));
  ASSERT (NewLoadContext->Description != NULL);

  NewMenuEntry->DisplayString = NewLoadContext->Description;

  CopyMem (
    NewLoadContext->Description,
    LoadOption.Description,
    StrSize (NvRamMap->BootDescriptionData)
    );

  NewLoadContext->FilePathList = AllocateZeroPool (GetDevicePathSize (CallbackData->LoadContext->FilePathList));
  ASSERT (NewLoadContext->FilePathList != NULL);
  CopyMem (
    NewLoadContext->FilePathList,
    LoadOption.FilePath,
    GetDevicePathSize (CallbackData->LoadContext->FilePathList)
    );

  NewMenuEntry->HelpString    = UiDevicePathToStr (NewLoadContext->FilePathList);
  NewMenuEntry->OptionNumber  = Index;
  NewMenuEntry->DisplayStringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, NewMenuEntry->DisplayString, NULL);
  NewMenuEntry->HelpStringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, NewMenuEntry->HelpString, NULL);

  if (OptionalDataExist) {
    NewLoadContext->OptionalData = AllocateZeroPool (LoadOption.OptionalDataSize);
    ASSERT (NewLoadContext->OptionalData != NULL);
    CopyMem (
      NewLoadContext->OptionalData,
      LoadOption.OptionalData,
      LoadOption.OptionalDataSize
      );
  }

  InsertTailList (&BootOptionMenu.Head, &NewMenuEntry->Link);
  BootOptionMenu.MenuNumber++;

  EfiBootManagerFreeLoadOption(&LoadOption);

  return EFI_SUCCESS;
}

/**
  This function update the "BootNext" EFI Variable. If there is 
  no "BootNext" specified in BMM, this EFI Variable is deleted.
  It also update the BMM context data specified the "BootNext"
  vaule.

  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS    The function complete successfully.
  @return                The EFI variable can be saved. See gRT->SetVariable 
                         for detail return information.

**/
EFI_STATUS
Var_UpdateBootNext (
  IN BMM_CALLBACK_DATA            *CallbackData
  )
{
  BM_MENU_ENTRY     *NewMenuEntry;
  BM_LOAD_CONTEXT   *NewLoadContext;
  BMM_FAKE_NV_DATA  *CurrentFakeNVMap;
  UINT16            Index;
  EFI_STATUS        Status;

  Status            = EFI_SUCCESS;
  CurrentFakeNVMap  = &CallbackData->BmmFakeNvData;
  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewMenuEntry = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    ASSERT (NULL != NewMenuEntry);

    NewLoadContext              = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
    NewLoadContext->IsBootNext  = FALSE;
  }

  if (CurrentFakeNVMap->BootNext == NONE_BOOTNEXT_VALUE) {
    EfiLibDeleteVariable (L"BootNext", &gEfiGlobalVariableGuid);
    return EFI_SUCCESS;
  }

  NewMenuEntry = BOpt_GetMenuEntry (
                  &BootOptionMenu,
                  CurrentFakeNVMap->BootNext
                  );
  ASSERT (NewMenuEntry != NULL);

  NewLoadContext = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
  Status = gRT->SetVariable (
                  L"BootNext",
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  sizeof (UINT16),
                  &NewMenuEntry->OptionNumber
                  );
  NewLoadContext->IsBootNext              = TRUE;
  CallbackData->BmmOldFakeNVData.BootNext = CurrentFakeNVMap->BootNext;
  return Status;
}

/**
  This function update the "BootOrder" EFI Variable based on
  BMM Formset's NV map. It then refresh BootOptionMenu
  with the new "BootOrder" list.

  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS             The function complete successfully.
  @retval EFI_OUT_OF_RESOURCES    Not enough memory to complete the function.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
Var_UpdateBootOrder (
  IN BMM_CALLBACK_DATA            *CallbackData
  )
{
  EFI_STATUS  Status;
  UINT16      Index;
  UINT16      OrderIndex;
  UINT16      *BootOrder;
  UINTN       BootOrderSize;
  UINT16      OptionNumber;

  //
  // First check whether BootOrder is present in current configuration
  //
  GetEfiGlobalVariable2 (L"BootOrder", (VOID **) &BootOrder, &BootOrderSize);
  if (BootOrder == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ASSERT (BootOptionMenu.MenuNumber <= (sizeof (CallbackData->BmmFakeNvData.BootOptionOrder) / sizeof (CallbackData->BmmFakeNvData.BootOptionOrder[0])));

  //
  // OptionOrder is subset of BootOrder
  //
  for (OrderIndex = 0; (OrderIndex < BootOptionMenu.MenuNumber) && (CallbackData->BmmFakeNvData.BootOptionOrder[OrderIndex] != 0); OrderIndex++) {
    for (Index = OrderIndex; Index < BootOrderSize / sizeof (UINT16); Index++) {
      if ((BootOrder[Index] == (UINT16) (CallbackData->BmmFakeNvData.BootOptionOrder[OrderIndex] - 1)) && (OrderIndex != Index)) {
        OptionNumber = BootOrder[Index];
        CopyMem (&BootOrder[OrderIndex + 1], &BootOrder[OrderIndex], (Index - OrderIndex) * sizeof (UINT16));
        BootOrder[OrderIndex] = OptionNumber;
      }
    }
  }

  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  BootOrderSize,
                  BootOrder
                  );
  FreePool (BootOrder);
  
  BOpt_FreeMenu (&BootOptionMenu);
  BOpt_GetBootOptions (CallbackData);

  return Status;

}

/**
  This function update the "DriverOrder" EFI Variable based on
  BMM Formset's NV map. It then refresh DriverOptionMenu
  with the new "DriverOrder" list.

  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS           The function complete successfully.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory to complete the function.
  @return The EFI variable can not be saved. See gRT->SetVariable for detail return information.

**/
EFI_STATUS
Var_UpdateDriverOrder (
  IN BMM_CALLBACK_DATA            *CallbackData
  )
{
  EFI_STATUS  Status;
  UINT16      Index;
  UINT16      *DriverOrderList;
  UINT16      *NewDriverOrderList;
  UINTN       DriverOrderListSize;

  DriverOrderList     = NULL;
  DriverOrderListSize = 0;

  //
  // First check whether DriverOrder is present in current configuration
  //
  GetEfiGlobalVariable2 (L"DriverOrder", (VOID **) &DriverOrderList, &DriverOrderListSize);
  NewDriverOrderList = AllocateZeroPool (DriverOrderListSize);

  if (NewDriverOrderList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // If exists, delete it to hold new DriverOrder
  //
  if (DriverOrderList != NULL) {
    EfiLibDeleteVariable (L"DriverOrder", &gEfiGlobalVariableGuid);
    FreePool (DriverOrderList);
  }

  ASSERT (DriverOptionMenu.MenuNumber <= (sizeof (CallbackData->BmmFakeNvData.DriverOptionOrder) / sizeof (CallbackData->BmmFakeNvData.DriverOptionOrder[0])));
  for (Index = 0; Index < DriverOptionMenu.MenuNumber; Index++) {
    NewDriverOrderList[Index] = (UINT16) (CallbackData->BmmFakeNvData.DriverOptionOrder[Index] - 1);
  }

  Status = gRT->SetVariable (
                  L"DriverOrder",
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  DriverOrderListSize,
                  NewDriverOrderList
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BOpt_FreeMenu (&DriverOptionMenu);
  BOpt_GetDriverOptions (CallbackData);
  return EFI_SUCCESS;
}

/**
  Update the Text Mode of Console.

  @param CallbackData  The context data for BMM.

  @retval EFI_SUCCSS If the Text Mode of Console is updated.
  @return Other value if the Text Mode of Console is not updated.

**/
EFI_STATUS
Var_UpdateConMode (
  IN BMM_CALLBACK_DATA            *CallbackData
  )
{
  EFI_STATUS        Status;
  UINTN             Mode;
  CONSOLE_OUT_MODE  ModeInfo;

  Mode = CallbackData->BmmFakeNvData.ConsoleOutMode;

  Status = gST->ConOut->QueryMode (gST->ConOut, Mode, &(ModeInfo.Column), &(ModeInfo.Row));
  if (!EFI_ERROR(Status)) {
    Status = PcdSet32S (PcdSetupConOutColumn, (UINT32) ModeInfo.Column);
    if (!EFI_ERROR (Status)) {
      Status = PcdSet32S (PcdSetupConOutRow, (UINT32) ModeInfo.Row);
    }
  }

  return Status;
}
