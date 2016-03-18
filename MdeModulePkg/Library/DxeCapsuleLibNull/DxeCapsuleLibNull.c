/** @file
  Null Dxe Capsule Library instance does nothing and returns unsupport status.

Copyright (c) 2007 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Uefi.h>
#include <Library/CapsuleLib.h>

/**
  The firmware checks whether the capsule image is supported 
  by the CapsuleGuid in CapsuleHeader or other specific information in capsule image.
  
  @param  CapsuleHeader    Point to the UEFI capsule image to be checked.
  
  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
SupportCapsuleImage (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  return EFI_UNSUPPORTED;
}

/**
  The firmware specific implementation processes the capsule image
  if it recognized the format of this capsule image.
  
  @param  CapsuleHeader    Point to the UEFI capsule image to be processed. 
   
  @retval EFI_UNSUPPORTED  Capsule image is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
ProcessCapsuleImage (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  )
{
  return EFI_UNSUPPORTED;
}


