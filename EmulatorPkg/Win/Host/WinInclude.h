/**@file
  Public include file for the WinNt Library

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __WIN_NT_INCLUDE_H__
#define __WIN_NT_INCLUDE_H__

//
// Win32 include files do not compile clean with /W4, so we use the warning
// pragma to suppress the warnings for Win32 only. This way our code can stil
// compile at /W4 (highest warning level) with /WX (warnings cause build
// errors).
//
#pragma warning(disable : 4115)
#pragma warning(disable : 4201)
#pragma warning(disable : 4028)
#pragma warning(disable : 4133)

#define GUID  _WINNT_DUP_GUID_____
#define _LIST_ENTRY  _WINNT_DUP_LIST_ENTRY_FORWARD
#define LIST_ENTRY   _WINNT_DUP_LIST_ENTRY
#if defined (MDE_CPU_IA32) && (_MSC_VER < 1800)
#define InterlockedIncrement _WINNT_DUP_InterlockedIncrement
#define InterlockedDecrement _WINNT_DUP_InterlockedDecrement
#define InterlockedCompareExchange64 _WINNT_DUP_InterlockedCompareExchange64
#endif
#undef UNALIGNED
#undef CONST
#undef VOID
#undef DEBUG_EVENT

// WQBugBug: This typedef is to make "windows.h" buildable.
//                   It should be removed after the root cause why
//                   size_t is undefined when go into the line below is found.
#if defined (MDE_CPU_IA32)
typedef UINT32 size_t ;
#endif

#include "windows.h"
#include "windowsx.h"

#undef GUID
#undef _LIST_ENTRY
#undef LIST_ENTRY
#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedCompareExchange64
#undef InterlockedCompareExchangePointer
#undef CreateEventEx

#define VOID void

//
// Prevent collisions with Windows API name macros that deal with Unicode/Not issues
//
#undef LoadImage
#undef CreateEvent
#undef FAR

//
// Set the warnings back on as the EFI code must be /W4.
//
#pragma warning(default : 4115)
#pragma warning(default : 4201)


#endif
