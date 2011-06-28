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

#ifndef __EMU_VIRTUAL_DISK_H__
#define __EMU_VIRTUAL_DISK_H__

#define EFI_EMU_VIRTUAL_DISK_GUID  \
 { 0xf2ba331a, 0x8985, 0x11db, { 0xa4, 0x06, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } }

extern EFI_GUID   gEmuVirtualDisksGuid;

#endif
