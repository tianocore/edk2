/** @file
  GUIDs used as HII FormSet and HII Package list GUID in Ip6Config driver. 
  
Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __IP6_CONFIG_HII_GUID_H__
#define __IP6_CONFIG_HII_GUID_H__

#define IP6_CONFIG_NVDATA_GUID \
  { \
    0x2eea107, 0x98db, 0x400e, { 0x98, 0x30, 0x46, 0xa, 0x15, 0x42, 0xd7, 0x99 } \
  }

extern EFI_GUID gIp6ConfigNvDataGuid;

#endif
