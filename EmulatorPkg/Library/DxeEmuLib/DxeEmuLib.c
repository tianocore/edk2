/*++ @file

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/EmuThunkLib.h>
#include <Library/BaseMemoryLib.h>

EMU_THUNK_PROTOCOL   *gEmuThunk = NULL;


/**
  The constructor function caches the pointer of EMU Thunk protocol.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeEmuLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HOB_GUID_TYPE        *GuidHob;

  GuidHob = GetFirstGuidHob (&gEmuThunkProtocolGuid);
  ASSERT (GuidHob != NULL);

  gEmuThunk = (EMU_THUNK_PROTOCOL *)(*(UINTN *)(GET_GUID_HOB_DATA (GuidHob)));
  ASSERT (gEmuThunk != NULL);

  return EFI_SUCCESS;
}


/**
  Serach the EMU IO Thunk database for a matching EMU IO Thunk
  Protocol instance.

  @param  Protocol   Protocol to search for.
  @param  Instance   Instance of protocol to search for.

  @retval NULL       Protocol and Instance not found.
  @retval other      EMU IO Thunk protocol that matched.

**/
EMU_IO_THUNK_PROTOCOL *
EFIAPI
GetIoThunkInstance (
  IN  EFI_GUID  *Protocol,
  IN  UINTN     Instance
  )
{
  EFI_STATUS              Status;
  EMU_IO_THUNK_PROTOCOL   *EmuIoThunk;

  for (Status = EFI_SUCCESS, EmuIoThunk = NULL; !EFI_ERROR (Status); ) {
    Status = gEmuThunk->GetNextProtocol (FALSE, &EmuIoThunk);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (EmuIoThunk->Instance == Instance) {
      if (CompareGuid (EmuIoThunk->Protocol, Protocol)) {
        return EmuIoThunk;
      }
    }
  }

  return NULL;
}
