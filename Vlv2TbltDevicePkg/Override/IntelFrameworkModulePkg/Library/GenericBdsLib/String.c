/** @file
  String support

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "String.h"

/**
  Get string by string id from HII Interface


  @param Id              String ID.

  @retval  CHAR16 *  String from ID.
  @retval  NULL      If error occurs.

**/
CHAR16 *
BdsLibGetStringById (
  IN  EFI_STRING_ID   Id
  )
{
  return HiiGetString (gBdsLibStringPackHandle, Id, NULL);
}
