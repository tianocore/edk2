/**@file
  Setup Variable data structure for NT32 platform.

Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef __WIN_NT_SYSTEM_CONFIGUE_H__
#define __WIN_NT_SYSTEM_CONFIGUE_H__

#define EFI_WIN_NT_SYSTEM_CONFIG_GUID  \
  { 0xb347f047, 0xaf8c, 0x490e, { 0xac, 0x07, 0x0a, 0xa9, 0xb7, 0xe5, 0x38, 0x58 }}

#pragma pack(1)
typedef struct {
  //
  // Console output mode
  //
  UINT32        ConOutColumn;
  UINT32        ConOutRow;
} WIN_NT_SYSTEM_CONFIGURATION;
#pragma pack()


extern EFI_GUID   gEfiWinNtSystemConfigGuid;

#endif
