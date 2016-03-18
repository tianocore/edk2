/** @file
  GUIDs used as HII FormSet and HII Package list GUID in IScsiConfig driver 
  that supports IP4 and IP6 both. 
  
Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ISCSI_CONFIG_HII_GUID_H__
#define __ISCSI_CONFIG_HII_GUID_H__

#define ISCSI_CONFIG_GUID \
  { \
    0x4b47d616, 0xa8d6, 0x4552, { 0x9d, 0x44, 0xcc, 0xad, 0x2e, 0xf, 0x4c, 0xf9 } \
  }

extern EFI_GUID gIScsiConfigGuid;

#endif
