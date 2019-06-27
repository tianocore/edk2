/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:
  WinNtLib.h

Abstract:
  Public include file for the WinNt Library

**/

#ifndef __WIN_NT_DXE_H__
#define __WIN_NT_DXE_H__

//
// This forces Windows.h WIN32 include file to be included
//  it's needed for WinNtThunk.h
//  WinNtIo.h depends on WinNtThunk.h
//
#include <Common/WinNtInclude.h>
#endif
