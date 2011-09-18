/** @file
  GUIDs used as HII FormSet and HII Package list GUID in VlanConfig driver. 
  
Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __VLAN_CONFIG_HII_GUID_H__
#define __VLAN_CONFIG_HII_GUID_H__

#define VLAN_CONFIG_FORM_SET_GUID \
  { \
    0xd79df6b0, 0xef44, 0x43bd, {0x97, 0x97, 0x43, 0xe9, 0x3b, 0xcf, 0x5f, 0xa8 } \
  }

extern EFI_GUID gVlanConfigFormSetGuid;

#endif
