/** @file
  Setup Variable data structure for Emu platform.

Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


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
