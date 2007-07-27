/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  BootManager.c

Abstract:

  The platform boot manager reference implement

--*/

#include "BootManager.h"

UINT16                            mKeyInput;
LIST_ENTRY                        *mBootOptionsList;
BDS_COMMON_OPTION                 *gOption;
EFI_HII_HANDLE                    gBootManagerHandle;
EFI_HANDLE                        BootManagerCallbackHandle;
EFI_FORM_CALLBACK_PROTOCOL        BootManagerCallback;
EFI_GUID                          gBmGuid = BOOT_MANAGER_GUID;

extern EFI_FORM_BROWSER_PROTOCOL  *gBrowser;
extern UINT8                      BootManagerVfrBin[];
extern BOOLEAN                    gConnectAllHappened;

EFI_STATUS
EFIAPI
BootManagerCallbackRoutine (
  IN EFI_FORM_CALLBACK_PROTOCOL       *This,
  IN UINT16                           KeyValue,
  IN EFI_IFR_DATA_ARRAY               *DataArray,
  OUT EFI_HII_CALLBACK_PACKET         **Packet
  )
/*++

Routine Description:

  This is the function that is called to provide results data to the driver.  This data
  consists of a unique key which is used to identify what data is either being passed back
  or being asked for.

Arguments:

  KeyValue -        A unique value which is sent to the original exporting driver so that it
                    can identify the type of data to expect.  The format of the data tends to
                    vary based on the op-code that geerated the callback.

  Data -            A pointer to the data being sent to the original exporting driver.

Returns:

--*/
{
  BDS_COMMON_OPTION       *Option;
  LIST_ENTRY              *Link;
  UINT16                  KeyCount;
  EFI_HII_CALLBACK_PACKET *DataPacket;

  //
  // Initialize the key count
  //
  KeyCount = 0;

  for (Link = mBootOptionsList->ForwardLink; Link != mBootOptionsList; Link = Link->ForwardLink) {
    Option = CR (Link, BDS_COMMON_OPTION, Link, BDS_LOAD_OPTION_SIGNATURE);

    KeyCount++;

    gOption = Option;

    //
    // Is this device the one chosen?
    //
    if (KeyCount == KeyValue) {
      //
      // Assigning the returned Key to a global allows the original routine to know what was chosen
      //
      mKeyInput = KeyValue;

      *Packet   = AllocateZeroPool (sizeof (EFI_HII_CALLBACK_PACKET) + 2);
      ASSERT (*Packet != NULL);

      //
      // Assign the buffer address to DataPacket
      //
      DataPacket                        = *Packet;

      DataPacket->DataArray.EntryCount  = 1;
      DataPacket->DataArray.NvRamMap    = NULL;
      ((EFI_IFR_DATA_ENTRY *) (((EFI_IFR_DATA_ARRAY *)DataPacket) + 1))->Flags = EXIT_REQUIRED | NV_NOT_CHANGED;
      return EFI_SUCCESS;
    } else {
      continue;
    }
  }

  return EFI_SUCCESS;
}

VOID
CallBootManager (
  VOID
  )
/*++

Routine Description:
  Hook to enable UI timeout override behavior.

Arguments:
  BdsDeviceList - Device List that BDS needs to connect.

  Entry - Pointer to current Boot Entry.

Returns:
  NONE

--*/
{
  EFI_STATUS          Status;
  EFI_HII_PACKAGES    *PackageList;
  BDS_COMMON_OPTION   *Option;
  LIST_ENTRY          *Link;
  EFI_HII_UPDATE_DATA *UpdateData;
  CHAR16              *ExitData;
  UINTN               ExitDataSize;
  STRING_REF          Token;
  STRING_REF          LastToken;
  EFI_INPUT_KEY       Key;
  UINT8               *Location;
  EFI_GUID            BmGuid;
  LIST_ENTRY          BdsBootOptionList;
  BOOLEAN	          BootMngrMenuResetRequired;

  gOption = NULL;
  InitializeListHead (&BdsBootOptionList);

  //
  // Connect all prior to entering the platform setup menu.
  //
  if (!gConnectAllHappened) {
    BdsLibConnectAllDriversToAllControllers ();
    gConnectAllHappened = TRUE;
  }
  //
  // BugBug: Here we can not remove the legacy refresh macro, so we need
  // get the boot order every time from "BootOrder" variable.
  // Recreate the boot option list base on the BootOrder variable
  //
  BdsLibEnumerateAllBootOption (&BdsBootOptionList);

  //
  // This GUID must be the same as what is defined in BootManagerVfr.vfr
  //
  BmGuid            = gBmGuid;

  mBootOptionsList  = &BdsBootOptionList;

  //
  // Post our VFR to the HII database
  //
  PackageList = PreparePackages (2, &BmGuid, BootManagerVfrBin, PlatformBdsStrings);
  Status      = gHii->NewPack (gHii, PackageList, &gBootManagerHandle);
  FreePool (PackageList);

  //
  // This example does not implement worker functions
  // for the NV accessor functions.  Only a callback evaluator
  //
  BootManagerCallback.NvRead    = NULL;
  BootManagerCallback.NvWrite   = NULL;
  BootManagerCallback.Callback  = BootManagerCallbackRoutine;

  //
  // Install protocol interface
  //
  BootManagerCallbackHandle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &BootManagerCallbackHandle,
                  &gEfiFormCallbackProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &BootManagerCallback
                  );
  ASSERT_EFI_ERROR (Status);

  LastToken = 0;
  gHii->NewString (gHii, NULL, gBootManagerHandle, &LastToken, L" ");

  //
  // Allocate space for creation of UpdateData Buffer
  //
  UpdateData = AllocateZeroPool (0x1000);
  ASSERT (UpdateData != NULL);

  //
  // Flag update pending in FormSet
  //
  UpdateData->FormSetUpdate = TRUE;
  //
  // Register CallbackHandle data for FormSet
  //
  UpdateData->FormCallbackHandle = (EFI_PHYSICAL_ADDRESS) (UINTN) BootManagerCallbackHandle;
  UpdateData->FormUpdate  = FALSE;
  UpdateData->FormTitle   = 0;
  UpdateData->DataCount   = 1;

  //
  // Create blank space.  Since when we update the contents of IFR data at a label, it is
  // inserted at the location of the label.  So if you want to add a string with an empty
  // space afterwards, you need to add the space first and then the string like below.
  //
  Status = CreateSubTitleOpCode (
            LastToken,        // Token Value for the string
            &UpdateData->Data // Buffer containing created op-code
            );

  gHii->UpdateForm (gHii, gBootManagerHandle, (EFI_FORM_LABEL) 0x0000, TRUE, UpdateData);

  //
  // Create "Boot Option Menu" title
  //
  Status = CreateSubTitleOpCode (
            STRING_TOKEN (STR_BOOT_OPTION_BANNER),  // Token Value for the string
            &UpdateData->Data                       // Buffer containing created op-code
            );

  gHii->UpdateForm (gHii, gBootManagerHandle, (EFI_FORM_LABEL) 0x0000, TRUE, UpdateData);

  Token                 = LastToken;
  mKeyInput             = 0;

  UpdateData->DataCount = 0;
  Location              = (UINT8 *) &UpdateData->Data;

  for (Link = BdsBootOptionList.ForwardLink; Link != &BdsBootOptionList; Link = Link->ForwardLink) {
    Option = CR (Link, BDS_COMMON_OPTION, Link, BDS_LOAD_OPTION_SIGNATURE);

    //
    // At this stage we are creating a menu entry, thus the Keys are reproduceable
    //
    mKeyInput++;
    Token++;

    Status = gHii->NewString (gHii, NULL, gBootManagerHandle, &Token, Option->Description);

    //
    // If we got an error it is almost certainly due to the token value being invalid.
    // Therefore we will set the Token to 0 to automatically add a token.
    //
    if (EFI_ERROR (Status)) {
      Token   = 0;
      Status  = gHii->NewString (gHii, NULL, gBootManagerHandle, &Token, Option->Description);
    }

    Status = CreateGotoOpCode (
              0x1000, // Form ID
              Token,  // Token Value for the string
              0,      // Help String (none)
              EFI_IFR_FLAG_INTERACTIVE | EFI_IFR_FLAG_NV_ACCESS,  // The Op-Code flags
              mKeyInput,                                          // The Key to get a callback on
              Location  // Buffer containing created op-code
              );

    UpdateData->DataCount++;
    Location = Location + ((EFI_IFR_OP_HEADER *) Location)->Length;

  }

  gHii->UpdateForm (gHii, gBootManagerHandle, (EFI_FORM_LABEL) 0x0001, TRUE, UpdateData);

  UpdateData->DataCount = 1;

  //
  // Create "Boot Option Menu" title
  //
  Status = CreateSubTitleOpCode (
            STRING_TOKEN (STR_HELP_FOOTER), // Token Value for the string
            &UpdateData->Data               // Buffer containing created op-code
            );

  gHii->UpdateForm (gHii, gBootManagerHandle, (EFI_FORM_LABEL) 0x0002, TRUE, UpdateData);

  Status = CreateSubTitleOpCode (
            LastToken,                      // Token Value for the string
            &UpdateData->Data               // Buffer containing created op-code
            );

  gHii->UpdateForm (gHii, gBootManagerHandle, (EFI_FORM_LABEL) 0x0002, TRUE, UpdateData);

  FreePool (UpdateData);

  ASSERT (gBrowser);

  BootMngrMenuResetRequired = FALSE;
  gBrowser->SendForm (
              gBrowser,
              TRUE,
              &gBootManagerHandle,
              1,
              NULL,
              NULL,
              NULL,
              NULL,
              &BootMngrMenuResetRequired
              );

  if (BootMngrMenuResetRequired) {
    EnableResetRequired ();
  }

  gHii->ResetStrings (gHii, gBootManagerHandle);

  if (gOption == NULL) {
    return ;
  }

  //
  //Will leave browser, check any reset required change is applied? if yes, reset system
  //
  SetupResetReminder ();

  //
  // BugBug: This code looks repeated from the BDS. Need to save code space.
  //

  //
  // parse the selected option
  //
  Status = BdsLibBootViaBootOption (gOption, gOption->DevicePath, &ExitDataSize, &ExitData);

  if (!EFI_ERROR (Status)) {
    PlatformBdsBootSuccess (gOption);
  } else {
    PlatformBdsBootFail (gOption, Status, ExitData, ExitDataSize);
    gST->ConOut->OutputString (
                  gST->ConOut,
                  GetStringById (STRING_TOKEN (STR_ANY_KEY_CONTINUE))
                  );

    //
    // BdsLibUiWaitForSingleEvent (gST->ConIn->WaitForKey, 0);
    //

    gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  }
}
