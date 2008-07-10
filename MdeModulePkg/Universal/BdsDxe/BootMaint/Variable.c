/** @file
  Variable operation that will be used by bootmaint

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
  Delete Boot Option that represent a Deleted state in BootOptionMenu.
  After deleting this boot option, call Var_ChangeBootOrder to
  make sure BootOrder is in valid state.

  @retval EFI_SUCCESS   If all boot load option EFI Variables corresponding to  
                        BM_LOAD_CONTEXT marked for deletion is deleted
  @return Others        If failed to update the "BootOrder" variable after deletion. 

**/
EFI_STATUS
Var_DelBootOption (
  VOID
  )
{
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  UINT16          BootString[10];
  EFI_STATUS      Status;
  UINTN           Index;
  UINTN           Index2;

  Status  = EFI_SUCCESS;
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

    UnicodeSPrint (
      BootString,
      sizeof (BootString),
      L"Boot%04x",
      NewMenuEntry->OptionNumber
      );

    EfiLibDeleteVariable (BootString, &gEfiGlobalVariableGuid);
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

  Status = Var_ChangeBootOrder ();
  return Status;
}

/**
  After any operation on Boot####, there will be a discrepancy in BootOrder.
  Since some are missing but in BootOrder, while some are present but are
  not reflected by BootOrder. Then a function rebuild BootOrder from
  scratch by content from BootOptionMenu is needed.


  

  @retval  EFI_SUCCESS  The boot order is updated successfully.
  @return               EFI_STATUS other than EFI_SUCCESS if failed to
                        Set the "BootOrder" EFI Variable.

**/
EFI_STATUS
Var_ChangeBootOrder (
  VOID
  )
{

  EFI_STATUS    Status;
  BM_MENU_ENTRY *NewMenuEntry;
  UINT16        *BootOrderList;
  UINT16        *BootOrderListPtr;
  UINTN         BootOrderListSize;
  UINTN         Index;

  BootOrderList     = NULL;
  BootOrderListSize = 0;

  //
  // First check whether BootOrder is present in current configuration
  //
  BootOrderList = BdsLibGetVariableAndSize (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    &BootOrderListSize
                    );

  //
  // If exists, delete it to hold new BootOrder
  //
  if (BootOrderList != NULL) {
    EfiLibDeleteVariable (L"BootOrder", &gEfiGlobalVariableGuid);
    SafeFreePool (BootOrderList);
    BootOrderList = NULL;
  }
  //
  // Maybe here should be some check method to ensure that
  // no new added boot options will be added
  // but the setup engine now will give only one callback
  // that is to say, user are granted only one chance to
  // decide whether the boot option will be added or not
  // there should be no indictor to show whether this
  // is a "new" boot option
  //
  BootOrderListSize = BootOptionMenu.MenuNumber;

  if (BootOrderListSize > 0) {
    BootOrderList = AllocateZeroPool (BootOrderListSize * sizeof (UINT16));
    ASSERT (BootOrderList != NULL);
    BootOrderListPtr = BootOrderList;

    //
    // Get all current used Boot#### from BootOptionMenu.
    // OptionNumber in each BM_LOAD_OPTION is really its
    // #### value.
    //
    for (Index = 0; Index < BootOrderListSize; Index++) {
      NewMenuEntry    = BOpt_GetMenuEntry (&BootOptionMenu, Index);
      *BootOrderList  = (UINT16) NewMenuEntry->OptionNumber;
      BootOrderList++;
    }

    BootOrderList = BootOrderListPtr;

    //
    // After building the BootOrderList, write it back
    //
    Status = gRT->SetVariable (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    BootOrderListSize * sizeof (UINT16),
                    BootOrderList
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  return EFI_SUCCESS;
}

/**
  Delete Load Option that represent a Deleted state in BootOptionMenu.
  After deleting this Driver option, call Var_ChangeDriverOrder to
  make sure DriverOrder is in valid state.


  

  @retval EFI_SUCCESS Load Option is successfully updated.
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
  UINT16          DriverString[12];
  EFI_STATUS      Status;
  UINTN           Index;
  UINTN           Index2;

  Status  = EFI_SUCCESS;
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

    UnicodeSPrint (
      DriverString,
      sizeof (DriverString),
      L"Driver%04x",
      NewMenuEntry->OptionNumber
      );

    EfiLibDeleteVariable (DriverString, &gEfiGlobalVariableGuid);
    Index2++;

    RemoveEntryList (&NewMenuEntry->Link);
    BOpt_DestroyMenuEntry (NewMenuEntry);
    NewMenuEntry = NULL;
  }

  DriverOptionMenu.MenuNumber -= Index2;

  Status = Var_ChangeDriverOrder ();
  return Status;
}

/**
  After any operation on Driver####, there will be a discrepancy in
  DriverOrder. Since some are missing but in DriverOrder, while some
  are present but are not reflected by DriverOrder. Then a function
  rebuild DriverOrder from scratch by content from DriverOptionMenu is
  needed.


  

  @retval  EFI_SUCCESS  The driver order is updated successfully.
  @return               EFI_STATUS other than EFI_SUCCESS if failed to
                                 Set the "DriverOrder" EFI Variable.

**/
EFI_STATUS
Var_ChangeDriverOrder (
  VOID
  )
{
  EFI_STATUS    Status;
  BM_MENU_ENTRY *NewMenuEntry;
  UINT16        *DriverOrderList;
  UINT16        *DriverOrderListPtr;
  UINTN         DriverOrderListSize;
  UINTN         Index;

  DriverOrderList     = NULL;
  DriverOrderListSize = 0;

  //
  // First check whether DriverOrder is present in current configuration
  //
  DriverOrderList = BdsLibGetVariableAndSize (
                      L"DriverOrder",
                      &gEfiGlobalVariableGuid,
                      &DriverOrderListSize
                      );

  //
  // If exists, delete it to hold new DriverOrder
  //
  if (DriverOrderList != NULL) {
    EfiLibDeleteVariable (L"DriverOrder", &gEfiGlobalVariableGuid);
    SafeFreePool (DriverOrderList);
    DriverOrderList = NULL;
  }

  DriverOrderListSize = DriverOptionMenu.MenuNumber;

  if (DriverOrderListSize > 0) {
    DriverOrderList = AllocateZeroPool (DriverOrderListSize * sizeof (UINT16));
    ASSERT (DriverOrderList != NULL);
    DriverOrderListPtr = DriverOrderList;

    //
    // Get all current used Driver#### from DriverOptionMenu.
    // OptionNumber in each BM_LOAD_OPTION is really its
    // #### value.
    //
    for (Index = 0; Index < DriverOrderListSize; Index++) {
      NewMenuEntry      = BOpt_GetMenuEntry (&DriverOptionMenu, Index);
      *DriverOrderList  = (UINT16) NewMenuEntry->OptionNumber;
      DriverOrderList++;
    }

    DriverOrderList = DriverOrderListPtr;

    //
    // After building the DriverOrderList, write it back
    //
    Status = gRT->SetVariable (
                    L"DriverOrder",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    DriverOrderListSize * sizeof (UINT16),
                    DriverOrderList
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  return EFI_SUCCESS;
}

/**
  Update the device path of "ConOut", "ConIn" and "ErrOut" 
  based on the new BaudRate, Data Bits, parity and Stop Bits
  set.

  

  

**/
VOID
Var_UpdateAllConsoleOption (
  VOID
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *OutDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *InpDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *ErrDevicePath;
  EFI_STATUS                Status;

  OutDevicePath = EfiLibGetVariable (L"ConOut", &gEfiGlobalVariableGuid);
  InpDevicePath = EfiLibGetVariable (L"ConIn", &gEfiGlobalVariableGuid);
  ErrDevicePath = EfiLibGetVariable (L"ErrOut", &gEfiGlobalVariableGuid);
  if (OutDevicePath != NULL) {
    ChangeVariableDevicePath (OutDevicePath);
    Status = gRT->SetVariable (
                    L"ConOut",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    GetDevicePathSize (OutDevicePath),
                    OutDevicePath
                    );
    ASSERT (!EFI_ERROR (Status));
  }

  if (InpDevicePath != NULL) {
    ChangeVariableDevicePath (InpDevicePath);
    Status = gRT->SetVariable (
                    L"ConIn",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    GetDevicePathSize (InpDevicePath),
                    InpDevicePath
                    );
    ASSERT (!EFI_ERROR (Status));
  }

  if (ErrDevicePath != NULL) {
    ChangeVariableDevicePath (ErrDevicePath);
    Status = gRT->SetVariable (
                    L"ErrOut",
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    GetDevicePathSize (ErrDevicePath),
                    ErrDevicePath
                    );
    ASSERT (!EFI_ERROR (Status));
  }
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
  @return                The EFI variable can be saved. See gRT->SetVariable 
                         for detail return information.

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

  ConDevicePath = EfiLibGetVariable (ConsoleName, &gEfiGlobalVariableGuid);
  if (ConDevicePath != NULL) {
    EfiLibDeleteVariable (ConsoleName, &gEfiGlobalVariableGuid);
    SafeFreePool (ConDevicePath);
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
  @return                The EFI variable can be saved. See gRT->SetVariable 
                         for detail return information.
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
  @return                The EFI variable can be saved. See gRT->SetVariable 
                         for detail return information.
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
  @return                The EFI variable can be saved. See gRT->SetVariable 
                         for detail return information.
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

  @retval EFI_OUT_OF_RESOURCES If not enought memory to complete the operation.
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
  UINT16          *DriverOrderList;
  UINT16          *NewDriverOrderList;
  UINT16          DriverString[12];
  UINTN           DriverOrderListSize;
  VOID            *Buffer;
  UINTN           BufferSize;
  UINT8           *Ptr;
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  BOOLEAN         OptionalDataExist;
  EFI_STATUS      Status;

  OptionalDataExist = FALSE;

  Index             = BOpt_GetDriverOptionNumber ();
  UnicodeSPrint (
    DriverString,
    sizeof (DriverString),
    L"Driver%04x",
    Index
    );

  if (*DescriptionData == 0x0000) {
    StrCpy (DescriptionData, DriverString);
  }

  BufferSize = sizeof (UINT32) + sizeof (UINT16) + StrSize (DescriptionData);
  BufferSize += GetDevicePathSize (CallbackData->LoadContext->FilePathList);

  if (*OptionalData != 0x0000) {
    OptionalDataExist = TRUE;
    BufferSize += StrSize (OptionalData);
  }

  Buffer = AllocateZeroPool (BufferSize);
  if (NULL == Buffer) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
  if (NULL == NewMenuEntry) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewLoadContext                  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
  NewLoadContext->Deleted         = FALSE;
  NewLoadContext->LoadOptionSize  = BufferSize;
  Ptr = (UINT8 *) Buffer;
  NewLoadContext->LoadOption = Ptr;
  *((UINT32 *) Ptr) = LOAD_OPTION_ACTIVE | (ForceReconnect << 1);
  NewLoadContext->Attributes = *((UINT32 *) Ptr);
  NewLoadContext->IsActive = TRUE;
  NewLoadContext->ForceReconnect = (BOOLEAN) (NewLoadContext->Attributes & LOAD_OPTION_FORCE_RECONNECT);

  Ptr += sizeof (UINT32);
  *((UINT16 *) Ptr) = (UINT16) GetDevicePathSize (CallbackData->LoadContext->FilePathList);
  NewLoadContext->FilePathListLength = *((UINT16 *) Ptr);

  Ptr += sizeof (UINT16);
  CopyMem (
    Ptr,
    DescriptionData,
    StrSize (DescriptionData)
    );

  NewLoadContext->Description = AllocateZeroPool (StrSize (DescriptionData));
  ASSERT (NewLoadContext->Description != NULL);
  NewMenuEntry->DisplayString = NewLoadContext->Description;
  CopyMem (
    NewLoadContext->Description,
    (VOID *) Ptr,
    StrSize (DescriptionData)
    );

  Ptr += StrSize (DescriptionData);
  CopyMem (
    Ptr,
    CallbackData->LoadContext->FilePathList,
    GetDevicePathSize (CallbackData->LoadContext->FilePathList)
    );

  NewLoadContext->FilePathList = AllocateZeroPool (GetDevicePathSize (CallbackData->LoadContext->FilePathList));
  ASSERT (NewLoadContext->FilePathList != NULL);

  CopyMem (
    NewLoadContext->FilePathList,
    (VOID *) Ptr,
    GetDevicePathSize (CallbackData->LoadContext->FilePathList)
    );

  NewMenuEntry->HelpString    = DevicePathToStr (NewLoadContext->FilePathList);
  NewMenuEntry->OptionNumber  = Index;
  NewMenuEntry->DisplayStringToken = GetStringTokenFromDepository (
                                      CallbackData,
                                      DriverOptionStrDepository
                                      );
  HiiLibNewString (HiiHandle, &NewMenuEntry->DisplayStringToken, NewMenuEntry->DisplayString);

  NewMenuEntry->HelpStringToken = GetStringTokenFromDepository (
                                    CallbackData,
                                    DriverOptionHelpStrDepository
                                    );
  HiiLibNewString (HiiHandle, &NewMenuEntry->HelpStringToken, NewMenuEntry->HelpString);

  if (OptionalDataExist) {
    Ptr += (UINT8) GetDevicePathSize (CallbackData->LoadContext->FilePathList);

    CopyMem (
      Ptr,
      OptionalData,
      StrSize (OptionalData)
      );
  }

  Status = gRT->SetVariable (
                  DriverString,
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  BufferSize,
                  Buffer
                  );
  ASSERT_EFI_ERROR (Status);
  DriverOrderList = BdsLibGetVariableAndSize (
                      L"DriverOrder",
                      &gEfiGlobalVariableGuid,
                      &DriverOrderListSize
                      );
  NewDriverOrderList = AllocateZeroPool (DriverOrderListSize + sizeof (UINT16));
  ASSERT (NewDriverOrderList != NULL);
  CopyMem (NewDriverOrderList, DriverOrderList, DriverOrderListSize);
  NewDriverOrderList[DriverOrderListSize / sizeof (UINT16)] = Index;
  if (DriverOrderList != NULL) {
    EfiLibDeleteVariable (L"DriverOrder", &gEfiGlobalVariableGuid);
  }

  Status = gRT->SetVariable (
                  L"DriverOrder",
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  DriverOrderListSize + sizeof (UINT16),
                  NewDriverOrderList
                  );
  ASSERT_EFI_ERROR (Status);
  SafeFreePool (DriverOrderList);
  DriverOrderList = NULL;
  SafeFreePool (NewDriverOrderList);
  NewDriverOrderList = NULL;
  InsertTailList (&DriverOptionMenu.Head, &NewMenuEntry->Link);
  DriverOptionMenu.MenuNumber++;

  *DescriptionData  = 0x0000;
  *OptionalData     = 0x0000;
  return EFI_SUCCESS;
}

/**
  This function create a currently loaded Boot Option from 
  the BMM. It then appends this Boot Option to the end of 
  the "BootOrder" list. It also append this Boot Opotion to the end
  of BootOptionMenu.

  @param CallbackData    The BMM context data.
  @param NvRamMap        The file explorer formset internal state.

  @retval EFI_OUT_OF_RESOURCES If not enought memory to complete the operation.
  @retval EFI_SUCCESS          If function completes successfully.

**/
EFI_STATUS
Var_UpdateBootOption (
  IN  BMM_CALLBACK_DATA                   *CallbackData,
  IN  FILE_EXPLORER_NV_DATA               *NvRamMap
  )
{
  UINT16          *BootOrderList;
  UINT16          *NewBootOrderList;
  UINTN           BootOrderListSize;
  UINT16          BootString[10];
  VOID            *Buffer;
  UINTN           BufferSize;
  UINT8           *Ptr;
  UINT16          Index;
  BM_MENU_ENTRY   *NewMenuEntry;
  BM_LOAD_CONTEXT *NewLoadContext;
  BOOLEAN         OptionalDataExist;
  EFI_STATUS      Status;

  OptionalDataExist = FALSE;

  Index = BOpt_GetBootOptionNumber () ;
  UnicodeSPrint (BootString, sizeof (BootString), L"Boot%04x", Index);

  if (NvRamMap->DescriptionData[0] == 0x0000) {
    StrCpy (NvRamMap->DescriptionData, BootString);
  }

  BufferSize = sizeof (UINT32) + sizeof (UINT16) + StrSize (NvRamMap->DescriptionData);
  BufferSize += GetDevicePathSize (CallbackData->LoadContext->FilePathList);

  if (NvRamMap->OptionalData[0] != 0x0000) {
    OptionalDataExist = TRUE;
    BufferSize += StrSize (NvRamMap->OptionalData);
  }

  Buffer = AllocateZeroPool (BufferSize);
  if (NULL == Buffer) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
  if (NULL == NewMenuEntry) {
    return EFI_OUT_OF_RESOURCES;
  }

  NewLoadContext                  = (BM_LOAD_CONTEXT *) NewMenuEntry->VariableContext;
  NewLoadContext->Deleted         = FALSE;
  NewLoadContext->LoadOptionSize  = BufferSize;
  Ptr = (UINT8 *) Buffer;
  NewLoadContext->LoadOption = Ptr;
  *((UINT32 *) Ptr) = LOAD_OPTION_ACTIVE;
  NewLoadContext->Attributes = *((UINT32 *) Ptr);
  NewLoadContext->IsActive = TRUE;
  NewLoadContext->ForceReconnect = (BOOLEAN) (NewLoadContext->Attributes & LOAD_OPTION_FORCE_RECONNECT);

  Ptr += sizeof (UINT32);
  *((UINT16 *) Ptr) = (UINT16) GetDevicePathSize (CallbackData->LoadContext->FilePathList);
  NewLoadContext->FilePathListLength = *((UINT16 *) Ptr);
  Ptr += sizeof (UINT16);

  CopyMem (
    Ptr,
    NvRamMap->DescriptionData,
    StrSize (NvRamMap->DescriptionData)
    );

  NewLoadContext->Description = AllocateZeroPool (StrSize (NvRamMap->DescriptionData));
  ASSERT (NewLoadContext->Description != NULL);

  NewMenuEntry->DisplayString = NewLoadContext->Description;
  CopyMem (
    NewLoadContext->Description,
    (VOID *) Ptr,
    StrSize (NvRamMap->DescriptionData)
    );

  Ptr += StrSize (NvRamMap->DescriptionData);
  CopyMem (
    Ptr,
    CallbackData->LoadContext->FilePathList,
    GetDevicePathSize (CallbackData->LoadContext->FilePathList)
    );

  NewLoadContext->FilePathList = AllocateZeroPool (GetDevicePathSize (CallbackData->LoadContext->FilePathList));
  ASSERT (NewLoadContext->FilePathList != NULL);

  CopyMem (
    NewLoadContext->FilePathList,
    (VOID *) Ptr,
    GetDevicePathSize (CallbackData->LoadContext->FilePathList)
    );

  NewMenuEntry->HelpString    = DevicePathToStr (NewLoadContext->FilePathList);
  NewMenuEntry->OptionNumber  = Index;
  NewMenuEntry->DisplayStringToken = GetStringTokenFromDepository (
                                      CallbackData,
                                      BootOptionStrDepository
                                      );
  HiiLibNewString (CallbackData->FeHiiHandle, &NewMenuEntry->DisplayStringToken, NewMenuEntry->DisplayString);

  NewMenuEntry->HelpStringToken = GetStringTokenFromDepository (
                                    CallbackData,
                                    BootOptionHelpStrDepository
                                    );
  HiiLibNewString (CallbackData->FeHiiHandle, &NewMenuEntry->HelpStringToken, NewMenuEntry->HelpString);

  if (OptionalDataExist) {
    Ptr += (UINT8) GetDevicePathSize (CallbackData->LoadContext->FilePathList);

    CopyMem (Ptr, NvRamMap->OptionalData, StrSize (NvRamMap->OptionalData));
  }

  Status = gRT->SetVariable (
                  BootString,
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  BufferSize,
                  Buffer
                  );
  ASSERT_EFI_ERROR (Status);

  BootOrderList = BdsLibGetVariableAndSize (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    &BootOrderListSize
                    );

  NewBootOrderList = AllocateZeroPool (BootOrderListSize + sizeof (UINT16));
  ASSERT (NewBootOrderList != NULL);
  CopyMem (NewBootOrderList, BootOrderList, BootOrderListSize);
  NewBootOrderList[BootOrderListSize / sizeof (UINT16)] = Index;

  if (BootOrderList != NULL) {
    EfiLibDeleteVariable (L"BootOrder", &gEfiGlobalVariableGuid);
  }

  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  BootOrderListSize + sizeof (UINT16),
                  NewBootOrderList
                  );
  ASSERT_EFI_ERROR (Status);

  SafeFreePool (BootOrderList);
  BootOrderList = NULL;
  SafeFreePool (NewBootOrderList);
  NewBootOrderList = NULL;
  InsertTailList (&BootOptionMenu.Head, &NewMenuEntry->Link);
  BootOptionMenu.MenuNumber++;

  NvRamMap->DescriptionData[0]  = 0x0000;
  NvRamMap->OptionalData[0]     = 0x0000;
  return EFI_SUCCESS;
}

/**
  This function update the "BootNext" EFI Variable. If there is 
  no "BootNex" specified in BMM, this EFI Variable is deleted.
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

  if (CurrentFakeNVMap->BootNext == BootOptionMenu.MenuNumber) {
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

  @retval EFI_SUCCESS    The function complete successfully.
  @retval EFI_SUCCESS    Not enough memory to complete the function.
  @return                The EFI variable can be saved. See gRT->SetVariable 
                         for detail return information.

**/
EFI_STATUS
Var_UpdateBootOrder (
  IN BMM_CALLBACK_DATA            *CallbackData
  )
{
  EFI_STATUS  Status;
  UINT16      Index;
  UINT16      *BootOrderList;
  UINT16      *NewBootOrderList;
  UINTN       BootOrderListSize;

  BootOrderList     = NULL;
  BootOrderListSize = 0;

  //
  // First check whether BootOrder is present in current configuration
  //
  BootOrderList = BdsLibGetVariableAndSize (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    &BootOrderListSize
                    );

  NewBootOrderList = AllocateZeroPool (BootOrderListSize);
  if (NewBootOrderList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // If exists, delete it to hold new BootOrder
  //
  if (BootOrderList != NULL) {
    EfiLibDeleteVariable (L"BootOrder", &gEfiGlobalVariableGuid);
  }

  for (Index = 0; Index < BootOptionMenu.MenuNumber; Index++) {
    NewBootOrderList[Index] = (UINT16) (CallbackData->BmmFakeNvData.OptionOrder[Index] - 1);
  }

  Status = gRT->SetVariable (
                  L"BootOrder",
                  &gEfiGlobalVariableGuid,
                  VAR_FLAG,
                  BootOrderListSize,
                  NewBootOrderList
                  );
  SafeFreePool (BootOrderList);
  SafeFreePool (NewBootOrderList);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BOpt_FreeMenu (&BootOptionMenu);
  BOpt_GetBootOptions (CallbackData);

  return EFI_SUCCESS;

}

/**
  This function update the "DriverOrder" EFI Variable based on
  BMM Formset's NV map. It then refresh DriverOptionMenu
  with the new "DriverOrder" list.

  @param CallbackData    The BMM context data.

  @retval EFI_SUCCESS    The function complete successfully.
  @retval EFI_SUCCESS    Not enough memory to complete the function.
  @return                The EFI variable can be saved. See gRT->SetVariable 
                         for detail return information.

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
  DriverOrderList = BdsLibGetVariableAndSize (
                      L"DriverOrder",
                      &gEfiGlobalVariableGuid,
                      &DriverOrderListSize
                      );

  NewDriverOrderList = AllocateZeroPool (DriverOrderListSize);

  if (NewDriverOrderList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // If exists, delete it to hold new DriverOrder
  //
  if (DriverOrderList != NULL) {
    EfiLibDeleteVariable (L"DriverOrder", &gEfiGlobalVariableGuid);
  }

  for (Index = 0; Index < DriverOrderListSize; Index++) {
    NewDriverOrderList[Index] = (UINT16) (CallbackData->BmmFakeNvData.OptionOrder[Index] - 1);
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

  SafeFreePool (DriverOrderList);

  BOpt_FreeMenu (&DriverOptionMenu);
  BOpt_GetDriverOptions (CallbackData);
  return EFI_SUCCESS;
}

/**
  Update the legacy BBS boot option. L"LegacyDevOrder" and EfiLegacyDevOrderGuid EFI Variable
  is udpated with the new Legacy Boot order. The EFI Variable of "Boot####" and gEfiGlobalVariableGuid
  is also updated.

  @param CallbackData    The context data for BMM.

  @return EFI_SUCCESS    The function completed successfully.
  @retval EFI_NOT_FOUND  If L"LegacyDevOrder" and EfiLegacyDevOrderGuid EFI Variable can be found.

**/
EFI_STATUS
Var_UpdateBBSOption (
  IN BMM_CALLBACK_DATA            *CallbackData
  )
{
  UINTN                       Index;
  UINTN                       Index2;
  VOID                        *BootOptionVar;
  CHAR16                      VarName[100];
  UINTN                       OptionSize;
  UINT8                       *Ptr;
  EFI_STATUS                  Status;
  CHAR16                      DescString[100];
  CHAR8                       DescAsciiString[100];
  UINTN                       NewOptionSize;
  UINT8                       *NewOptionPtr;
  UINT8                       *TempPtr;
  UINT32                      *Attribute;
  BM_MENU_OPTION              *OptionMenu;
  BM_LEGACY_DEVICE_CONTEXT    *LegacyDeviceContext;
  UINT8                       *LegacyDev;
  UINT8                       *VarData;
  UINTN                       VarSize;
  BM_MENU_ENTRY               *NewMenuEntry;
  BM_LEGACY_DEV_ORDER_CONTEXT *DevOrder;
  UINT8                       *OriginalPtr;
  UINT8                       *DisMap;
  UINTN                       Pos;
  UINTN                       Bit;
  UINT16                      *NewOrder;
  UINT16                      Tmp;

  LegacyDeviceContext = NULL;
  DisMap              = NULL;
  NewOrder            = NULL;

  if (FORM_SET_FD_ORDER_ID == CallbackData->BmmPreviousPageId) {
    OptionMenu            = (BM_MENU_OPTION *) &LegacyFDMenu;
    LegacyDev             = CallbackData->BmmFakeNvData.LegacyFD;
    CallbackData->BbsType = BBS_FLOPPY;
  } else {
    if (FORM_SET_HD_ORDER_ID == CallbackData->BmmPreviousPageId) {
      OptionMenu            = (BM_MENU_OPTION *) &LegacyHDMenu;
      LegacyDev             = CallbackData->BmmFakeNvData.LegacyHD;
      CallbackData->BbsType = BBS_HARDDISK;
    } else {
      if (FORM_SET_CD_ORDER_ID == CallbackData->BmmPreviousPageId) {
        OptionMenu            = (BM_MENU_OPTION *) &LegacyCDMenu;
        LegacyDev             = CallbackData->BmmFakeNvData.LegacyCD;
        CallbackData->BbsType = BBS_CDROM;
      } else {
        if (FORM_SET_NET_ORDER_ID == CallbackData->BmmPreviousPageId) {
          OptionMenu            = (BM_MENU_OPTION *) &LegacyNETMenu;
          LegacyDev             = CallbackData->BmmFakeNvData.LegacyNET;
          CallbackData->BbsType = BBS_EMBED_NETWORK;
        } else {
          OptionMenu            = (BM_MENU_OPTION *) &LegacyBEVMenu;
          LegacyDev             = CallbackData->BmmFakeNvData.LegacyBEV;
          CallbackData->BbsType = BBS_BEV_DEVICE;
        }
      }
    }
  }

  DisMap  = CallbackData->BmmOldFakeNVData.DisableMap;
  Status  = EFI_SUCCESS;

  //
  // Find the first device's context
  // If all devices are disabled( 0xFF == LegacyDev[0]), LegacyDeviceContext can be set to any VariableContext
  // because we just use it to fill the desc string, and user can not see the string in UI
  //
  for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
    NewMenuEntry        = BOpt_GetMenuEntry (OptionMenu, Index);
    LegacyDeviceContext = (BM_LEGACY_DEVICE_CONTEXT *) NewMenuEntry->VariableContext;
    if (0xFF != LegacyDev[0] && LegacyDev[0] == LegacyDeviceContext->Index) {
      DEBUG ((DEBUG_ERROR, "DescStr: %s\n", LegacyDeviceContext->Description));
      break;
    }
  }
  //
  // Update the Variable "LegacyDevOrder"
  //
  VarData = (UINT8 *) BdsLibGetVariableAndSize (
                        VAR_LEGACY_DEV_ORDER,
                        &EfiLegacyDevOrderGuid,
                        &VarSize
                        );

  if (NULL == VarData) {
    return EFI_NOT_FOUND;
  }

  OriginalPtr = VarData;
  DevOrder    = (BM_LEGACY_DEV_ORDER_CONTEXT *) VarData;

  while (VarData < VarData + VarSize) {
    if (DevOrder->BbsType == CallbackData->BbsType) {
      break;
    }

    VarData += sizeof (BBS_TYPE);
    VarData += *(UINT16 *) VarData;
    DevOrder = (BM_LEGACY_DEV_ORDER_CONTEXT *) VarData;
  }

  if (VarData >= VarData + VarSize) {
    SafeFreePool (OriginalPtr);
    return EFI_NOT_FOUND;
  }

  NewOrder = (UINT16 *) AllocateZeroPool (DevOrder->Length - sizeof (UINT16));
  if (NULL == NewOrder) {
    SafeFreePool (VarData);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < OptionMenu->MenuNumber; Index++) {
    if (0xFF == LegacyDev[Index]) {
      break;
    }

    NewOrder[Index] = LegacyDev[Index];
  }
  //
  // Only the enable/disable state of each boot device with same device type can be changed,
  // so we can count on the index information in DevOrder.
  // DisMap bit array is the only reliable source to check a device's en/dis state,
  // so we use DisMap to set en/dis state of each item in NewOrder array
  //
  for (Index2 = 0; Index2 < OptionMenu->MenuNumber; Index2++) {
    Tmp = *(UINT16 *) ((UINT8 *) DevOrder + sizeof (BBS_TYPE) + sizeof (UINT16) + Index2 * sizeof (UINT16));
    Tmp &= 0xFF;
    Pos = Tmp / 8;
    Bit = 7 - (Tmp % 8);
    if ((DisMap[Pos] & (1 << Bit)) != 0) {
      NewOrder[Index] = (UINT16) (0xFF00 | Tmp);
      Index++;
    }
  }

  CopyMem (
    (UINT8 *) DevOrder + sizeof (BBS_TYPE) + sizeof (UINT16),
    NewOrder,
    DevOrder->Length - sizeof (UINT16)
    );
  SafeFreePool (NewOrder);

  Status = gRT->SetVariable (
                  VAR_LEGACY_DEV_ORDER,
                  &EfiLegacyDevOrderGuid,
                  VAR_FLAG,
                  VarSize,
                  OriginalPtr
                  );

  SafeFreePool (OriginalPtr);

  //
  // Update Optional Data of Boot####
  //
  BootOptionVar = GetLegacyBootOptionVar (CallbackData->BbsType, &Index, &OptionSize);

  if (NULL != BootOptionVar) {
    CopyMem (
      DescString,
      LegacyDeviceContext->Description,
      StrSize (LegacyDeviceContext->Description)
      );

    UnicodeToAscii (DescString, StrSize (DescString), DescAsciiString);

    NewOptionSize = sizeof (UINT32) + sizeof (UINT16) + StrSize (DescString) +
                    sizeof (BBS_BBS_DEVICE_PATH);
    NewOptionSize += AsciiStrLen (DescAsciiString) +
                    EFI_END_DEVICE_PATH_LENGTH + sizeof (BBS_TABLE) + sizeof (UINT16);

    UnicodeSPrint (VarName, 100, L"Boot%04x", Index);

    Ptr       = BootOptionVar;

    Attribute = (UINT32 *) Ptr;
    *Attribute |= LOAD_OPTION_ACTIVE;
    if (0xFF == LegacyDev[0]) {
      //
      // Disable this legacy boot option
      //
      *Attribute &= ~LOAD_OPTION_ACTIVE;
    }

    Ptr += sizeof (UINT32);

    Ptr += sizeof (UINT16);
    Ptr += StrSize ((CHAR16 *) Ptr);

    NewOptionPtr = AllocateZeroPool (NewOptionSize);
    if (NULL == NewOptionPtr) {
      return EFI_OUT_OF_RESOURCES;
    }

    TempPtr = NewOptionPtr;

    //
    // Attribute
    //
    CopyMem (
      TempPtr,
      BootOptionVar,
      sizeof (UINT32)
      );

    TempPtr += sizeof (UINT32);

    //
    // BBS device path Length
    //
    *((UINT16 *) TempPtr) = (UINT16) (sizeof (BBS_BBS_DEVICE_PATH) +
                         AsciiStrLen (DescAsciiString) +
                         EFI_END_DEVICE_PATH_LENGTH);

    TempPtr += sizeof (UINT16);

    //
    // Description string
    //
    CopyMem (
      TempPtr,
      DescString,
      StrSize (DescString)
      );

    TempPtr += StrSize (DescString);

    //
    // BBS device path
    //
    CopyMem (
      TempPtr,
      Ptr,
      sizeof (BBS_BBS_DEVICE_PATH)
      );

    CopyMem (
      ((BBS_BBS_DEVICE_PATH*) TempPtr)->String,
      DescAsciiString,
      AsciiStrSize (DescAsciiString)
      );

    SetDevicePathNodeLength (
          (EFI_DEVICE_PATH_PROTOCOL *) TempPtr,
          sizeof (BBS_BBS_DEVICE_PATH) + AsciiStrLen (DescAsciiString)
          );

    TempPtr += sizeof (BBS_BBS_DEVICE_PATH) + AsciiStrLen (DescAsciiString);

    //
    // End node
    //
    CopyMem (
      TempPtr,
      EndDevicePath,
      EFI_END_DEVICE_PATH_LENGTH
      );
    TempPtr += EFI_END_DEVICE_PATH_LENGTH;

    //
    // Now TempPtr point to optional data, i.e. Bbs Table
    //
    CopyMem (
      TempPtr,
      LegacyDeviceContext->BbsTable,
      sizeof (BBS_TABLE)
      );

    //
    // Now TempPtr point to BBS index
    //
    TempPtr += sizeof (BBS_TABLE);
    *((UINT16 *) TempPtr) = (UINT16) LegacyDeviceContext->Index;

    Status = gRT->SetVariable (
                    VarName,
                    &gEfiGlobalVariableGuid,
                    VAR_FLAG,
                    NewOptionSize,
                    NewOptionPtr
                    );

    SafeFreePool (NewOptionPtr);
    SafeFreePool (BootOptionVar);
  }

  BOpt_GetBootOptions (CallbackData);
  return Status;
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
  if (EFI_ERROR(Status)) {
    ModeInfo.Column = 80;
    ModeInfo.Row = 25;
  }

  Status = gRT->SetVariable (
                  VAR_CON_OUT_MODE,
                  &gEfiGenericPlatformVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  sizeof (CONSOLE_OUT_MODE),
                  &ModeInfo
                  );

  return Status;
}
