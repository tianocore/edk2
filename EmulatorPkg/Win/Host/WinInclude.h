/**@file
  Public include file for the WinNt Library

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __WIN_NT_INCLUDE_H__
#define __WIN_NT_INCLUDE_H__

#undef VOID

#define GUID              _WINNT_DUP_GUID_____
#define _LIST_ENTRY       _WINNT_DUP_LIST_ENTRY_FORWARD
#define LIST_ENTRY        _WINNT_DUP_LIST_ENTRY
#define RUNTIME_FUNCTION  _WINNT_DUP_RUNTIME_FUNCTION

#include "windows.h"
#include "windowsx.h"

#undef GUID
#undef _LIST_ENTRY
#undef LIST_ENTRY
#undef RUNTIME_FUNCTION
//
// Prevent collision with FAR field of EFI_SYSTEM_CONTEXT_AARCH64
//
#undef FAR

//
// Windows SDKs define IMAGE_FILE_MACHINE_ARM64 as "0xAA64"
// MdePkg defines EFI_IMAGE_FILE_MACHINE_ARM64 as "0xAA64"
// Mingw ucrt winnt.h defines IMAGE_FILE_MACHINE_ARM64 as "0xaa64" which is not
// the exact same string as "0xAA64". This generates a macro redefinition error
// with Mingw clang builds.
//
// Redefining a macro to the exact same string is not flagged an error.
//
// If IMAGE_FILE_MACHINE_ARM64 is defined, check if it matches the unexpected
// value of "0xaa64". If "0xaa64" is detected, then undefine and redefine to
// "0xAA64" which is the same value used in Windows SDKs and MdePkg includes.
//
#if defined (IMAGE_FILE_MACHINE_ARM64)
  #if IMAGE_FILE_MACHINE_ARM64 == 0xaa64
    #undef IMAGE_FILE_MACHINE_ARM64
#define IMAGE_FILE_MACHINE_ARM64  0xAA64
  #endif
#endif

#endif
