/*++ @file

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved. 
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/EmuThunkLib.h>

#include <Protocol/EmuThunk.h>


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
