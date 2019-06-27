/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  WinNtLib.c

Abstract:

  WinNt Library 

**/

#include <PiDxe.h>
#include <WinNtDxe.h>
#include <Library/WinNtLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>


EFI_WIN_NT_THUNK_PROTOCOL *gWinNt;

EFI_STATUS
WinNtLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

Arguments:

Returns:

--*/
{
  EFI_HOB_GUID_TYPE        *GuidHob;

  GuidHob = GetFirstGuidHob (&gEfiWinNtThunkProtocolGuid);
  ASSERT (GuidHob != NULL);
  gWinNt = (EFI_WIN_NT_THUNK_PROTOCOL *)(*(UINTN *)(GET_GUID_HOB_DATA (GuidHob)));
  ASSERT (gWinNt != NULL);
  return EFI_SUCCESS;
}
