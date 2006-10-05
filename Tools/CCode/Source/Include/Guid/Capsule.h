/** @file
  GUIDs used for EFI Capsule

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  Capsule.h

  @par Revision Reference:
  GUIDs defined in Capsule Spec Version 0.9

**/

#ifndef __CAPSULE_GUID_H__
#define __CAPSULE_GUID_H__

//
// This is the GUID of the capsule header of the image on disk.
//
#define EFI_CAPSULE_GUID \
  { \
    0x3B6686BD, 0x0D76, 0x4030, {0xB7, 0x0E, 0xB5, 0x51, 0x9E, 0x2F, 0xC5, 0xA0 } \
  }

//
// This is the GUID of the configuration results file created by the capsule
// application.
//
#define EFI_CONFIG_FILE_NAME_GUID \
  { \
    0x98B8D59B, 0xE8BA, 0x48EE, {0x98, 0xDD, 0xC2, 0x95, 0x39, 0x2F, 0x1E, 0xDB } \
  }

extern EFI_GUID gEfiCapsuleGuid;
extern EFI_GUID gEfiConfigFileNameGuid;

#endif
