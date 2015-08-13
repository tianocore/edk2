/** @file
  GUIDs used as HII FormSet and HII Package list GUID in Tcg2Config driver. 
  
Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TCG2_CONFIG_HII_GUID_H__
#define __TCG2_CONFIG_HII_GUID_H__

#define TCG2_CONFIG_FORM_SET_GUID \
  { \
    0x6339d487, 0x26ba, 0x424b, { 0x9a, 0x5d, 0x68, 0x7e, 0x25, 0xd7, 0x40, 0xbc } \
  }

extern EFI_GUID gTcg2ConfigFormSetGuid;

#endif
