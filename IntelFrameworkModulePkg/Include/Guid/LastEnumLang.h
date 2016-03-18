/** @file
  GUID used as EFI variable to store platform language at last time enumeration.
  
Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __LAST_ENUM_LANGUAGE_GUID_H__
#define __LAST_ENUM_LANGUAGE_GUID_H__

///
/// This GUID is used for Set/Get platform language into/from variable at last time enumeration 
/// to ensure the enumeration will only execute once.
///
#define LAST_ENUM_LANGUAGE_GUID \
  { \
  0xe8c545b, 0xa2ee, 0x470d, { 0x8e, 0x26, 0xbd, 0xa1, 0xa1, 0x3c, 0xa, 0xa3 } \
  }

#define LAST_ENUM_LANGUAGE_VARIABLE_NAME L"LastEnumLang"

extern EFI_GUID gLastEnumLangGuid;

#endif
