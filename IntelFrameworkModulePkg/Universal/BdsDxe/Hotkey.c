/** @file
  Provides a way for 3rd party applications to register themselves for launch by the
  Boot Manager based on hot key

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Hotkey.h"


LIST_ENTRY        mHotkeyList = INITIALIZE_LIST_HEAD_VARIABLE (mHotkeyList);
BDS_COMMON_OPTION *mHotkeyBootOption = NULL;
EFI_EVENT         mHotkeyEvent;
VOID              *mHotkeyRegistration;


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
  Try to boot the boot option triggered by hotkey.
  @retval  EFI_SUCCESS             There is HotkeyBootOption & it is processed
  @retval  EFI_NOT_FOUND           There is no HotkeyBootOption
**/
EFI_STATUS
HotkeyBoot (
  VOID
  )
{
  EFI_STATUS           Status;
  UINTN                ExitDataSize;
  CHAR16               *ExitData;

  if (mHotkeyBootOption == NULL) {
    return EFI_NOT_FOUND;
  }

  BdsLibConnectDevicePath (mHotkeyBootOption->DevicePath);

  //
  // Clear the screen before launch this BootOption
  //
  gST->ConOut->Reset (gST->ConOut, FALSE);

  Status = BdsLibBootViaBootOption (mHotkeyBootOption, mHotkeyBootOption->DevicePath, &ExitDataSize, &ExitData);

  if (EFI_ERROR (Status)) {
    //
    // Call platform action to indicate the boot fail
    //
    mHotkeyBootOption->StatusString = GetStringById (STRING_TOKEN (STR_BOOT_FAILED));
    PlatformBdsBootFail (mHotkeyBootOption, Status, ExitData, ExitDataSize);
  } else {
    //
    // Call platform action to indicate the boot success
    //
    mHotkeyBootOption->StatusString = GetStringById (STRING_TOKEN (STR_BOOT_SUCCEEDED));
    PlatformBdsBootSuccess (mHotkeyBootOption);
  }
  FreePool (mHotkeyBootOption->Description);
  FreePool (mHotkeyBootOption->DevicePath);
  FreePool (mHotkeyBootOption->LoadOptions);
  FreePool (mHotkeyBootOption);

  mHotkeyBootOption = NULL;

  return EFI_SUCCESS;
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
EFIAPI
HotkeyCallback (
  IN EFI_KEY_DATA     *KeyData
)
{
  BOOLEAN            HotkeyCatched;
  LIST_ENTRY         BootLists;
  LIST_ENTRY         *Link;
  BDS_HOTKEY_OPTION  *Hotkey;
  UINT16             Buffer[10];
  EFI_STATUS         Status;
  EFI_KEY_DATA       *HotkeyData;

  if (mHotkeyBootOption != NULL) {
    //
    // Do not process sequential hotkey stroke until the current boot option returns
    //
    return EFI_SUCCESS;
  }

  Status                 = EFI_SUCCESS;

  for ( Link = GetFirstNode (&mHotkeyList)
      ; !IsNull (&mHotkeyList, Link)
      ; Link = GetNextNode (&mHotkeyList, Link)
      ) {
    HotkeyCatched = FALSE;
    Hotkey = BDS_HOTKEY_OPTION_FROM_LINK (Link);

    //
    // Is this Key Stroke we are waiting for?
    //
    ASSERT (Hotkey->WaitingKey < (sizeof (Hotkey->KeyData) / sizeof (Hotkey->KeyData[0])));
    HotkeyData = &Hotkey->KeyData[Hotkey->WaitingKey];
    if ((KeyData->Key.ScanCode == HotkeyData->Key.ScanCode) &&
        (KeyData->Key.UnicodeChar == HotkeyData->Key.UnicodeChar) &&
        (((KeyData->KeyState.KeyShiftState & EFI_SHIFT_STATE_VALID) != 0) ?
          (KeyData->KeyState.KeyShiftState == HotkeyData->KeyState.KeyShiftState) : TRUE
        )
       ) {
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
      mHotkeyBootOption = BdsLibVariableToOption (&BootLists, Buffer);
    }
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
    } while ((Index < Hotkey->CodeCount) && (Index < (sizeof (Hotkey->KeyData) / sizeof (EFI_KEY_DATA))));

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
  EFI_BOOT_KEY_DATA  KeyOptions;
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

  KeyOptions = KeyOption->KeyData;

  HotkeyLeft->CodeCount = (UINT8) KeyOptions.Options.InputKeyCount;

  //
  // Map key shift state from KeyOptions to EFI_KEY_DATA.KeyState
  //
  KeyShiftStateRight = EFI_SHIFT_STATE_VALID;
  if (KeyOptions.Options.ShiftPressed) {
    KeyShiftStateRight |= EFI_RIGHT_SHIFT_PRESSED;
  }
  if (KeyOptions.Options.ControlPressed) {
    KeyShiftStateRight |= EFI_RIGHT_CONTROL_PRESSED;
  }
  if (KeyOptions.Options.AltPressed) {
    KeyShiftStateRight |= EFI_RIGHT_ALT_PRESSED;
  }
  if (KeyOptions.Options.LogoPressed) {
    KeyShiftStateRight |= EFI_RIGHT_LOGO_PRESSED;
  }
  if (KeyOptions.Options.MenuPressed) {
    KeyShiftStateRight |= EFI_MENU_KEY_PRESSED;
  }
  if (KeyOptions.Options.SysReqPressed) {
    KeyShiftStateRight |= EFI_SYS_REQ_PRESSED;
  }

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
  Return TRUE when the variable pointed by Name and Guid is a Key#### variable.

  @param Name         The name of the variable.
  @param Guid         The GUID of the variable.
  @param OptionNumber Return the option number parsed from the Name.

  @retval TRUE  The variable pointed by Name and Guid is a Key#### variable.
  @retval FALSE The variable pointed by Name and Guid isn't a Key#### variable.
**/
BOOLEAN
IsKeyOptionVariable (
  CHAR16        *Name,
  EFI_GUID      *Guid,
  UINT16        *OptionNumber
  )
{
  UINTN         Index;

  if (!CompareGuid (Guid, &gEfiGlobalVariableGuid) ||
      (StrSize (Name) != sizeof (L"Key####")) ||
      (StrnCmp (Name, L"Key", 3) != 0)
     ) {
    return FALSE;
  }

  *OptionNumber = 0;
  for (Index = 3; Index < 7; Index++) {
    if ((Name[Index] >= L'0') && (Name[Index] <= L'9')) {
      *OptionNumber = *OptionNumber * 16 + Name[Index] - L'0';
    } else if ((Name[Index] >= L'A') && (Name[Index] <= L'F')) {
      *OptionNumber = *OptionNumber * 16 + Name[Index] - L'A' + 10;
    } else {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Return an array of key option numbers.

  @param Count       Return the count of key option numbers.

  @return UINT16*    Pointer to an array of key option numbers;
**/
UINT16 *
EFIAPI
HotkeyGetOptionNumbers (
  OUT UINTN     *Count
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  CHAR16                      *Name;
  EFI_GUID                    Guid;
  UINTN                       NameSize;
  UINTN                       NewNameSize;
  UINT16                      *OptionNumbers;
  UINT16                      OptionNumber;

  if (Count == NULL) {
    return NULL;
  }

  *Count        = 0;
  OptionNumbers = NULL;

  NameSize = sizeof (CHAR16);
  Name     = AllocateZeroPool (NameSize);
  ASSERT (Name != NULL);
  while (TRUE) {
    NewNameSize = NameSize;
    Status = gRT->GetNextVariableName (&NewNameSize, Name, &Guid);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Name = ReallocatePool (NameSize, NewNameSize, Name);
      ASSERT (Name != NULL);
      Status = gRT->GetNextVariableName (&NewNameSize, Name, &Guid);
      NameSize = NewNameSize;
    }

    if (Status == EFI_NOT_FOUND) {
      break;
    }
    ASSERT_EFI_ERROR (Status);

    if (IsKeyOptionVariable (Name ,&Guid, &OptionNumber)) {
      OptionNumbers = ReallocatePool (
                        *Count * sizeof (UINT16),
                        (*Count + 1) * sizeof (UINT16),
                        OptionNumbers
                        );
      ASSERT (OptionNumbers != NULL);
      for (Index = 0; Index < *Count; Index++) {
        if (OptionNumber < OptionNumbers[Index]) {
          break;
        }
      }
      CopyMem (&OptionNumbers[Index + 1], &OptionNumbers[Index], (*Count - Index) * sizeof (UINT16));
      OptionNumbers[Index] = OptionNumber;
      (*Count)++;
    }
  }

  FreePool (Name);

  return OptionNumbers;
}

/**

  Process all the "Key####" variables, associate Hotkeys with corresponding Boot Options.

  @retval  EFI_SUCCESS    Hotkey services successfully initialized.
**/
EFI_STATUS
InitializeHotkeyService (
  VOID
  )
{
  EFI_STATUS      Status;
  UINT32          BootOptionSupport;
  UINT16          *KeyOptionNumbers;
  UINTN           KeyOptionCount;
  UINTN           Index;
  CHAR16          KeyOptionName[8];
  EFI_KEY_OPTION  *KeyOption;

  //
  // Export our capability - EFI_BOOT_OPTION_SUPPORT_KEY and EFI_BOOT_OPTION_SUPPORT_APP.
  // with maximum number of key presses of 3
  // Do not report the hotkey capability if PcdConInConnectOnDemand is enabled.
  //
  BootOptionSupport = EFI_BOOT_OPTION_SUPPORT_APP;
  if (!PcdGetBool (PcdConInConnectOnDemand)) {
    BootOptionSupport |= EFI_BOOT_OPTION_SUPPORT_KEY;
    SET_BOOT_OPTION_SUPPORT_KEY_COUNT (BootOptionSupport, 3);
  }

  Status = gRT->SetVariable (
                  L"BootOptionSupport",
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (UINT32),
                  &BootOptionSupport
                  );
  //
  // Platform needs to make sure setting volatile variable before calling 3rd party code shouldn't fail.
  //
  ASSERT_EFI_ERROR (Status);

  KeyOptionNumbers = HotkeyGetOptionNumbers (&KeyOptionCount);
  for (Index = 0; Index < KeyOptionCount; Index ++) {
    UnicodeSPrint (KeyOptionName, sizeof (KeyOptionName), L"Key%04x", KeyOptionNumbers[Index]);
    GetEfiGlobalVariable2 (KeyOptionName, (VOID **) &KeyOption, NULL);
    ASSERT (KeyOption != NULL);
    if (IsKeyOptionValid (KeyOption)) {
      HotkeyInsertList (KeyOption);
    }
    FreePool (KeyOption);
  }

  if (KeyOptionNumbers != NULL) {
    FreePool (KeyOptionNumbers);
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

