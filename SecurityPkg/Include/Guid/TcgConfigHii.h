/** @file
  GUIDs used as HII FormSet and HII Package list GUID in TcgConfig driver. 
  
Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TCG_CONFIG_HII_GUID_H__
#define __TCG_CONFIG_HII_GUID_H__

#define TCG_CONFIG_FORM_SET_GUID \
  { \
    0xb0f901e4, 0xc424, 0x45de, {0x90, 0x81, 0x95, 0xe2, 0xb, 0xde, 0x6f, 0xb5 } \
  }

extern EFI_GUID gTcgConfigFormSetGuid;

#endif
