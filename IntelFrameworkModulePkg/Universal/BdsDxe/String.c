/** @file
  String support

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Bds.h"
#include "Language.h"
#include "FrontPage.h"

EFI_HII_HANDLE gStringPackHandle;

EFI_GUID mBdsStringPackGuid = {
  0x7bac95d3, 0xddf, 0x42f3, {0x9e, 0x24, 0x7c, 0x64, 0x49, 0x40, 0x37, 0x9a}
};

/**
  Initialize HII global accessor for string support.

**/
VOID
InitializeStringSupport (
  VOID
  )
{
  gStringPackHandle = HiiAddPackages (
                         &mBdsStringPackGuid,
                         gImageHandle,
                         BdsDxeStrings,
                         NULL
                         );
  ASSERT (gStringPackHandle != NULL);
}

/**
  Get string by string id from HII Interface


  @param Id              String ID.

  @retval  CHAR16 *  String from ID.
  @retval  NULL      If error occurs.

**/
CHAR16 *
GetStringById (
  IN  EFI_STRING_ID   Id
  )
{
  return HiiGetString (gStringPackHandle, Id, NULL);
}
