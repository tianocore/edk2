/** @file
  Main PAL API's defined in IPF PAL Spec.

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  PalApi.h

**/

#ifndef __PAL_API_H__
#define __PAL_API_H__

//
// IPF Specific Functions
//

typedef struct {
  UINT64                    Status;
  UINT64                    r9;
  UINT64                    r10;
  UINT64                    r11;
} PAL_CALL_RETURN;

#endif
