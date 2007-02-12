/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  ShadowRom.c

Abstract:

  Shadow all option rom

Revision History

--*/

UINT8 mShadowRomFlag = 0;

VOID
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
                    &LegacyBios
                    );
    if (!EFI_ERROR (Status)) {
      LegacyBios->PrepareToBootEfi (LegacyBios, NULL, NULL);
    }

    mShadowRomFlag = 1;
  }

  return ;
}
