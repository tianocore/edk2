/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  WinNtThunk.h

Abstract:

**/

#ifndef _WIN_NT_THUNK_H_
#define _WIN_NT_THUNK_H_

// TODO: add protective #ifndef


//
// WinNtThunk Device Path Protocol Instance Type
//
typedef struct {
  VENDOR_DEVICE_PATH        Vendor;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} WIN_NT_THUNK_DEVICE_PATH;

#endif
