/** @file
  Emulator Thunk to abstract OS services from pure EFI code

  Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/EmuIoThunk.h>


#define EMU_IO_THUNK_PROTOCOL_DATA_SIGNATURE SIGNATURE_32('E','m','u','T')

typedef struct {
  UINTN                 Signature;
  EMU_IO_THUNK_PROTOCOL Data;
  BOOLEAN               EmuBusDriver;
  LIST_ENTRY            Link;
} EMU_IO_THUNK_PROTOCOL_DATA;

LIST_ENTRY  mThunkList = INITIALIZE_LIST_HEAD_VARIABLE (mThunkList);


EFI_STATUS
EFIAPI
AddThunkProtocol (
  IN  EMU_IO_THUNK_PROTOCOL   *ThunkIo,
  IN  CHAR16                  *ConfigString,
  IN  BOOLEAN                 EmuBusDriver
  )
{
  CHAR16                      *StartString;
  CHAR16                      *SubString;
  UINTN                       Instance;
  EMU_IO_THUNK_PROTOCOL_DATA  *Private;

  if (ThunkIo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = 0;
  StartString = AllocatePool (StrSize (ConfigString));
  StrCpy (StartString, ConfigString);
  while (*StartString != '\0') {

    //
    // Find the end of the sub string
    //
    SubString = StartString;
    while (*SubString != '\0' && *SubString != '!') {
      SubString++;
    }

    if (*SubString == '!') {
      //
      // Replace token with '\0' to make sub strings. If this is the end
      //  of the string SubString will already point to NULL.
      //
      *SubString = '\0';
      SubString++;
    }

    Private = AllocatePool (sizeof (EMU_IO_THUNK_PROTOCOL_DATA));
    if (Private == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Private->Signature          = EMU_IO_THUNK_PROTOCOL_DATA_SIGNATURE;
    Private->EmuBusDriver       = EmuBusDriver;

    CopyMem (&Private->Data, ThunkIo, sizeof (EMU_IO_THUNK_PROTOCOL));
    Private->Data.Instance      = Instance++;
    Private->Data.ConfigString  = StartString;

    InsertTailList (&mThunkList, &Private->Link);

    //
    // Parse Next sub string. This will point to '\0' if we are at the end.
    //
    StartString = SubString;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
GetNextThunkProtocol (
  IN  BOOLEAN                 EmuBusDriver,
  OUT EMU_IO_THUNK_PROTOCOL   **Instance  OPTIONAL
  )
{
  LIST_ENTRY                   *Link;
  EMU_IO_THUNK_PROTOCOL_DATA   *Private;

  if (mThunkList.ForwardLink == &mThunkList) {
    // Skip parsing an empty list
    return EFI_NOT_FOUND;
  }

  for (Link = mThunkList.ForwardLink; Link != &mThunkList; Link = Link->ForwardLink) {
    Private = CR (Link, EMU_IO_THUNK_PROTOCOL_DATA, Link, EMU_IO_THUNK_PROTOCOL_DATA_SIGNATURE);
    if (EmuBusDriver & !Private->EmuBusDriver) {
      continue;
    } else if (*Instance == NULL) {
      // Find 1st match in list
      *Instance = &Private->Data;
      return EFI_SUCCESS;
    } else if (*Instance == &Private->Data) {
      // Matched previous call so look for valid next entry
      Link = Link->ForwardLink;
      if (Link == &mThunkList) {
        return EFI_NOT_FOUND;
      }
      Private = CR (Link, EMU_IO_THUNK_PROTOCOL_DATA, Link, EMU_IO_THUNK_PROTOCOL_DATA_SIGNATURE);
      *Instance = &Private->Data;
      return EFI_SUCCESS;
    }
  }


  return EFI_NOT_FOUND;
}

