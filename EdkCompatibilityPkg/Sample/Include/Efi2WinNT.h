/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Efi2WinNT.h

Abstract:

  Tiano mechanism to create WIN32 EFI code.

  Only used for NT emulation environment. This file replaces the Tiano.h
  when you need to access Win32 stuff in EFI.

  Don't access Win32 APIs directly allways go WinNtThunk protocol.

  In most envirnments gWinNt is a pointer to a EFI_WIN_NT_THUNK_PROTOCOL.

--*/

#ifndef _EFI_WIN_NT_H_
#define _EFI_WIN_NT_H_

//
// Win32 include files do not compile clean with /W4, so we use the warning
// pragma to suppress the warnings for Win32 only. This way our code can stil
// compile at /W4 (highest warning level) with /WX (warnings cause build
// errors).
//
#pragma warning(disable : 4115)
#pragma warning(disable : 4201)
#pragma warning(disable : 4214)
#pragma warning(disable : 4028)
#pragma warning(disable : 4133)

#include "windows.h"

//
// Set the warnings back on so the EFI code must be /W4.
//
#pragma warning(default : 4115)
#pragma warning(default : 4201)
#pragma warning(default : 4214)
//
// We replaced Tiano.h so we must include it.
//
#include "Tiano.h"

#include EFI_PROTOCOL_DEFINITION (WinNtThunk)

#endif
