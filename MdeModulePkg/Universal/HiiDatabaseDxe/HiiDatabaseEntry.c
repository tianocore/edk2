/** @file
This file contains the entry code to the HII database, which is defined by
UEFI 2.1 specification.

Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"

//
// Global variables
//
EFI_EVENT gHiiKeyboardLayoutChanged;

HII_DATABASE_PRIVATE_DATA mPrivate = {
  HII_DATABASE_PRIVATE_DATA_SIGNATURE,
  {
    (LIST_ENTRY *) NULL,
    (LIST_ENTRY *) NULL
  },
  {
    (LIST_ENTRY *) NULL,
    (LIST_ENTRY *) NULL
  },
  {
    HiiStringToImage,
    HiiStringIdToImage,
    HiiGetGlyph,
    HiiGetFontInfo
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    HiiNewString,
    HiiGetString,
    HiiSetString,
    HiiGetLanguages,
    HiiGetSecondaryLanguages
  },
  {
    HiiNewPackageList,
    HiiRemovePackageList,
    HiiUpdatePackageList,
    HiiListPackageLists,
    HiiExportPackageLists,
    HiiRegisterPackageNotify,
    HiiUnregisterPackageNotify,
    HiiFindKeyboardLayouts,
    HiiGetKeyboardLayout,
    HiiSetKeyboardLayout,
    HiiGetPackageListHandle
  },
  {
    HiiConfigRoutingExtractConfig,
    HiiConfigRoutingExportConfig,
    HiiConfigRoutingRouteConfig,
    HiiBlockToConfig,
    HiiConfigToBlock,
    HiiGetAltCfg
  },
  {
    EfiConfigKeywordHandlerSetData,
    EfiConfigKeywordHandlerGetData
  },
  {
    (LIST_ENTRY *) NULL,
    (LIST_ENTRY *) NULL
  },
  0,
  {
    (LIST_ENTRY *) NULL,
    (LIST_ENTRY *) NULL
  },
  EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK),
  {
    0x00000000,
    0x0000,
    0x0000,
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
  },
  NULL
};

GLOBAL_REMOVE_IF_UNREFERENCED CONST EFI_HII_IMAGE_PROTOCOL mImageProtocol = {
  HiiNewImage,
  HiiGetImage,
  HiiSetImage,
  HiiDrawImage,
  HiiDrawImageId
};

/**
  The default event handler for gHiiKeyboardLayoutChanged
  event group.

  This is internal function.

  @param Event           The event that triggered this notification function.
  @param Context         Pointer to the notification functions context.

**/
VOID
EFIAPI
KeyboardLayoutChangeNullEvent (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  return;
}

/**
  Initialize HII Database.


  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval EFI_SUCCESS    The Hii database is setup correctly.
  @return Other value if failed to create the default event for
          gHiiKeyboardLayoutChanged. Check gBS->CreateEventEx for
          details. Or failed to insatll the protocols.
          Check gBS->InstallMultipleProtocolInterfaces for details.

**/
EFI_STATUS
EFIAPI
InitializeHiiDatabase (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                             Status;
  EFI_HANDLE                             Handle;

  //
  // There will be only one HII Database in the system
  // If there is another out there, someone is trying to install us
  // again.  Fail that scenario.
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiHiiDatabaseProtocolGuid);
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiHiiFontProtocolGuid);
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiHiiImageProtocolGuid);
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiHiiStringProtocolGuid);
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiHiiConfigRoutingProtocolGuid);
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiConfigKeywordHandlerProtocolGuid);
  
  InitializeListHead (&mPrivate.DatabaseList);
  InitializeListHead (&mPrivate.DatabaseNotifyList);
  InitializeListHead (&mPrivate.HiiHandleList);
  InitializeListHead (&mPrivate.FontInfoList);

  //
  // Create a event with EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID group type.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  KeyboardLayoutChangeNullEvent,
                  NULL,
                  &gEfiHiiKeyBoardLayoutGuid,
                  &gHiiKeyboardLayoutChanged
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiHiiFontProtocolGuid,
                  &mPrivate.HiiFont,
                  &gEfiHiiStringProtocolGuid,
                  &mPrivate.HiiString,
                  &gEfiHiiDatabaseProtocolGuid,
                  &mPrivate.HiiDatabase,
                  &gEfiHiiConfigRoutingProtocolGuid,
                  &mPrivate.ConfigRouting,
                  &gEfiConfigKeywordHandlerProtocolGuid,
                  &mPrivate.ConfigKeywordHandler,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FeaturePcdGet (PcdSupportHiiImageProtocol)) {
    CopyMem (&mPrivate.HiiImage, &mImageProtocol, sizeof (mImageProtocol));

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Handle,
                    &gEfiHiiImageProtocolGuid,
                    &mPrivate.HiiImage,
                    NULL
                    );

  }

  return Status;
}

