/** @file
  GUIDs used as HII FormSet and HII Package list GUID in Ip4ConfigDxe driver. 
  
Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __IP4_CONFIG_HII_GUID_H__
#define __IP4_CONFIG_HII_GUID_H__

#define EFI_NIC_IP4_CONFIG_NVDATA_GUID \
  { \
    0x9d5b53f, 0xf4b0, 0x4f59, { 0xa0, 0xb1, 0x7b, 0x57, 0xd3, 0x5c, 0xe, 0x5 } \
  }

extern EFI_GUID gNicIp4ConfigNvDataGuid;

#endif
