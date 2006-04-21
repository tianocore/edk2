/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FwVolAttrib.c

Abstract:

  Implements get/set firmware volume attributes

--*/

#include <DxeMain.h>

EFI_STATUS
EFIAPI
FvGetVolumeAttributes (
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL  *This,
  OUT EFI_FV_ATTRIBUTES             *Attributes
  )
/*++

Routine Description:
    Retrieves attributes, insures positive polarity of attribute bits, returns
    resulting attributes in output parameter

Arguments:
    This        - Calling context
    Attributes  - output buffer which contains attributes

Returns:
    EFI_SUCCESS         - Successfully got volume attributes

--*/
{
  EFI_STATUS                                Status;
  FV_DEVICE                                 *FvDevice;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL        *Fvb;
  EFI_FVB_ATTRIBUTES                        FvbAttributes;

  FvDevice = FV_DEVICE_FROM_THIS (This);
  Fvb = FvDevice->Fvb;

  if (FvDevice->CachedFv == NULL) {
    Status = FvCheck (FvDevice);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // First get the Firmware Volume Block Attributes
  //
  Status = Fvb->GetVolumeAttributes (Fvb, &FvbAttributes);

  //
  // Mask out Fvb bits that are not defined in FV 
  //
  FvbAttributes &= 0xfffff0ff;
  
  *Attributes = (EFI_FV_ATTRIBUTES)FvbAttributes; 
  
  return Status;
}


EFI_STATUS
EFIAPI
FvSetVolumeAttributes (
  IN EFI_FIRMWARE_VOLUME_PROTOCOL   *This,
  IN OUT EFI_FV_ATTRIBUTES          *Attributes
  )
/*++

Routine Description:
    Sets current attributes for volume

Arguments:
    This       - Calling context
    Attributes - At input, contains attributes to be set.  At output contains
      new value of FV

Returns:
    EFI_UNSUPPORTED   - Could not be set.

--*/
{
  return EFI_UNSUPPORTED;
}

