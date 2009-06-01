/** @file
  GUID used to pass DEBUG() macro information through the Status Code Protocol
  and Status Code PPI

  Copyright (c) 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _STATUS_CODE_DATA_TYPE_DEBUG_H_
#define _STATUS_CODE_DATA_TYPE_DEBUG_H_

//
// Debug DataType defintions.
//

#define EFI_STATUS_CODE_DATA_TYPE_DEBUG_GUID \
  { 0x9A4E9246, 0xD553, 0x11D5, { 0x87, 0xE2, 0x00, 0x06, 0x29, 0x45, 0xC3, 0xb9 } }

typedef struct {
  UINT32  ErrorLevel;
  //
  // 12 * sizeof (UINT64) Var Arg stack
  //
  // ascii DEBUG () Format string
  //
} EFI_DEBUG_INFO;

extern EFI_GUID gEfiStatusCodeDataTypeDebugGuid;

#endif // _STATUS_CODE_DATA_TYPE_DEBUG_H_
