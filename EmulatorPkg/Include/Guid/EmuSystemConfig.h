/** @file
  Setup Variable data structure for Emu platform.

Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef __EMU_SYSTEM_CONFIG_H__
#define __EMU_SYSTEM_CONFIG_H__

#define EFI_EMU_SYSTEM_CONFIG_GUID  \
 { 0x9C4FB516, 0x3A1E, 0xD847, { 0xA1, 0xA1, 0x70, 0x58, 0xB6, 0x98, 0x67, 0x32 } }


#pragma pack(1)
typedef struct {
  //
  // Console output mode
  //
  UINT32        ConOutColumn;
  UINT32        ConOutRow;
} EMU_SYSTEM_CONFIGURATION;
#pragma pack()


extern EFI_GUID   gEmuSystemConfigGuid;

#endif
