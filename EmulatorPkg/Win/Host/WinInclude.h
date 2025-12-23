/**@file
  Public include file for the WinNt Library

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __WIN_NT_INCLUDE_H__
#define __WIN_NT_INCLUDE_H__

#define GUID              _WINNT_DUP_GUID_____
#define _LIST_ENTRY       _WINNT_DUP_LIST_ENTRY_FORWARD
#define LIST_ENTRY        _WINNT_DUP_LIST_ENTRY
#define RUNTIME_FUNCTION  _WINNT_DUP_RUNTIME_FUNCTION
#if defined (MDE_CPU_IA32) && (_MSC_VER < 1800)
#define InterlockedIncrement          _WINNT_DUP_InterlockedIncrement
#define InterlockedDecrement          _WINNT_DUP_InterlockedDecrement
#define InterlockedCompareExchange64  _WINNT_DUP_InterlockedCompareExchange64
#endif
#undef UNALIGNED
#undef CONST
#undef VOID
#undef DEBUG_EVENT

// WQBugBug: This typedef is to make "windows.h" buildable.
//                   It should be removed after the root cause why
//                   size_t is undefined when go into the line below is found.
#if defined (MDE_CPU_IA32)
typedef UINT32 size_t;
#endif

#include "windows.h"
#include "windowsx.h"

#undef GUID
#undef _LIST_ENTRY
#undef LIST_ENTRY
#undef RUNTIME_FUNCTION
#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedCompareExchange64
#undef InterlockedCompareExchangePointer
#undef CreateEventEx
#undef IMAGE_FILE_MACHINE_ARM64

#define VOID  void

//
// Prevent collisions with Windows API name macros that deal with Unicode/Not issues
//
#undef LoadImage
#undef CreateEvent
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
