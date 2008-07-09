/** @file
  Shadow all option rom

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//@MT:#include "Tiano.h"
//@MT:#include "EfiDriverLib.h"

//@MT:#include EFI_PROTOCOL_DEFINITION (LegacyBios)

#include "InternalBdsLib.h"

UINT8 mShadowRomFlag = 0;

VOID
EFIAPI
ShadowAllOptionRom()
{
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  //
  // Rom shadow only do once.
  //
  if (mShadowRomFlag == 0) {
    Status = gBS->LocateProtocol (
                    &gEfiLegacyBiosProtocolGuid,
                    NULL,
                    (VOID **) &LegacyBios
                    );
    if (!EFI_ERROR (Status)) {
      LegacyBios->PrepareToBootEfi (LegacyBios, NULL, NULL);
    }

    mShadowRomFlag = 1;
  }

  return ;
}
