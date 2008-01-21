/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  DeviceManager.c

Abstract:

  The platform device manager reference implement

--*/

#include "DeviceManager.h"

STATIC UINT16                     mTokenCount;
EFI_FRONTPAGE_CALLBACK_INFO       FPCallbackInfo;
extern UINTN                      gCallbackKey;
extern EFI_FORM_BROWSER_PROTOCOL  *gBrowser;
extern BOOLEAN                    gConnectAllHappened;

STRING_REF                        gStringTokenTable[] = {
  STR_VIDEO_DEVICE,
  STR_NETWORK_DEVICE,
  STR_INPUT_DEVICE,
  STR_ON_BOARD_DEVICE,
  STR_OTHER_DEVICE,
  STR_EMPTY_STRING,
  0xFFFF
};

EFI_STATUS
EFIAPI
DeviceManagerCallbackRoutine (
  IN EFI_FORM_CALLBACK_PROTOCOL       *This,
  IN UINT16                           KeyValue,
  IN FRAMEWORK_EFI_IFR_DATA_ARRAY               *DataArray,
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
  //
  // The KeyValue corresponds in this case to the handle which was requested to be displayed
  //
  EFI_FRONTPAGE_CALLBACK_INFO *CallbackInfo;

  CallbackInfo = EFI_FP_CALLBACK_DATA_FROM_THIS (This);
  switch (KeyValue) {
  case 0x2000:
    CallbackInfo->Data.VideoBIOS = (UINT8) (UINTN) (((FRAMEWORK_EFI_IFR_DATA_ENTRY *)(DataArray + 1))->Data);
    gRT->SetVariable (
          L"VBIOS",
          &gEfiGenericPlatformVariableGuid,
          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
          sizeof (UINT8),
          &CallbackInfo->Data.VideoBIOS
          );
    break;

  default:
    break;
  }

  gCallbackKey = KeyValue;
  return EFI_SUCCESS;
}

EFI_STATUS
InitializeDeviceManager (
  VOID
  )
/*++

Routine Description:

  Initialize HII information for the FrontPage

Arguments:
  None

Returns:

--*/
{
  EFI_STATUS          Status;
  EFI_HII_PACKAGES    *PackageList;
  EFI_HII_UPDATE_DATA *UpdateData;

  //
  // Allocate space for creation of UpdateData Buffer
  //
  UpdateData = AllocateZeroPool (0x1000);
  ASSERT (UpdateData != NULL);

  PackageList = PreparePackages (1, &gEfiCallerIdGuid, DeviceManagerVfrBin);
  Status      = gHii->NewPack (gHii, PackageList, &FPCallbackInfo.DevMgrHiiHandle);
  FreePool (PackageList);

  //
  // This example does not implement worker functions for the NV accessor functions.  Only a callback evaluator
  //
  FPCallbackInfo.Signature                = EFI_FP_CALLBACK_DATA_SIGNATURE;
  FPCallbackInfo.DevMgrCallback.NvRead    = NULL;
  FPCallbackInfo.DevMgrCallback.NvWrite   = NULL;
  FPCallbackInfo.DevMgrCallback.Callback  = DeviceManagerCallbackRoutine;

  //
  // Install protocol interface
  //
  FPCallbackInfo.CallbackHandle = NULL;

  Status = gBS->InstallProtocolInterface (
                  &FPCallbackInfo.CallbackHandle,
                  &gEfiFormCallbackProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &FPCallbackInfo.DevMgrCallback
                  );

  ASSERT_EFI_ERROR (Status);

  //
  // Flag update pending in FormSet
  //
  UpdateData->FormSetUpdate = TRUE;
  //
  // Register CallbackHandle data for FormSet
  //
  UpdateData->FormCallbackHandle = (EFI_PHYSICAL_ADDRESS) (UINTN) FPCallbackInfo.CallbackHandle;
  //
  // Simply registering the callback handle
  //
  gHii->UpdateForm (gHii, FPCallbackInfo.DevMgrHiiHandle, (EFI_FORM_LABEL) 0x0000, TRUE, UpdateData);

  FreePool (UpdateData);
  return Status;
}

EFI_STATUS
CallDeviceManager (
  VOID
  )
/*++

Routine Description:

  Call the browser and display the device manager

Arguments:

  None

Returns:
  EFI_SUCCESS            - Operation is successful.
  EFI_INVALID_PARAMETER  - If the inputs to SendForm function is not valid.

--*/
{
  EFI_STATUS          Status;
  UINTN               BufferSize;
  UINTN               Count;
  FRAMEWORK_EFI_HII_HANDLE       Index;
  UINT8               *Buffer;
  FRAMEWORK_EFI_IFR_FORM_SET    *FormSetData;
  CHAR16              *String;
  UINTN               StringLength;
  EFI_HII_UPDATE_DATA *UpdateData;
  STRING_REF          Token;
  STRING_REF          TokenHelp;
  IFR_OPTION          *IfrOptionList;
  UINT8               *VideoOption;
  UINTN               VideoOptionSize;
  FRAMEWORK_EFI_HII_HANDLE       *HiiHandles;
  UINT16              HandleBufferLength;
  BOOLEAN	          BootDeviceMngrMenuResetRequired;

  IfrOptionList       = NULL;
  VideoOption         = NULL;
  HiiHandles          = NULL;
  HandleBufferLength  = 0;

  //
  // Connect all prior to entering the platform setup menu.
  //
  if (!gConnectAllHappened) {
    BdsLibConnectAllDriversToAllControllers ();
    gConnectAllHappened = TRUE;
  }
  //
  // Allocate space for creation of UpdateData Buffer
  //
  UpdateData = AllocateZeroPool (0x1000);
  ASSERT (UpdateData != NULL);

  Status        = EFI_SUCCESS;
  Buffer        = NULL;
  FormSetData   = NULL;
  gCallbackKey  = 0;
  if (mTokenCount == 0) {
    gHii->NewString (gHii, NULL, FPCallbackInfo.DevMgrHiiHandle, &mTokenCount, L" ");
  }

  Token     = mTokenCount;
  TokenHelp = (UINT16) (Token + 1);

  //
  // Reset the menu
  //
  for (Index = 0, Count = 1; Count < 0x10000; Count <<= 1, Index++) {
    //
    // We will strip off all previous menu entries
    //
    UpdateData->DataCount = 0xFF;

    //
    // Erase entries on this label
    //
    gHii->UpdateForm (gHii, FPCallbackInfo.DevMgrHiiHandle, (EFI_FORM_LABEL) Count, FALSE, UpdateData);

    //
    // Did we reach the end of the Token Table?
    //
    if (gStringTokenTable[Index] == 0xFFFF) {
      break;
    }

    CreateSubTitleOpCode (gStringTokenTable[Index], &UpdateData->Data);
    //
    // Add a single menu item - in this case a subtitle for the device type
    //
    UpdateData->DataCount = 1;

    //
    // Add default title for this label
    //
    gHii->UpdateForm (gHii, FPCallbackInfo.DevMgrHiiHandle, (EFI_FORM_LABEL) Count, TRUE, UpdateData);
  }
  //
  // Add a space and an exit string.  Remember since we add things at the label and push other things beyond the
  // label down, we add this in reverse order
  //
  CreateSubTitleOpCode (STRING_TOKEN (STR_EXIT_STRING), &UpdateData->Data);
  gHii->UpdateForm (gHii, FPCallbackInfo.DevMgrHiiHandle, (EFI_FORM_LABEL) Count, TRUE, UpdateData);
  CreateSubTitleOpCode (STR_EMPTY_STRING, &UpdateData->Data);
  gHii->UpdateForm (gHii, FPCallbackInfo.DevMgrHiiHandle, (EFI_FORM_LABEL) Count, TRUE, UpdateData);

  //
  // Get all the gHii handles
  //
  Status = BdsLibGetHiiHandles (gHii, &HandleBufferLength, &HiiHandles);
  ASSERT_EFI_ERROR (Status);

  for (Index = 1, BufferSize = 0; Index < HandleBufferLength; Index++) {
    //
    // Am not initializing Buffer since the first thing checked is the size
    // this way I can get the real buffersize in the smallest code size
    //
    Status = gHii->GetForms (gHii, Index, 0, &BufferSize, Buffer);

    if (Status != EFI_NOT_FOUND) {
      //
      // BufferSize should have the real size of the forms now
      //
      Buffer = AllocateZeroPool (BufferSize);
      ASSERT (Buffer != NULL);

      //
      // Am not initializing Buffer since the first thing checked is the size
      // this way I can get the real buffersize in the smallest code size
      //
      Status = gHii->GetForms (gHii, Index, 0, &BufferSize, Buffer);

      //
      // Skip EFI_HII_PACK_HEADER, advance to FRAMEWORK_EFI_IFR_FORM_SET data.
      //
      FormSetData = (FRAMEWORK_EFI_IFR_FORM_SET *) (Buffer + sizeof (EFI_HII_PACK_HEADER));

      //
      // If this formset belongs in the device manager, add it to the menu
      //
      if (FormSetData->Class != EFI_NON_DEVICE_CLASS) {

        StringLength  = 0x1000;
        String        = AllocateZeroPool (StringLength);
        ASSERT (String != NULL);

        Status  = gHii->GetString (gHii, Index, FormSetData->FormSetTitle, TRUE, NULL, &StringLength, String);
        Status  = gHii->NewString (gHii, NULL, FPCallbackInfo.DevMgrHiiHandle, &Token, String);

        //
        // If token value exceeded real token value - we need to add a new token values
        //
        if (Status == EFI_INVALID_PARAMETER) {
          Token     = 0;
          TokenHelp = 0;
          Status    = gHii->NewString (gHii, NULL, FPCallbackInfo.DevMgrHiiHandle, &Token, String);
        }

        StringLength = 0x1000;
        if (FormSetData->Help == 0) {
          TokenHelp = 0;
        } else {
          Status = gHii->GetString (gHii, Index, FormSetData->Help, TRUE, NULL, &StringLength, String);
          if (StringLength == 0x02) {
            TokenHelp = 0;
          } else {
            Status = gHii->NewString (gHii, NULL, FPCallbackInfo.DevMgrHiiHandle, &TokenHelp, String);
            if (Status == EFI_INVALID_PARAMETER) {
              TokenHelp = 0;
              Status    = gHii->NewString (gHii, NULL, FPCallbackInfo.DevMgrHiiHandle, &TokenHelp, String);
            }
          }
        }

        FreePool (String);

        CreateGotoOpCode (
          0x1000,     // Device Manager Page
          Token,      // Description String Token
          TokenHelp,  // Description Help String Token
          FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE | FRAMEWORK_EFI_IFR_FLAG_NV_ACCESS,  // Flag designating callback is active
          (UINT16) Index,                                     // Callback key value
          &UpdateData->Data                                   // Buffer to fill with op-code
          );

        //
        // In the off-chance that we have lots of extra tokens allocated to the DeviceManager
        // this ensures we are fairly re-using the tokens instead of constantly growing the token
        // storage for this one handle.  If we incremented the token value beyond what it normally
        // would use, we will fall back into the error path which seeds the token value with a 0
        // so that we can correctly add a token value.
        //
        if (TokenHelp == 0) {
          //
          // Since we didn't add help, only advance Token by 1
          //
          Token++;
        } else {
          Token     = (UINT16) (Token + 2);
          TokenHelp = (UINT16) (TokenHelp + 2);
        }
        //
        // This for loop basically will take the Class value which is a bitmask and
        // update the form for every active bit.  There will be a label at each bit
        // location.  So if someone had a device which a class of EFI_DISK_DEVICE_CLASS |
        // EFI_ON_BOARD_DEVICE_CLASS, this routine will unwind that mask and drop the menu entry
        // on each corresponding label.
        //
        for (Count = 1; Count < 0x10000; Count <<= 1) {
          //
          // This is an active bit, so update the form
          //
          if (FormSetData->Class & Count) {
            gHii->UpdateForm (
                  gHii,
                  FPCallbackInfo.DevMgrHiiHandle,
                  (EFI_FORM_LABEL) (FormSetData->Class & Count),
                  TRUE,
                  UpdateData
                  );
          }
        }
      }

      BufferSize = 0;
      //
      // Reset Buffer pointer to original location
      //
      FreePool (Buffer);
    }
  }
  //
  // Add oneof for video BIOS selection
  //
  VideoOption = BdsLibGetVariableAndSize (
                  L"VBIOS",
                  &gEfiGenericPlatformVariableGuid,
                  &VideoOptionSize
                  );
  if (NULL == VideoOption) {
    FPCallbackInfo.Data.VideoBIOS = 0;
  } else {
    FPCallbackInfo.Data.VideoBIOS = VideoOption[0];
    FreePool (VideoOption);
  }

  ASSERT (FPCallbackInfo.Data.VideoBIOS <= 1);

  IfrOptionList = AllocatePool (2 * sizeof (IFR_OPTION));
  if (IfrOptionList != NULL) {
    IfrOptionList[0].Flags        = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
    IfrOptionList[0].Key          = SET_VIDEO_BIOS_TYPE_QUESTION_ID + 0x2000;
    IfrOptionList[0].StringToken  = STRING_TOKEN (STR_ONE_OF_PCI);
    IfrOptionList[0].Value        = 0;
    IfrOptionList[0].OptionString = NULL;
    IfrOptionList[1].Flags        = FRAMEWORK_EFI_IFR_FLAG_INTERACTIVE;
    IfrOptionList[1].Key          = SET_VIDEO_BIOS_TYPE_QUESTION_ID + 0x2000;
    IfrOptionList[1].StringToken  = STRING_TOKEN (STR_ONE_OF_AGP);
    IfrOptionList[1].Value        = 1;
    IfrOptionList[1].OptionString = NULL;
    IfrOptionList[FPCallbackInfo.Data.VideoBIOS].Flags |= FRAMEWORK_EFI_IFR_FLAG_DEFAULT;

    CreateOneOfOpCode (
      SET_VIDEO_BIOS_TYPE_QUESTION_ID,
      (UINT8) 1,
      STRING_TOKEN (STR_ONE_OF_VBIOS),
      STRING_TOKEN (STR_ONE_OF_VBIOS_HELP),
      IfrOptionList,
      2,
      &UpdateData->Data
      );

    UpdateData->DataCount = 4;
    gHii->UpdateForm (gHii, FPCallbackInfo.DevMgrHiiHandle, (EFI_FORM_LABEL) EFI_VBIOS_CLASS, TRUE, UpdateData);
    FreePool (IfrOptionList);
  }

  BootDeviceMngrMenuResetRequired = FALSE;
  Status = gBrowser->SendForm (
                      gBrowser,
                      TRUE,                             // Use the database
                      &FPCallbackInfo.DevMgrHiiHandle,  // The HII Handle
                      1,
                      NULL,
                      FPCallbackInfo.CallbackHandle,
                      (UINT8 *) &FPCallbackInfo.Data,
                      NULL,
                      &BootDeviceMngrMenuResetRequired
                      );

  if (BootDeviceMngrMenuResetRequired) {
    EnableResetRequired ();
  }

  gHii->ResetStrings (gHii, FPCallbackInfo.DevMgrHiiHandle);

  //
  // We will have returned from processing a callback - user either hit ESC to exit, or selected
  // a target to display
  //
  if (gCallbackKey != 0 && gCallbackKey < 0x2000) {
    BootDeviceMngrMenuResetRequired = FALSE;
    Status = gBrowser->SendForm (
                        gBrowser,
                        TRUE,                             // Use the database
                        (FRAMEWORK_EFI_HII_HANDLE  *) &gCallbackKey, // The HII Handle
                        1,
                        NULL,
                        NULL,                             // This is the handle that the interface to the callback was installed on
                        NULL,
                        NULL,
                        &BootDeviceMngrMenuResetRequired
                        );

    if (BootDeviceMngrMenuResetRequired) {
      EnableResetRequired ();
    }
    //
    // Force return to Device Manager
    //
    gCallbackKey = 4;
  }

  if (gCallbackKey >= 0x2000) {
    gCallbackKey = 4;
  }

  FreePool (UpdateData);
  FreePool (HiiHandles);

  return Status;
}
