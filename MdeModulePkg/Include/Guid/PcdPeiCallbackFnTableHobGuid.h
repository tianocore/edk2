/** @file
  Hob guid for Pcd Pei Callback Function Table

  Copyright (c) 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PCD_PEI_CALLBACK_FNTABLE_HOB_GUID_H_
#define _PCD_PEI_CALLBACK_FNTABLE_HOB_GUID_H_

#define PCD_PEI_CALLBACK_FNTABLE_HOB_GUID \
  { \
    0xC625F4B2, 0xEA09, 0x4675, { 0x82, 0xD7, 0xBA, 0x36, 0x82, 0x15, 0x7A, 0x14 } \
  }

extern EFI_GUID gPcdPeiCallbackFnTableHobGuid;

#endif
