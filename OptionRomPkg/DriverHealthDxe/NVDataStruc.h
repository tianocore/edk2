/*++

Copyright (c) 2006 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  NVDataStruc.h

Abstract:

  NVData structure used by the sample driver
 
Revision History: 
 
--*/

#ifndef _NVDATASTRUC_H
#define _NVDATASTRUC_H

#define EFI_CALLER_ID_GUID {0xD1D9694C, 0x90B4, 0x46a7, {0x87, 0x53, 0x60, 0x41, 0x3C, 0xEF, 0xE0, 0xA1}}

#pragma pack(1)
typedef struct {
  UINT8  ConfigGood;
} DISK_IO_NV_DATA;
#pragma pack()

#endif
