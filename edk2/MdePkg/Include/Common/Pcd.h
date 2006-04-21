/** @file
  This file defines the common macro and data structure shared between PCD PEIM and
  PCD DXE driver.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  Pcd.h

**/
#ifndef _COMMON_PCD_H
#define _COMMON_PCD_H

typedef UINT32  PCD_TOKEN_NUMBER;
typedef UINT8   SKU_ID;

#define PCD_INVALID_TOKEN     ((PCD_TOKEN_NUMBER)(-1))

typedef
VOID
(EFIAPI *PCD_PROTOCOL_CALLBACK) (
  IN  UINT32       CallBackToken,
  IN  VOID         *TokenData,
  IN  UINTN        TokenDataSize
  );

#endif
