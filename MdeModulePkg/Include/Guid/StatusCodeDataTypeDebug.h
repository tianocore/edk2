/** @file
  This file defines the GUID and data structure used to pass DEBUG() macro 
  information to the Status Code Protocol and Status Code PPI.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _STATUS_CODE_DATA_TYPE_DEBUG_H_
#define _STATUS_CODE_DATA_TYPE_DEBUG_H_

///
/// The Global ID used to identify a structure of type EFI_DEBUG_INFO.
///
#define EFI_STATUS_CODE_DATA_TYPE_DEBUG_GUID \
  { \
    0x9A4E9246, 0xD553, 0x11D5, { 0x87, 0xE2, 0x00, 0x06, 0x29, 0x45, 0xC3, 0xb9 } \
  }

///
/// The maximum size of an EFI_DEBUG_INFO structure.
///
#define EFI_STATUS_CODE_DATA_MAX_SIZE 200

///
/// This structure contains the ErrorLevel passed into the DEBUG() macro, followed 
/// by a 96-byte buffer that contains the variable argument list passed to the 
/// DEBUG() macro that has been converted to a BASE_LIST.  The 96-byte buffer is 
/// followed by a Null-terminated ASCII string that is the Format string passed 
/// to the DEBUG() macro.  The maximum size of this structure is defined by 
/// EFI_STATUS_CODE_DATA_MAX_SIZE.
///
typedef struct {
  ///
  /// The debug error level passed into a DEBUG() macro.
  ///
  UINT32  ErrorLevel;
} EFI_DEBUG_INFO;

extern EFI_GUID gEfiStatusCodeDataTypeDebugGuid;

#endif // _STATUS_CODE_DATA_TYPE_DEBUG_H_
