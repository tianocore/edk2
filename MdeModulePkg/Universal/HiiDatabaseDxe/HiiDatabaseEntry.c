/** @file

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    HiiDatabaseEntry.c

Abstract:

    This file contains the entry code to the HII database, which is defined by
    UEFI 2.1 specification.

Revision History


**/


#include "HiiDatabase.h"

//
// Global variables
//
EFI_EVENT gHiiKeyboardLayoutChanged;
STATIC EFI_GUID gHiiSetKbdLayoutEventGuid = EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID;

STATIC HII_DATABASE_PRIVATE_DATA mPrivate = {
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
#ifndef DISABLE_UNUSED_HII_PROTOCOLS
  {
    HiiNewImage,
    HiiGetImage,
    HiiSetImage,
    HiiDrawImage,
    HiiDrawImageId
  },
#endif
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

STATIC
VOID
EFIAPI
KeyboardLayoutChangeNullEvent (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  return;
}

EFI_STATUS
EFIAPI
InitializeHiiDatabase (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:
  Initialize HII Database

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:
  EFI_SUCCESS -
  other       -

--*/
{
  EFI_STATUS                             Status;
  EFI_HANDLE                             Handle;
  EFI_HANDLE                             *HandleBuffer;
  UINTN                                  HandleCount;

  //
  // There will be only one HII Database in the system
  // If there is another out there, someone is trying to install us
  // again.  Fail that scenario.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  //
  // If there was no error, assume there is an installation and fail to load
  //
  if (!EFI_ERROR (Status)) {
    if (HandleBuffer != NULL) {
      gBS->FreePool (HandleBuffer);
    }
    return EFI_DEVICE_ERROR;
  }

  InitializeListHead (&mPrivate.DatabaseList);
  InitializeListHead (&mPrivate.DatabaseNotifyList);
  InitializeListHead (&mPrivate.HiiHandleList);
  InitializeListHead (&mPrivate.FontInfoList);

  //
  // Create a event with EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID group type.
  //
  Status = gBS->CreateEventEx (
                  EFI_EVENT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  KeyboardLayoutChangeNullEvent,
                  NULL,
                  &gHiiSetKbdLayoutEventGuid,
                  &gHiiKeyboardLayoutChanged
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Handle = NULL;
  return gBS->InstallMultipleProtocolInterfaces (
                &Handle,
                &gEfiHiiFontProtocolGuid,
                &mPrivate.HiiFont,
#ifndef DISABLE_UNUSED_HII_PROTOCOLS
                &gEfiHiiImageProtocolGuid,
                &mPrivate.HiiImage,
#endif
                &gEfiHiiStringProtocolGuid,
                &mPrivate.HiiString,
                &gEfiHiiDatabaseProtocolGuid,
                &mPrivate.HiiDatabase,
                &gEfiHiiConfigRoutingProtocolGuid,
                &mPrivate.ConfigRouting,
                NULL
                );
}

