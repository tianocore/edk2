/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  WinNtThunk.c

Abstract:

  This protocol allows an EFI driver (DLL) in the NT emulation envirnment
  to make Win32 API calls.

--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (WinNtThunk)

EFI_GUID  gEfiWinNtThunkProtocolGuid = EFI_WIN_NT_THUNK_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiWinNtThunkProtocolGuid, "EFI Win NT Thunk", "Win32 API thunk protocol");
