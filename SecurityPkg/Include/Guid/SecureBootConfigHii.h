/** @file
  GUIDs used as HII FormSet and HII Package list GUID in SecureBootConfigDxe driver. 
  
Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SECUREBOOT_CONFIG_HII_GUID_H__
#define __SECUREBOOT_CONFIG_HII_GUID_H__

#define SECUREBOOT_CONFIG_FORM_SET_GUID \
  { \
    0x5daf50a5, 0xea81, 0x4de2, {0x8f, 0x9b, 0xca, 0xbd, 0xa9, 0xcf, 0x5c, 0x14} \
  }


extern EFI_GUID gSecureBootConfigFormSetGuid;

#endif
