/** @file
  Definitions EFI_SMM_COMMUNICATE_HEADER used by EFI_SMM_BASE_PROTOCOL.Communicate() functions

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SmmCommunicate.h

  @par Revision Reference:
  GUIDs defined in SmmCis spec version 0.9

**/

#ifndef __SMM_COMMUNICATE_GUID_H__
#define __SMM_COMMUNICATE_GUID_H__

//******************************************************
// EFI_SMM_COMMUNICATE_HEADER
//******************************************************
#define SMM_COMMUNICATE_HEADER_GUID \
  { \
    0xf328e36c, 0x23b6, 0x4a95, {0x85, 0x4b, 0x32, 0xe1, 0x95, 0x34, 0xcd, 0x75 } \
  }

typedef struct {
  EFI_GUID                         HeaderGuid;
  UINTN                            MessageLength;
  UINT8                            Data[1];
} EFI_SMM_COMMUNICATE_HEADER;

extern EFI_GUID gSmmCommunicateHeaderGuid;

#endif
