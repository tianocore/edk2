/** @file
  Provide boot option support for Application "BootMaint"

  Include file system navigation, system handle selection

  Boot option manipulation

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BootMaintenanceManager.h"

///
/// Define the maximum characters that will be accepted.
///
#define MAX_CHAR  480

/**

  Check whether a reset is needed, if reset is needed, Popup a menu to notice user.

**/
VOID
BmmSetupResetReminder (
  VOID
  )
{
  EFI_INPUT_KEY                           Key;
  CHAR16                                  *StringBuffer1;
  CHAR16                                  *StringBuffer2;
  EFI_STATUS                              Status;
  EDKII_FORM_BROWSER_EXTENSION2_PROTOCOL  *FormBrowserEx2;

  //
  // Use BrowserEx2 protocol to check whether reset is required.
  //
  Status = gBS->LocateProtocol (&gEdkiiFormBrowserEx2ProtocolGuid, NULL, (VOID **)&FormBrowserEx2);

  //
  // check any reset required change is applied? if yes, reset system
  //
  if (!EFI_ERROR (Status) && FormBrowserEx2->IsResetRequired ()) {
    StringBuffer1 = AllocateZeroPool (MAX_CHAR * sizeof (CHAR16));
    ASSERT (StringBuffer1 != NULL);
    StringBuffer2 = AllocateZeroPool (MAX_CHAR * sizeof (CHAR16));
    ASSERT (StringBuffer2 != NULL);
    StrCpyS (StringBuffer1, MAX_CHAR, L"Configuration changed. Reset to apply it Now.");
    StrCpyS (StringBuffer2, MAX_CHAR, L"Press ENTER to reset");
    //
    // Popup a menu to notice user
    //
    do {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, StringBuffer1, StringBuffer2, NULL);
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

    FreePool (StringBuffer1);
    FreePool (StringBuffer2);

    gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  }
}

/**
  Create a menu entry by given menu type.

  @param MenuType        The Menu type to be created.

  @retval NULL           If failed to create the menu.
  @return the new menu entry.

**/
BM_MENU_ENTRY *
BOpt_CreateMenuEntry (
  UINTN  MenuType
  )
{
  BM_MENU_ENTRY  *MenuEntry;
  UINTN          ContextSize;

  //
  // Get context size according to menu type
  //
  switch (MenuType) {
    case BM_LOAD_CONTEXT_SELECT:
      ContextSize = sizeof (BM_LOAD_CONTEXT);
      break;

    case BM_FILE_CONTEXT_SELECT:
      ContextSize = sizeof (BM_FILE_CONTEXT);
      break;

    case BM_CONSOLE_CONTEXT_SELECT:
      ContextSize = sizeof (BM_CONSOLE_CONTEXT);
      break;

    case BM_TERMINAL_CONTEXT_SELECT:
      ContextSize = sizeof (BM_TERMINAL_CONTEXT);
      break;

    case BM_HANDLE_CONTEXT_SELECT:
      ContextSize = sizeof (BM_HANDLE_CONTEXT);
      break;

    default:
      ContextSize = 0;
      break;
  }

  if (ContextSize == 0) {
    return NULL;
  }

  //
  // Create new menu entry
  //
  MenuEntry = AllocateZeroPool (sizeof (BM_MENU_ENTRY));
  if (MenuEntry == NULL) {
    return NULL;
  }

  MenuEntry->VariableContext = AllocateZeroPool (ContextSize);
  if (MenuEntry->VariableContext == NULL) {
    FreePool (MenuEntry);
    return NULL;
  }

  MenuEntry->Signature        = BM_MENU_ENTRY_SIGNATURE;
  MenuEntry->ContextSelection = MenuType;
  return MenuEntry;
}

/**
  Free up all resource allocated for a BM_MENU_ENTRY.

  @param MenuEntry   A pointer to BM_MENU_ENTRY.

**/
VOID
BOpt_DestroyMenuEntry (
  BM_MENU_ENTRY  *MenuEntry
  )
{
  BM_LOAD_CONTEXT      *LoadContext;
  BM_FILE_CONTEXT      *FileContext;
  BM_CONSOLE_CONTEXT   *ConsoleContext;
  BM_TERMINAL_CONTEXT  *TerminalContext;
  BM_HANDLE_CONTEXT    *HandleContext;

  //
  //  Select by the type in Menu entry for current context type
  //
  switch (MenuEntry->ContextSelection) {
    case BM_LOAD_CONTEXT_SELECT:
      LoadContext = (BM_LOAD_CONTEXT *)MenuEntry->VariableContext;
      FreePool (LoadContext->FilePathList);
      if (LoadContext->OptionalData != NULL) {
        FreePool (LoadContext->OptionalData);
      }

      FreePool (LoadContext);
      break;

    case BM_FILE_CONTEXT_SELECT:
      FileContext = (BM_FILE_CONTEXT *)MenuEntry->VariableContext;

      if (!FileContext->IsRoot) {
        FreePool (FileContext->DevicePath);
      } else {
        if (FileContext->FHandle != NULL) {
          FileContext->FHandle->Close (FileContext->FHandle);
        }
      }

      if (FileContext->FileName != NULL) {
        FreePool (FileContext->FileName);
      }

      if (FileContext->Info != NULL) {
        FreePool (FileContext->Info);
      }

      FreePool (FileContext);
      break;

    case BM_CONSOLE_CONTEXT_SELECT:
      ConsoleContext = (BM_CONSOLE_CONTEXT *)MenuEntry->VariableContext;
      FreePool (ConsoleContext->DevicePath);
      FreePool (ConsoleContext);
      break;

    case BM_TERMINAL_CONTEXT_SELECT:
      TerminalContext = (BM_TERMINAL_CONTEXT *)MenuEntry->VariableContext;
      FreePool (TerminalContext->DevicePath);
      FreePool (TerminalContext);
      break;

    case BM_HANDLE_CONTEXT_SELECT:
      HandleContext = (BM_HANDLE_CONTEXT *)MenuEntry->VariableContext;
      FreePool (HandleContext);
      break;

    default:
      break;
  }

  FreePool (MenuEntry->DisplayString);
  if (MenuEntry->HelpString != NULL) {
    FreePool (MenuEntry->HelpString);
  }

  FreePool (MenuEntry);
}

/**
  Get the Menu Entry from the list in Menu Entry List.

  If MenuNumber is great or equal to the number of Menu
  Entry in the list, then ASSERT.

  @param MenuOption      The Menu Entry List to read the menu entry.
  @param MenuNumber      The index of Menu Entry.

  @return The Menu Entry.

**/
BM_MENU_ENTRY *
BOpt_GetMenuEntry (
  BM_MENU_OPTION  *MenuOption,
  UINTN           MenuNumber
  )
{
  BM_MENU_ENTRY  *NewMenuEntry;
  UINTN          Index;
  LIST_ENTRY     *List;

  ASSERT (MenuNumber < MenuOption->MenuNumber);

  List = MenuOption->Head.ForwardLink;
  for (Index = 0; Index < MenuNumber; Index++) {
    List = List->ForwardLink;
  }

  NewMenuEntry = CR (List, BM_MENU_ENTRY, Link, BM_MENU_ENTRY_SIGNATURE);

  return NewMenuEntry;
}

/**
  Free resources allocated in Allocate Rountine.

  @param FreeMenu        Menu to be freed
**/
VOID
BOpt_FreeMenu (
  BM_MENU_OPTION  *FreeMenu
  )
{
  BM_MENU_ENTRY  *MenuEntry;

  while (!IsListEmpty (&FreeMenu->Head)) {
    MenuEntry = CR (
                  FreeMenu->Head.ForwardLink,
                  BM_MENU_ENTRY,
                  Link,
                  BM_MENU_ENTRY_SIGNATURE
                  );
    RemoveEntryList (&MenuEntry->Link);
    BOpt_DestroyMenuEntry (MenuEntry);
  }

  FreeMenu->MenuNumber = 0;
}

/**

  Build the BootOptionMenu according to BootOrder Variable.
  This Routine will access the Boot#### to get EFI_LOAD_OPTION.

  @param CallbackData The BMM context data.

  @return EFI_NOT_FOUND Fail to find "BootOrder" variable.
  @return EFI_SUCESS    Success build boot option menu.

**/
EFI_STATUS
BOpt_GetBootOptions (
  IN  BMM_CALLBACK_DATA  *CallbackData
  )
{
  UINTN                         Index;
  UINT16                        BootString[10];
  UINT8                         *LoadOptionFromVar;
  UINTN                         BootOptionSize;
  BOOLEAN                       BootNextFlag;
  UINT16                        *BootOrderList;
  UINTN                         BootOrderListSize;
  UINT16                        *BootNext;
  UINTN                         BootNextSize;
  BM_MENU_ENTRY                 *NewMenuEntry;
  BM_LOAD_CONTEXT               *NewLoadContext;
  UINT8                         *LoadOptionPtr;
  UINTN                         StringSize;
  UINTN                         OptionalDataSize;
  UINT8                         *LoadOptionEnd;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  UINTN                         MenuCount;
  UINT8                         *Ptr;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOption;
  UINTN                         BootOptionCount;

  MenuCount         = 0;
  BootOrderListSize = 0;
  BootNextSize      = 0;
  BootOrderList     = NULL;
  BootNext          = NULL;
  LoadOptionFromVar = NULL;
  BOpt_FreeMenu (&BootOptionMenu);
  InitializeListHead (&BootOptionMenu.Head);

  //
  // Get the BootOrder from the Var
  //
  GetEfiGlobalVariable2 (L"BootOrder", (VOID **)&BootOrderList, &BootOrderListSize);
  if (BootOrderList == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Get the BootNext from the Var
  //
  GetEfiGlobalVariable2 (L"BootNext", (VOID **)&BootNext, &BootNextSize);
  if (BootNext != NULL) {
    if (BootNextSize != sizeof (UINT16)) {
      FreePool (BootNext);
      BootNext = NULL;
    }
  }

  BootOption = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);
  for (Index = 0; Index < BootOrderListSize / sizeof (UINT16); Index++) {
    //
    // Don't display the hidden/inactive boot option
    //
    if (((BootOption[Index].Attributes & LOAD_OPTION_HIDDEN) != 0) || ((BootOption[Index].Attributes & LOAD_OPTION_ACTIVE) == 0)) {
      continue;
    }

    UnicodeSPrint (BootString, sizeof (BootString), L"Boot%04x", BootOrderList[Index]);
    //
    //  Get all loadoptions from the VAR
    //
    GetEfiGlobalVariable2 (BootString, (VOID **)&LoadOptionFromVar, &BootOptionSize);
    if (LoadOptionFromVar == NULL) {
      continue;
    }

    if (BootNext != NULL) {
      BootNextFlag = (BOOLEAN)(*BootNext == BootOrderList[Index]);
    } else {
      BootNextFlag = FALSE;
    }

    NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
    ASSERT (NULL != NewMenuEntry);

    NewLoadContext = (BM_LOAD_CONTEXT *)NewMenuEntry->VariableContext;

    LoadOptionPtr = LoadOptionFromVar;
    LoadOptionEnd = LoadOptionFromVar + BootOptionSize;

    NewMenuEntry->OptionNumber = BootOrderList[Index];
    NewLoadContext->Deleted    = FALSE;
    NewLoadContext->IsBootNext = BootNextFlag;

    //
    // Is a Legacy Device?
    //
    Ptr = (UINT8 *)LoadOptionFromVar;

    //
    // Attribute = *(UINT32 *)Ptr;
    //
    Ptr += sizeof (UINT32);

    //
    // FilePathSize = *(UINT16 *)Ptr;
    //
    Ptr += sizeof (UINT16);

    //
    // Description = (CHAR16 *)Ptr;
    //
    Ptr += StrSize ((CHAR16 *)Ptr);

    //
    // Now Ptr point to Device Path
    //
    DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)Ptr;
    if ((BBS_DEVICE_PATH == DevicePath->Type) && (BBS_BBS_DP == DevicePath->SubType)) {
      NewLoadContext->IsLegacy = TRUE;
    } else {
      NewLoadContext->IsLegacy = FALSE;
    }

    //
    // LoadOption is a pointer type of UINT8
    // for easy use with following LOAD_OPTION
    // embedded in this struct
    //

    NewLoadContext->Attributes = *(UINT32 *)LoadOptionPtr;

    LoadOptionPtr += sizeof (UINT32);

    NewLoadContext->FilePathListLength = *(UINT16 *)LoadOptionPtr;
    LoadOptionPtr                     += sizeof (UINT16);

    StringSize = StrSize ((UINT16 *)LoadOptionPtr);

    NewLoadContext->Description = AllocateZeroPool (StrSize ((UINT16 *)LoadOptionPtr));
    ASSERT (NewLoadContext->Description != NULL);
    StrCpyS (NewLoadContext->Description, StrSize ((UINT16 *)LoadOptionPtr) / sizeof (UINT16), (UINT16 *)LoadOptionPtr);

    ASSERT (NewLoadContext->Description != NULL);
    NewMenuEntry->DisplayString      = NewLoadContext->Description;
    NewMenuEntry->DisplayStringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, NewMenuEntry->DisplayString, NULL);

    LoadOptionPtr += StringSize;

    NewLoadContext->FilePathList = AllocateZeroPool (NewLoadContext->FilePathListLength);
    ASSERT (NewLoadContext->FilePathList != NULL);
    CopyMem (
      NewLoadContext->FilePathList,
      (EFI_DEVICE_PATH_PROTOCOL *)LoadOptionPtr,
      NewLoadContext->FilePathListLength
      );

    NewMenuEntry->HelpString      = UiDevicePathToStr (NewLoadContext->FilePathList);
    NewMenuEntry->HelpStringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, NewMenuEntry->HelpString, NULL);

    LoadOptionPtr += NewLoadContext->FilePathListLength;

    if (LoadOptionPtr < LoadOptionEnd) {
      OptionalDataSize = BootOptionSize -
                         sizeof (UINT32) -
                         sizeof (UINT16) -
                         StringSize -
                         NewLoadContext->FilePathListLength;

      NewLoadContext->OptionalData = AllocateZeroPool (OptionalDataSize);
      ASSERT (NewLoadContext->OptionalData != NULL);
      CopyMem (
        NewLoadContext->OptionalData,
        LoadOptionPtr,
        OptionalDataSize
        );
    }

    InsertTailList (&BootOptionMenu.Head, &NewMenuEntry->Link);
    MenuCount++;
    FreePool (LoadOptionFromVar);
  }

  EfiBootManagerFreeLoadOptions (BootOption, BootOptionCount);

  if (BootNext != NULL) {
    FreePool (BootNext);
  }

  if (BootOrderList != NULL) {
    FreePool (BootOrderList);
  }

  BootOptionMenu.MenuNumber = MenuCount;
  return EFI_SUCCESS;
}

/**

  Find drivers that will be added as Driver#### variables from handles
  in current system environment
  All valid handles in the system except those consume SimpleFs, LoadFile
  are stored in DriverMenu for future use.

  @retval EFI_SUCCESS The function complets successfully.
  @return Other value if failed to build the DriverMenu.

**/
EFI_STATUS
BOpt_FindDrivers (
  VOID
  )
{
  UINTN                            NoDevicePathHandles;
  EFI_HANDLE                       *DevicePathHandle;
  UINTN                            Index;
  EFI_STATUS                       Status;
  BM_MENU_ENTRY                    *NewMenuEntry;
  BM_HANDLE_CONTEXT                *NewHandleContext;
  EFI_HANDLE                       CurHandle;
  UINTN                            OptionNumber;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFs;
  EFI_LOAD_FILE_PROTOCOL           *LoadFile;

  SimpleFs = NULL;
  LoadFile = NULL;

  InitializeListHead (&DriverMenu.Head);

  //
  // At first, get all handles that support Device Path
  // protocol which is the basic requirement for
  // Driver####
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDevicePathProtocolGuid,
                  NULL,
                  &NoDevicePathHandles,
                  &DevicePathHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OptionNumber = 0;
  for (Index = 0; Index < NoDevicePathHandles; Index++) {
    CurHandle = DevicePathHandle[Index];

    Status = gBS->HandleProtocol (
                    CurHandle,
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID **)&SimpleFs
                    );
    if (Status == EFI_SUCCESS) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    CurHandle,
                    &gEfiLoadFileProtocolGuid,
                    (VOID **)&LoadFile
                    );
    if (Status == EFI_SUCCESS) {
      continue;
    }

    NewMenuEntry = BOpt_CreateMenuEntry (BM_HANDLE_CONTEXT_SELECT);
    if (NULL == NewMenuEntry) {
      FreePool (DevicePathHandle);
      return EFI_OUT_OF_RESOURCES;
    }

    NewHandleContext                 = (BM_HANDLE_CONTEXT *)NewMenuEntry->VariableContext;
    NewHandleContext->Handle         = CurHandle;
    NewHandleContext->DevicePath     = DevicePathFromHandle (CurHandle);
    NewMenuEntry->DisplayString      = UiDevicePathToStr (NewHandleContext->DevicePath);
    NewMenuEntry->DisplayStringToken = HiiSetString (mBmmCallbackInfo->BmmHiiHandle, 0, NewMenuEntry->DisplayString, NULL);
    NewMenuEntry->HelpString         = NULL;
    NewMenuEntry->HelpStringToken    = NewMenuEntry->DisplayStringToken;
    NewMenuEntry->OptionNumber       = OptionNumber;
    OptionNumber++;
    InsertTailList (&DriverMenu.Head, &NewMenuEntry->Link);
  }

  if (DevicePathHandle != NULL) {
    FreePool (DevicePathHandle);
  }

  DriverMenu.MenuNumber = OptionNumber;
  return EFI_SUCCESS;
}

/**

  Get the Option Number that has not been allocated for use.

  @param Type  The type of Option.

  @return The available Option Number.

**/
UINT16
BOpt_GetOptionNumber (
  CHAR16  *Type
  )
{
  UINT16  *OrderList;
  UINTN   OrderListSize;
  UINTN   Index;
  CHAR16  StrTemp[20];
  UINT16  *OptionBuffer;
  UINT16  OptionNumber;
  UINTN   OptionSize;

  OrderListSize = 0;
  OrderList     = NULL;
  OptionNumber  = 0;
  Index         = 0;

  UnicodeSPrint (StrTemp, sizeof (StrTemp), L"%sOrder", Type);

  GetEfiGlobalVariable2 (StrTemp, (VOID **)&OrderList, &OrderListSize);
  for (OptionNumber = 0; ; OptionNumber++) {
    if (OrderList != NULL) {
      for (Index = 0; Index < OrderListSize / sizeof (UINT16); Index++) {
        if (OptionNumber == OrderList[Index]) {
          break;
        }
      }
    }

    if (Index < OrderListSize / sizeof (UINT16)) {
      //
      // The OptionNumber occurs in the OrderList, continue to use next one
      //
      continue;
    }

    UnicodeSPrint (StrTemp, sizeof (StrTemp), L"%s%04x", Type, (UINTN)OptionNumber);
    DEBUG ((DEBUG_ERROR, "Option = %s\n", StrTemp));
    GetEfiGlobalVariable2 (StrTemp, (VOID **)&OptionBuffer, &OptionSize);
    if (NULL == OptionBuffer) {
      //
      // The Boot[OptionNumber] / Driver[OptionNumber] NOT occurs, we found it
      //
      break;
    }
  }

  return OptionNumber;
}

/**

  Get the Option Number for Boot#### that does not used.

  @return The available Option Number.

**/
UINT16
BOpt_GetBootOptionNumber (
  VOID
  )
{
  return BOpt_GetOptionNumber (L"Boot");
}

/**

  Get the Option Number for Driver#### that does not used.

  @return The unused Option Number.

**/
UINT16
BOpt_GetDriverOptionNumber (
  VOID
  )
{
  return BOpt_GetOptionNumber (L"Driver");
}

/**

  Build up all DriverOptionMenu

  @param CallbackData The BMM context data.

  @retval EFI_SUCESS           The functin completes successfully.
  @retval EFI_OUT_OF_RESOURCES Not enough memory to compete the operation.
  @retval EFI_NOT_FOUND        Fail to get "DriverOrder" variable.

**/
EFI_STATUS
BOpt_GetDriverOptions (
  IN  BMM_CALLBACK_DATA  *CallbackData
  )
{
  UINTN   Index;
  UINT16  DriverString[12];
  UINT8   *LoadOptionFromVar;
  UINTN   DriverOptionSize;

  UINT16           *DriverOrderList;
  UINTN            DriverOrderListSize;
  BM_MENU_ENTRY    *NewMenuEntry;
  BM_LOAD_CONTEXT  *NewLoadContext;
  UINT8            *LoadOptionPtr;
  UINTN            StringSize;
  UINTN            OptionalDataSize;
  UINT8            *LoadOptionEnd;

  DriverOrderListSize = 0;
  DriverOrderList     = NULL;
  DriverOptionSize    = 0;
  LoadOptionFromVar   = NULL;
  BOpt_FreeMenu (&DriverOptionMenu);
  InitializeListHead (&DriverOptionMenu.Head);
  //
  // Get the DriverOrder from the Var
  //
  GetEfiGlobalVariable2 (L"DriverOrder", (VOID **)&DriverOrderList, &DriverOrderListSize);
  if (DriverOrderList == NULL) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < DriverOrderListSize / sizeof (UINT16); Index++) {
    UnicodeSPrint (
      DriverString,
      sizeof (DriverString),
      L"Driver%04x",
      DriverOrderList[Index]
      );
    //
    //  Get all loadoptions from the VAR
    //
    GetEfiGlobalVariable2 (DriverString, (VOID **)&LoadOptionFromVar, &DriverOptionSize);
    if (LoadOptionFromVar == NULL) {
      continue;
    }

    NewMenuEntry = BOpt_CreateMenuEntry (BM_LOAD_CONTEXT_SELECT);
    if (NULL == NewMenuEntry) {
      return EFI_OUT_OF_RESOURCES;
    }

    NewLoadContext             = (BM_LOAD_CONTEXT *)NewMenuEntry->VariableContext;
    LoadOptionPtr              = LoadOptionFromVar;
    LoadOptionEnd              = LoadOptionFromVar + DriverOptionSize;
    NewMenuEntry->OptionNumber = DriverOrderList[Index];
    NewLoadContext->Deleted    = FALSE;
    NewLoadContext->IsLegacy   = FALSE;

    //
    // LoadOption is a pointer type of UINT8
    // for easy use with following LOAD_OPTION
    // embedded in this struct
    //

    NewLoadContext->Attributes = *(UINT32 *)LoadOptionPtr;

    LoadOptionPtr += sizeof (UINT32);

    NewLoadContext->FilePathListLength = *(UINT16 *)LoadOptionPtr;
    LoadOptionPtr                     += sizeof (UINT16);

    StringSize                  = StrSize ((UINT16 *)LoadOptionPtr);
    NewLoadContext->Description = AllocateZeroPool (StringSize);
    ASSERT (NewLoadContext->Description != NULL);
    CopyMem (
      NewLoadContext->Description,
      (UINT16 *)LoadOptionPtr,
      StringSize
      );
    NewMenuEntry->DisplayString      = NewLoadContext->Description;
    NewMenuEntry->DisplayStringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, NewMenuEntry->DisplayString, NULL);

    LoadOptionPtr += StringSize;

    NewLoadContext->FilePathList = AllocateZeroPool (NewLoadContext->FilePathListLength);
    ASSERT (NewLoadContext->FilePathList != NULL);
    CopyMem (
      NewLoadContext->FilePathList,
      (EFI_DEVICE_PATH_PROTOCOL *)LoadOptionPtr,
      NewLoadContext->FilePathListLength
      );

    NewMenuEntry->HelpString      = UiDevicePathToStr (NewLoadContext->FilePathList);
    NewMenuEntry->HelpStringToken = HiiSetString (CallbackData->BmmHiiHandle, 0, NewMenuEntry->HelpString, NULL);

    LoadOptionPtr += NewLoadContext->FilePathListLength;

    if (LoadOptionPtr < LoadOptionEnd) {
      OptionalDataSize = DriverOptionSize -
                         sizeof (UINT32) -
                         sizeof (UINT16) -
                         StringSize -
                         NewLoadContext->FilePathListLength;

      NewLoadContext->OptionalData = AllocateZeroPool (OptionalDataSize);
      ASSERT (NewLoadContext->OptionalData != NULL);
      CopyMem (
        NewLoadContext->OptionalData,
        LoadOptionPtr,
        OptionalDataSize
        );
    }

    InsertTailList (&DriverOptionMenu.Head, &NewMenuEntry->Link);
    FreePool (LoadOptionFromVar);
  }

  if (DriverOrderList != NULL) {
    FreePool (DriverOrderList);
  }

  DriverOptionMenu.MenuNumber = Index;
  return EFI_SUCCESS;
}

/**
  Get option number according to Boot#### and BootOrder variable.
  The value is saved as #### + 1.

  @param CallbackData    The BMM context data.
**/
VOID
GetBootOrder (
  IN  BMM_CALLBACK_DATA  *CallbackData
  )
{
  BMM_FAKE_NV_DATA  *BmmConfig;
  UINT16            Index;
  UINT16            OptionOrderIndex;
  UINTN             DeviceType;
  BM_MENU_ENTRY     *NewMenuEntry;
  BM_LOAD_CONTEXT   *NewLoadContext;

  ASSERT (CallbackData != NULL);

  DeviceType = (UINTN)-1;
  BmmConfig  = &CallbackData->BmmFakeNvData;
  ZeroMem (BmmConfig->BootOptionOrder, sizeof (BmmConfig->BootOptionOrder));

  for (Index = 0, OptionOrderIndex = 0; ((Index < BootOptionMenu.MenuNumber) &&
                                         (OptionOrderIndex < (sizeof (BmmConfig->BootOptionOrder) / sizeof (BmmConfig->BootOptionOrder[0]))));
       Index++)
  {
    NewMenuEntry   = BOpt_GetMenuEntry (&BootOptionMenu, Index);
    NewLoadContext = (BM_LOAD_CONTEXT *)NewMenuEntry->VariableContext;

    if (NewLoadContext->IsLegacy) {
      if (((BBS_BBS_DEVICE_PATH *)NewLoadContext->FilePathList)->DeviceType != DeviceType) {
        DeviceType = ((BBS_BBS_DEVICE_PATH *)NewLoadContext->FilePathList)->DeviceType;
      } else {
        //
        // Only show one legacy boot option for the same device type
        // assuming the boot options are grouped by the device type
        //
        continue;
      }
    }

    BmmConfig->BootOptionOrder[OptionOrderIndex++] = (UINT32)(NewMenuEntry->OptionNumber + 1);
  }
}

/**
  Get driver option order from globalc DriverOptionMenu.

  @param CallbackData    The BMM context data.

**/
VOID
GetDriverOrder (
  IN  BMM_CALLBACK_DATA  *CallbackData
  )
{
  BMM_FAKE_NV_DATA  *BmmConfig;
  UINT16            Index;
  UINT16            OptionOrderIndex;
  UINTN             DeviceType;
  BM_MENU_ENTRY     *NewMenuEntry;
  BM_LOAD_CONTEXT   *NewLoadContext;

  ASSERT (CallbackData != NULL);

  DeviceType = (UINTN)-1;
  BmmConfig  = &CallbackData->BmmFakeNvData;
  ZeroMem (BmmConfig->DriverOptionOrder, sizeof (BmmConfig->DriverOptionOrder));

  for (Index = 0, OptionOrderIndex = 0; ((Index < DriverOptionMenu.MenuNumber) &&
                                         (OptionOrderIndex < (sizeof (BmmConfig->DriverOptionOrder) / sizeof (BmmConfig->DriverOptionOrder[0]))));
       Index++)
  {
    NewMenuEntry   = BOpt_GetMenuEntry (&DriverOptionMenu, Index);
    NewLoadContext = (BM_LOAD_CONTEXT *)NewMenuEntry->VariableContext;

    if (NewLoadContext->IsLegacy) {
      if (((BBS_BBS_DEVICE_PATH *)NewLoadContext->FilePathList)->DeviceType != DeviceType) {
        DeviceType = ((BBS_BBS_DEVICE_PATH *)NewLoadContext->FilePathList)->DeviceType;
      } else {
        //
        // Only show one legacy boot option for the same device type
        // assuming the boot options are grouped by the device type
        //
        continue;
      }
    }

    BmmConfig->DriverOptionOrder[OptionOrderIndex++] = (UINT32)(NewMenuEntry->OptionNumber + 1);
  }
}

/**
  Boot the file specified by the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
BootFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  EFI_BOOT_MANAGER_LOAD_OPTION  BootOption;
  CHAR16                        *FileName;

  FileName = NULL;

  FileName = ExtractFileNameFromDevicePath (FilePath);
  if (FileName != NULL) {
    EfiBootManagerInitializeLoadOption (
      &BootOption,
      0,
      LoadOptionTypeBoot,
      LOAD_OPTION_ACTIVE,
      FileName,
      FilePath,
      NULL,
      0
      );
    //
    // Since current no boot from removable media directly is allowed */
    //
    gST->ConOut->ClearScreen (gST->ConOut);
    //
    // Check whether need to reset system.
    //
    BmmSetupResetReminder ();

    BmmSetConsoleMode (FALSE);
    EfiBootManagerBoot (&BootOption);
    BmmSetConsoleMode (TRUE);

    FreePool (FileName);

    EfiBootManagerFreeLoadOption (&BootOption);
  }

  return FALSE;
}

/**
  Display the form base on the selected file.

  @param FilePath   Point to the file path.
  @param FormId     The form need to display.

**/
BOOLEAN
ReSendForm (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN  EFI_FORM_ID               FormId
  )
{
  gBootMaintenancePrivate.LoadContext->FilePathList = FilePath;

  UpdateOptionPage (&gBootMaintenancePrivate, FormId, FilePath);

  gBootMaintenancePrivate.FormBrowser2->SendForm (
                                          gBootMaintenancePrivate.FormBrowser2,
                                          &gBootMaintenancePrivate.BmmHiiHandle,
                                          1,
                                          &mBootMaintGuid,
                                          FormId,
                                          NULL,
                                          NULL
                                          );
  return TRUE;
}

/**
  Create boot option base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
CreateBootOptionFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return ReSendForm (FilePath, FORM_BOOT_ADD_ID);
}

/**
  Create driver option base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.

**/
BOOLEAN
EFIAPI
CreateDriverOptionFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  return ReSendForm (FilePath, FORM_DRV_ADD_FILE_ID);
}
