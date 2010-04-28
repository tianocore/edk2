/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

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
