/** @file
  String support

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _STRING_H_
#define _STRING_H_

#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

extern EFI_HII_HANDLE gBdsLibStringPackHandle;

//
// This is the VFR compiler generated header file which defines the
// string identifiers.
//

extern UINT8  GenericBdsLibStrings[];

/**
  Get string by string id from HII Interface


  @param Id              String ID.

  @retval  CHAR16 *  String from ID.
  @retval  NULL      If error occurs.

**/
CHAR16 *
BdsLibGetStringById (
  IN  EFI_STRING_ID   Id
  );

#endif // _STRING_H_
