/** @file
  Provides a way for 3rd party applications to register themselves for launch by the
  Boot Manager based on hot key

Copyright (c) 2007 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Hotkey.h"


LIST_ENTRY      mHotkeyList = INITIALIZE_LIST_HEAD_VARIABLE (mHotkeyList);
BOOLEAN         mHotkeyCallbackPending = FALSE;
EFI_EVENT       mHotkeyEvent;
VOID            *mHotkeyRegistration;


/**
  Check if the Key Option is valid or not.

  @param KeyOption       The Hot Key Option to be checked.

  @retval  TRUE          The Hot Key Option is valid.
  @retval  FALSE         The Hot Key Option is invalid.

**/
BOOLEAN
IsKeyOptionValid (
  IN EFI_KEY_OPTION     *KeyOption
)
{
  UINT16   BootOptionName[10];
  UINT8    *BootOptionVar;
  UINTN    BootOptionSize;
  UINT32   Crc;

  //
  // Check whether corresponding Boot Option exist
  //
  UnicodeSPrint (BootOptionName, sizeof (BootOptionName), L"Boot%04x", KeyOption->BootOption);
  BootOptionVar = BdsLibGetVariableAndSize (
                    BootOptionName,
                    &gEfiGlobalVariableGuid,
                    &BootOptionSize
                    );

  if (BootOptionVar == NULL || BootOptionSize == 0) {
    return FALSE;
  }

  //
  // Check CRC for Boot Option
  //
  gBS->CalculateCrc32 (BootOptionVar, BootOptionSize, &Crc);
  FreePool (BootOptionVar);

  return (BOOLEAN) ((KeyOption->BootOptionCrc == Crc) ? TRUE : FALSE);
}

/**
  Create Key#### for the given hotkey.

  @param KeyOption       The Hot Key Option to be added.
  @param KeyOptionNumber The key option number for Key#### (optional).

  @retval  EFI_SUCCESS            Register hotkey successfully.
  @retval  EFI_INVALID_PARAMETER  The hotkey option is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Fail to allocate memory resource.

**/
EFI_STATUS
RegisterHotkey (
  IN EFI_KEY_OPTION     *KeyOption,
  OUT UINT16            *KeyOptionNumber
)
{
  UINT16          KeyOptionName[10];
  UINT16          *KeyOrder;
  UINTN           KeyOrderSize;
  UINT16          *NewKeyOrder;
  UINTN           Index;
  UINT16          MaxOptionNumber;
  UINT16          RegisterOptionNumber;
  EFI_KEY_OPTION  *TempOption;
  UINTN           TempOptionSize;
  EFI_STATUS      Status;
  UINTN           KeyOptionSize;
  BOOLEAN         UpdateBootOption;

  //
  // Validate the given key option
  //
  if (!IsKeyOptionValid (KeyOption)) {
    return EFI_INVALID_PARAMETER;
  }

  KeyOptionSize = sizeof (EFI_KEY_OPTION) + GET_KEY_CODE_COUNT (KeyOption->KeyData.PackedValue) * sizeof (EFI_INPUT_KEY);
  UpdateBootOption = FALSE;

  //
  // check whether HotKey conflict with keys used by Setup Browser
  //
  KeyOrder = BdsLibGetVariableAndSize (
               VAR_KEY_ORDER,
               &gEfiGlobalVariableGuid,
               &KeyOrderSize
               );
  if (KeyOrder == NULL) {
    KeyOrderSize = 0;
  }

  //
  // Find free key option number
  //
  MaxOptionNumber = 0;
  TempOption = NULL;
  for (Index = 0; Index < KeyOrderSize / sizeof (UINT16); Index++) {
    if (MaxOptionNumber < KeyOrder[Index]) {
      MaxOptionNumber = KeyOrder[Index];
    }

    UnicodeSPrint (KeyOptionName, sizeof (KeyOptionName), L"Key%04x", KeyOrder[Index]);
    TempOption = BdsLibGetVariableAndSize (
                   KeyOptionName,
                   &gEfiGlobalVariableGuid,
                   &TempOptionSize
                   );

    if (CompareMem (TempOption, KeyOption, TempOptionSize) == 0) {
      //
      // Got the option, so just return
      //
      FreePool (TempOption);
      FreePool (KeyOrder);
      return EFI_SUCCESS;
    }

    if (KeyOption->KeyData.PackedValue == TempOption->KeyData.PackedValue) {
      if (GET_KEY_CODE_COUNT (KeyOption->KeyData.PackedValue) == 0 ||
          CompareMem (
            ((UINT8 *) TempOption) + sizeof (EFI_KEY_OPTION),
            ((UINT8 *) KeyOption) + sizeof (EFI_KEY_OPTION),
            KeyOptionSize - sizeof (EFI_KEY_OPTION)
            ) == 0) {
          //
          // Hotkey is the same but BootOption changed, need update
          //
          UpdateBootOption = TRUE;
          break;
      }
    }

    FreePool (TempOption);
  }

  if (UpdateBootOption) {
    RegisterOptionNumber = KeyOrder[Index];
    FreePool (TempOption);
  } else {
    RegisterOptionNumber = (UINT16) (MaxOptionNumber + 1);
  }

  if (KeyOptionNumber != NULL) {
    *KeyOptionNumber = RegisterOptionNumber;
  }

  //
  // Create variable Key####
  //
  UnicodeSPrint (KeyOptionName, sizeof (KeyOptionName), L"Key%04x", RegisterOptionNumber);
  Status = gRT->SetVariable (
                  KeyOptionName,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  KeyOptionSize,
                  KeyOption
                  );
  if (EFI_ERROR (Status)) {
    FreePool (KeyOrder);
    return Status;
  }

  //
  // Update the key order variable - "KeyOrder"
  //
  if (!UpdateBootOption) {
    Index = KeyOrderSize / sizeof (UINT16);
    KeyOrderSize += sizeof (UINT16);
  }

  NewKeyOrder = AllocatePool (KeyOrderSize);
  if (NewKeyOrder == NULL) {
    FreePool (KeyOrder);
    return EFI_OUT_OF_RESOURCES;
  }

  if (KeyOrder != NULL) {
    CopyMem (NewKeyOrder, KeyOrder, KeyOrderSize);
  }

  NewKeyOrder[Index] = RegisterOptionNumber;

  Status = gRT->SetVariable (
                  VAR_KEY_ORDER,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  KeyOrderSize,
                  NewKeyOrder
                  );

  FreePool (KeyOrder);
  FreePool (NewKeyOrder);

  return Status;
}

/**

  Delete Key#### for the given Key Option number.

  @param KeyOptionNumber Key option number for Key####

  @retval  EFI_SUCCESS            Unregister hotkey successfully.
  @retval  EFI_NOT_FOUND          No Key#### is found for the given Key Option number.

**/
EFI_STATUS
UnregisterHotkey (
  IN UINT16     KeyOptionNumber
)
{
  UINT16      KeyOption[10];
  UINTN       Index;
  EFI_STATUS  Status;
  UINTN       Index2Del;
  UINT16      *KeyOrder;
  UINTN       KeyOrderSize;

  //
  // Delete variable Key####
  //
  UnicodeSPrint (KeyOption, sizeof (KeyOption), L"Key%04x", KeyOptionNumber);
  gRT->SetVariable (
         KeyOption,
         &gEfiGlobalVariableGuid,
         EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
         0,
         NULL
         );

  //
  // Adjust key order array
  //
  KeyOrder = BdsLibGetVariableAndSize (
               VAR_KEY_ORDER,
               &gEfiGlobalVariableGuid,
               &KeyOrderSize
               );
  if (KeyOrder == NULL) {
    return EFI_SUCCESS;
  }

  Index2Del = 0;
  for (Index = 0; Index < KeyOrderSize / sizeof (UINT16); Index++) {
    if (KeyOrder[Index] == KeyOptionNumber) {
      Index2Del = Index;
      break;
    }
  }

  if (Index != KeyOrderSize / sizeof (UINT16)) {
    //
    // KeyOptionNumber found in "KeyOrder", delete it
    //
    for (Index = Index2Del; Index < KeyOrderSize / sizeof (UINT16) - 1; Index++) {
      KeyOrder[Index] = KeyOrder[Index + 1];
    }

    KeyOrderSize -= sizeof (UINT16);
  }

  Status = gRT->SetVariable (
                  VAR_KEY_ORDER,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  KeyOrderSize,
                  KeyOrder
                  );

  FreePool (KeyOrder);

  return Status;
}

/**

  This is the common notification function for HotKeys, it will be registered
  with SimpleTextInEx protocol interface - RegisterKeyNotify() of ConIn handle.

  @param KeyData         A pointer to a buffer that is filled in with the keystroke
                         information for the key that was pressed.

  @retval  EFI_SUCCESS   KeyData is successfully processed.
  @return  EFI_NOT_FOUND Fail to find boot option variable.
**/
EFI_STATUS
HotkeyCallback (
  IN EFI_KEY_DATA     *KeyData
)
{
  BOOLEAN            HotkeyCatched;
  LIST_ENTRY         BootLists;
  LIST_ENTRY         *Link;
  BDS_HOTKEY_OPTION  *Hotkey;
  UINT16             Buffer[10];
  BDS_COMMON_OPTION  *BootOption;
  UINTN              ExitDataSize;
  CHAR16             *ExitData;
  EFI_STATUS         Status;
  EFI_KEY_DATA       *HotkeyData;

  if (mHotkeyCallbackPending) {
    //
    // When responsing to a Hotkey, ignore sequential hotkey stroke until
    // the current Boot#### load option returned
    //
    return EFI_SUCCESS;
  }

  Status = EFI_SUCCESS;
  Link = GetFirstNode (&mHotkeyList);

  while (!IsNull (&mHotkeyList, Link)) {
    HotkeyCatched = FALSE;
    Hotkey = BDS_HOTKEY_OPTION_FROM_LINK (Link);

    //
    // Is this Key Stroke we are waiting for?
    //
    ASSERT (Hotkey->WaitingKey < (sizeof (Hotkey->KeyData) / sizeof (Hotkey->KeyData[0])));
    HotkeyData = &Hotkey->KeyData[Hotkey->WaitingKey];
    if ((KeyData->Key.ScanCode == HotkeyData->Key.ScanCode) &&
       (KeyData->Key.UnicodeChar == HotkeyData->Key.UnicodeChar) &&
       ((HotkeyData->KeyState.KeyShiftState & EFI_SHIFT_STATE_VALID) ? (KeyData->KeyState.KeyShiftState == HotkeyData->KeyState.KeyShiftState) : 1)) {
      //
      // Receive an expecting key stroke
      //
      if (Hotkey->CodeCount > 1) {
        //
        // For hotkey of key combination, transit to next waiting state
        //
        Hotkey->WaitingKey++;

        if (Hotkey->WaitingKey == Hotkey->CodeCount) {
          //
          // Received the whole key stroke sequence
          //
          HotkeyCatched = TRUE;
        }
      } else {
        //
        // For hotkey of single key stroke
        //
        HotkeyCatched = TRUE;
      }
    } else {
      //
      // Receive an unexpected key stroke, reset to initial waiting state
      //
      Hotkey->WaitingKey = 0;
    }

    if (HotkeyCatched) {
      //
      // Reset to initial waiting state
      //
      Hotkey->WaitingKey = 0;

      //
      // Launch its BootOption
      //
      InitializeListHead (&BootLists);

      UnicodeSPrint (Buffer, sizeof (Buffer), L"Boot%04x", Hotkey->BootOptionNumber);
      BootOption = BdsLibVariableToOption (&BootLists, Buffer);
      if (BootOption == NULL) {
        return EFI_NOT_FOUND;
      }
      BootOption->BootCurrent = Hotkey->BootOptionNumber;
      BdsLibConnectDevicePath (BootOption->DevicePath);

      //
      // Clear the screen before launch this BootOption
      //
      gST->ConOut->Reset (gST->ConOut, FALSE);

      mHotkeyCallbackPending = TRUE;
      Status = BdsLibBootViaBootOption (BootOption, BootOption->DevicePath, &ExitDataSize, &ExitData);
      mHotkeyCallbackPending = FALSE;

      if (EFI_ERROR (Status)) {
        //
        // Call platform action to indicate the boot fail
        //
        BootOption->StatusString = GetStringById (STRING_TOKEN (STR_BOOT_FAILED));
        PlatformBdsBootFail (BootOption, Status, ExitData, ExitDataSize);
      } else {
        //
        // Call platform action to indicate the boot success
        //
        BootOption->StatusString = GetStringById (STRING_TOKEN (STR_BOOT_SUCCEEDED));
        PlatformBdsBootSuccess (BootOption);
      }
    }

    Link = GetNextNode (&mHotkeyList, Link);
  }

  return Status;
}

/**
  Register the common HotKey notify function to given SimpleTextInEx protocol instance.

  @param SimpleTextInEx  Simple Text Input Ex protocol instance

  @retval  EFI_SUCCESS            Register hotkey notification function successfully.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate necessary data structures.

**/
EFI_STATUS
HotkeyRegisterNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *SimpleTextInEx
)
{
  UINTN              Index;
  EFI_STATUS         Status;
  LIST_ENTRY         *Link;
  BDS_HOTKEY_OPTION  *Hotkey;

  //
  // Register notification function for each hotkey
  //
  Link = GetFirstNode (&mHotkeyList);

  while (!IsNull (&mHotkeyList, Link)) {
    Hotkey = BDS_HOTKEY_OPTION_FROM_LINK (Link);

    Index = 0;
    do {
      Status = SimpleTextInEx->RegisterKeyNotify (
                                 SimpleTextInEx,
                                 &Hotkey->KeyData[Index],
                                 HotkeyCallback,
                                 &Hotkey->NotifyHandle
                                 );
      if (EFI_ERROR (Status)) {
        //
        // some of the hotkey registry failed
        //
        return Status;
      }
      Index ++;
    } while (Index < Hotkey->CodeCount);

    Link = GetNextNode (&mHotkeyList, Link);
  }

  return EFI_SUCCESS;
}

/**
  Callback function for SimpleTextInEx protocol install events

  @param Event           the event that is signaled.
  @param Context         not used here.

**/
VOID
EFIAPI
HotkeyEvent (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_STATUS                         Status;
  UINTN                              BufferSize;
  EFI_HANDLE                         Handle;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *SimpleTextInEx;

  while (TRUE) {
    BufferSize = sizeof (EFI_HANDLE);
    Status = gBS->LocateHandle (
                    ByRegisterNotify,
                    NULL,
                    mHotkeyRegistration,
                    &BufferSize,
                    &Handle
                    );
    if (EFI_ERROR (Status)) {
      //
      // If no more notification events exist
      //
      return ;
    }

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiSimpleTextInputExProtocolGuid,
                    (VOID **) &SimpleTextInEx
                    );
    ASSERT_EFI_ERROR (Status);

    HotkeyRegisterNotify (SimpleTextInEx);
  }
}

/**
  Insert Key Option to hotkey list.

  @param KeyOption       The Hot Key Option to be added to hotkey list.

  @retval EFI_SUCCESS           Add to hotkey list success.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate memory resource.
**/
EFI_STATUS
HotkeyInsertList (
  IN EFI_KEY_OPTION     *KeyOption
)
{
  BDS_HOTKEY_OPTION  *HotkeyLeft;
  BDS_HOTKEY_OPTION  *HotkeyRight;
  UINTN              Index;
  UINT32             KeyOptions;
  UINT32             KeyShiftStateLeft;
  UINT32             KeyShiftStateRight;
  EFI_INPUT_KEY      *InputKey;
  EFI_KEY_DATA       *KeyData;

  HotkeyLeft = AllocateZeroPool (sizeof (BDS_HOTKEY_OPTION));
  if (HotkeyLeft == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HotkeyLeft->Signature = BDS_HOTKEY_OPTION_SIGNATURE;
  HotkeyLeft->BootOptionNumber = KeyOption->BootOption;

  KeyOptions = KeyOption->KeyData.PackedValue;

  HotkeyLeft->CodeCount = (UINT8) GET_KEY_CODE_COUNT (KeyOptions);

  //
  // Map key shift state from KeyOptions to EFI_KEY_DATA.KeyState
  //
  KeyShiftStateRight = (KeyOptions & EFI_KEY_OPTION_SHIFT) |
                       ((KeyOptions & EFI_KEY_OPTION_CONTROL) << 1) |
                       ((KeyOptions & EFI_KEY_OPTION_ALT) << 2) |
                       ((KeyOptions & EFI_KEY_OPTION_LOGO) << 3) |
                       ((KeyOptions & (EFI_KEY_OPTION_MENU | EFI_KEY_OPTION_SYSREQ)) << 4) |
                       EFI_SHIFT_STATE_VALID;

  KeyShiftStateLeft = (KeyShiftStateRight & 0xffffff00) | ((KeyShiftStateRight & 0xff) << 1);

  InputKey = (EFI_INPUT_KEY *) (((UINT8 *) KeyOption) + sizeof (EFI_KEY_OPTION));

  Index = 0;
  KeyData = &HotkeyLeft->KeyData[0];
  do {
    //
    // If Key CodeCount is 0, then only KeyData[0] is used;
    // if Key CodeCount is n, then KeyData[0]~KeyData[n-1] are used
    //
    KeyData->Key.ScanCode = InputKey[Index].ScanCode;
    KeyData->Key.UnicodeChar = InputKey[Index].UnicodeChar;
    KeyData->KeyState.KeyShiftState = KeyShiftStateLeft;

    Index++;
    KeyData++;
  } while (Index < HotkeyLeft->CodeCount);
  InsertTailList (&mHotkeyList, &HotkeyLeft->Link);

  if (KeyShiftStateLeft != KeyShiftStateRight) {
    //
    // Need an extra hotkey for shift key on right
    //
    HotkeyRight = AllocateCopyPool (sizeof (BDS_HOTKEY_OPTION), HotkeyLeft);
    if (HotkeyRight == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Index = 0;
    KeyData = &HotkeyRight->KeyData[0];
    do {
      //
      // Key.ScanCode and Key.UnicodeChar have already been initialized,
      // only need to update KeyState.KeyShiftState
      //
      KeyData->KeyState.KeyShiftState = KeyShiftStateRight;

      Index++;
      KeyData++;
    } while (Index < HotkeyRight->CodeCount);
    InsertTailList (&mHotkeyList, &HotkeyRight->Link);
  }

  return EFI_SUCCESS;
}

/**

  Process all the "Key####" variables, associate Hotkeys with corresponding Boot Options.

  @retval  EFI_SUCCESS    Hotkey services successfully initialized.
  @retval  EFI_NOT_FOUND  Can not find the "KeyOrder" variable
**/
EFI_STATUS
InitializeHotkeyService (
  VOID
  )
{
  EFI_STATUS      Status;
  UINT32          BootOptionSupport;
  UINT16          *KeyOrder;
  UINTN           KeyOrderSize;
  UINTN           Index;
  UINT16          KeyOptionName[8];
  UINTN           KeyOptionSize;
  EFI_KEY_OPTION  *KeyOption;

  //
  // Export our capability - EFI_BOOT_OPTION_SUPPORT_KEY and EFI_BOOT_OPTION_SUPPORT_APP
  //
  BootOptionSupport = EFI_BOOT_OPTION_SUPPORT_KEY | EFI_BOOT_OPTION_SUPPORT_APP;
  Status = gRT->SetVariable (
                  L"BootOptionSupport",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  sizeof (UINT32),
                  &BootOptionSupport
                  );

  //
  // Get valid Key Option List from private EFI variable "KeyOrder"
  //
  KeyOrder = BdsLibGetVariableAndSize (
               VAR_KEY_ORDER,
               &gEfiGlobalVariableGuid,
               &KeyOrderSize
               );

  if (KeyOrder == NULL) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < KeyOrderSize / sizeof (UINT16); Index ++) {
    UnicodeSPrint (KeyOptionName, sizeof (KeyOptionName), L"Key%04x", KeyOrder[Index]);
    KeyOption = BdsLibGetVariableAndSize (
                  KeyOptionName,
                  &gEfiGlobalVariableGuid,
                  &KeyOptionSize
                  );

    if (KeyOption == NULL || !IsKeyOptionValid (KeyOption)) {
      UnregisterHotkey (KeyOrder[Index]);
    } else {
      HotkeyInsertList (KeyOption);
    }
  }

  //
  // Register Protocol notify for Hotkey service
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  HotkeyEvent,
                  NULL,
                  &mHotkeyEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifications on this event
  //
  Status = gBS->RegisterProtocolNotify (
                  &gEfiSimpleTextInputExProtocolGuid,
                  mHotkeyEvent,
                  &mHotkeyRegistration
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

