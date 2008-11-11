/*++
 
Copyright (c) 2004 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  TianoSpecDevicePath.h

Abstract:

  Tiano Device Path definitions in Tiano Spec.

--*/

#ifndef _TIANO_SPEC_DEVICE_PATH_H
#define _TIANO_SPEC_DEVICE_PATH_H

#pragma pack(1)

//
// EFI Specification extension on Media Device Path. MEDIA_FW_VOL_FILEPATH_DEVICE_PATH is adopted by UEFI later and added in UEFI2.10. 
// In EdkCompatibility Package, we only support MEDIA_FW_VOL_FILEPATH_DEVICE_PATH that complies with
// EFI 1.10 and UEFI 2.10.
//
#define MEDIA_FV_FILEPATH_DP  0x06
typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  NameGuid;
} MEDIA_FW_VOL_FILEPATH_DEVICE_PATH;


#if (EFI_SPECIFICATION_VERSION > 0x00020000)
//
// EFI Specification extension on Media Device Path was added to UEFI 2.1 specification,
// so the definition is moved to EfiDevicePath.h
//
#endif

#pragma pack()
#endif
