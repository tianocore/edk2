/** @file
  Definitions EFI_SMM_COMMUNICATE_HEADER used by EFI_SMM_BASE_PROTOCOL.Communicate()
  functions.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Revision Reference:
  GUIDs defined in SmmCis spec version 0.9.

**/

#ifndef _SMM_COMMUNICATE_GUID_H_
#define _SMM_COMMUNICATE_GUID_H_

///
/// Inconsistent with specification here: 
/// GUID definition format has been changed, because the GUID format in the Framework specification is incorrect.
///
#define SMM_COMMUNICATE_HEADER_GUID \
  { \
    0xf328e36c, 0x23b6, 0x4a95, {0x85, 0x4b, 0x32, 0xe1, 0x95, 0x34, 0xcd, 0x75 } \
  }

extern EFI_GUID gSmmCommunicateHeaderGuid;

#endif
