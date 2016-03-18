/** @file
  GUID used as HII Package list GUID in GenericBdsLib module.
  
Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __BDS_LIB_HII_GUID_H__
#define __BDS_LIB_HII_GUID_H__

#define BDS_LIB_STRING_PACKAGE_GUID \
  { \
  0x3b4d9b23, 0x95ac, 0x44f6, { 0x9f, 0xcd, 0xe, 0x95, 0x94, 0x58, 0x6c, 0x72 } \
  }

extern EFI_GUID gBdsLibStringPackageGuid;

#endif
