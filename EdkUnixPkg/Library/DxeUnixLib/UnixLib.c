/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixLib.c

Abstract:

  Unix Library 

--*/



EFI_UNIX_THUNK_PROTOCOL *gUnix;

EFI_STATUS
UnixLibConstructor (
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

  GuidHob = GetFirstGuidHob (&gEfiUnixThunkProtocolGuid);
  ASSERT (GuidHob != NULL);
  gUnix = (EFI_UNIX_THUNK_PROTOCOL *)(*(UINTN *)(GET_GUID_HOB_DATA (GuidHob)));
  ASSERT (gUnix != NULL);
  return EFI_SUCCESS;
}
