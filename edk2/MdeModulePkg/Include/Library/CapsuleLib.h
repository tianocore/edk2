/** @file

  This library class defines a set of interfaces on how to process capusle image update.

  Copyright (c) 2007 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __CAPSULE_LIB_H__
#define __CAPSULE_LIB_H__

/**
  The firmware checks whether the capsule image is supported 
  by the CapsuleGuid in CapsuleHeader or other specific information in capsule image.
  
  @param  CapsuleHeader    Point to the UEFI capsule image to be checked.
  
  @retval EFI_SUCESS       Input capsule is supported by firmware.
  @retval EFI_UNSUPPORTED  Input capsule is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
SupportCapsuleImage (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  );

/**
  The firmware specific implementation processes the capsule image
  if it recognized the format of this capsule image.
  
  @param  CapsuleHeader    Point to the UEFI capsule image to be processed. 
   
  @retval EFI_SUCESS       Process Capsule Image successfully. 
  @retval EFI_UNSUPPORTED  Capsule image is not supported by the firmware.
**/
EFI_STATUS
EFIAPI
ProcessCapsuleImage (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
  );

#endif
