/** @file
  GUIDs used as HII FormSet and HII Package list GUID in HTTP boot driver.
  
Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __HTTP_BOOT_HII_GUID_H__
#define __HTTP_BOOT_HII_GUID_H__

#define HTTP_BOOT_CONFIG_GUID \
  { \
    0x4d20583a, 0x7765, 0x4e7a, { 0x8a, 0x67, 0xdc, 0xde, 0x74, 0xee, 0x3e, 0xc5 } \
  }

extern EFI_GUID gHttpBootConfigGuid;

#endif
