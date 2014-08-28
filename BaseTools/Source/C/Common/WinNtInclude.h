/** @file
Include file for the WinNt Library

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __WIN_NT_INCLUDE_H__
#define __WIN_NT_INCLUDE_H__

#define GUID  _WINNT_DUP_GUID_____
#define _LIST_ENTRY  _WINNT_DUP_LIST_ENTRY_FORWARD
#define LIST_ENTRY   _WINNT_DUP_LIST_ENTRY

#if (_MSC_VER < 1800)
#define InterlockedIncrement _WINNT_DUP_InterlockedIncrement
#define InterlockedDecrement _WINNT_DUP_InterlockedDecrement
#define InterlockedCompareExchange64 _WINNT_DUP_InterlockedCompareExchange64
#endif

#undef UNALIGNED
#undef CONST
#undef VOID

#ifndef __GNUC__
#include "windows.h"

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

//
// Set the warnings back on as the EFI code must be /W4.
//
#pragma warning(default : 4115)
#pragma warning(default : 4201)
#pragma warning(default : 4214)

#endif

#undef GUID
#undef _LIST_ENTRY
#undef LIST_ENTRY
#undef InterlockedIncrement
#undef InterlockedDecrement
#undef InterlockedCompareExchange64
#undef InterlockedCompareExchangePointer

#define VOID void

//
// Prevent collisions with Windows API name macros that deal with Unicode/Not issues
//
#undef LoadImage
#undef CreateEvent

#endif
