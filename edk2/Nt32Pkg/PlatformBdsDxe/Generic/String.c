/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  string.c

Abstract:

  String support

Revision History

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "Bds.h"
#include "String.h"
#include "Language.h"

EFI_GUID      gBdsStringPackGuid = { 0x7bac95d3, 0xddf, 0x42f3, 0x9e, 0x24, 0x7c, 0x64, 0x49, 0x40, 0x37, 0x9a };
extern        UINT8 PlatformBdsStrings[];

EFI_STATUS
InitializeStringSupport (
  VOID
  )
/*++

Routine Description:

  Initialize HII global accessor for string support

Arguments:
  None

Returns:
  String from ID.

--*/
{
  EFI_STATUS        Status;
  EFI_HII_PACKAGES  *PackageList;
  //
  // There should only ever be one HII protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiProtocolGuid,
                  NULL,
                  &Hii
                  );
  if (!EFI_ERROR (Status)) {
    PackageList = PreparePackages (1, &gBdsStringPackGuid, PlatformBdsStrings);
    Status      = Hii->NewPack (Hii, PackageList, &gStringPackHandle);
    FreePool (PackageList);
  }

  return Status;
}

CHAR16 *
GetStringById (
  IN  STRING_REF   Id
  )
/*++

Routine Description:

  Get string by string id from HII Interface

Arguments:

  Id       - String ID.

Returns:

  CHAR16 * - String from ID.
  NULL     - If error occurs.

--*/
{
  CHAR16      *String;
  UINTN       StringLength;
  EFI_STATUS  Status;

  //
  // Set default string size assumption at no more than 256 bytes
  //
  StringLength  = 0x100;

  String        = AllocateZeroPool (StringLength);
  if (String == NULL) {
    //
    // If this happens, we are oh-so-dead, but return a NULL in any case.
    //
    return NULL;
  }
  //
  // Get the current string for the current Language
  //
  Status = Hii->GetString (Hii, gStringPackHandle, Id, FALSE, NULL, &StringLength, String);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_BUFFER_TOO_SMALL) {
      //
      // Free the old pool
      //
      FreePool (String);

      //
      // Allocate new pool with correct value
      //
      String = AllocatePool (StringLength);
      ASSERT (String != NULL);

      Status = Hii->GetString (Hii, gStringPackHandle, Id, FALSE, NULL, &StringLength, String);
      if (!EFI_ERROR (Status)) {
        return String;
      }
    }

    return NULL;
  }

  return String;
}
